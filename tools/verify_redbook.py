#!/usr/bin/env python3
"""Exercise the native/WASM MCI backend and compare its entire PCM output to CDDA.

This verifies source playback with a matched clock, not the Windows hardware
mixer or complete game behavior.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emcc", default="emcc")
    parser.add_argument("--node", default="node")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="dd2-redbook-") as tmp:
        directory = Path(tmp)
        game = ROOT / "DestructionDerby2"
        subprocess.run(["python3", str(ROOT / "tools/generate_cd_toc.py"),
                        str(game / "Redbook/disc.json"), str(directory / "dd2_disc.h")], check=True)
        common = ["-std=gnu99", "-Wall", "-Wextra", "-Werror", f"-I{directory}",
                  f"-I{ROOT / 're_out'}", str(ROOT / "re_out/dd2_cd.c"),
                  str(ROOT / "tools/redbook_test.c")]
        native, wasm = directory / "native", directory / "wasm.js"
        subprocess.run(["gcc", "-m32", *common, "-o", str(native)], check=True)
        subprocess.run([args.emcc, *common, "-sNODERAWFS=1", "-sEXIT_RUNTIME=1",
                        "-o", str(wasm)], check=True)
        env = {key: value for key, value in os.environ.items() if not key.startswith("DD2_")}
        env["DD2_CD_ROOT"] = str(game / "Redbook")
        outputs = []
        for name, command in (("native", [str(native)]), ("wasm", [args.node, str(wasm)])):
            output = directory / f"{name}.pcm"
            subprocess.run([*command, str(output)], env=env, cwd=game, check=True, timeout=60)
            outputs.append(output)
        assert outputs[0].read_bytes() == outputs[1].read_bytes(), "native/WASM CD PCM differs"
        tracks = json.loads((game / "Redbook/disc.json").read_text())["tracks"][1:]
        with outputs[0].open("rb") as actual:
            for track in tracks:
                with (game / "Redbook" / track["file"]).open("rb") as source:
                    assert actual.read(8820*4) == source.read(8820*4), f"track {track['number']} prefix differs"
            with (game / "Redbook" / tracks[0]["file"]).open("rb") as source:
                while chunk := source.read(1024*1024):
                    assert actual.read(len(chunk)) == chunk, "complete track02 differs"
            assert actual.read(1) == b"", "extra samples beyond track end"
        print(f"Native/WASM CD PCM exact against original CDDA: {outputs[0].stat().st_size} bytes")


if __name__ == "__main__":
    main()
