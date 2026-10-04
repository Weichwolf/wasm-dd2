#!/usr/bin/env python3
"""Compare copied font metadata and all 96 glyphs against unchanged original x86.

256 cases execute both original Setup_Font/Duplicate_Font call orders, all
32 page numbers, varied coordinates/CLUTs and four glyph/input patterns.
Opaque source-font page stack residue is excluded; duplicate pages and complete
copied glyph banks, source fields and guards are compared. This does not accept
live menu frames, full championships or A/V parity.
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
CASES = 256
SNAPSHOT = 2922
RECORD = 12 + 2 * SNAPSHOT


def compare(reference, actual):
    if len(reference)!=CASES*RECORD or len(actual)!=len(reference):
        raise ValueError('Incomplete copied font output')
    failures=[]
    for case in range(CASES):
        a=reference[case*RECORD:(case+1)*RECORD];b=actual[case*RECORD:(case+1)*RECORD]
        if a[:12+SNAPSHOT]!=b[:12+SNAPSHOT]:raise ValueError('Explicit copied font inputs differ')
        if a!=b:
            first=next(i for i,(x,y) in enumerate(zip(a,b)) if x!=y)
            failures.append(dict(case=case,offset=first,original=a[first],port=b[first]))
    return dict(pass_=not failures,cases=CASES,failing_cases=len(failures),first_failures=failures[:12])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=prepare_output(args.output)
    if WORK not in out.parents or out.exists():parser.error('Use a fresh /tmp/wasm-dd2/ directory')
    out.mkdir(parents=True);exe=ROOT/'DestructionDerby2/dd2h.exe'
    if hashlib.sha256(exe.read_bytes()).hexdigest()!=EXE_SHA:raise ValueError('Supported unmodified original required')
    engine=(ROOT/'build/dd2.c').read_text();functions=[]
    for name,address in [('Search_For_Sprite','004166d8'),('Setup_Sprite','0041673c'),('FUN_00416764','00416764'),('Setup_Font','004211a8'),('Duplicate_Font','004211f4')]:
        matches=re.findall(r'/\* ===== '+name+' @ '+address+r' ===== \*/(.*?)(?=/\* =====|\Z)',engine,re.S)
        if len(matches)!=1:raise ValueError('Actual engine function required: '+name)
        functions.append(matches[0])
    for name in ('dd2_symbols.h','ghidra_compat.h'):shutil.copyfile(ROOT/'build'/name,out/name)
    header='#define DD2_NO_FOPEN_WRAP\n#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n'
    body='void FUN_00416764(int,char*,short*);\n'+''.join(functions)
    if body.count('  _fd[9] = 0;')!=1:raise ValueError('Original duplicate blend residue required')
    (out/'engine.c').write_text(header+body)
    for mode in (1,2,3):
        (out/('mode'+str(mode)+'.c')).write_text(header+body.replace('  _fd[9] = 0;', '  _fd[9] = '+str(mode)+';',1))
    data={}
    for target in ('original-x86','native','native-asan','wasm','native-mode1','wasm-mode1','native-mode2','wasm-mode2','native-mode3','wasm-mode3'):
        wasm=target.startswith('wasm');binary=out/(target+'.js' if wasm else target)
        compiler=['emcc','-mllvm','-fast-isel=false','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760'] if wasm else ['gcc','-m32','-no-pie']
        command=compiler+['-O0','-std=gnu99','-w','-Wno-incompatible-pointer-types','-Wno-int-conversion','-fno-strict-aliasing','-I'+str(out),str(ROOT/'tools/reference/duplicate_font_fixture.c')]
        if target=='native-asan':command+=['-fsanitize=address']
        command+=['-DDD2_ORIGINAL_DUPLICATE_FONT'] if target=='original-x86' else [str(out/('engine.c' if target in ('native','native-asan','wasm') else target.split('-',1)[1]+'.c'))]
        command+=['-o',str(binary)]
        with (out/(target+'-build.log')).open('w') as log:run_bounded(command,directory=out,timeout=120,check=True,stdout=log,stderr=subprocess.STDOUT)
        checkpoint=out/(target+'.bin')
        with (out/(target+'.log')).open('w') as log:run_bounded((['node',str(binary)] if wasm else [str(binary)])+[str(exe),str(checkpoint)],directory=out,timeout=60,check=True,stdout=log,stderr=subprocess.STDOUT)
        data[target]=checkpoint.read_bytes()
    for case in range(CASES):
        record=data['original-x86'][case*RECORD:(case+1)*RECORD]
        page=int.from_bytes(record[4:8],'little')
        after=record[12+SNAPSHOT:]
        for font in range(3,6):
            offset=26+(font-3)*8+2
            actual=int.from_bytes(after[offset:offset+2],'little')
            if actual!=(page+font)%32:raise AssertionError('Original copied font blend residue is not zero')
    results={target:compare(data['original-x86'],raw) for target,raw in data.items() if target!='original-x86'}
    passed=all(results[target]['pass_'] for target in ('native','native-asan','wasm'))
    if any(results[target]['pass_'] for target in ('native-mode1','wasm-mode1','native-mode2','wasm-mode2','native-mode3','wasm-mode3')):raise AssertionError('Incorrect font blend mode accepted')
    damaged=bytearray(data['original-x86']);damaged[12+SNAPSHOT+58+447]^=1
    if compare(data['original-x86'],damaged)['pass_']:raise AssertionError('Damaged glyph guard accepted')
    try:compare(data['original-x86'],data['original-x86'][:-1])
    except ValueError:pass
    else:raise AssertionError('Truncated output accepted')
    report=dict(scope=__doc__.strip(),pass_=passed,original_exe_sha256=EXE_SHA,cases=CASES,record_bytes=RECORD,results=results,
        output_sha256={target:hashlib.sha256(raw).hexdigest() for target,raw in data.items()},
        original_duplicate_blend_byte_zero=True,engine_functions_sha256=hashlib.sha256(''.join(functions).encode()).hexdigest(),damaged_glyph_guard_rejected=True,truncated_output_rejected=True)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS' if passed else 'FAIL','copied font fields:',CASES,'original/native/ASan/WASM cases')
    if passed:
        opened=open_files()
        if any((p.stat().st_dev,p.stat().st_ino) in opened for p in out.glob('*.bin')):raise RuntimeError('Component output still in use')
        discard_frames(out)
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
