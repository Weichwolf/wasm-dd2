#!/usr/bin/env python3
"""Compare every rendered phase of matching original/native/browser menu cycles.

No pixel mask, tolerance or best-frame search. Frames are paired by the engine's
64-step highlight counter at presentation. Wall-clock/audio timing and
other animation states are not covered by this gate.
"""
import argparse
import json
from pathlib import Path


def cycles(root, expected_input):
    navigation = json.loads((root / "navigation.json").read_text())
    if navigation.get("input") != expected_input:
        raise ValueError(f"Unexpected input provenance: {root}")
    keys, checkpoints = navigation["keys"], navigation["checkpoints"]
    if checkpoints != [f"step{i:02d}-{key or 'boot'}" for i,key in enumerate([None,*keys])]:
        raise ValueError(f"Incomplete navigation: {root}")
    frames = {}
    for checkpoint in checkpoints:
        directory = root / checkpoint / "cycle"
        cycle = json.loads((directory / "cycle.json").read_text())
        required_stage = "browser platform present" if expected_input == "browser keyboard events" else "Draw_All entry / pending presentation"
        if cycle["stage"] != required_stage:
            raise ValueError(f"Wrong rendered capture stage: {directory}")
        sequence = cycle["frames"]
        if len(sequence) != 64 or {frame["phase"] for frame in sequence} != set(range(64)):
            raise ValueError(f"Need exactly all 64 phases: {directory}")
        if any(frame["phase"] != (sequence[i-1]["phase"]+1)%64 for i,frame in enumerate(sequence) if i):
            raise ValueError(f"Cycle skipped a rendered phase: {directory}")
        for frame in sequence:
            if frame["level"] != 0 or frame["cf"] != 0:
                raise ValueError(f"Need a settled menu cycle: {directory}")
            if expected_input == "browser keyboard events" and frame.get("canvas_mismatches") != 0:
                raise ValueError(f"Browser canvas does not match the indexed capture: {directory}")
            pair = []
            for region,size in (("framebuf",307200),("palette",1024)):
                data = (directory / f"{frame['prefix']}-{region}.bin").read_bytes()
                if len(data) != size:
                    raise ValueError(f"Incomplete {region}: {directory}")
                pair.append(data)
            frames[(checkpoint,frame["phase"])] = pair
    return navigation, frames


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--native", type=Path)
    parser.add_argument("--browser", type=Path)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    if not (args.native or args.browser):
        parser.error("provide --native and/or --browser captures")
    navigation, reference = cycles(args.reference,"real X11 keys")
    if not navigation.get("acknowledged_keys"):
        parser.error("original capture must acknowledge actual press/release")
    for name in navigation["checkpoints"]:
        checkpoint = json.loads((args.reference / name / "checkpoint.json").read_text())
        if checkpoint.get("exe_modified") is not False or checkpoint.get("exe_sha256") != "0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2":
            raise ValueError("Need the supported unmodified original")
    results = []
    for target,directory,input_type in (("native",args.native,"dd2_key_event"),("browser",args.browser,"browser keyboard events")):
        if directory is None:
            continue
        actual_nav, actual = cycles(directory,input_type)
        if actual_nav["keys"] != navigation["keys"] or set(actual) != set(reference):
            raise ValueError(f"{target}: navigation/capture keys differ")
        failures = []
        for key,pair in reference.items():
            for index,region in enumerate(("framebuffer","palette")):
                if pair[index] != actual[key][index]:
                    failures.append({"checkpoint":key[0],"phase":key[1],"region":region,
                        "different_bytes":sum(a!=b for a,b in zip(pair[index],actual[key][index]))})
        result = {"target":target,"pass":not failures,"frames":len(reference),"failures":failures}
        results.append(result)
        print(f"{'PASS' if result['pass'] else 'FAIL'} {target} vs original: {len(reference)} complete framebuffer/palette comparisons; {len(failures)} mismatches")
    args.report.write_text(json.dumps({"scope":"rendered highlight cycles; full video/audio streams and timing pending", "results":results},indent=2)+"\n")
    return 0 if all(result["pass"] for result in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
