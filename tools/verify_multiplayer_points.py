#!/usr/bin/env python3
"""Compare post-result multiplayer score functions with unchanged original x86.

Explicit inputs cover human/computer result transfer, accumulation, averaging,
race-position sorting and league sorting, including ties and WORD boundaries.
Sorting inputs obey the original signed -1 sentinel precondition. Complete
targeted regions and their guards must match. This does not verify live earned
points, Calculate_Results, saving/loading, UI or chronological racing/audio.
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
FUNCTIONS = ('FUN_0044c5d8', 'Add_Computer_Info', 'Update_League_Info',
             'FUN_0044c6f8', 'Sort_RacePos', 'Sort_MultiLeague', 'FUN_0044c9c0')
MODES = ('human-copy', 'computer-add', 'accumulate', 'average', 'race-sort', 'league-sort')
COUNTS = (2496, 1344, 64, 512, 256, 64)
REGIONS = ((0x93de80, 0x600), (0x795c20, 0x200), (0x467640, 0x40))
SNAPSHOT = sum(size for _, size in REGIONS)
RECORD = 20 + 2 * SNAPSHOT
CASES = sum(COUNTS)


def compare(reference, actual):
    if len(reference) != CASES * RECORD or len(actual) != len(reference):
        raise ValueError('Incomplete score-component output')
    seen, failures, first = [0] * len(MODES), [0] * len(MODES), []
    for case in range(CASES):
        start = case * RECORD
        left, right = reference[start:start + RECORD], actual[start:start + RECORD]
        descriptor = struct.unpack_from('<5I', left)
        mode = descriptor[0]
        if mode >= len(MODES):
            raise ValueError('Invalid score operation')
        seen[mode] += 1
        if left[:20 + SNAPSHOT] != right[:20 + SNAPSHOT]:
            raise ValueError('Actual explicit score inputs differ')
        if left != right:
            failures[mode] += 1
            if len(first) < 12:
                position = next(i for i, (a, b) in enumerate(zip(left, right)) if a != b)
                offset = position - 20 - SNAPSHOT
                address = None
                for region, size in REGIONS:
                    if 0 <= offset < size:
                        address = hex(region + offset)
                        break
                    offset -= size
                first.append(dict(case=case, descriptor=descriptor, address=address,
                                  original=left[position], port=right[position]))
    if tuple(seen) != COUNTS:
        raise ValueError('Incomplete score-operation matrix')
    return dict(pass_=not any(failures), cases=CASES,
                failing_cases_by_mode=dict(zip(MODES, failures)), first_failures=first)


def negative_controls(reference):
    controls = []
    for mode in range(len(MODES)):
        case = next(i for i in range(CASES)
                    if struct.unpack_from('<I', reference, i * RECORD)[0] == mode)
        offset = 0
        for address, size in REGIONS:
            damaged = bytearray(reference)
            damaged[case * RECORD + 20 + SNAPSHOT + offset] ^= 1
            if compare(reference, damaged)['pass_']:
                raise AssertionError('Damaged score-region guard accepted')
            controls.append(dict(mode=MODES[mode], damaged_guard=hex(address)))
            offset += size
    for label, damaged in [('truncated', reference[:-1]),
                           ('changed-input', bytearray(reference))]:
        if label == 'changed-input':
            damaged[20] ^= 1
        try:
            compare(reference, damaged)
        except ValueError:
            controls.append(dict(rejected=label))
        else:
            raise AssertionError('Invalid score output accepted: ' + label)
    return controls


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = prepare_output(args.output)
    if WORK not in out.parents or out.exists():
        parser.error('Use a fresh directory under /tmp/wasm-dd2/')
    exe = ROOT / 'DestructionDerby2/dd2h.exe'
    if hashlib.sha256(exe.read_bytes()).hexdigest() != EXE_SHA:
        raise ValueError('Supported unchanged original executable required')
    out.mkdir(parents=True)
    source = out / 'source'
    source.mkdir()
    for name in ('dd2_symbols.h', 'ghidra_compat.h'):
        shutil.copy2(ROOT / 'build' / name, source / name)
    shutil.copy2(ROOT / 'tools/reference/pe_fixture.h', source / 'pe_fixture.h')
    engine = (ROOT / 'build/dd2.c').read_text()
    units = []
    for name in FUNCTIONS:
        matches = re.findall(r'/\* ===== ' + name + r' @ [0-9a-f]+ ===== \*/(.*?)(?=/\* =====|\Z)', engine, re.S)
        if len(matches) != 1:
            raise ValueError('Actual engine function missing: ' + name)
        units.append(matches[0])
    (source / 'engine.c').write_text('#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n'
                                   'void FUN_0044c9c0(int*,int);\n' + '\n'.join(units))
    shutil.copy2(ROOT / 'tools/reference/multiplayer_points_fixture.c', source / 'fixture.c')
    report = dict(scope=__doc__.strip(), pass_=False, original_exe_sha256=EXE_SHA,
                  verifier_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  functions=FUNCTIONS, regions=REGIONS, cases=CASES, record_bytes=RECORD,
                  cases_by_mode=dict(zip(MODES, COUNTS)), results={},
                  source_sha256={p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in source.iterdir()}, output_sha256={})
    data = {}
    try:
        for target in ('original-x86', 'native', 'native-asan', 'wasm'):
            wasm = target == 'wasm'
            binary = out / (target + '.js' if wasm else target)
            compiler = (['emcc', '-mllvm', '-fast-isel=false', '-sNODERAWFS=1',
                         '-sEXIT_RUNTIME=1', '-sGLOBAL_BASE=10485760'] if wasm
                        else ['gcc', '-m32', '-no-pie'])
            command = compiler + ['-O0', '-std=gnu99', '-w', '-fno-strict-aliasing',
                                   '-I' + str(source), '-I' + str(ROOT / 'tools/reference'),
                                   str(source / 'fixture.c')]
            command += (['-DDD2_ORIGINAL_MULTI_POINTS'] if target == 'original-x86'
                        else [str(source / 'engine.c')])
            if target == 'native-asan':
                command += ['-fsanitize=address', '-g']
            with (out / (target + '-build.log')).open('wb') as log:
                run_bounded(command + ['-o', str(binary)], directory=out, timeout=120,
                            check=True, stdout=log, stderr=subprocess.STDOUT)
            raw = out / (target + '.bin')
            command = (['node', str(binary)] if wasm else [str(binary)]) + [str(exe), str(raw)]
            with (out / (target + '.log')).open('wb') as log:
                run_bounded(command, directory=out, timeout=60, check=True,
                            stdout=log, stderr=subprocess.STDOUT)
            data[target] = raw.read_bytes()
            report['output_sha256'][target] = hashlib.sha256(data[target]).hexdigest()
            if target != 'original-x86':
                report['results'][target] = compare(data['original-x86'], data[target])
        report['negative_controls'] = negative_controls(data['original-x86'])
        report['pass_'] = all(r['pass_'] for r in report['results'].values())
    except Exception as error:
        report['error'] = repr(error)
        raise
    finally:
        (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(pass_=report['pass_'], results=report['results'],
                          negative_controls=len(report['negative_controls'])), indent=2))
    if report['pass_']:
        opened = open_files()
        if any((p.stat().st_dev, p.stat().st_ino) in opened for p in out.glob('*.bin')):
            raise RuntimeError('Completed score output is still in use')
        discard_frames(out)
    return 0 if report['pass_'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
