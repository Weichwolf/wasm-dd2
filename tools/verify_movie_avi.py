#!/usr/bin/env python3
"""Verify the production AVI container against actual Wine codec output.

Complete original files, rather than pre-extracted packet bundles, feed the
native/WASM AVI interface. Every decoded RGB and PCM byte is compared with
real Win32 ICCVID/ACM. Random/repeated/forward/backward frame requests also
compare directly; malformed container extents/headers must fail safely.
Playback, scaling, output sinks and clock/control semantics are separate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
from verify_movie_codec import packets,riff_chunks,compare
from verify_sound_cursor import wine_probe

ROOT=Path(__file__).resolve().parent.parent
SOURCES=[ROOT/"re_out"/f"dd2_{name}.c" for name in ("avi","cinepak","msadpcm")]
FIXTURE=ROOT/"tools/movie_avi_test.c"


def sha(path):
    with path.open("rb") as file:return hashlib.file_digest(file,"sha256").hexdigest()


def original_metadata(path):
    (width,height),frames=packets(path)
    headers=[d for k,d in riff_chunks(path) if k==b"strh"]
    video=next(d for d in headers if d[:4]==b"vids")
    audio=next(d for d in headers if d[:4]==b"auds")
    fmt=next(d for k,d in riff_chunks(path) if k==b"strf" and d[:2]==b"\x02\x00")
    data=b"".join(d for k,d in riff_chunks(path) if k==b"01wb")
    count=len(frames)
    expected=dict(width=width,height=height,frames=count,empty_frames=sum(not f for f in frames),
                  video_scale=struct.unpack_from("<I",video,20)[0],video_rate=struct.unpack_from("<I",video,24)[0],
                  video_start=struct.unpack_from("<I",video,28)[0],
                  audio_scale=struct.unpack_from("<I",audio,20)[0],audio_rate=struct.unpack_from("<I",audio,24)[0],
                  audio_start=struct.unpack_from("<I",audio,28)[0],audio_initial_frames=struct.unpack_from("<I",audio,16)[0],
                  pcm_rate=struct.unpack_from("<I",fmt,4)[0],pcm_channels=struct.unpack_from("<H",fmt,2)[0],
                  pcm_frames=len(data)//struct.unpack_from("<H",fmt,12)[0]*struct.unpack_from("<H",fmt,18)[0],
                  seeks=[count-1,0,1,count//2,count//2,count-2,2])
    return expected,frames,fmt,data


def altered_containers(original):
    # Locate headers/packets with RIFF boundaries, never search compressed data
    # for byte strings that could happen to resemble a chunk ID.
    offsets={}
    def walk(at,end):
        while at<end:
            kind=original[at:at+4];size=struct.unpack_from("<I",original,at+4)[0]
            offsets.setdefault(kind,[]).append(at)
            if kind in (b"LIST",b"RIFF"):walk(at+12,at+8+size)
            at+=8+size+(size&1)
    walk(12,len(original))
    for label,offset,value in (
            ("wrong-riff-length",4,len(original)-9),
            ("oversize-video-packet",offsets[b"00dc"][0]+4,0xffffffff),
            ("wrong-frame-count",offsets[b"avih"][0]+8+16,1),
            ("wrong-stream-count",offsets[b"avih"][0]+8+24,3),
            ("wrong-video-width",offsets[b"strf"][0]+8+4,321),
            ("zero-video-rate",offsets[b"strh"][0]+8+24,0),
            ("wrong-audio-block-count",offsets[b"strh"][1]+8+32,1),
            ("partial-audio-block",offsets[b"01wb"][0]+4,1023)):
        bad=bytearray(original);struct.pack_into("<I",bad,offset,value)
        yield label,bad
    yield "truncated-file",original[:-1]
    bad=bytearray(original);bad[offsets[b"01wb"][0]+8]=7
    yield "invalid-audio-predictor",bad


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mingw",default="i686-w64-mingw32-gcc")
    parser.add_argument("--emcc",default="emcc")
    parser.add_argument("--node",default="node")
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
    output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    report={"scope":__doc__,"sources":{str(p.relative_to(ROOT)):sha(p) for p in [*SOURCES,FIXTURE,Path(__file__).resolve()]},"films":[],
            "wine_version":subprocess.check_output(["wine","--version"],text=True).strip()}
    with tempfile.TemporaryDirectory(prefix="dd2-movie-avi-") as tmp:
        directory=Path(tmp);native=directory/"native";wasm=directory/"wasm.js"
        common=["-std=gnu99","-O2","-Wall","-Wextra","-Werror",f"-I{ROOT/'re_out'}",*map(str,SOURCES),str(FIXTURE)]
        subprocess.run(["gcc","-m32","-no-pie","-fsanitize=address,undefined","-fno-sanitize-recover=all",*common,"-o",str(native)],check=True)
        subprocess.run([args.emcc,*common,"-sNODERAWFS=1","-sEXIT_RUNTIME=1","-sINITIAL_MEMORY=67108864",
                        "-sALLOW_MEMORY_GROWTH=1","-sMAXIMUM_MEMORY=268435456","-o",str(wasm)],check=True)
        codecs=[]
        for kind,source,library in (("video","movie_codec_test.c","msvfw32"),("audio","movie_audio_test.c","msacm32")):
            exe=output/f"{kind}.exe"
            options=["-DDD2_MOVIE_DIB32"] if kind=="video" else []
            subprocess.run([args.mingw,"-O2","-Wall","-Wextra","-Werror",*options,str(ROOT/"tools"/source),f"-l{library}","-o",str(exe)],check=True)
            codecs.append(exe)
            report.setdefault("reference_fixtures",{})[kind]={"source_sha256":sha(ROOT/"tools"/source),"exe_sha256":sha(exe)}
            if kind=="video":report["reference_fixtures"][kind]["dib_bits"]=32
        for filename in ("Intro.avi","Outro.avi"):
            path=ROOT/"DestructionDerby2"/filename;film=output/path.stem.lower();film.mkdir()
            expected,frames,fmt,audio=original_metadata(path)
            bundle=film/"video.packets"
            with bundle.open("wb") as file:
                file.write(struct.pack("<III",expected["width"],expected["height"],len(frames)))
                for frame in frames:file.write(struct.pack("<I",len(frame)));file.write(frame)
            reference=film/"wine.rgb";pcm=film/"wine.pcm"
            for exe,input_path,target in ((codecs[0],bundle,reference),(codecs[1],film/"audio.packets",pcm)):
                if exe==codecs[1]:input_path.write_bytes(struct.pack("<II",len(fmt),len(audio))+fmt+audio)
                records=wine_probe(exe,film,env,arguments=tuple("Z:"+str(p).replace("/","\\") for p in (input_path,target)))
                shutil.move(film/"wine.log",film/f"{exe.stem}-wine.log")
                shutil.rmtree(film/"wine-prefix");input_path.unlink()
                if exe==codecs[0]:
                    if records!=dict(width=expected["width"],height=expected["height"],frames=expected["frames"],empty_packets=expected["empty_frames"]):
                        raise RuntimeError("Actual Win32 video fixture extent differs")
                elif records!=dict(rate=expected["pcm_rate"],channels=expected["pcm_channels"],frames=expected["pcm_frames"],bytes=expected["pcm_frames"]*expected["pcm_channels"]*2):
                    raise RuntimeError("Actual Win32 audio fixture extent differs")
            pixel_bytes=expected["width"]*expected["height"]*3
            selected=film/"wine-seeks.rgb"
            with reference.open("rb") as file,selected.open("wb") as dest:
                for frame in expected["seeks"]:
                    file.seek(frame*pixel_bytes);dest.write(file.read(pixel_bytes))
            result={"file":filename,"avi_sha256":sha(path),"metadata":expected,"rgb_sha256":sha(reference),
                    "pcm_sha256":sha(pcm),"seek_sha256":sha(selected),"targets":[]}
            for target,command in (("native-asan-ubsan",[str(native)]),("wasm",[args.node,str(wasm)])):
                rgb=film/f"{target}.rgb";decoded=film/f"{target}.pcm";seeks=film/f"{target}-seeks.rgb"
                run=subprocess.run([*command,str(path),str(rgb),str(decoded),str(seeks)],env=env,capture_output=True,text=True,timeout=60)
                (film/f"{target}.log").write_text(run.stdout+run.stderr)
                if run.returncode or json.loads(run.stdout)!=expected:raise RuntimeError(f"{target}: AVI interface failed: {run.stderr}")
                if compare(reference,rgb)!=pixel_bytes*expected["frames"] or compare(pcm,decoded)!=expected["pcm_frames"]*expected["pcm_channels"]*2:
                    raise RuntimeError("Wrong full AVI decoded extent")
                compare(selected,seeks)
                # Reuse the complete target stream: a changed bit must fail
                # the identical full-byte verifier without length changes.
                with rgb.open("r+b") as file:
                    file.seek(1000);byte=file.read(1);file.seek(1000);file.write(bytes([byte[0]^1]))
                try:compare(reference,rgb)
                except RuntimeError:pass
                else:raise RuntimeError("Accepted altered AVI decoded pixel")
                rgb.unlink();decoded.unlink();seeks.unlink()
                bad_path=directory/"invalid.avi";rejected=[]
                for label,bad in altered_containers(path.read_bytes()):
                    bad_path.write_bytes(bad)
                    failure=subprocess.run([*command,str(bad_path),str(rgb),str(decoded),str(seeks)],env=env,capture_output=True,text=True,timeout=30)
                    if (not failure.returncode or any(p.exists() for p in (rgb,decoded,seeks)) or "movie AVI: invalid container" not in failure.stderr
                            or "AddressSanitizer" in failure.stderr or "runtime error:" in failure.stderr):
                        raise RuntimeError(f"{target}: malformed {label} did not fail safely: {failure.stderr}")
                    rejected.append(label)
                result["targets"].append({"target":target,"all_rgb_pcm_bytes_exact":True,"all_seek_bytes_exact":True,
                                          "altered_pixel_rejected":True,"invalid_containers_rejected":rejected})
                print(f"PASS {filename} {target}: all {expected['frames']} RGB frames and {expected['pcm_frames']} PCM frames exact, seeks and {len(rejected)} malformed files checked",flush=True)
            reference.unlink();selected.unlink()
            report["films"].append(result);(output/"report.json").write_text(json.dumps(report,indent=2)+"\n")


if __name__=="__main__":main()
