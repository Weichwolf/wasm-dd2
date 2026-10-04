#!/usr/bin/env python3
"""Check live/replay pit-camera controls against unchanged original x86.

3,600 explicit cases cover all six sections, both directions and simultaneous
presses, signed rotation/angle boundaries, and negative/zero/positive WORD
offsets. Compare camera/replay state and complete high-detail model guards.
This component proof does not accept full championship, race or A/V parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

from artifacts import WORK, discard_frames, open_files, prepare_output, run_bounded

ROOT = Path(__file__).resolve().parents[1]
EXE_SHA = '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
CASES = 3600
SNAPSHOT = 36 + 28 + 64 + 4608
RECORD = 24 + 2 * SNAPSHOT


def compare(reference, actual):
    if len(reference) != CASES * RECORD or len(actual) != len(reference):
        raise ValueError('Incomplete pit-camera component output')
    failures = []
    for case in range(CASES):
        a = reference[case * RECORD:(case + 1) * RECORD]
        b = actual[case * RECORD:(case + 1) * RECORD]
        if a[:24 + SNAPSHOT] != b[:24 + SNAPSHOT]:
            raise ValueError('Explicit pit-camera inputs differ')
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
    functions = []
    for name, address in [('Pit_Camera_Control', '00429b48'), ('SetHighLight', '00447120'), ('Highlight_Area', '0043b6f0')]:
        matches = re.findall(r'/\* ===== ' + name + ' @ ' + address + r' ===== \*/(.*?)(?=/\* =====|\Z)', engine, re.S)
        if len(matches) != 1:
            raise ValueError('Actual engine function required: ' + name)
        functions.append(matches[0])
    for name in ('dd2_symbols.h', 'ghidra_compat.h'):
        shutil.copyfile(ROOT / 'build' / name, out / name)
    header = '#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n'
    prototypes = ('void SetHighLight(unsigned,int);void Highlight_Area(int,unsigned,unsigned char);'
                  'void UndentCar(int,unsigned);void Allocate_Sound_Effect(int,int,void*);'
                  'void Record_Event(unsigned,int);void FUN_00429ac4(void);\n')
    # The explicit inputs never request repairs, pit exit or Record_Event.
    # Unexpected calls fail instead of supplying invented side effects.
    stubs = ('\nvoid UndentCar(int a,unsigned b){__builtin_trap();}'
             'void Allocate_Sound_Effect(int a,int b,void*c){__builtin_trap();}'
             'void Record_Event(unsigned a,int b){__builtin_trap();}'
             'void FUN_00429ac4(void){__builtin_trap();}\n')
    body = prototypes + ''.join(functions) + stubs
    (out / 'engine.c').write_text(header + body)
    mutations = {
        'unsigned-section': '#undef camera_section\n#define camera_section (*(unsigned*)GIMG(0x00464a84))\n' + body,
        'unsigned-offset': '#undef camera_offset\n#define camera_offset (*(unsigned short*)GIMG(0x00464a88))\n' + body,
        'wrong-left-stride': body.replace('*(short *)((byte *)&Old_Cam_Mode + camera_section * 4)', '(&Old_Cam_Mode)[camera_section * 2]'),
    }
    if mutations['wrong-left-stride'] == body:
        raise ValueError('Correct original left-offset WORD loads required')
    for name, mutation in mutations.items():
        (out / (name + '.c')).write_text(header + mutation)
    data = {}
    targets = ['original-x86', 'native', 'wasm'] + [platform + '-' + name for name in mutations for platform in ('native', 'wasm')]
    for target in targets:
        wasm = target.startswith('wasm')
        binary = out / (target + '.js' if wasm else target)
        compiler = ['emcc', '-mllvm', '-fast-isel=false', '-sNODERAWFS=1', '-sEXIT_RUNTIME=1', '-sGLOBAL_BASE=10485760'] if wasm else ['gcc', '-m32', '-no-pie']
        command = compiler + ['-O0', '-std=gnu99', '-w', '-Wno-incompatible-pointer-types', '-fno-strict-aliasing', '-I' + str(out),
                              str(ROOT / 'tools/reference/pit_camera_fixture.c')]
        command += ['-DDD2_ORIGINAL_PIT_CAMERA'] if target == 'original-x86' else [str(out / ('engine.c' if target in ('native', 'wasm') else target.split('-', 1)[1] + '.c'))]
        command += ['-o', str(binary)]
        with (out / (target + '-build.log')).open('w') as log:
            run_bounded(command, directory=out, timeout=120, check=True, stdout=log, stderr=subprocess.STDOUT)
        checkpoint = out / (target + '.bin')
        with (out / (target + '.log')).open('w') as log:
            run_bounded((['node', str(binary)] if wasm else [str(binary)]) + [str(exe), str(checkpoint)],
                        directory=out, timeout=60, check=True, stdout=log, stderr=subprocess.STDOUT)
        data[target] = checkpoint.read_bytes()
    results = {target: compare(data['original-x86'], raw) for target, raw in data.items() if target != 'original-x86'}
    passed = all(results[target]['pass_'] for target in ('native', 'wasm'))
    if any(results[target]['pass_'] for target in results if target not in ('native', 'wasm')):
        raise AssertionError('Incorrect pit-camera reconstruction was accepted')
    damaged = bytearray(data['original-x86']); damaged[24 + SNAPSHOT + 36 + 28 + 64] ^= 1
    if compare(data['original-x86'], damaged)['pass_']:
        raise AssertionError('Damaged polygon guard accepted')
    try:
        compare(data['original-x86'], data['original-x86'][:-1])
    except ValueError:
        pass
    else:
        raise AssertionError('Truncated output accepted')
    report = dict(scope=__doc__.strip(), pass_=passed, original_exe_sha256=EXE_SHA,
        cases=CASES, record_bytes=RECORD, results=results,
        output_sha256={target: hashlib.sha256(raw).hexdigest() for target, raw in data.items()},
        engine_functions_sha256=hashlib.sha256(''.join(functions).encode()).hexdigest(),
        damaged_guard_rejected=True, truncated_output_rejected=True)
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS' if passed else 'FAIL', 'pit camera:', CASES, 'original/native/WASM cases; all three mutations rejected')
    if passed:
        opened = open_files()
        if any((p.stat().st_dev, p.stat().st_ino) in opened for p in out.glob('*.bin')):
            raise RuntimeError('Completed component output is still in use')
        discard_frames(out)
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
