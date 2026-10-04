#!/usr/bin/env python3
"""Check actual Wine duplicate/COM lifetime semantics and exact port playback."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
from verify_sound_cursor import wine_probe
from artifacts import WORK, prepare_output
from redbook_verification import add_backend_arguments, output_directory, compile_backends, finish_report

ROOT=Path(__file__).resolve().parent.parent
SOURCE=ROOT/"tools/sound_lifetime_test.c"


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wine",action="store_true")
    parser.add_argument("--mingw",default="i686-w64-mingw32-gcc")
    parser.add_argument("--node",default="node")
    parser.add_argument("--emcc",default="emcc")
    parser.add_argument("--asan",action="store_true",help="compatibility option; ASan is always verified")
    parser.add_argument("--report",type=Path)
    parser.add_argument("--output",type=Path)
    add_backend_arguments(parser)
    args=parser.parse_args()
    if args.report and args.report.exists():parser.error('report already exists; use a fresh path')
    if args.report and WORK not in prepare_output(args.report).parents:
        parser.error('report must be inside /tmp/wasm-dd2/')
    expected={"shared_source_frames":64,"addref":2,"release_retained":1,"final_release":0}
    samples=[-12345 if i==32 else i*201-6201 for i in range(64)]
    pcm=b"".join(struct.pack("<ff",samples[(17+i)%64]/32768,samples[(17+i)%64]/32768) for i in range(441))
    env={k:v for k,v in os.environ.items() if not k.startswith("DD2_")}
    env["DD2_SND_RATE"]="44100"
    report={"scope":__doc__,"api":expected,"controlled_pcm_bytes":len(pcm),"targets":[],
            "fixture_source_sha256":hashlib.sha256(SOURCE.read_bytes()).hexdigest()}
    WORK.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="sound-lifetime-",dir=WORK) as tmp:
        directory=Path(tmp)
        output=output_directory(args,directory)
        targets,report['backend']=compile_backends(args,directory,SOURCE,ROOT/'DestructionDerby2')
        if args.wine:
            executable=directory/"lifetime.exe"
            subprocess.run([args.mingw,"-Wall","-Wextra","-Werror",str(SOURCE),"-ldsound","-o",str(executable)],check=True)
            report["fixture_exe_sha256"]=hashlib.sha256(executable.read_bytes()).hexdigest()
            report["wine_version"]=subprocess.check_output(["wine","--version"],env=env,text=True).strip()
            actual=wine_probe(executable,directory,env)
            if actual!=expected:raise RuntimeError("Actual Wine COM/source lifetime differs")
            report["wine"]=actual
            print("PASS actual Wine: source sharing, independent cursors, surviving duplicates and AddRef/Release",flush=True)
        for target,command in targets:
            capture=output/f"{target}.pcm"
            run=subprocess.run([*command,str(capture)],env=env,stdout=subprocess.PIPE,text=True,check=True,timeout=30)
            if json.loads(run.stdout)!=expected or capture.read_bytes()!=pcm:
                raise RuntimeError(f"{target}: lifetime/API/controlled PCM differs")
            report["targets"].append({"target":target,"pcm_bytes":len(pcm)})
            print(f"PASS {target}: released original, duplicate mutations and {len(pcm)} exact surviving-source PCM bytes",flush=True)
        finish_report(output,report,args.clean)
    if args.report:args.report.write_text(json.dumps(report,indent=2)+"\n")


if __name__=="__main__":main()
