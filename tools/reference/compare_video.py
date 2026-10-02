#!/usr/bin/env python3
"""Compare native/WASM bytes with original L9 Draw_All checkpoints.

Accept one capture or capture.py's --frames manifest. Each port runs once,
capturing all requested counters. This is video/palette checkpoint evidence,
not full original fidelity or audio verification. Failed captures are retained.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BASE = 0x400000
REGIONS = (("framebuffer", 0x700450, 307200, "framebuf.bin"),
           ("palette", 0x700050, 1024, "palette.bin"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--native", default="/tmp/dd2_native")
    parser.add_argument("--wasm", default="/tmp/lvltest/dd2run.js")
    parser.add_argument("--node", default="node")
    parser.add_argument("--timeout", type=float, default=120)
    args = parser.parse_args()
    manifest = args.capture / "video-checkpoints.json"
    references = []
    if manifest.exists():
        document = json.loads(manifest.read_text())
        items = document["checkpoints"]
        if not items or document.get("requested_frames") != [item["requested_cf"] for item in items]:
            parser.error("incomplete original checkpoint manifest")
        for item in items:
            directory = (args.capture / item["directory"]).resolve()
            if directory.parent != args.capture.resolve():
                parser.error("checkpoint directories must be direct children of the capture")
            checkpoint = json.loads((directory / "checkpoint.json").read_text())
            if checkpoint != item["checkpoint"] or checkpoint["cf"] < item["requested_cf"]:
                parser.error("checkpoint metadata differs from the original manifest")
            references.append(directory)
    else:
        references.append(args.capture)
    checkpoints = [json.loads((directory / "checkpoint.json").read_text()) for directory in references]
    if not checkpoints:
        parser.error("capture has no checkpoints")
    for checkpoint in checkpoints:
        if (checkpoint["level"] != 9 or not isinstance(checkpoint["cf"], int) or not 1 <= checkpoint["cf"] <= 700 or
                checkpoint["phase"] != "Draw_All entry @0x420c9c"):
            parser.error("expected L9 Draw_All checkpoints within cf1..700")
    frames = sorted({checkpoint["cf"] for checkpoint in checkpoints})
    capture = Path(tempfile.mkdtemp(prefix="dd2-ref-video-"))
    results = []
    for target, command in (("native", [args.native]), ("wasm", [args.node, args.wasm, "9"])):
        directory = capture / target
        directory.mkdir()
        env = {key: value for key, value in os.environ.items() if not key.startswith("DD2_")}
        env.update(DD2_LEVEL="9", DD2_SOUND="1", DD2_IMGDUMP=",".join(map(str,frames)),
                   DD2_FRAMEDIR=str(directory), DD2_CFONLY="1")
        run_failures = []
        log = capture / f"{target}.log"
        with log.open("wb") as output:
            try:
                process = subprocess.run(command, cwd=ROOT / "DestructionDerby2", env=env,
                    stdout=output, stderr=subprocess.STDOUT, timeout=args.timeout)
                if process.returncode:
                    run_failures.append(f"exit {process.returncode}")
            except (OSError, subprocess.TimeoutExpired) as error:
                run_failures.append(str(error))
        if re.search(r"abort|RuntimeError|SIGSEGV|SIGBUS|SIGFPE|FATAL", log.read_text(errors="replace"), re.I):
            run_failures.append("engine error in log")
        for reference_dir, checkpoint in zip(references, checkpoints):
            frame = checkpoint["cf"]
            failures = list(run_failures)
            image = directory / f"img{frame:05d}_0.bin"
            if not image.exists() or image.stat().st_size != 0x580400:
                failures.append("first Draw_All image capture missing/incomplete")
            else:
                data = image.read_bytes()
                for name, address, size, filename in REGIONS:
                    reference = (reference_dir / filename).read_bytes()
                    if len(reference) != size:
                        failures.append(f"incomplete original {name}")
                    elif data[address - BASE:address - BASE + size] != reference:
                        failures.append(f"{name} bytes differ")
            results.append({"target": target, "pass": not failures, "cf": frame, "failures": failures})
            print(f"{target} vs original L9/cf{frame}: {'FAIL' if failures else 'framebuffer and palette exact'} {failures}", flush=True)
    report = args.capture / "video-comparison.json"
    report.write_text(json.dumps({"scope": "video checkpoints; complete original video/audio streams pending", "results": results,
                                 "logs": str(capture)}, indent=2) + "\n")
    success = all(result["pass"] for result in results)
    if success:
        for directory in (capture / "native", capture / "wasm"):
            shutil.rmtree(directory)
    print(f"Video comparison: {report}")
    return 0 if success else 1


if __name__ == "__main__":
    raise SystemExit(main())
