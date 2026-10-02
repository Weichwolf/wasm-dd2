#!/usr/bin/env python3
"""Compare drained/live MCI Pause/Resume modes with Wine and exact port PCM.

The one-second drain wait deliberately excludes Wine's asynchronous ring-end
timing. This is a transport-state API fixture, not original game A/V acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
from verify_sound_cursor import wine_probe

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "tools/redbook_controls_test.c"
sys.path.insert(0, str(ROOT / "tools/reference"))
from cdrom import build_cdrom


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--node", default="node")
    parser.add_argument("--emcc", default="emcc")
    parser.add_argument("--wine", action="store_true")
    parser.add_argument("--mingw", default="i686-w64-mingw32-gcc")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    env = {k: v for k, v in os.environ.items() if not k.startswith("DD2_")}
    game = ROOT / "DestructionDerby2"
    env.update(DD2_CD_ROOT=str(game / "Redbook"), DD2_SND_RATE="44100")
    labels = ["drained", "drained-pause", "drained-resume", "drained-resume-again",
              "drained-pause-again", "replay", "paused", "paused-again",
              "resumed", "resumed-again", "stopped"]
    expected = [dict(label=label, mode=mode) for label, mode in
                zip(labels, [525]*5 + [526, 529, 529, 526, 526, 525])]
    report = {"scope": __doc__, "records": expected,
              "fixture_source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
              "targets": []}
    with tempfile.TemporaryDirectory(prefix="dd2-cd-controls-") as tmp:
        directory = Path(tmp)
        output = args.output.resolve() if args.output else directory / "results"
        output.mkdir(parents=True, exist_ok=False)
        if args.wine:
            executable = output / "controls.exe"
            subprocess.run([args.mingw, "-Wall", "-Wextra", "-Werror", str(SOURCE),
                            "-lwinmm", "-o", str(executable)], check=True)
            _, libraries = build_cdrom(game, directory / "cd-libraries")
            device = directory / "cd-device"; device.touch()
            wine_env = {**env, "DD2_CD_DEVICE": str(device), "LD_PRELOAD": "dd2_cdrom.so",
                        "LD_LIBRARY_PATH": ":".join(map(str, libraries))}
            actual = wine_probe(executable, output, wine_env, cd_device=device)
            shutil.rmtree(output / "wine-prefix")
            (output / "wine-records.json").write_text(json.dumps(actual, indent=2)+"\n")
            if actual != expected:
                raise RuntimeError(f"Actual Wine MCI controls differ: {actual}")
            report["wine"] = {"records": actual,
                "executable_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
                "version": subprocess.check_output(["wine", "--version"], text=True).strip()}
            print("PASS real Wine: all 11 drained/live MCI transport records", flush=True)
        subprocess.run([sys.executable, str(ROOT / "tools/generate_cd_toc.py"),
                        str(game / "Redbook/disc.json"), str(directory / "dd2_disc.h")], check=True)
        common = ["-std=gnu99", "-w", "-DDD2_NO_FOPEN_WRAP", "-ffunction-sections", "-fdata-sections",
                  f"-I{directory}", f"-I{ROOT / 're_out'}", str(ROOT / "re_out/dd2_cd.c"),
                  str(ROOT / "re_out/dd2h_stubs.c"), str(SOURCE), "-Wl,--gc-sections"]
        native, wasm = directory / "native", directory / "wasm.js"
        subprocess.run(["gcc", "-m32", "-no-pie", *common, "-o", str(native)], check=True)
        subprocess.run([args.emcc, *common, "-sNODERAWFS=1", "-sEXIT_RUNTIME=1",
                        "-sGLOBAL_BASE=10485760", "--pre-js", str(ROOT / "tools/node_env.js"),
                        "-o", str(wasm)], check=True)
        raw = (game / "Redbook/track02.cdda").read_bytes()
        short = raw[6*44100*4:6*44100*4+8*2352]
        source = short + raw[:7938*4]
        def float_pcm(pcm):
            return b"".join(struct.pack("<ff", left/32768, right/32768)
                            for left, right in struct.iter_unpack("<hh", pcm))
        # Exact external schedule: short play, silence until replay at 1080ms,
        # 80ms live, 200ms paused, then 100ms resumed. No cursor-derived lengths.
        mixed = (float_pcm(short) + bytes((47628-4704)*8) + float_pcm(raw[:3528*4])
                 + bytes(8820*8) + float_pcm(raw[3528*4:7938*4]))
        report["controlled_source_pcm_bytes"] = len(source)
        report["controlled_source_sha256"] = hashlib.sha256(source).hexdigest()
        report["controlled_mixed_pcm_bytes"] = len(mixed)
        report["controlled_mixed_sha256"] = hashlib.sha256(mixed).hexdigest()
        for target, command in (("native", [str(native)]), ("wasm", [args.node, str(wasm)])):
            capture = output / f"{target}.pcm"
            device = output / f"{target}-mixed.pcm"
            music = output / f"{target}-music.pcm"
            result = subprocess.run(command, env={**env, "DD2_CDPCM": str(capture),
                                    "DD2_MIXPCM": str(device), "DD2_MUSICPCM": str(music)},
                                    capture_output=True, text=True, timeout=30)
            if result.returncode:
                raise RuntimeError(f"{target} failed ({result.returncode}): {result.stderr}")
            actual = json.loads(result.stdout)
            (output / f"{target}-records.json").write_text(json.dumps(actual, indent=2)+"\n")
            if actual != expected:
                raise RuntimeError(f"{target}: drained/live MCI controls differ: {actual}")
            if capture.read_bytes() != source:
                raise RuntimeError(f"{target}: drained resume or repeated live controls changed exact CD source")
            for pcm in (device, music):
                if pcm.read_bytes() != mixed:
                    raise RuntimeError(f"{target}: complete mixed/music stream differs, including pause/drain silence")
                if json.loads(Path(str(pcm)+".json").read_text()) != {
                        "format": "FLOAT_LE", "rate": 44100, "channels": 2}:
                    raise RuntimeError(f"{target}: device format metadata differs")
            report["targets"].append(target)
            print(f"PASS {target}: 11 transport records, {len(source)} source and "
                  f"{len(mixed)} complete mixed/music PCM bytes exact", flush=True)
        (output / "report.json").write_text(json.dumps(report, indent=2)+"\n")


if __name__ == "__main__":
    main()
