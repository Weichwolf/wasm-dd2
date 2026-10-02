#!/usr/bin/env python3
"""Check extracted Play_Movie parameter storage/control flow on both ports.

The independent mock checks the dd2h.exe 0x414e80..0x414f75 call contract.
Optional MinGW checks use real WinMM headers to validate 32-bit field offsets.
This does not implement an AVI decoder or establish original movie A/V parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parent.parent
FIXTURE=ROOT/"tools/movie_params_test.c"
HEADER_CHECK=r"""
#include <stddef.h>
#include <windows.h>
#include <digitalv.h>
_Static_assert(sizeof(void*)==4,"original 32-bit ABI");
_Static_assert(sizeof(MCI_OPEN_PARMSA)==20,"open block size");
_Static_assert(offsetof(MCI_OPEN_PARMSA,wDeviceID)==4,"open returned ID");
_Static_assert(offsetof(MCI_OPEN_PARMSA,lpstrDeviceType)==8,"open type");
_Static_assert(offsetof(MCI_OPEN_PARMSA,lpstrElementName)==12,"open filename");
_Static_assert(offsetof(MCI_DGV_WINDOW_PARMSA,hWnd)==4,"window handle");
_Static_assert(sizeof(MCI_DGV_RECT_PARMS)==20,"rectangle block size");
_Static_assert(offsetof(MCI_DGV_RECT_PARMS,rc.left)==4,"rectangle left");
_Static_assert(offsetof(MCI_DGV_RECT_PARMS,rc.top)==8,"rectangle top");
_Static_assert(offsetof(MCI_DGV_RECT_PARMS,rc.right)==12,"rectangle width");
_Static_assert(offsetof(MCI_DGV_RECT_PARMS,rc.bottom)==16,"rectangle height");
_Static_assert(offsetof(MCI_PLAY_PARMS,dwCallback)==0,"play notify window");
_Static_assert(offsetof(MCI_PLAY_PARMS,dwFrom)==4,"play from");
_Static_assert(offsetof(MCI_PLAY_PARMS,dwTo)==8,"play to");
_Static_assert(sizeof(MCI_GENERIC_PARMS)==4,"close block size");
"""


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine",type=Path,default=ROOT/"build/dd2.c")
    parser.add_argument("--mingw",help="also compile original ABI checks against actual WinMM headers")
    parser.add_argument("--node",default="node")
    parser.add_argument("--emcc",default="emcc")
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    engine=args.engine.read_text()
    marker=engine.index("/* ===== Play_Movie @ 00414e80 ===== */")
    begin=engine.index("void __cdecl Play_Movie(",marker)
    end=engine.index("/* ===== Load_Null",begin)
    function=engine[begin:end]
    report={"scope":__doc__,"targets":[],"engine_function_sha256":hashlib.sha256(function.encode()).hexdigest(),
            "fixture_sha256":hashlib.sha256(FIXTURE.read_bytes()).hexdigest()}
    with tempfile.TemporaryDirectory(prefix="dd2-movie-params-") as tmp:
        directory=Path(tmp)
        output=args.output.resolve() if args.output else directory/"results"
        output.mkdir(parents=True,exist_ok=False)
        (directory/"movie-function.c").write_text(function)
        if args.mingw:
            header=directory/"winmm-layout.c";header.write_text(HEADER_CHECK)
            subprocess.run([args.mingw,"-std=c11","-Wall","-Wextra","-Werror","-c",str(header),
                            "-o",str(directory/"winmm-layout.o")],check=True)
            report["winmm_header_layout"]=True
            print("PASS actual WinMM headers: original 32-bit MCI field offsets",flush=True)
        native,wasm=directory/"native",directory/"wasm.js"
        common=["-std=gnu99",f"-I{directory}",str(FIXTURE)]
        subprocess.run(["gcc","-m32","-no-pie","-fsanitize=address",*common,"-o",str(native)],check=True)
        subprocess.run([args.emcc,*common,"-sEXIT_RUNTIME=1","-o",str(wasm)],check=True)
        previous=None
        for target,command in (("native-asan",[str(native)]),("wasm",[args.node,str(wasm)])):
            run=subprocess.run(command,capture_output=True,text=True,timeout=30)
            (output/f"{target}-run.log").write_text(run.stdout+run.stderr)
            if run.returncode:
                raise RuntimeError(f"{target}: movie parameter/control contract failed: {run.stderr}")
            records=json.loads(run.stdout)
            if len(records)!=9 or (previous is not None and records!=previous):
                raise RuntimeError(f"{target}: movie contract records differ")
            previous=records;report["records"]=records;report["targets"].append(target)
            print(f"PASS {target}: nine success/completion/early-error movie call paths",flush=True)
        (output/"report.json").write_text(json.dumps(report,indent=2)+"\n")


if __name__=="__main__":
    main()
