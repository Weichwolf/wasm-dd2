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
        common = ["-std=gnu99", "-w", "-DDD2_NO_FOPEN_WRAP", "-ffunction-sections", "-fdata-sections", f"-I{directory}",
                  f"-I{ROOT / 're_out'}", str(ROOT / "re_out/dd2_cd.c"),
                  str(ROOT / "re_out/dd2h_stubs.c"),str(ROOT / "tools/redbook_test.c"),"-Wl,--gc-sections"]
        native, wasm = directory / "native", directory / "wasm.js"
        subprocess.run(["gcc", "-m32", "-no-pie", *common, "-o", str(native)], check=True)
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
            for index,track in enumerate(tracks):
                # Controls occur on the common 44100Hz device grid. A 101ms
                # interval and the 99ms resume after 777ms silence can consume
                # 8819/8820/8821 frames depending on that grid's fractional phase.
                # This is calculated from the explicit external schedule, not
                # inferred from the port cursor or accepted output length.
                start=index*977
                prefix_frames=((start+101)*44100//1000-start*44100//1000 +
                               (start+977)*44100//1000-(start+878)*44100//1000)
                with (game / "Redbook" / track["file"]).open("rb") as source:
                    assert actual.read(prefix_frames*4) == source.read(prefix_frames*4), f"track {track['number']} prefix differs"
            with (game / "Redbook" / tracks[0]["file"]).open("rb") as source:
                while chunk := source.read(1024*1024):
                    assert actual.read(len(chunk)) == chunk, "complete track02 differs"
            began=18*977+500000
            resumed=began+73+911
            paused_frames=((began+73)*44100//1000-began*44100//1000 +
                           (resumed+27)*44100//1000-resumed*44100//1000)
            with (game / "Redbook" / tracks[1]["file"]).open("rb") as source:
                assert actual.read(paused_frames*4)==source.read(paused_frames*4), "MCI pause/resume source differs"
            with (game / "Redbook" / tracks[0]["file"]).open("rb") as source:
                source.seek(-2*2352,os.SEEK_END)
                assert actual.read(2*2352)==source.read(), "track02 boundary suffix differs"
            with (game / "Redbook" / tracks[1]["file"]).open("rb") as source:
                assert actual.read(2*2352)==source.read(2*2352), "track03 boundary prefix differs"
            assert actual.read(1) == b"", "extra samples beyond track end"
        print(f"Native/WASM CD PCM exact against original CDDA: {outputs[0].stat().st_size} bytes")


if __name__ == "__main__":
    main()
