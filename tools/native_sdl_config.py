#!/usr/bin/env python3
"""Locate the 32-bit SDL headers/runtime or provision Debian headers locally.

Provisioning extracts a development package under ignored third_party; it does
not install system packages. Native runtime libraries must support i386.
"""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parent.parent
SDK=ROOT/"third_party/sdl2-sdk"


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode",choices=("cflags","libs","provision"))
    args=parser.parse_args()
    if args.mode=="provision":
        SDK.mkdir(parents=True,exist_ok=True)
        subprocess.run(["apt-get","download","libsdl2-dev:i386"],cwd=SDK,check=True)
        packages=list(SDK.glob("libsdl2-dev_*_i386.deb"))
        if not packages:raise RuntimeError("Downloaded SDL development package missing")
        package=max(packages,key=lambda path:path.stat().st_mtime_ns)
        subprocess.run(["dpkg-deb","-x",str(package),str(SDK)],check=True)
        print(f"Local 32-bit SDL headers: {SDK}")
        return
    if (SDK/"usr/include/i386-linux-gnu/SDL2/_real_SDL_config.h").is_file():
        flags=[f"-I{SDK/'usr/include'}",f"-I{SDK/'usr/include/i386-linux-gnu'}"]
    elif Path("/usr/include/i386-linux-gnu/SDL2/_real_SDL_config.h").is_file():
        flags=[]
    else:
        raise RuntimeError("32-bit SDL headers missing; run make provision-native or install libsdl2-dev:i386")
    library=Path(subprocess.check_output(["gcc","-m32","-print-file-name=libSDL2-2.0.so.0"],text=True).strip())
    if not library.is_file() or library.read_bytes()[:5]!=b"\x7fELF\x01":
        raise RuntimeError("32-bit SDL runtime missing; install libsdl2-2.0-0:i386")
    for flag in flags if args.mode=="cflags" else ["-Wl,-l:libSDL2-2.0.so.0"]:
        print(flag)


if __name__=="__main__":
    try:main()
    except (RuntimeError,subprocess.CalledProcessError) as error:
        print(error,file=sys.stderr);sys.exit(1)
