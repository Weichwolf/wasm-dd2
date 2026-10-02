#!/usr/bin/env python3
"""Check DirectSound cursor controls and exact controlled PCM on both ports.

--wine additionally compares the same stopped-buffer COM probe with real Wine
DirectSound. This API fixture is not original dd2h.exe mixed-output acceptance.
"""
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "tools/sound_cursor_test.c"


def wine_probe(executable, directory, env, *, alsa_config="pcm.!default { type null }\n", arguments=()):
    prefix = directory / "wine-prefix"
    config = directory / "asound.conf"
    config.write_text(alsa_config)
    env = {**env, "WINEPREFIX": str(prefix), "WINEARCH": "win32", "WINEDEBUG": "-all",
           "ALSA_CONFIG_PATH": str(config)}
    with (directory / "wine.log").open("wb") as log:
        display = subprocess.Popen(["Xvfb", "-displayfd", "1", "-screen", "0", "640x480x16"],
                                   stdout=subprocess.PIPE, stderr=log)
        try:
            number = display.stdout.readline().decode().strip()
            if not number:
                raise RuntimeError("Xvfb did not start for the DirectSound fixture")
            env["DISPLAY"] = ":" + number
            subprocess.run(["wineboot", "--init"], env=env, stdout=log, stderr=log,
                           check=True, timeout=60)
            subprocess.run(["wine", "reg", "add", r"HKCU\Software\Wine\Drivers",
                            "/v", "Audio", "/t", "REG_SZ", "/d", "alsa", "/f"],
                           env=env, stdout=log, stderr=log, check=True, timeout=30)
            subprocess.run(["wineserver", "-k"], env=env, check=True, timeout=10)
            subprocess.run(["wineserver", "-w"], env=env, check=True, timeout=10)
            result = subprocess.run(["wine", str(executable), *arguments], env=env, stdout=subprocess.PIPE,
                                    stderr=log, text=True, check=True, timeout=30)
            return json.loads(result.stdout)
        finally:
            subprocess.run(["wineserver", "-k"], env=env, stdout=log, stderr=log, timeout=10)
            display.terminate()
            display.wait(timeout=5)


def expected_positions():
    records = []
    for align in (1, 4):
        position = 0
        for request in (0, 1, 3, 17, 63, 64, 0xffffffff):
            valid = request < 64
            if valid:
                position = request - request % align
            records.append({"align": align, "request": request, "result": 0 if valid else 0x80070057,
                            "play": position, "write": position})
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--node", default="node")
    parser.add_argument("--emcc", default="emcc")
    parser.add_argument("--wine", action="store_true")
    parser.add_argument("--mingw", default="i686-w64-mingw32-gcc")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    if args.report and args.report.exists():
        parser.error("report already exists; use a fresh path")
    env = {key: value for key, value in os.environ.items() if not key.startswith("DD2_")}
    expected = expected_positions()
    source = [(i - 128) * 256 for i in range(256)]
    samples = source[17:39] + source[39:61] + [0]*220 + source[61:83]
    samples += source[250:] + source[:16] + source[255:] + [0]*21 + source[:22]
    pcm = b"".join(struct.pack("<ff", sample/32768, sample/32768) for sample in samples)
    reference=json.loads((ROOT/"tools/reference/sound_resample_wine10.json").read_text())
    cycle=bytes.fromhex(next(c["cycle_hex"] for c in reference["loops"] if c["frequency"]==176400))
    pcm+=cycle*22
    report = {"scope": "stopped DirectSound API fixture and controlled port PCM; full original mix parity pending",
              "positions": expected, "controlled_pcm_bytes": len(pcm), "targets": [], "wine": False}
    with tempfile.TemporaryDirectory(prefix="dd2-sound-cursor-") as tmp:
        directory = Path(tmp)
        common = ["-std=gnu89", "-w", "-DDD2_NO_FOPEN_WRAP", "-ffunction-sections", "-fdata-sections",
                  f"-I{ROOT / 're_out'}", str(ROOT / "re_out/dd2h_stubs.c"), str(SOURCE), "-Wl,--gc-sections"]
        native, wasm = directory / "native", directory / "wasm.js"
        subprocess.run(["gcc", "-m32", "-no-pie", *common, "-o", str(native)], check=True)
        subprocess.run([args.emcc, *common, "-sNODERAWFS=1", "-sEXIT_RUNTIME=1",
                        "-sGLOBAL_BASE=10485760", "-o", str(wasm)], check=True)
        # Establish real API evidence before running the port comparison.
        if args.wine:
            executable = directory / "cursor.exe"
            subprocess.run([args.mingw, "-Wall", "-Wextra", "-Werror", str(SOURCE),
                            "-ldsound", "-o", str(executable)], check=True)
            actual = wine_probe(executable, directory, env)
            if actual != expected:
                raise RuntimeError(f"Wine stopped cursor contract differs: {actual}")
            report["wine"] = True
            print("PASS real Wine DirectSound: all 14 stopped cursor records", flush=True)
        for target, command in (("native", [str(native)]), ("wasm", [args.node, str(wasm)])):
            output = directory / f"{target}.pcm"
            result = subprocess.run([*command, str(output)], env=env, capture_output=True,
                                    text=True, timeout=30)
            if result.returncode:
                raise RuntimeError(f"{target} failed ({result.returncode}): {result.stderr}")
            if json.loads(result.stdout) != expected:
                raise RuntimeError(f"{target} stopped cursor contract differs")
            if output.read_bytes() != pcm:
                raise RuntimeError(f"{target} seek/repeated Play/Stop/resume/end PCM differs")
            report["targets"].append(target)
            print(f"PASS {target}: all 14 cursor records and {len(pcm)} exact PCM bytes", flush=True)
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
