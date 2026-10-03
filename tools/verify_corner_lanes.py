#!/usr/bin/env python3
"""Check actual FD lane aliases on Native/WASM, with the old patch as a negative.

This isolated store test does not establish complete original engine parity.
"""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

from artifacts import WORK, prepare_output, run_bounded

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    WORK.mkdir(parents=True, exist_ok=True)
    out = prepare_output(args.output or tempfile.mkdtemp(prefix='corner-lanes-', dir=WORK))
    out.mkdir(parents=True, exist_ok=True)
    results = []
    for variant in ('fixed', 'old'):
        headers = out / variant
        headers.mkdir(exist_ok=False)
        shutil.copyfile(ROOT / 'build/dd2_symbols.h', headers / 'dd2_symbols.h')
        shutil.copyfile(ROOT / 'build/ghidra_compat.h', headers / 'ghidra_compat.h')
        if variant == 'old':
            with (ROOT / 'patches/846-corner-lane-byte-stores.diff').open() as patch:
                subprocess.run(['patch', '-R', '-F0', '-p1'], cwd=headers, stdin=patch,
                               stdout=subprocess.DEVNULL, check=True)
        for target in ('native', 'wasm'):
            binary = headers / ('fixture' if target == 'native' else 'fixture.js')
            command = ['gcc', '-m32'] if target == 'native' else ['emcc', '-sEXIT_RUNTIME=1']
            if target == 'native' and variant == 'fixed':
                command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
            command += ['-I'+str(headers), str(ROOT / 'tools/corner_lane_test.c'), '-o', str(binary)]
            with (out / f'{variant}-{target}-build.log').open('w') as log:
                run_bounded(command, directory=out, timeout=90, stdout=log,
                            stderr=subprocess.STDOUT, check=True)
            with (out / f'{variant}-{target}.log').open('w') as log:
                run = run_bounded([str(binary)] if target == 'native' else ['node', str(binary)],
                                  directory=out, timeout=30, stdout=log, stderr=subprocess.STDOUT)
            message = (out / f'{variant}-{target}.log').read_text()
            expected = 0 if variant == 'fixed' else 1
            if (run.returncode != expected or
                    ('grounded_count=0' if variant == 'old' else 'eight exact BYTE stores') not in message):
                raise RuntimeError(f'{variant} {target} fixture failed: {message}')
            results.append(dict(variant=variant, target=target, exit=run.returncode, pass_=True))
    report = dict(scope=__doc__, cases=results)
    (out / 'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(f'Native/WASM lane-byte stores preserve all neighbors; old aliases rejected on both: {out}')


if __name__ == '__main__':
    main()
