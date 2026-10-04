#!/usr/bin/env python3
"""Compare every pit-camera highlight area and complete model guards with original x86.

Forty explicit component inputs cover all ten valid highlight areas and four
RGB intensities on the original high-detail car's polygon group layout. Execute
unmodified original machine code and the actual native/WASM engine function.
This does not accept full race, pit UI, timing or audio/video parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil

from artifacts import WORK, discard_frames, open_files, prepare_output, run_bounded

ROOT = Path(__file__).resolve().parents[1]
EXE_SHA = '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
REGIONS = ((0x781418, 28), (0x900000, 64), (0x900f00, 4608))
SNAPSHOT = sum(size for _, size in REGIONS)
RECORD = 8 + 2 * SNAPSHOT
CASES = 40


def compare(reference, actual):
    if len(reference) != CASES * RECORD or len(actual) != len(reference):
        raise ValueError('Incomplete highlight component output')
    failures = []
    for case in range(CASES):
        a = reference[case * RECORD:(case + 1) * RECORD]
        b = actual[case * RECORD:(case + 1) * RECORD]
        if a[:8 + SNAPSHOT] != b[:8 + SNAPSHOT]:
            raise ValueError('Explicit highlight inputs differ')
        if a != b:
            first = next(i for i, (x, y) in enumerate(zip(a, b)) if x != y)
            failures.append(dict(case=case, offset=first, original=a[first], port=b[first]))
    return dict(pass_=not failures, cases=CASES, failing_cases=len(failures), first_failures=failures[:12])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = prepare_output(args.output)
    if WORK not in out.parents or out.exists():
        parser.error('Use a fresh directory under /tmp/wasm-dd2/')
    out.mkdir(parents=True)
    exe = ROOT / 'DestructionDerby2/dd2h.exe'
    if hashlib.sha256(exe.read_bytes()).hexdigest() != EXE_SHA:
        raise ValueError('Supported unmodified original required')
    engine = (ROOT / 'build/dd2.c').read_text()
    matches = re.findall(r'/\* ===== Highlight_Area @ 0043b6f0 ===== \*/(.*?)(?=/\* =====|\Z)', engine, re.S)
    if len(matches) != 1:
        raise ValueError('Actual engine Highlight_Area function required')
    for name in ('dd2_symbols.h', 'ghidra_compat.h'):
        shutil.copyfile(ROOT / 'build' / name, out / name)
    (out / 'engine.c').write_text('#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n' + matches[0])
    # A finite nonempty replacement for the omitted sentinel must be rejected;
    # do not depend on undefined host stack contents to reproduce a bad list.
    mutation = matches[0].replace('local_14[0] = -1;', 'local_14[0] = 0; local_14[1] = -1;')
    if mutation == matches[0]:
        raise ValueError('Original empty-list initialization missing')
    (out / 'mutated.c').write_text('#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n' + mutation)
    data = {}
    for target in ('original-x86', 'native', 'wasm', 'native-mutated', 'wasm-mutated'):
        wasm = target.startswith('wasm')
        binary = out / (target + '.js' if wasm else target)
        compiler = ['emcc', '-mllvm', '-fast-isel=false', '-sNODERAWFS=1', '-sEXIT_RUNTIME=1', '-sGLOBAL_BASE=10485760'] if wasm else ['gcc', '-m32', '-no-pie']
        command = compiler + ['-O0', '-std=gnu99', '-w', '-Wno-incompatible-pointer-types', '-fno-strict-aliasing', '-I' + str(out),
                              str(ROOT / 'tools/reference/highlight_fixture.c')]
        command += ['-DDD2_ORIGINAL_HIGHLIGHT'] if target == 'original-x86' else [str(out / ('mutated.c' if target.endswith('-mutated') else 'engine.c'))]
        command += ['-o', str(binary)]
        with (out / (target + '-build.log')).open('w') as log:
            run_bounded(command, directory=out, timeout=120, check=True, stdout=log, stderr=subprocess.STDOUT)
        checkpoint = out / (target + '.bin')
        with (out / (target + '.log')).open('w') as log:
            run_bounded((['node', str(binary)] if wasm else [str(binary)]) + [str(exe), str(checkpoint)],
                        directory=out, timeout=30, check=True, stdout=log, stderr=subprocess.STDOUT)
        data[target] = checkpoint.read_bytes()
    results = {target: compare(data['original-x86'], raw) for target, raw in data.items() if target != 'original-x86'}
    passed = all(results[target]['pass_'] for target in ('native', 'wasm'))
    if any(results[target]['failing_cases'] != 20 for target in ('native-mutated', 'wasm-mutated')):
        raise AssertionError('Incorrect empty polygon list must fail all four colors in exactly five areas')
    damaged = bytearray(data['original-x86']); damaged[8 + SNAPSHOT] ^= 1
    if compare(data['original-x86'], damaged)['pass_']:
        raise AssertionError('Damaged model guard accepted')
    try:
        compare(data['original-x86'], data['original-x86'][:-1])
    except ValueError:
        pass
    else:
        raise AssertionError('Truncated output accepted')
    report = dict(scope=__doc__.strip(), pass_=passed, original_exe_sha256=EXE_SHA,
        cases=CASES, record_bytes=RECORD, regions=REGIONS, results=results,
        output_sha256={target: hashlib.sha256(raw).hexdigest() for target, raw in data.items()},
        engine_function_sha256=hashlib.sha256(matches[0].encode()).hexdigest(),
        damaged_guard_rejected=True, truncated_output_rejected=True)
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    if passed:
        opened = open_files()
        if any((p.stat().st_dev, p.stat().st_ino) in opened for p in out.glob('*.bin')):
            raise RuntimeError('Completed component output is still in use')
        discard_frames(out)
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
