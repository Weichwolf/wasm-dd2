#!/usr/bin/env python3
"""Compare every presented indexed frame, palette, flip/RNG log and PCM byte.

This verifies native/WASM demo parity. It does not compare with the Windows original.
Successful captures are deleted; failed captures remain beside the result log.
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", default="/tmp/dd2_native")
    parser.add_argument("--wasm", default="/tmp/lvltest/dd2run.js")
    parser.add_argument("--node", default="node")
    parser.add_argument("--levels", type=int, nargs="+", default=list(range(1, 11)))
    parser.add_argument("--logs", type=Path, default=Path("/tmp/dd2-parity"))
    parser.add_argument("--timeout", type=float, default=180)
    args = parser.parse_args()
    if any(level not in range(1, 11) for level in args.levels):
        parser.error("levels must be between 1 and 10")
    args.logs.mkdir(parents=True, exist_ok=True)
    results = []
    for level in args.levels:
        capture = Path(tempfile.mkdtemp(prefix=f"level{level:02d}-", dir=args.logs))
        failures = []
        for target in ("native", "wasm"):
            frames = capture / target
            frames.mkdir()
            env = {k: v for k, v in os.environ.items() if not k.startswith("DD2_")}
            env.update(DD2_LEVEL=str(level), DD2_SOUND="1", DD2_FRAMEDIR=str(frames),
                       DD2_PALDUMP="1", DD2_FLIPLOG="1", DD2_SNDPCM=str(frames / "audio.pcm"),
                       DD2_CDPCM=str(frames / "redbook.pcm"),
                       DD2_MIXPCM=str(frames / "mixed.pcm"), DD2_MUSICPCM=str(frames / "music.pcm"))
            cmd = [args.native] if target == "native" else [args.node, args.wasm, str(level)]
            with (capture / f"{target}.log").open("wb") as output:
                try:
                    run = subprocess.run(cmd, cwd=ROOT / "DestructionDerby2", env=env,
                                         stdout=output, stderr=subprocess.STDOUT, timeout=args.timeout)
                    if run.returncode:
                        failures.append(f"{target} exit {run.returncode}")
                except (OSError, subprocess.TimeoutExpired) as exc:
                    failures.append(f"{target}: {exc}")
        native = capture / "native"
        wasm = capture / "wasm"
        files = {p.name for p in native.iterdir()}
        others = {p.name for p in wasm.iterdir()}
        if files != others:
            failures.append(f"file sets differ: {sorted(files ^ others)[:10]}")
        frame_count = sum(name.startswith("f") and name.endswith(".bin") for name in files)
        if not frame_count:
            failures.append("no video frames")
        if sum(name.startswith("f") and name.endswith(".pal") for name in files) != frame_count:
            failures.append("per-frame palettes missing")
        if "audio.pcm" not in files or not (native / "audio.pcm").stat().st_size:
            failures.append("PCM output missing/empty")
        if "redbook.pcm" not in files or not (native / "redbook.pcm").stat().st_size:
            failures.append("CD PCM output missing/empty")
        for name in ("mixed.pcm", "music.pcm"):
            if name not in files or not (native / name).stat().st_size:
                failures.append(f"{name} missing/empty")
            if name+".json" not in files:
                failures.append(f"{name} format metadata missing")
        for name in sorted(files & others):
            # Compare bytes directly, without image conversions or approximate thresholds.
            with (native / name).open("rb") as left, (wasm / name).open("rb") as right:
                offset = 0
                while True:
                    a, b = left.read(65536), right.read(65536)
                    if a != b:
                        failures.append(f"{name}: byte difference in block at {offset}")
                        break
                    if not a:
                        break
                    offset += len(a)
        result = {"level": level, "pass": not failures, "frames": frame_count,
                  "pcm_bytes": (native / "audio.pcm").stat().st_size if "audio.pcm" in files else 0,
                  "cd_pcm_bytes": (native / "redbook.pcm").stat().st_size if "redbook.pcm" in files else 0,
                  "mixed_pcm_bytes": (native / "mixed.pcm").stat().st_size if "mixed.pcm" in files else 0,
                  "music_pcm_bytes": (native / "music.pcm").stat().st_size if "music.pcm" in files else 0,
                  "failures": failures, "captures": str(capture) if failures else None}
        results.append(result)
        if not failures:
            # Keep logs, but avoid retaining gigabytes of identical frame captures.
            for target in ("native", "wasm"):
                shutil.copyfile(capture / f"{target}.log", args.logs / f"level{level:02d}-{target}.log")
            shutil.rmtree(capture)
        print(f"L{level}: {'FAIL' if failures else 'exact'}; {frame_count} frames, "
              f"{result['pcm_bytes']} PCM bytes; {failures[:3]}", flush=True)
        (args.logs / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return 0 if all(result["pass"] for result in results) else 1


if __name__ == "__main__":
    sys.exit(main())
