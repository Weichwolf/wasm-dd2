#!/usr/bin/env python3
"""Compare movie display conversion with real Win32 GDI on native/WASM.

Verified Wine codec checkpoints and a gradient covering all component values
feed RGB32 StretchDIBits into RGB565, then GetDIBits display expansion. Compare
all ARGB bytes for original 2x/1x and clipped/offset rectangles. This is a GDI
surface fixture, not full original dd2h.exe window/timing acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from verify_movie_codec import compare
from verify_sound_cursor import wine_probe
ROOT=Path(__file__).resolve().parent.parent


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mingw",default="i686-w64-mingw32-gcc")
    parser.add_argument("--source",type=Path,required=True,help="complete Wine codec proof directory with saved RGB checkpoints")
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--node",default="node")
    args=parser.parse_args();output=args.output.resolve();output.mkdir(parents=True,exist_ok=False)
    source=json.loads((args.source/"report.json").read_text())
    report={"scope":__doc__,"cases":[]}
    fixture=ROOT/"tools/movie_surface_test.c";surface=ROOT/"re_out/dd2_movie_surface.c"
    report["sources"]={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (fixture,surface)}
    with tempfile.TemporaryDirectory(prefix="dd2-movie-surface-") as tmp:
        directory=Path(tmp);exe=output/"surface.exe";native=directory/"native";wasm=directory/"wasm.js"
        subprocess.run([args.mingw,"-O2","-Wall","-Wextra","-Werror",str(fixture),"-lgdi32","-o",str(exe)],check=True)
        report["wine_fixture_sha256"]=hashlib.sha256(exe.read_bytes()).hexdigest()
        common=["-std=gnu99","-O2","-Wall","-Wextra","-Werror",f"-I{ROOT/'re_out'}",str(surface),str(fixture)]
        subprocess.run(["gcc","-m32","-no-pie","-fsanitize=address,undefined","-fno-sanitize-recover=all",*common,"-o",str(native)],check=True)
        subprocess.run(["emcc",*common,"-sNODERAWFS=1","-sEXIT_RUNTIME=1","-o",str(wasm)],check=True)
        gradient=directory/"gradient.rgb"
        gradient.write_bytes(bytes(value for y in range(192) for x in range(320) for value in (x&255,(y*7+x*5)&255,(x*13+y*11)&255)))
        inputs=[("gradient",gradient)]
        for film in source["films"]:
            for frame in film["saved_checkpoints"]:
                path=args.source/Path(film["file"]).stem.lower()/f"frame{frame:04d}.rgb"
                if hashlib.sha256(path.read_bytes()).hexdigest()!=film["frame_sha256"][frame]:raise RuntimeError("Changed source codec checkpoint")
                inputs.append((f"{Path(film['file']).stem.lower()}-{frame}",path.resolve()))
        for label,path in inputs:
            case=output/label;case.mkdir();reference=case/"wine.argb"
            actual=wine_probe(exe,case,os.environ,arguments=tuple("Z:"+str(p).replace("/","\\") for p in (path,reference)))
            shutil.rmtree(case/"wine-prefix")
            if actual!={"rectangles":4,"argb_bytes":4915200}:raise RuntimeError("Unexpected GDI extent")
            result={"label":label,"source_sha256":hashlib.sha256(path.read_bytes()).hexdigest(),"targets":[]}
            for target,command in (("native-asan-ubsan",[str(native)]),("wasm",[args.node,str(wasm)])):
                pixels=case/f"{target}.argb"
                run=subprocess.run([*command,str(path),str(pixels)],capture_output=True,text=True,timeout=30)
                (case/f"{target}.log").write_text(run.stdout+run.stderr)
                if run.returncode or json.loads(run.stdout)!=actual:raise RuntimeError(f"{target}: {run.stderr}")
                if compare(reference,pixels)!=actual["argb_bytes"]:raise RuntimeError("Wrong GDI output extent")
                with pixels.open("r+b") as file:
                    file.seek(1000);byte=file.read(1);file.seek(1000);file.write(bytes([byte[0]^1]))
                try:compare(reference,pixels)
                except RuntimeError:pass
                else:raise RuntimeError("Accepted altered GDI display bit")
                pixels.unlink();result["targets"].append(target)
            result["altered_display_bit_rejected"]=True
            result["argb_sha256"]=hashlib.sha256(reference.read_bytes()).hexdigest();reference.unlink()
            report["cases"].append(result);(output/"report.json").write_text(json.dumps(report,indent=2)+"\n")
            print(f"PASS {label}: all 4915200 GDI display bytes exact on both ports",flush=True)


if __name__=="__main__":main()
