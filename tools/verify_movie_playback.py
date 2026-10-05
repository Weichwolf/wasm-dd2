#!/usr/bin/env python3
"""Compare real engine Play_Movie through the production AVI/MCI backend.

A controlled sink/clock captures every presented ARGB byte and original PCM
submission; compare both targets directly and complete PCM with real Wine ACM.
Check frame deadlines, 32-bit clock wrap, deferred notify until audio drains,
and early Close. Frames stream through a pipe; only a lossless first-target
archive is retained until the literal second-target comparison completes.
This does not prove original full movie output timing/sinks.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import select
import struct
import time
import zlib
from artifacts import WORK, check_space, prepare_output
from verify_movie_codec import compare
ROOT=Path(__file__).resolve().parent.parent


def read_exact(process,count,deadline):
    data=bytearray()
    while len(data)<count:
        remaining=deadline-time.monotonic()
        if remaining<=0 or not select.select([process.stdout],[],[],remaining)[0]:
            raise RuntimeError("Movie component output timed out")
        part=os.read(process.stdout.fileno(),count-len(data))
        if not part:raise RuntimeError("Incomplete movie component output")
        data.extend(part)
    return bytes(data)


def video_equal(expected,actual):
    if expected!=actual:raise RuntimeError("Native/WASM movie display bytes differ")


def capture_component(command,path,audio,timing,mode,frames,archive,reference,env,log,output):
    """Read literal frames through a pipe; keep only a compressed first stream."""
    sha=hashlib.sha256();changed_rejected=False
    with log.open("wb") as errors,subprocess.Popen(
            [*command,str(path),"-",str(audio),str(timing),str(mode)],
            env=env,stdout=subprocess.PIPE,stderr=errors,bufsize=0) as process:
        deadline=time.monotonic()+300
        try:
            with archive.open("rb" if reference else "xb") as saved:
                for frame in range(frames):
                    actual=read_exact(process,640*480*4,deadline);sha.update(actual)
                    if reference:
                        header=saved.read(4)
                        if len(header)!=4:raise RuntimeError("Truncated movie frame archive")
                        size=struct.unpack("<I",header)[0]
                        if not 0<size<=640*480*4+1024:raise RuntimeError("Invalid movie archive extent")
                        packed=saved.read(size);inflater=zlib.decompressobj()
                        expected=inflater.decompress(packed,640*480*4+1)
                        if (len(packed)!=size or len(expected)!=640*480*4 or not inflater.eof
                                or inflater.unused_data or inflater.unconsumed_tail):
                            raise RuntimeError("Incomplete literal movie archive frame")
                        video_equal(expected,actual)
                        if frame==0:
                            changed=bytearray(actual);changed[1000]^=1
                            try:video_equal(expected,bytes(changed))
                            except RuntimeError:changed_rejected=True
                            else:raise RuntimeError("Accepted changed actual MCI display bit")
                    else:
                        packed=zlib.compress(actual,1)
                        saved.write(struct.pack("<I",len(packed)));saved.write(packed)
                    if frame%32==0:
                        if not reference:saved.flush()
                        check_space(output)
                if reference and saved.read(1):raise RuntimeError("Extra movie archive frames")
            # Binary frames precede one bounded JSON line from the same pipe.
            line=bytearray()
            while not line.endswith(b"\n"):
                line.extend(read_exact(process,1,deadline))
                if len(line)>4096:raise RuntimeError("Invalid movie component record")
            if process.wait(timeout=max(.1,deadline-time.monotonic())) or process.stdout.read(1):
                raise RuntimeError("Movie component failed or emitted extra output")
            record=json.loads(line)
        finally:
            if process.poll() is None:process.kill();process.wait(timeout=10)
    check_space(output)
    return record,sha.hexdigest(),changed_rejected


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source",type=Path,required=True,help="Wine AVI source proof with complete retained PCM")
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--node",default="node")
    args=parser.parse_args();output=prepare_output(args.output)
    if WORK not in output.parents:raise ValueError("Use /tmp/wasm-dd2/")
    output.mkdir(parents=True,exist_ok=False);check_space(output)
    source=json.loads((args.source/"report.json").read_text())
    engine=(ROOT/"build/dd2.c").read_text();start=engine.index("void __cdecl Play_Movie(",engine.index("/* ===== Play_Movie @"))
    function=engine[start:engine.index("/* ===== Load_Null",start)]
    report={"scope":__doc__,"cases":[],"engine_function_sha256":hashlib.sha256(function.encode()).hexdigest()}
    with tempfile.TemporaryDirectory(prefix="movie-playback-",dir=WORK) as tmp:
        directory=Path(tmp);(directory/"movie-function.c").write_text(function)
        native=directory/"native";wasm=directory/"wasm.js"
        units=[ROOT/"build"/f"dd2_{name}.c" for name in ("movie","movie_surface","avi","cinepak","msadpcm")]
        units.append(ROOT/"tools/movie_playback_test.c")
        report["sources"]={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in units}
        common=["-std=gnu99","-O2","-D_FILE_OFFSET_BITS=64","-ffunction-sections","-fdata-sections",f"-I{ROOT/'build'}",f"-I{directory}",*map(str,units),"-Wl,--gc-sections"]
        subprocess.run(["gcc","-m32","-no-pie","-fsanitize=address,undefined","-fno-sanitize-recover=all",*common,"-o",str(native)],check=True)
        subprocess.run(["emcc",*common,"-sNODERAWFS=1","-sINITIAL_MEMORY=67108864","-sALLOW_MEMORY_GROWTH=1","-sEXIT_RUNTIME=1","-o",str(wasm)],check=True)
        env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
        for film in source["films"]:
            path=ROOT/"DestructionDerby2"/film["file"];name=path.stem.lower()
            if hashlib.sha256(path.read_bytes()).hexdigest()!=film["avi_sha256"]:raise RuntimeError("Changed original AVI")
            reference=args.source/name/"wine.pcm"
            if hashlib.sha256(reference.read_bytes()).hexdigest()!=film["pcm_sha256"]:raise RuntimeError("Changed actual source PCM")
            for mode in (0,1,2):
                skip=mode==1;audio_failure=mode==2
                case=output/f"{name}-{('full','skip','audio-unavailable')[mode]}";case.mkdir();previous=None;result={"file":film["file"],"skip":skip,"audio_unavailable":audio_failure,"targets":[]}
                expected_frames=26 if skip else film["metadata"]["frames"]
                for target,command in (("native-asan-ubsan",[str(native)]),("wasm",[args.node,str(wasm)])):
                    video=case/"native-video.zlib";audio=case/f"{target}.pcm";timing=case/f"{target}.timeline"
                    actual,video_sha,changed=capture_component(command,path,audio,timing,mode,
                        expected_frames,video,previous is not None,env,case/f"{target}.log",output)
                    if actual["frames"]!=expected_frames or actual["pcm_frames"]!=film["metadata"]["pcm_frames"] or actual["rate"]!=22050 or actual["channels"]!=2:
                        raise RuntimeError("Wrong actual movie transport/source extent")
                    if audio_failure:
                        if audio.stat().st_size:raise RuntimeError("Invented PCM for unavailable audio device")
                    elif compare(reference,audio)!=actual["pcm_frames"]*4:raise RuntimeError("Incomplete actual PCM submission")
                    deadlines=[list(map(int,line.split())) for line in timing.read_text().splitlines()]
                    if deadlines!=[[frame,frame*40] for frame in range(expected_frames)]:raise RuntimeError("Frame deadlines/wrapped clock differ")
                    if previous:
                        if video_sha!=previous[3] or not changed:raise RuntimeError("Movie display stream differs")
                        result["changed_display_bit_rejected"]=True
                        compare(previous[1],timing)
                        if previous[2]!=actual:raise RuntimeError("Native/WASM transport records differ")
                        result["video_sha256"]=video_sha
                        result["literal_video_bytes_per_target"]=expected_frames*640*480*4
                        for p in (video,previous[1],timing):p.unlink()
                    else:previous=(video,timing,actual,video_sha)
                    audio.unlink();result["targets"].append(target)
                    audio_scope="no invented PCM for unavailable device" if audio_failure else "whole original source PCM"
                    print(f"PASS {name} mode={mode} {target}: {expected_frames} actual MCI frames, {audio_scope}, wrap/deadlines/drain/notify",flush=True)
                result["records"]=actual;report["cases"].append(result);(output/"report.json").write_text(json.dumps(report,indent=2)+"\n")


if __name__=="__main__":main()
