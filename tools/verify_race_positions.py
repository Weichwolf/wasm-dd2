#!/usr/bin/env python3
"""Compare race initialization, lap timers, record loading and standings with x86.

Explicit component inputs cover all seven road level slots, all twelve slots for
standings, valid 1/2/8/20-car grids, rank permutations/ties, forward/backward
finish-line crossings, record/timer limits, dead/withdrawn cars, practice and
championship finishes, and rank refresh delays. Compare complete targeted
regions and their guards. The track table is the original's static table,
including the 999 strip-count placeholders for slots 6/7, before road loading.
This does not prove loaded track metadata, ordinary races, scores UI,
chronological A/V or whole-game original parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess

from artifacts import WORK, discard_frames, open_files, prepare_output, run_bounded

ROOT = Path(__file__).resolve().parents[1]
EXE_SHA = '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
FUNCTIONS = ('Init_End_Race', 'FUN_00443c40', 'Calc_Track_Positions', 'Get_Race_Positions', 'FUN_0044d810')
REGIONS = ((0x466dd0,0xc8),(0x4673f0,0x60),(0x467658,0x20),
           (0x792640,0x25c0),(0x795c20,0x220),(0x784290,0x10),(0x936ff0,0x10),(0x4680b0,0x260))
SNAPSHOT = sum(size for _, size in REGIONS)
RECORD = 20 + 2*SNAPSHOT
CASES = 2*31 + 7*20*12 + 12*31*12 + 7*8
PATCH = ROOT/'patches/891-race-finish-and-lap-records.diff'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def compare(reference, actual):
    if len(reference) != CASES*RECORD or len(actual) != len(reference):
        raise ValueError('Incomplete race component output')
    counts = [0]*len(FUNCTIONS)
    recorded = [0]*len(FUNCTIONS)
    failures = []
    for case in range(CASES):
        offset = case*RECORD
        a, b = reference[offset:offset+RECORD], actual[offset:offset+RECORD]
        mode, level, cars, rotation, shape = struct.unpack_from('<5I', a)
        if a[:20+SNAPSHOT] != b[:20+SNAPSHOT]:
            raise ValueError('Component inputs differ')
        if a == b:
            continue
        counts[mode] += 1
        if recorded[mode] >= 8:
            continue
        recorded[mode] += 1
        first = next(i for i,(x,y) in enumerate(zip(a,b)) if x != y)
        position, address = first-(20+SNAPSHOT), None
        for start,size in REGIONS:
            if 0 <= position < size:
                address = hex(start+position)
                break
            position -= size
        failures.append(dict(case=case, mode=mode, level=level, cars=cars,
            rotation=rotation, shape=shape, first_difference=dict(offset=first,
            address=address, original=a[first], port=b[first])))
    return dict(pass_=not any(counts), cases=CASES,
                failing_cases_by_function=dict(zip(FUNCTIONS, counts)), first_failures=failures)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = prepare_output(args.output)
    if WORK not in out.parents or out.exists():
        parser.error('Use a fresh directory under /tmp/wasm-dd2/')
    out.mkdir(parents=True)
    exe = ROOT/'DestructionDerby2/dd2h.exe'
    if digest(exe) != EXE_SHA:
        raise ValueError('Supported unmodified original required')
    sources = {}
    for variant in (('fixed','old') if PATCH.exists() else ('fixed',)):
        directory = out/variant
        directory.mkdir()
        for name in ('dd2.c','dd2_symbols.h','ghidra_compat.h'):
            shutil.copyfile(ROOT/'build'/name, directory/name)
        if variant == 'old':
            with PATCH.open() as patch:
                subprocess.run(['patch','-R','-F0','-p1'],cwd=directory,stdin=patch,
                               check=True,stdout=subprocess.DEVNULL)
        engine = (directory/'dd2.c').read_text()
        units = []
        for name in FUNCTIONS:
            matches = re.findall(r'/\* ===== '+name+r' @ [0-9a-f]+ ===== \*/(.*?)(?=/\* =====|\Z)',engine,re.S)
            if len(matches) != 1:
                raise ValueError('Actual engine function missing: '+name)
            units.append(matches[0])
        (directory/'engine.c').write_text('#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n'+'\n'.join(units))
        (directory/'dd2.c').unlink()
        sources[variant] = directory
    fixture_dir = out/'fixture'
    fixture_dir.mkdir()
    for name in ('race_position_fixture.c','pe_fixture.h'):
        shutil.copyfile(ROOT/'tools/reference'/name,fixture_dir/name)
    fixture = fixture_dir/'race_position_fixture.c'
    data, hashes = {}, {}
    targets = ['original-x86','native','native-asan','wasm'] + (['native-old','wasm-old'] if 'old' in sources else [])
    for target in targets:
        directory = sources['old' if target.endswith('-old') else 'fixed']
        wasm = target.startswith('wasm')
        binary = out/(target+'.js' if wasm else target)
        compiler = ['emcc','-mllvm','-fast-isel=false','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760'] if wasm else ['gcc','-m32','-no-pie']
        command = compiler+['-O0','-std=gnu99','-w','-fno-strict-aliasing','-I'+str(directory),str(fixture)]
        if target == 'native-asan':command += ['-fsanitize=address','-g']
        command += ['-DDD2_ORIGINAL_POSITIONS'] if target == 'original-x86' else [str(directory/'engine.c')]
        command += ['-o',str(binary)]
        with (out/(target+'-build.log')).open('w') as log:
            run_bounded(command,directory=out,timeout=120,check=True,stdout=log,stderr=subprocess.STDOUT)
        checkpoint = out/(target+'.bin')
        command = (['node',str(binary)] if wasm else [str(binary)])+[str(exe),str(checkpoint)]
        env = dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1')
        with (out/(target+'.log')).open('w') as log:
            run_bounded(command,directory=out,timeout=120,check=True,env=env,stdout=log,stderr=subprocess.STDOUT)
        data[target] = checkpoint.read_bytes()
        hashes[target] = dict(binary=digest(binary), output=digest(checkpoint),
                             **({'wasm':digest(binary.with_suffix('.wasm'))} if wasm else {}))
    results = {target:compare(data['original-x86'],raw) for target,raw in data.items() if target != 'original-x86'}
    passed = all(results[target]['pass_'] for target in ('native','native-asan','wasm'))
    if 'old' in sources:
        for target in ('native-old','wasm-old'):
            counts = results[target]['failing_cases_by_function']
            if results[target]['pass_'] or any(counts[name] for name in FUNCTIONS[:2]) or not all(counts[name] for name in FUNCTIONS[2:]):
                raise AssertionError('Old finish/record regressions not reproduced')
            if any(f['first_difference']['address'] != '0x795df4' for f in results[target]['first_failures'] if f['mode']==3):
                raise AssertionError('Old finish counter failure differs')
    damaged = bytearray(data['original-x86'])
    damaged[20+SNAPSHOT] ^= 1
    if compare(data['original-x86'],damaged)['pass_']:
        raise AssertionError('Damaged region guard accepted')
    try:
        compare(data['original-x86'],data['original-x86'][:-1])
    except ValueError:
        pass
    else:
        raise AssertionError('Truncated output accepted')
    report = dict(scope=__doc__.strip(), pass_=passed, original_port_parity='component only',
        original_exe_sha256=EXE_SHA,functions=FUNCTIONS,cases=CASES,record_bytes=RECORD,
        regions=REGIONS,results=results,hashes=hashes,
        sources={name:{file.name:digest(file) for file in directory.iterdir()} for name,directory in sources.items()},
        fixture_sources={file.name:digest(file) for file in fixture_dir.iterdir()},
        verifier_sha256=digest(Path(__file__)),patch_sha256=digest(PATCH) if PATCH.exists() else None,
        damaged_guard_rejected=True,truncated_output_rejected=True)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(pass_=passed,cases=CASES,results=results),indent=2))
    if passed:
        opened = open_files()
        if any((p.stat().st_dev,p.stat().st_ino) in opened for p in out.glob('*.bin')):
            raise RuntimeError('Completed component output is still in use')
        discard_frames(out)
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
