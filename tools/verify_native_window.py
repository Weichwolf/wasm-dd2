#!/usr/bin/env python3
"""Navigate/drive the real native SDL game using X11 keyboard events.

Reads state without engine writes, validates actual renderer readback and every
accepted audio byte up to a declared presentation checkpoint. An isolated save
copy prevents changing the user's save. Physical hardware and original complete
stream/timing equivalence are not established by this test.
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
from verify_native_sdl import build_observer,validate_frames

ROOT=Path(__file__).resolve().parent.parent


def symbols(binary):
    result={}
    for line in subprocess.check_output(["nm","-an",str(binary)],text=True).splitlines():
        fields=line.split()
        if len(fields)==3:
            try:result[fields[2]]=int(fields[0],16)
            except ValueError:pass
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary",type=Path,default=Path("/tmp/dd2_native"))
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    binary=args.binary.resolve()
    if args.output:
        output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    else:output=Path(tempfile.mkdtemp(prefix="dd2-native-window-output-"))
    table=symbols(binary)
    with tempfile.TemporaryDirectory(prefix="dd2-native-window-") as tmp:
        directory=Path(tmp);library=build_observer(directory)
        game=directory/"game";game.mkdir()
        for asset in (ROOT/"DestructionDerby2").iterdir():
            if asset.name=="SaveGames":shutil.copyfile(asset,game/asset.name)
            else:(game/asset.name).symlink_to(asset,target_is_directory=asset.is_dir())
        env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
        display=process=memory=None;captures=0
        with (output/"run.log").open("wb") as log:
            try:
                display=subprocess.Popen(["Xvfb","-displayfd","1","-screen","0","1280x1024x24"],stdout=subprocess.PIPE,stderr=log)
                number=display.stdout.readline().decode().strip()
                if not number:raise RuntimeError("Xvfb did not start")
                env["DISPLAY"]=":"+number
                process=subprocess.Popen([str(binary)],cwd=game,env={**env,"DD2_WINDOW":"1","SDL_AUDIODRIVER":"dummy",
                    "LD_PRELOAD":str(library),"DD2_NATIVE_OBSERVE":str(output),"DD2_MIXPCM":str(output/"c-mixed.pcm"),
                    "DD2_NATIVE_PALETTE_ADDRESS":hex(table["g_palette"])},stdout=log,stderr=log)
                def read(address,count):
                    nonlocal memory
                    if memory is None:
                        # CLOEXEC can wake Popen before the new mm is installed.
                        # A proc mem descriptor pins the old mm permanently;
                        # wait for the child's actual executable before opening.
                        if Path(f"/proc/{process.pid}/exe").resolve()!=binary:
                            raise OSError("Child exec has not installed the native executable")
                        memory=open(f"/proc/{process.pid}/mem","rb",buffering=0)
                    return os.pread(memory.fileno(),count,address)
                def integer(address):return struct.unpack("<i",read(address,4))[0]
                def text(address):
                    pointer=struct.unpack("<I",read(address,4))[0]
                    if not 0x400000<pointer<0x980400:return ""
                    return read(pointer,64).split(b"\0",1)[0].decode("ascii",errors="replace")
                def state():return {"sb":read(0x460005,1)[0],"level":integer(0x936ff4),"cf":integer(0x462ff0),
                                     "cars":integer(0x46765c),"label":text(0x46975c)}
                def wait(condition,timeout=20):
                    deadline=time.monotonic()+timeout
                    while time.monotonic()<deadline:
                        if process.poll() is not None:raise RuntimeError(f"Native window game exited: {process.returncode}")
                        try:
                            value=condition()
                            if value:return value
                        except (FileNotFoundError,OSError,struct.error):pass
                        time.sleep(0.02)
                    raise RuntimeError(f"Native window condition timed out: {state()}; "
                        f"CD={text(0x469d64)!r}, flip={integer(table['g_frameno'])}, "
                        f"pad={read(0x46303e,20).hex()}, raw={read(0x754448,4).hex()}, slab={read(0x46996c,2).hex()}")
                wait(lambda:integer(table["g_frameno"])>=200 and state()["level"]==0 and "Wrecking" in state()["label"])
                actions={0x469b7c:"View_Statistics",0x469ebc:"FUN_0045220c",
                         0x46aaac:"FUN_00453bd0",0x46af90:"FUN_00453c00",
                         0x46afb8:"FUN_004546f8",0x46afcc:"View_Statistics"}
                for address,name in actions.items():
                    actual=struct.unpack("<I",read(address,4))[0]
                    if actual!=table[name]:raise RuntimeError(f"Frontend snapshot overwrote {name} action at {address:#x}: {actual:#x}")
                window=subprocess.check_output(["xdotool","search","--name","^Destruction Derby 2$"],env=env,text=True).splitlines()[-1]
                subprocess.run(["xdotool","windowfocus","--sync",window],env=env,check=True)
                def edge(key,down):
                    subprocess.run(["xdotool","keydown" if down else "keyup",key],env=env,check=True)
                def key(code):
                    # Keep the real key down until ReadPad has sampled it.
                    # A short wall-clock pulse can be entirely inside a busy
                    # audio tick or slab animation, losing the engine edge.
                    vk={"Right":0x27,"Left":0x25,"Down":0x28,"Up":0x26,"Return":0x0d,"Escape":0x1b}[code]
                    mapping=read(0x46302c,14)
                    bits=(1,8,0x10,0x20,0x40,0x80,0x100,0x200,0x400,0x800,0x1000,0x2000,0x4000,0x8000)
                    # Setup_Pad changes Escape from menu Back to race Start.
                    mask=next(bits[i] for i in (0,1,2,3,4,5,8,6,9,7,10,11,12,13) if mapping[i]==vk)
                    edge(code,True)
                    wait(lambda:struct.unpack("<H",read(0x754448,2))[0]&mask)
                    edge(code,False)
                    wait(lambda:not (struct.unpack("<H",read(0x754448,2))[0]&mask))
                    time.sleep(0.25)
                    print("X11 key",code,"label",text(0x46975c),"CD category",text(0x469d64),flush=True)
                def capture():
                    nonlocal captures
                    pending=output/"request.tmp";pending.write_text(str(captures)+"\n");pending.replace(output/"request")
                    wait(lambda:(output/f"frame{captures:05d}.json").is_file())
                    result=json.loads((output/f"frame{captures:05d}.json").read_text());captures+=1
                    return result
                def settled():
                    # Rotate_Slab_On draws eight rotation frames followed by
                    # fifteen bounce entries. The final four entries are zero;
                    # five distinct zero-angle flips establish the menu loop.
                    stable=0;previous=-1
                    def ready():
                        nonlocal stable,previous
                        flip=integer(table["g_frameno"])
                        if flip!=previous:
                            stable=stable+1 if read(0x46996c,2)==b"\0\0" else 0
                            previous=flip
                        return stable>=5
                    wait(ready)
                capture();key("Right");wait(lambda:"Select Car" in text(0x46975c));capture()
                key("Left");wait(lambda:"Wrecking" in text(0x46975c))
                key("Down");wait(lambda:"CD Audio Player" in text(0x46975c));key("Return");capture()
                wait(lambda:"Prev" in text(0x469d64));settled();key("Right");wait(lambda:"Play" in text(0x469d64))
                key("Return");wait(lambda:integer(0x462d70)!=0);capture()
                key("Right");wait(lambda:"Stop" in text(0x469d64));key("Return");wait(lambda:integer(0x462d70)==0);capture()
                key("Escape");wait(lambda:"CD Audio Player" in text(0x46975c))
                settled()
                key("Up");wait(lambda:"Wrecking" in text(0x46975c))
                key("Down");key("Down");wait(lambda:"Go!" in text(0x46975c));key("Return")
                first=wait(lambda:state() if state()["sb"]==89 and state()["level"]>0 else None,30)
                wait(lambda:state()["cf"]>first["cf"]+4);capture()
                # The countdown advances on presented frames, whereas cf can
                # advance several physics ticks under load. Read the actual
                # control gate used by Play_Game instead of assuming cf60.
                wait(lambda:integer(0x784298)<1,20)
                player=integer(0x93ded0)
                if not 0<=player<20:raise RuntimeError("Invalid native player-car index")
                # Original 442dc0..442de7 projects X at struct+0x30 and Z
                # at +0x38; +0x24 is heading and +0x34 is height. The old
                # browser pad probes accidentally measured heading/height.
                def position():return [integer(0x792a30+player*0x1b2),integer(0x792a38+player*0x1b2)]
                before=position()
                edge("a",True)
                wait(lambda:read(table["dd2_keystate"]+0x41,1)==b"\x01")
                start=state()["cf"];wait(lambda:state()["cf"]>=start+40,10)
                if not any(read(0x46304b,2)):raise RuntimeError("Physical accelerate key did not reach engine pad flags")
                after=position()
                end=state()["cf"]
                if after==before:raise RuntimeError(f"Native player car did not move: player={player}, cf={start}..{end}, "
                    f"position={before}, map={read(0x46302c,14).hex()}, raw={read(0x754448,10).hex()}, "
                    f"countdown={integer(0x784298)}, replay={integer(0x467074)}, pit={integer(0x46704c)}, "
                    f"throttle={integer(0x792a86+player*0x1b2)}")
                capture();edge("a",False);wait(lambda:read(table["dd2_keystate"]+0x41,1)==b"\0")
                key("Escape");time.sleep(0.4);paused=state()["cf"];time.sleep(0.4)
                if state()["cf"]!=paused:raise RuntimeError("Native Escape did not pause the race")
                capture();key("Return");wait(lambda:state()["cf"]>paused);final=capture()
                if state()["cars"]!=20:raise RuntimeError("Native race has no full opponent field")
                # The SDL observer records an explicit accepted-byte boundary at
                # this presentation. C has flushed the shared mixer before it.
                accepted=final["audio_bytes"]
                actual=(output/"audio.pcm").read_bytes()[:accepted]
                expected=(output/"c-mixed.pcm").read_bytes()[:accepted]
                if not accepted or len(actual)!=accepted or actual!=expected or not any(actual):
                    raise RuntimeError("Accepted native SDL audio differs from complete C mixed prefix")
                pixels=validate_frames(output,captures)
                report={"scope":__doc__,"pass":True,"renderer_pixels":pixels,"captures":captures,
                        "accepted_audio_bytes":accepted,"race":state(),"input":"actual X11 keyboard -> SDL -> engine",
                        "binary":str(binary),"binary_sha256":hashlib.sha256(binary.read_bytes()).hexdigest(),
                        "driving":{"player":player,"start_cf":start,"end_cf":end,"position_before":before,"position_after":after}}
                (output/"report.json").write_text(json.dumps(report,indent=2)+"\n")
                print(f"PASS native window: real menu/CD controls, populated race, accelerate and pause/resume; "
                      f"{pixels} exact rendered pixels and {accepted} exact accepted mixed audio bytes",flush=True)
                print(f"Report: {output/'report.json'}",flush=True)
            finally:
                if memory is not None:memory.close()
                if process is not None and process.poll() is None:
                    process.terminate()
                    try:process.wait(timeout=5)
                    except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
                if display is not None:display.terminate();display.wait(timeout=5)


if __name__=="__main__":main()
