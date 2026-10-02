#!/usr/bin/env python3
"""Verify elapsed-time DirectSound playback at fixed cf on native and WASM.

Known mono8 samples, silence and half-frequency output are compared exactly.
This checks backend clock/control behavior, not the original Windows mixer.
"""
import argparse
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--node", default="node")
    parser.add_argument("--emcc", default="emcc")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="dd2-menu-audio-") as tmp:
        directory = Path(tmp)
        common = ["-std=gnu89", "-w", "-DDD2_NO_FOPEN_WRAP", "-ffunction-sections", "-fdata-sections",
                  f"-I{ROOT / 're_out'}", str(ROOT / "re_out/dd2h_stubs.c"),
                  str(ROOT / "tools/menu_audio_test.c"), "-Wl,--gc-sections"]
        native, wasm = directory / "native", directory / "wasm.js"
        subprocess.run(["gcc", "-m32", "-no-pie", *common, "-o", str(native)], check=True)
        subprocess.run([args.emcc, *common, "-sNODERAWFS=1", "-sEXIT_RUNTIME=1",
                        "-sGLOBAL_BASE=10485760", "--pre-js", str(ROOT/"tools/node_env.js"), "-o", str(wasm)], check=True)
        source = [0, 16384, 32512, -32768]
        samples = [source[i % 4] for i in range(441)] + [0]*727
        samples += source + [0]*18
        expected = b"".join(struct.pack("<ff", value/32768, value/32768) for value in samples)
        # Measured real Wine short-loop FIR cycle, also independently recaptured
        # and compared in make verify-sound-resample --wine.
        reference=json.loads((ROOT/"tools/reference/sound_resample_wine10.json").read_text())
        cycle=bytes.fromhex(next(c["cycle_hex"] for c in reference["loops"] if c["frequency"]==11025))
        expected+=b"".join(cycle[(i%8)*8:(i%8+1)*8] for i in range(22))
        env = {key: value for key, value in os.environ.items() if not key.startswith("DD2_")}
        env["DD2_SND_RATE"]="22050" # Exercise the established menu-clock fixture device.
        for target, command in (("native", [str(native)]), ("wasm", [args.node, str(wasm)])):
            output = directory / f"{target}.pcm"
            subprocess.run([*command, str(output)], env=env, check=True, timeout=30)
            actual = output.read_bytes()
            if actual != expected:
                raise RuntimeError(f"{target}: PCM differs from expected {len(expected)} bytes (got {len(actual)})")
        print(f"Native/WASM menu audio exact for controlled 55ms: {len(expected)} bytes")


if __name__ == "__main__":
    main()
