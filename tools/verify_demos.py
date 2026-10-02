#!/usr/bin/env python3
"""Run all ten demos and fail on timeout, nonzero exit or engine errors."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
ERROR = re.compile(r"abort|RuntimeError|exception thrown|SIGSEGV|SIGBUS|SIGFPE|FATAL", re.I)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target", choices=("native", "wasm"))
    parser.add_argument("--native", default="/tmp/dd2_native")
    parser.add_argument("--wasm", default="/tmp/lvltest/dd2run.js")
    parser.add_argument("--node", default="node")
    parser.add_argument("--game-dir", type=Path, default=ROOT / "DestructionDerby2")
    parser.add_argument("--logs", type=Path)
    parser.add_argument("--timeout", type=float, default=120)
    args = parser.parse_args()
    logs = args.logs or Path(f"/tmp/dd2-verify-{args.target}")
    logs.mkdir(parents=True, exist_ok=True)
    results = []
    for level in range(1, 11):
        env = {k: v for k, v in os.environ.items() if not k.startswith("DD2_")}
        env["DD2_LEVEL"] = str(level)
        command = ([args.native] if args.target == "native" else
                   [args.node, args.wasm, str(level)])
        log = logs / f"level{level:02d}.log"
        reason = ""
        with log.open("wb") as output:
            try:
                run = subprocess.run(command, cwd=args.game_dir, env=env,
                                     stdout=output, stderr=subprocess.STDOUT,
                                     timeout=args.timeout)
                if run.returncode:
                    reason = f"exit {run.returncode}"
            except subprocess.TimeoutExpired:
                reason = "timeout"
            except OSError as exc:
                reason = str(exc)
        text = log.read_text(errors="replace")
        if not reason and ERROR.search(text):
            reason = "engine error in log"
        if not reason and args.target == "native" and "demo returned (no crash!)" not in text:
            reason = "demo did not return"
        results.append({"level": level, "pass": not reason, "reason": reason, "log": str(log)})
        print(f"{args.target} L{level}: {'FAIL ' + reason if reason else 'ok'}", flush=True)
    (logs / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    passed = sum(result["pass"] for result in results)
    print(f"{args.target}: {passed}/10 demos returned; logs: {logs}")
    return 0 if passed == 10 else 1


if __name__ == "__main__":
    sys.exit(main())
