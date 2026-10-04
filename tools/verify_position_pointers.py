#!/usr/bin/env python3
"""Compare original top-three position pointer packets, ordering tables and GTE.

2,800 cases cover every car, both buffers, ranks zero through four, depth
boundaries, global/camera/car visibility gates, signed vector subtraction,
rotation and concurrent ranked cars. Native, ASan and WASM run actual patched
engine functions; the reference executes unchanged x86. This component check
does not establish complete racing video, championship or audio parity.
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
CASES = 2800
SNAPSHOT = sum((30,4,4,4,4,40,4,400,12720,576,16,288,256,32832))
RECORD = 16 + 2 * SNAPSHOT


def compare(reference, actual):
    if len(reference) != CASES * RECORD or len(actual) != len(reference):
        raise ValueError('Incomplete position pointer output')
    failures = []
    for case in range(CASES):
        a = reference[case * RECORD:(case + 1) * RECORD]
        b = actual[case * RECORD:(case + 1) * RECORD]
        if a[:16 + SNAPSHOT] != b[:16 + SNAPSHOT]:
            raise ValueError('Explicit pointer inputs differ')
        if a != b:
            first = next(i for i, (x, y) in enumerate(zip(a, b)) if x != y)
            failures.append(dict(case=case, offset=first, original=a[first], port=b[first]))
    return dict(pass_=not failures, cases=CASES, failing_cases=len(failures), first_failures=failures[:12])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(); out = prepare_output(args.output)
    if WORK not in out.parents or out.exists():
        parser.error('Use a fresh directory under /tmp/wasm-dd2/')
    out.mkdir(parents=True)
    exe = ROOT / 'DestructionDerby2/dd2h.exe'
    if hashlib.sha256(exe.read_bytes()).hexdigest() != EXE_SHA:
        raise ValueError('Supported unmodified original required')
    engine = (ROOT / 'build/dd2.c').read_text(); functions = []
    for name, address in [('Display_Position_Pointers','004300ac'), ('GTERPS','004139e7'),
                          ('gte_SetRotMatrix','004149a0'), ('FUN_00413f85','00413f85'),
                          ('FUN_00414016','00414016'), ('FUN_00414099','00414099')]:
        matches = re.findall(r'/\* ===== '+name+' @ '+address+r' ===== \*/(.*?)(?=/\* =====|\Z)',engine,re.S)
        if len(matches) != 1: raise ValueError('Actual engine function required: '+name)
        functions.append(matches[0])
    for name in ('dd2_symbols.h','ghidra_compat.h'):
        shutil.copyfile(ROOT / 'build' / name, out / name)
    header = '#define DD2_NO_FOPEN_WRAP\n#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n'
    prototypes = ('int _g_esi,_g_edi,_g_ebx,_g_ebp,_g_eax;\n'
                  'void GTERPS(void);void gte_SetRotMatrix(unsigned short*);'
                  'void FUN_00413f85(void);void FUN_00414016(void);void FUN_00414099(void);\n')
    body = prototypes + ''.join(functions)
    # Deliberately restore the diagnosed scalar rank write for a sanitizer
    # rejection. Actual projection and packet functions remain unchanged.
    old = body.replace('(&iStack_34)[iVar4] = local_24;',
                       '{ int _old_position_scalar; (&_old_position_scalar)[iVar4] = local_24; }', 1)
    if old == body or 'int _position_stack[4];' not in body:
        raise ValueError('Original contiguous rank/vector block required')
    (out / 'engine.c').write_text(header + body)
    (out / 'old.c').write_text(header + old)
    data = {}; old_asan_rejected = False
    for target in ('original-x86','native','native-asan','wasm','native-old-asan'):
        wasm = target == 'wasm'; binary = out / (target+'.js' if wasm else target)
        compiler = ['emcc','-mllvm','-fast-isel=false','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760'] if wasm else ['gcc','-m32','-no-pie']
        command = compiler + ['-O0','-std=gnu99','-w','-Wno-incompatible-pointer-types','-Wno-int-conversion','-fno-strict-aliasing','-I'+str(out),str(ROOT / 'tools/reference/position_pointers_fixture.c')]
        if target.endswith('asan'): command += ['-fsanitize=address']
        command += ['-DDD2_ORIGINAL_POSITION_POINTERS'] if target == 'original-x86' else [str(out / ('old.c' if target == 'native-old-asan' else 'engine.c'))]
        command += ['-o',str(binary)]
        with (out / (target+'-build.log')).open('w') as log:
            run_bounded(command,directory=out,timeout=120,check=True,stdout=log,stderr=subprocess.STDOUT)
        checkpoint = out / (target+'.bin'); logpath = out / (target+'.log')
        with logpath.open('w') as log:
            result = run_bounded((['node',str(binary)] if wasm else [str(binary)])+[str(exe),str(checkpoint)],directory=out,timeout=120,check=target!='native-old-asan',stdout=log,stderr=subprocess.STDOUT)
        if target == 'native-old-asan':
            trace = logpath.read_text()
            old_asan_rejected = result.returncode != 0 and 'AddressSanitizer: stack-buffer-overflow' in trace and 'Display_Position_Pointers' in trace
        else: data[target] = checkpoint.read_bytes()
    results = {target:compare(data['original-x86'],raw) for target,raw in data.items() if target!='original-x86'}
    passed = all(row['pass_'] for row in results.values())
    if not old_asan_rejected: raise AssertionError('Incorrect scalar rank write accepted')
    damaged = bytearray(data['original-x86']); damaged[16+SNAPSHOT+SNAPSHOT-1] ^= 1
    if compare(data['original-x86'],damaged)['pass_']: raise AssertionError('Damaged ordering-table guard accepted')
    try: compare(data['original-x86'],data['original-x86'][:-1])
    except ValueError: pass
    else: raise AssertionError('Truncated output accepted')
    report = dict(scope=__doc__.strip(),pass_=passed,original_exe_sha256=EXE_SHA,cases=CASES,record_bytes=RECORD,
        results=results,output_sha256={target:hashlib.sha256(raw).hexdigest() for target,raw in data.items()},
        engine_functions_sha256=hashlib.sha256(''.join(functions).encode()).hexdigest(),
        old_scalar_asan_rejected=old_asan_rejected,damaged_guard_rejected=True,truncated_output_rejected=True)
    (out / 'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS' if passed else 'FAIL','position pointers:',CASES,'original/native/ASan/WASM cases; scalar overflow rejected')
    if passed:
        opened = open_files()
        if any((p.stat().st_dev,p.stat().st_ino) in opened for p in out.glob('*.bin')):
            raise RuntimeError('Component output still in use')
        discard_frames(out)
    return 0 if passed else 1


if __name__ == '__main__': raise SystemExit(main())
