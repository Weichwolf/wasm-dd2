#!/usr/bin/env python3
"""Compare real engine Play_Movie through the production AVI/MCI backend.

A controlled sink/clock captures every presented ARGB byte and original PCM
submission; compare both targets directly and complete PCM with real Wine ACM.
Check frame deadlines, 32-bit clock wrap, deferred notify until audio drains,
and early Close. This does not prove original full movie output timing/sinks.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
from verify_movie_codec import compare
ROOT=Path(__file__).resolve().parent.parent


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source",type=Path,required=True,help="Wine AVI source proof with complete retained PCM")
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--node",default="node")
    args=parser.parse_args();output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    source=json.loads((args.source/"report.json").read_text())
    engine=(ROOT/"build/dd2.c").read_text();start=engine.index("void __cdecl Play_Movie(",engine.index("/* ===== Play_Movie @"))
    function=engine[start:engine.index("/* ===== Load_Null",start)]
    report={"scope":__doc__,"cases":[],"engine_function_sha256":hashlib.sha256(function.encode()).hexdigest()}
    with tempfile.TemporaryDirectory(prefix="dd2-movie-playback-") as tmp:
        directory=Path(tmp);(directory/"movie-function.c").write_text(function)
        native=directory/"native";wasm=directory/"wasm.js"
        units=[ROOT/"re_out"/f"dd2_{name}.c" for name in ("movie","movie_surface","avi","cinepak","msadpcm")]
        units.append(ROOT/"tools/movie_playback_test.c")
        report["sources"]={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in units}
        common=["-std=gnu99","-O2","-D_FILE_OFFSET_BITS=64","-ffunction-sections","-fdata-sections",f"-I{ROOT/'re_out'}",f"-I{directory}",*map(str,units),"-Wl,--gc-sections"]
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
                    video=case/f"{target}.argb";audio=case/f"{target}.pcm";timing=case/f"{target}.timeline"
                    run=subprocess.run([*command,str(path),str(video),str(audio),str(timing),str(mode)],env=env,capture_output=True,text=True,timeout=300)
                    (case/f"{target}.log").write_text(run.stdout+run.stderr)
                    if run.returncode:raise RuntimeError(f"{target}: {run.stderr}")
                    actual=json.loads(run.stdout)
                    if actual["frames"]!=expected_frames or actual["pcm_frames"]!=film["metadata"]["pcm_frames"] or actual["rate"]!=22050 or actual["channels"]!=2:
                        raise RuntimeError("Wrong actual movie transport/source extent")
                    if audio_failure:
                        if audio.stat().st_size:raise RuntimeError("Invented PCM for unavailable audio device")
                    elif compare(reference,audio)!=actual["pcm_frames"]*4:raise RuntimeError("Incomplete actual PCM submission")
                    deadlines=[list(map(int,line.split())) for line in timing.read_text().splitlines()]
                    if deadlines!=[[frame,frame*40] for frame in range(expected_frames)]:raise RuntimeError("Frame deadlines/wrapped clock differ")
                    if previous:
                        if compare(previous[0],video)!=expected_frames*640*480*4:raise RuntimeError("Incomplete video output")
                        with video.open("r+b") as file:
                            file.seek(1000);byte=file.read(1);file.seek(1000);file.write(bytes([byte[0]^1]))
                        try:compare(previous[0],video)
                        except RuntimeError:result["changed_display_bit_rejected"]=True
                        else:raise RuntimeError("Accepted changed actual MCI display bit")
                        compare(previous[1],timing)
                        if previous[2]!=actual:raise RuntimeError("Native/WASM transport records differ")
                        for p in (previous[0],previous[1],video,timing):p.unlink()
                    else:previous=(video,timing,actual)
                    audio.unlink();result["targets"].append(target)
                    audio_scope="no invented PCM for unavailable device" if audio_failure else "whole original source PCM"
                    print(f"PASS {name} mode={mode} {target}: {expected_frames} actual MCI frames, {audio_scope}, wrap/deadlines/drain/notify",flush=True)
                result["records"]=actual;report["cases"].append(result);(output/"report.json").write_text(json.dumps(report,indent=2)+"\n")


if __name__=="__main__":main()
