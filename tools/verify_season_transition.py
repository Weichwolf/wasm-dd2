#!/usr/bin/env python3
"""Compare championship transfers, reset/sort and end-of-season with original x86.

Explicit valid 20-driver permutations cover all human league/rank positions,
point ties/order, five statistics-history slots and existing/new unlock limits.
Compare complete targeted standings/statistics/scalar regions and their guards.
This is a component check, not ordinary race finishes, UI or chronological A/V.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess

from artifacts import WORK, discard_frames, open_files, prepare_output, run_bounded

ROOT = Path(__file__).resolve().parents[1]
EXE_SHA = '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
FUNCTIONS = ('FUN_0044c9c0', 'Sort_Leagues', 'Reset_League_Info',
             'Promote_And_Relegate', 'Check_League_Standing', 'FUN_0044d594',
             'Do_End_Of_Season_Stuff')
REGIONS = ((0x93de80, 0x1c00), (0x467400, 16), (0x467658, 12), (0x4682ec, 16))
SNAPSHOT = sum(size for _, size in REGIONS)
RECORD = 24 + SNAPSHOT + 4 + SNAPSHOT
CASES = 6240
PATCH = ROOT / 'patches/852-championship-transfer-counter.diff'


def compare(reference, actual):
    if len(reference) != CASES * RECORD or len(actual) != len(reference):
        raise ValueError('Incomplete season component output')
    counts = [0] * 4
    failures = []
    for case in range(CASES):
        offset = case * RECORD
        a = reference[offset:offset + RECORD]
        b = actual[offset:offset + RECORD]
        mode, rotation, stride, shape, stats, unlocks = struct.unpack_from('<6I', a)
        if a[:24 + SNAPSHOT] != b[:24 + SNAPSHOT]:
            raise ValueError('Explicit component inputs differ')
        first = next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), None) if a != b else None
        if first is not None:
            counts[mode] += 1
            address = None
            position = first - (24 + SNAPSHOT + 4)
            for start, size in REGIONS:
                if 0 <= position < size:
                    address = hex(start + position)
                    break
                position -= size
            if len(failures) < 12:
                failures.append(dict(case=case, mode=mode, rotation=rotation,
                    stride=stride, points=shape, statistics_slot=stats, unlocks=unlocks,
                    first_difference=dict(offset=first, address=address,
                                          original=a[first], port=b[first])))
    return dict(pass_=not any(counts), cases=CASES,
                failing_cases_by_mode=dict(zip(('promote', 'reset', 'sort', 'end'), counts)),
                first_failures=failures)


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
    sources = {}
    for variant in (('fixed', 'old') if PATCH.exists() else ('fixed',)):
        directory = out / variant
        directory.mkdir()
        for name in ('dd2.c', 'dd2_symbols.h', 'ghidra_compat.h'):
            shutil.copyfile(ROOT / 'build' / name, directory / name)
        if variant == 'old':
            with PATCH.open() as patch:
                subprocess.run(['patch', '-R', '-F0', '-p1'], cwd=directory, stdin=patch,
                               check=True, stdout=subprocess.DEVNULL)
        engine = (directory / 'dd2.c').read_text()
        units = []
        for name in FUNCTIONS:
            matches = re.findall(r'/\* ===== ' + name + r' @ [0-9a-f]+ ===== \*/(.*?)(?=/\* =====|\Z)', engine, re.S)
            if len(matches) != 1:
                raise ValueError('Actual engine function missing: ' + name)
            units.append(matches[0])
        (directory / 'engine.c').write_text('#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n' + '\n'.join(units))
        (directory / 'dd2.c').unlink()
        sources[variant] = directory
    data = {}
    targets = ['original-x86', 'native', 'wasm'] + (['native-old', 'wasm-old'] if 'old' in sources else [])
    for target in targets:
        directory = sources['old' if target.endswith('-old') else 'fixed']
        wasm = target.startswith('wasm')
        binary = out / (target + '.js' if wasm else target)
        compiler = ['emcc', '-mllvm', '-fast-isel=false', '-sNODERAWFS=1', '-sEXIT_RUNTIME=1', '-sGLOBAL_BASE=10485760'] if wasm else ['gcc', '-m32', '-no-pie']
        command = compiler + ['-O0', '-std=gnu99', '-w', '-fno-strict-aliasing', '-I' + str(directory),
                              str(ROOT / 'tools/reference/season_transition_fixture.c')]
        command += ['-DDD2_ORIGINAL_SEASON'] if target == 'original-x86' else [str(directory / 'engine.c')]
        command += ['-o', str(binary)]
        with (out / (target + '-build.log')).open('w') as log:
            run_bounded(command, directory=out, timeout=120, check=True, stdout=log, stderr=subprocess.STDOUT)
        checkpoint = out / (target + '.bin')
        command = (['node', str(binary)] if wasm else [str(binary)]) + [str(exe), str(checkpoint)]
        with (out / (target + '.log')).open('w') as log:
            run_bounded(command, directory=out, timeout=60, check=True, stdout=log, stderr=subprocess.STDOUT)
        data[target] = checkpoint.read_bytes()
    results = {target: compare(data['original-x86'], raw) for target, raw in data.items() if target != 'original-x86'}
    passed = all(results[target]['pass_'] for target in ('native', 'wasm'))
    old_overwrite = {}
    if 'old' in sources:
        for target in ('native-old', 'wasm-old'):
            if results[target]['failing_cases_by_mode'] != dict(promote=480, reset=0, sort=0, end=4320):
                raise AssertionError('Old transfer/continuing-season failure was not reproduced')
            address = 0x93def2 + 20 * 54
            before = 24 + address - REGIONS[0][0]
            after = 24 + SNAPSHOT + 4 + address - REGIONS[0][0]
            original = data['original-x86'][after:after + 4]
            old = data[target][after:after + 4]
            if original != data['original-x86'][before:before + 4] or old != b'\x01\x00\x00\x00':
                raise AssertionError('Original guard/old car-index-20 overwrite not observed')
            old_overwrite[target] = dict(address=hex(address), original=original.hex(), port=old.hex())
    damaged = bytearray(data['original-x86'])
    damaged[24 + SNAPSHOT + 4] ^= 1
    if compare(data['original-x86'], damaged)['pass_']:
        raise AssertionError('Damaged region guard accepted')
    try:
        compare(data['original-x86'], data['original-x86'][:-1])
    except ValueError:
        pass
    else:
        raise AssertionError('Truncated output accepted')
    report = dict(scope=__doc__.strip(), pass_=passed, original_exe_sha256=EXE_SHA,
        functions=FUNCTIONS, cases=CASES, record_bytes=RECORD, regions=REGIONS,
        results=results, output_sha256={target: hashlib.sha256(raw).hexdigest() for target, raw in data.items()},
        old_car_index_20_overwrite=old_overwrite,
        damaged_guard_rejected=True, truncated_output_rejected=True)
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: value for key, value in report.items() if key != 'output_sha256'}, indent=2))
    if passed:
        opened = open_files()
        if any((p.stat().st_dev, p.stat().st_ino) in opened for p in out.glob('*.bin')):
            raise RuntimeError('Completed component output is still in use')
        discard_frames(out)
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
