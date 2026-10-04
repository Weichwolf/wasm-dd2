#!/usr/bin/env python3
"""Check the numeric Print parser against unchanged original x86.

849 cases cover every first-byte value, three text starting positions, signed
numbers, byte/int boundaries, delimiters and the original ten-character bound.
Compare actual values, returned cursors and all source/destination guards.
This component check does not accept full text rendering or menu/A/V parity.
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
CASES = 849
SNAPSHOT = 128
RECORD = 12 + 2 * SNAPSHOT + 4


def compare(reference, actual):
    if len(reference)!=CASES*RECORD or len(actual)!=len(reference):
        raise ValueError('Incomplete numeric parser output')
    failures=[]
    for case in range(CASES):
        a=reference[case*RECORD:(case+1)*RECORD]
        b=actual[case*RECORD:(case+1)*RECORD]
        if a[:12+SNAPSHOT]!=b[:12+SNAPSHOT]:
            raise ValueError('Explicit numeric parser inputs differ')
        if a!=b:
            first=next(i for i,(x,y) in enumerate(zip(a,b)) if x!=y)
            failures.append(dict(case=case,offset=first,original=a[first],port=b[first]))
    return dict(pass_=not failures,cases=CASES,failing_cases=len(failures),first_failures=failures[:12])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=prepare_output(args.output)
    if WORK not in out.parents or out.exists():
        parser.error('Use a fresh directory under /tmp/wasm-dd2/')
    out.mkdir(parents=True)
    exe=ROOT/'DestructionDerby2/dd2h.exe'
    if hashlib.sha256(exe.read_bytes()).hexdigest()!=EXE_SHA:
        raise ValueError('Supported unmodified original required')
    matches=re.findall(r'/\* ===== FUN_004222b4 @ 004222b4 ===== \*/(.*?)(?=/\* =====|\Z)',(ROOT/'build/dd2.c').read_text(),re.S)
    if len(matches)!=1:raise ValueError('Actual numeric parser required')
    for name in ('dd2_symbols.h','ghidra_compat.h'):
        shutil.copyfile(ROOT/'build'/name,out/name)
    header='#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n'
    (out/'engine.c').write_text(header+matches[0])
    (out/'wrong-stride.c').write_text(header+'#undef _IsTable\n#define _IsTable (*(int*)GIMG(0x0046fdb0))\n'+matches[0])
    data={}
    for target in ('original-x86','native','native-asan','wasm','native-wrong-stride','wasm-wrong-stride'):
        wasm=target.startswith('wasm');binary=out/(target+'.js' if wasm else target)
        compiler=['emcc','-mllvm','-fast-isel=false','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760'] if wasm else ['gcc','-m32','-no-pie']
        command=compiler+['-O0','-std=gnu99','-w','-Wno-incompatible-pointer-types','-fno-strict-aliasing','-I'+str(out),str(ROOT/'tools/reference/print_number_fixture.c')]
        if target=='native-asan':command+=['-fsanitize=address']
        command+=['-DDD2_ORIGINAL_PRINT_NUMBER'] if target=='original-x86' else [str(out/('wrong-stride.c' if target.endswith('wrong-stride') else 'engine.c'))]
        command+=['-o',str(binary)]
        with (out/(target+'-build.log')).open('w') as log:
            run_bounded(command,directory=out,timeout=120,check=True,stdout=log,stderr=subprocess.STDOUT)
        checkpoint=out/(target+'.bin')
        with (out/(target+'.log')).open('w') as log:
            run_bounded((['node',str(binary)] if wasm else [str(binary)])+[str(exe),str(checkpoint)],directory=out,timeout=60,check=True,stdout=log,stderr=subprocess.STDOUT)
        data[target]=checkpoint.read_bytes()
    results={target:compare(data['original-x86'],raw) for target,raw in data.items() if target!='original-x86'}
    passed=all(results[target]['pass_'] for target in ('native','native-asan','wasm'))
    if any(results[target]['pass_'] for target in ('native-wrong-stride','wasm-wrong-stride')):
        raise AssertionError('Incorrect character table stride accepted')
    # Assert a literal original decimal result as well as relative comparisons.
    decimal=data['original-x86'][49*3*RECORD:(49*3+1)*RECORD]
    if struct.unpack_from('<i',decimal,12+SNAPSHOT+96+16)[0]!=117 or struct.unpack_from('<i',decimal,RECORD-4)[0]!=4:
        raise AssertionError('Original decimal value/cursor was not exercised')
    damaged=bytearray(data['original-x86']);damaged[12+SNAPSHOT+96]^=1
    if compare(data['original-x86'],damaged)['pass_']:raise AssertionError('Damaged output guard accepted')
    try:compare(data['original-x86'],data['original-x86'][:-1])
    except ValueError:pass
    else:raise AssertionError('Truncated output accepted')
    report=dict(scope=__doc__.strip(),pass_=passed,original_exe_sha256=EXE_SHA,cases=CASES,record_bytes=RECORD,results=results,
        output_sha256={target:hashlib.sha256(raw).hexdigest() for target,raw in data.items()},
        engine_function_sha256=hashlib.sha256(matches[0].encode()).hexdigest(),damaged_guard_rejected=True,truncated_output_rejected=True)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS' if passed else 'FAIL','numeric text parser:',CASES,'original/native/ASan/WASM cases; wrong-stride variants rejected')
    if passed:
        opened=open_files()
        if any((p.stat().st_dev,p.stat().st_ino) in opened for p in out.glob('*.bin')):
            raise RuntimeError('Component output still in use')
        discard_frames(out)
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
