#!/usr/bin/env python3
"""Check engine naming on both targets and optional score text against a live original capture.

Naming fixtures cover multiple human counts. The optional comparison reuses
original standings as input to the real score builder; it is not a live native race.
"""
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
NAMES = ["The Master", "The Trashman", "The Skum", "The Pro", "The Goddess", "Learner Driver",
         "Psycho", "The Chief", "The Optician", "The General", "Heavy Metal H.", "Barmy Army",
         "Pyromaniac", "The Beast", "Passion Wagon", "The Undertaker", "Suicide Squad", "The Bouncer", "Rivit"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path)
    parser.add_argument("--browser", type=Path, help="optional live browser scores.json to compare with original")
    parser.add_argument("--node", default="node")
    parser.add_argument("--emcc", default="emcc")
    args = parser.parse_args()
    reference, ref_directories, ref_image = {}, {}, None
    if args.reference:
        navigation = json.loads((args.reference / "navigation.json").read_text())
        if navigation.get("input") != "real X11 keys" or navigation.get("acknowledged_keys") is not True:
            raise ValueError("Need completed original navigation with acknowledged real key input")
        held = navigation.get("held_pad_polls", [])
        if len(held) != len(navigation["keys"]) or not all(isinstance(count, int) and count >= 1 for count in held):
            raise ValueError("Need a held-poll count for every original key action")
        for directory in sorted(args.reference.glob("step*")):
            if not (directory / "image.bin").exists():
                continue
            image = (directory / "image.bin").read_bytes()
            checkpoint = json.loads((directory / "checkpoint.json").read_text())
            if checkpoint.get("exe_modified") is not False or checkpoint.get("exe_sha256") != "0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2" or checkpoint.get("phase") != "Draw_All entry @0x420c9c":
                raise ValueError("Need a supported unmodified original checkpoint")
            if len(image) != 0x580400:
                raise ValueError("Incomplete original image")
            division = struct.unpack_from("<i", image, 0x46ad00-0x400000)[0]
            read = lambda a, n: image[a-0x400000:a-0x400000+n].split(b"\0")[0].decode("ascii")
            rows = [{"name": read(0x940290+i*26,26), "points": read(0x940240+i*16,16)} for i in range(5)]
            if division in range(4) and all(row["name"].startswith("%R%JL%T/") and row["points"].startswith("%R%JL%T/") for row in rows):
                reference[division] = rows
                ref_directories[division] = directory
                ref_image = directory / "image.bin"
        if set(reference) != set(range(4)):
            raise ValueError("Need populated original captures of all four score divisions")
    elif args.browser:
        parser.error("--browser requires --reference")
    expected = b"".join(name.encode().ljust(54,b"\0") for humans in (1,2,5,10)
                        for name in [*[f"PLAYER{i+1}" for i in range(humans)], *NAMES[humans-1:]])
    with tempfile.TemporaryDirectory(prefix="dd2-champ-builders-") as tmp:
        directory = Path(tmp)
        common = ["-std=gnu89", "-w", "-DDD2_NO_FOPEN_WRAP", "-ffunction-sections", "-fdata-sections",
                  "-Wno-int-conversion", "-Wno-incompatible-pointer-types", "-Wno-implicit-function-declaration",
                  "-Wno-builtin-declaration-mismatch", "-Wno-return-type", "-Wno-return-mismatch",
                  f"-I{ROOT / 'build'}", str(ROOT / "build/dd2.c"), str(ROOT / "build/dd2h_stubs.c"), str(ROOT / "build/dd2_filio.c"),
                  str(ROOT / "tools/champ_names_test.c"), "-Wl,--gc-sections"]
        native, wasm = directory / "native", directory / "wasm.js"
        subprocess.run(["gcc", "-m32", "-no-pie", *common, "-o", str(native)], check=True)
        subprocess.run([args.emcc, "-mllvm", "-fast-isel=false", *common, "-sNODERAWFS=1",
                        "-sGLOBAL_BASE=10485760", "-sEXIT_RUNTIME=1", "-o", str(wasm)], check=True)
        env = {key: value for key, value in os.environ.items() if not key.startswith("DD2_")}
        for target, command in (("native", [str(native)]), ("wasm", [args.node, str(wasm)])):
            output, scores = directory / f"{target}.bin", directory / f"{target}-scores.bin"
            command += [str(ROOT / "DestructionDerby2/dd2_image.bin"), str(output)]
            if ref_image:
                command += [str(ref_image), str(scores)]
            subprocess.run(command, env=env, check=True, timeout=60)
            if output.read_bytes() != expected:
                raise RuntimeError(f"{target}: incorrect driver-name records")
            if ref_image:
                expected_scores = b"".join(row["name"].encode().ljust(26,b"\0") + row["points"].encode().ljust(16,b"\0")
                                           for division in range(4) for row in reference[division])
                if scores.read_bytes() != expected_scores:
                    raise RuntimeError(f"{target}: score builder differs from original strings")
            print(f"PASS {target}: complete name records for 1/2/5/10 humans" +
                  ("; all 20 original score/name strings exact with reference input" if ref_image else ""))
    if args.browser:
        browser = json.loads(args.browser.read_text())
        if len(browser) != 4 or any(item["division"] != i or item["rows"] != reference[i] for i,item in enumerate(browser)):
            raise RuntimeError("Live browser names/points differ from original")
        print("PASS live browser vs original: all 20 names/points exact after championship Retire/Yes")
        captures = args.browser.parent
        for division in range(4):
            for name, filename, size in (("framebuffer", "framebuf.bin", 307200), ("palette", "palette.bin", 1024)):
                original = (ref_directories[division] / filename).read_bytes()
                port = (captures / f"division{division}-{name}.bin").read_bytes()
                if len(original) != size or port != original:
                    raise RuntimeError(f"Live browser division {division}: {name} bytes differ")
        print("PASS live browser vs original: all 4 score pages framebuffer/palette byte-exact")


if __name__ == "__main__":
    main()
