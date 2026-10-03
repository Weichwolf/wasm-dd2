#!/usr/bin/env python3
"""Run the real engine MCI movie path in an actual SDL/X11 window.

Check all renderer pixels against the submitted movie texture, observe each
accepted audio device/queue/close and compare complete PCM with real Wine ACM.
Live key-up must retain playback; key-down must exit/close it. SDL's dummy
audio device is declared; physical DAC and original final output timing remain
unproven. The full original intro PCM reference also has driver-specific zeros.
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
import time
from verify_native_sdl import config
from verify_movie_codec import compare
ROOT=Path(__file__).resolve().parent.parent


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary",type=Path,required=True)
    parser.add_argument("--source",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--skip-only",action="store_true")
    parser.add_argument("--skip-key",default="d",help="real X11 key used for movie cancellation")
    parser.add_argument("--release-key",default="Escape",help="real X11 release which must retain playback")
    args=parser.parse_args();output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    source=json.loads((args.source/"report.json").read_text());binary=args.binary.resolve()
    report={"scope":__doc__,"binary_sha256":hashlib.sha256(binary.read_bytes()).hexdigest(),"cases":[]}
    with tempfile.TemporaryDirectory(prefix="dd2-native-movie-") as tmp:
        directory=Path(tmp);observer=directory/"observer.so"
        subprocess.run(["gcc","-m32","-shared","-fPIC","-O2","-Wall","-Wextra","-Werror",*config("cflags"),str(ROOT/"tools/native_movie_observer.c"),*config("libs"),"-ldl","-o",str(observer)],check=True)
        for film in source["films"]:
            path=ROOT/"DestructionDerby2"/film["file"]
            if hashlib.sha256(path.read_bytes()).hexdigest()!=film["avi_sha256"]:raise RuntimeError("Changed original movie")
            name=path.stem.lower();reference=args.source/name/"wine.pcm"
            if hashlib.sha256(reference.read_bytes()).hexdigest()!=film["pcm_sha256"]:raise RuntimeError("Changed Wine PCM")
            for skip in ((True,) if args.skip_only else (False,True)):
                case=output/f"{name}-{'skip' if skip else 'full'}";case.mkdir();display=process=memory=None
                with (case/"run.log").open("w") as log:
                    try:
                        display=subprocess.Popen(["Xvfb","-displayfd","1","-screen","0","640x480x24"],stdout=subprocess.PIPE,stderr=log)
                        number=display.stdout.readline().decode().strip()
                        if not number:raise RuntimeError("Xvfb failed")
                        env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
                        env.update(DISPLAY=":"+number,DD2_WINDOW="1",DD2_MOVIE=film["file"].upper(),SDL_AUDIODRIVER="dummy",LD_PRELOAD=str(observer),DD2_NATIVE_MOVIE_OBSERVE=str(case))
                        control_env={k:v for k,v in env.items() if k!="LD_PRELOAD"}
                        process=subprocess.Popen([str(binary)],cwd=ROOT/"DestructionDerby2",env=env,stdout=log,stderr=log)
                        if skip:
                            deadline=time.monotonic()+10
                            while time.monotonic()<deadline:
                                if process.poll() is not None:raise RuntimeError("Movie exited before key test")
                                if (case/"events.jsonl").exists() and sum(json.loads(s)["event"]=="present" for s in (case/"events.jsonl").read_text().splitlines())>=26:break
                                time.sleep(0.05)
                            else:raise RuntimeError("No actual movie frames")
                            memory=open(f"/proc/{process.pid}/mem","rb",buffering=0)
                            def playing():return struct.unpack("<I",os.pread(memory.fileno(),4,0x462cd4))[0]
                            if playing()!=1:raise RuntimeError("Original Movie_Playing flag not active")
                            subprocess.run(["xdotool","search","--name","Destruction Derby 2","windowfocus","keyup",args.release_key],env=control_env,check=True,timeout=5)
                            time.sleep(0.2)
                            if process.poll() is not None or playing()!=1:raise RuntimeError("Key-up skipped the movie")
                            subprocess.run(["xdotool","search","--name","Destruction Derby 2","windowfocus","keydown",args.skip_key],env=control_env,check=True,timeout=5)
                        if process.wait(timeout=100):raise RuntimeError("Actual engine movie failed")
                    finally:
                        if memory:memory.close()
                        if process and process.poll() is None:process.terminate();process.wait(timeout=5)
                        if display:display.terminate();display.wait(timeout=5)
                events=[json.loads(s) for s in (case/"events.jsonl").read_text().splitlines()]
                opens=[e for e in events if e["event"]=="open"]
                movie_devices=[e for e in opens if e["device"] and e["rate"]==22050 and e["format"]==0x8010 and e["channels"]==2]
                if len(movie_devices)!=1:raise RuntimeError("No unique actual PCM16 stereo movie device")
                # SDL can reuse the retired Float32 device ID for the movie.
                # Only events within this actual open/close lifetime belong
                # to the movie stream; an integer ID is not a lifetime token.
                opened=events.index(movie_devices[0])
                closed=next((i for i in range(opened+1,len(events)) if events[i]["event"]=="close" and events[i]["device"]==movie_devices[0]["device"]),None)
                if closed is None:raise RuntimeError("Movie device did not close")
                queues=[e for e in events[opened+1:closed] if e["event"]=="queue" and e["device"]==movie_devices[0]["device"]]
                if not queues:raise RuntimeError("Movie device accepted no audio")
                if len({e["stream"] for e in queues})!=1:raise RuntimeError("Movie device/stream reused")
                stream=queues[0]["stream"];pcm=case/f"stream{stream}.pcm"
                offset=0
                for e in queues:
                    if e["offset"]!=offset or e["result"]!=0:raise RuntimeError("Accepted audio journal inconsistent")
                    offset+=e["bytes"]
                if offset!=pcm.stat().st_size or compare(reference,pcm)!=film["metadata"]["pcm_frames"]*4:raise RuntimeError("Actual SDL movie PCM differs")
                if not any(e["event"]=="close" and e["stream"]==stream and e["bytes"]==offset for e in events):raise RuntimeError("Movie device did not close")
                presents=[e for e in events if e["event"]=="present"]
                if [e["frame"] for e in presents]!=list(range(len(presents))):raise RuntimeError("Movie present journal inconsistent")
                if not skip and len(presents)!=film["metadata"]["frames"]:raise RuntimeError("Incomplete actual movie presentation")
                if skip and not 26<=len(presents)<film["metadata"]["frames"]:raise RuntimeError("Key-down did not exit active movie")
                result={"file":film["file"],"skip":skip,"presented_frames":len(presents),"exact_rendered_pixels":len(presents)*307200,"pcm_bytes":offset,"key_up_retained":skip,"key_down_closed":skip,"release_key":args.release_key if skip else None,"skip_key":args.skip_key if skip else None}
                report["cases"].append(result);(output/"report.json").write_text(json.dumps(report,indent=2)+"\n")
                print(f"PASS native {name} skip={skip}: {len(presents)} actual SDL frames, every rendered pixel exact, all {offset} accepted PCM bytes exact",flush=True)


if __name__=="__main__":main()
