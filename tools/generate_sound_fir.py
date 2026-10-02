#!/usr/bin/env python3
"""Generate the observed Wine 10 Gaussian-windowed sinc mathematically.

The constants/formula are documented by Wine's DirectSound filter generator:
https://raw.githubusercontent.com/wine-mirror/wine/wine-10.0/dlls/dsound/make_fir
No Wine C source or coefficient header is copied. Decimal output is rounded to
ten places before conversion to Float32, matching that reference generator.
This calibrates Wine 10, not a Windows hardware device.
"""
import argparse
import math
from pathlib import Path

OUTPUT = Path(__file__).resolve().parent.parent / "re_out/dd2_sound_fir.h"
STEP = 120
WING = int(28 / 0.85 * STEP + 1)


def coefficients():
    bandwidth = 28 / WING
    result = []
    for index in range(2 * WING + 1):
        x = (index - WING) * math.pi * bandwidth
        sinc = math.sin(x) / x if x else 1.0
        result.append(f"{sinc * math.exp(-(x / 41)**2) * (bandwidth * STEP):.10f}")
    return result


def render():
    values = coefficients()
    rows = ["    " + ",".join(x + "f" for x in values[i:i+8]) + ","
            for i in range(0, len(values), 8)]
    return ("/* Generated mathematically by tools/generate_sound_fir.py; see that\n"
            " * script for the observed backend and filter parameters. */\n"
            f"#define DD2_FIR_STEP {STEP}\n#define DD2_FIR_LENGTH {len(values)}\n"
            f"static const float dd2_sound_fir[{len(values)}] = {{\n" + "\n".join(rows) + "\n};\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    expected = render()
    if args.check:
        if OUTPUT.read_text() != expected:
            raise SystemExit("FIR table differs; run tools/generate_sound_fir.py")
        print("PASS all 7907 FIR coefficients reproduce exactly")
    else:
        OUTPUT.write_text(expected)


if __name__ == "__main__":
    main()
