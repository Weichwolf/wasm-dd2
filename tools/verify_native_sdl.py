#!/usr/bin/env python3
"""Verify the actual native SDL video/audio boundary and input transport.

Runs a known indexed/palette fixture, reads real renderer pixels, captures the
actual X11 window and observes only successful SDL audio queues. The input test
uses real SDL events and a boot-time virtual controller. This does not establish
original game full-stream timing or physical sound/controller hardware fidelity.
"""
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
from PIL import Image, ImageGrab

ROOT=Path(__file__).resolve().parent.parent


def config(mode):
    return subprocess.check_output(["python3",str(ROOT/"tools/native_sdl_config.py"),mode],text=True).splitlines()


def build_observer(directory):
    library=directory/"sdl_observer.so"
    subprocess.run(["gcc","-m32","-shared","-fPIC","-Wall","-Wextra","-Werror",*config("cflags"),
                    str(ROOT/"tools/native_sdl_observer.c"),*config("libs"),"-ldl","-o",str(library)],check=True)
    return library


def validate_frames(output, count):
    total=0
    for index in range(count):
        prefix=output/f"frame{index:05d}"
        state=json.loads(prefix.with_suffix(".json").read_text())
        fb,pal=prefix.with_suffix(".bin").read_bytes(),prefix.with_suffix(".pal").read_bytes()
        if len(fb)!=640*480 or len(pal)!=1024:
            raise RuntimeError("Missing source framebuffer/palette")
        rgba=b"".join(pal[byte*4:byte*4+3]+b"\xff" for byte in fb)
        if (state["width"],state["height"])!=(640,480) or prefix.with_suffix(".rgba").read_bytes()!=rgba:
            raise RuntimeError("Actual SDL rendered bytes differ from indexed source/palette")
        total+=len(fb)
    return total


def validate_audio(output, expected):
    metadata=json.loads((output/"audio.json").read_text())
    if (metadata["rate"],metadata["format"],metadata["channels"])!=(44100,0x8120,2):
        raise RuntimeError("Native SDL device accepted a different audio format")
    payload=(output/"audio.pcm").read_bytes()
    rows=[json.loads(row) for row in (output/"audio.jsonl").read_text().splitlines()]
    offset=0
    for row in rows:
        if row["offset"]!=offset or row["result"]!=0 or row["bytes"]%8 or row["device"]!=metadata["device"]:
            raise RuntimeError("Accepted SDL queue journal differs")
        offset+=row["bytes"]
    if offset!=len(payload) or not payload or payload!=expected:
        raise RuntimeError("Actual accepted SDL audio differs from expected Float32 bytes")
    return {"queues":len(rows),"bytes":len(payload),"frames":len(payload)//8}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="dd2-native-sdl-") as tmp:
        directory=Path(tmp)
        output=args.output.resolve() if args.output else directory/"captures"
        output.mkdir(parents=True,exist_ok=False)
        library=build_observer(directory)
        executable=directory/"fixture"
        subprocess.run(["gcc","-m32","-no-pie","-DDD2_NATIVE_SDL","-Wall","-Wextra","-Werror",
                        *config("cflags"),f"-I{ROOT/'re_out'}",str(ROOT/"re_out/dd2_native.c"),
                        str(ROOT/"tools/native_sdl_test.c"),*config("libs"),"-o",str(executable)],check=True)
        env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
        display=process=None
        with (output/"run.log").open("wb") as log:
            try:
                display=subprocess.Popen(["Xvfb","-displayfd","1","-screen","0","1280x1024x24"],stdout=subprocess.PIPE,stderr=log)
                number=display.stdout.readline().decode().strip()
                if not number:raise RuntimeError("Xvfb did not start")
                env.update(DISPLAY=":"+number)
                process=subprocess.Popen([str(executable)],env={**env,"DD2_WINDOW":"1","SDL_AUDIODRIVER":"dummy",
                    "LD_PRELOAD":str(library),"DD2_NATIVE_OBSERVE":str(output)},stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=log,text=True)
                for index in range(2):
                    if json.loads(process.stdout.readline())!={"ready":index}:
                        raise RuntimeError("SDL fixture did not reach synchronized presentation")
                    window=subprocess.check_output(["xdotool","search","--name","^Destruction Derby 2$"],env=env,text=True).splitlines()[-1]
                    geometry=subprocess.check_output(["xdotool","getwindowgeometry","--shell",window],env=env,text=True)
                    values=dict(line.split("=",1) for line in geometry.splitlines())
                    x,y,w,h=[int(values[key]) for key in ("X","Y","WIDTH","HEIGHT")]
                    image=ImageGrab.grab(xdisplay=env["DISPLAY"]).crop((x,y,x+w,y+h)).convert("RGB")
                    expected=bytearray()
                    for i in range(640*480):
                        byte=(i*17+i//640)%256
                        expected.extend(((byte*37+(91 if index else 0))%256,
                                         (byte*73+(137 if index else 0))%256,
                                         (byte*13+(203 if index else 0))%256))
                    image.save(output/f"window{index}.png")
                    if image.size!=(640,480) or image.tobytes()!=expected:
                        raise RuntimeError("Actual X11 window pixels differ from independent known palette fixture")
                    process.stdin.write("\n");process.stdin.flush()
                result=json.loads(process.stdout.readline())
                process.wait(timeout=20)
                if process.returncode or result!={"pass":True,"keyboard":True,"gamepad":True,"frames":2,"audio_frames":997}:
                    raise RuntimeError("Native SDL transport fixture failed")
            finally:
                if process is not None and process.poll() is None:process.kill();process.wait(timeout=5)
                if display is not None:display.terminate();display.wait(timeout=5)
        pixels=validate_frames(output,2)
        expected=b"".join(struct.pack("<f",(i-997)/512) for i in range(997*2))
        audio=validate_audio(output,expected)
        report={"scope":__doc__,"renderer_pixels":pixels,"x11_pixels":pixels,"audio":audio,"input":result}
        (output/"report.json").write_text(json.dumps(report,indent=2)+"\n")
        print(f"PASS native SDL: {pixels} exact renderer/X11 pixels, {audio['bytes']} accepted Float32 bytes; SDL keyboard/controller transport",flush=True)


if __name__=="__main__":main()
