#!/usr/bin/env python3
"""Check complete Print text records and both glyph-packet buffers against x86.

1,152 explicit cases cover four fonts, shader modes, six RGB colors, live color
commands including byte overflow, mid-string color changes and three alignments.
Compare records, packets, return values and all source/destination guards.
This component proof does not accept frontend frames, full menus or A/V parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

from artifacts import WORK, discard_frames, open_files, prepare_output, run_bounded

ROOT=Path(__file__).resolve().parents[1]
EXE_SHA='0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
CASES=1152
SNAPSHOT=12+16+512+96+704+704+64+4096+96
RECORD=20+2*SNAPSHOT+4


def compare(reference,actual):
    if len(reference)!=CASES*RECORD or len(actual)!=len(reference):
        raise ValueError('Incomplete Print RGB component output')
    failures=[]
    for case in range(CASES):
        a=reference[case*RECORD:(case+1)*RECORD]
        b=actual[case*RECORD:(case+1)*RECORD]
        if a[:20+SNAPSHOT]!=b[:20+SNAPSHOT]:
            raise ValueError('Explicit Print inputs differ')
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
    engine=(ROOT/'build/dd2.c').read_text();functions=[]
    for name,address in [('Print','00421404'),('FUN_00420e20','00420e20'),('FUN_004221fc','004221fc'),
                         ('FUN_00422258','00422258'),('FUN_004222b4','004222b4'),('FUN_0042231c','0042231c')]:
        matches=re.findall(r'/\* ===== '+name+' @ '+address+r' ===== \*/(.*?)(?=/\* =====|\Z)',engine,re.S)
        if len(matches)!=1:raise ValueError('Actual engine function required: '+name)
        functions.append(matches[0])
    for name in ('dd2_symbols.h','ghidra_compat.h'):
        shutil.copyfile(ROOT/'build'/name,out/name)
    header='#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n'
    prototypes=('int FUN_004222b4(int*,int,int);int FUN_004221fc(unsigned char*,int,int);'
                'int FUN_00422258(int*,int,int);void FUN_0042231c(void);'
                'void FUN_00420e20(int,unsigned short,unsigned short,unsigned char*,unsigned);'
                'void System_Error(char*,char*);\nint _local_26,_local_1e;\n')
    # Valid explicit input never reaches error handling or H/V formatting.
    # Fail unexpected paths rather than supplying invented successful effects.
    stubs='\nvoid System_Error(char*a,char*b){__builtin_trap();}int FUN_004568ee(char*a,const char*b,...){__builtin_trap();}\n'
    body=prototypes+''.join(functions)+stubs
    block='  undefined1 _print_rgb[3];\n#define local_70 (_print_rgb[0])\n#define local_6f (_print_rgb[1])\n#define local_6e (_print_rgb[2])'
    old=body.replace(block,'  undefined1 local_70;\n  undefined1 local_6f;\n  undefined1 local_6e;',1)
    if old==body:raise ValueError('Original contiguous RGB block required')
    (out/'engine.c').write_text(header+body)
    (out/'old.c').write_text(header+old)
    data={};old_asan_rejected=False
    for target in ('original-x86','native','native-asan','wasm','native-old','wasm-old','native-old-asan'):
        wasm=target.startswith('wasm');binary=out/(target+'.js' if wasm else target)
        compiler=['emcc','-mllvm','-fast-isel=false','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760'] if wasm else ['gcc','-m32','-no-pie']
        command=compiler+['-O0','-std=gnu99','-w','-Wno-incompatible-pointer-types','-fno-strict-aliasing','-I'+str(out),str(ROOT/'tools/reference/print_rgb_fixture.c')]
        if target.endswith('asan'):command+=['-fsanitize=address']
        command+=['-DDD2_ORIGINAL_PRINT_RGB'] if target=='original-x86' else [str(out/('old.c' if '-old' in target else 'engine.c'))]
        command+=['-o',str(binary)]
        with (out/(target+'-build.log')).open('w') as log:
            run_bounded(command,directory=out,timeout=120,check=True,stdout=log,stderr=subprocess.STDOUT)
        checkpoint=out/(target+'.bin');logpath=out/(target+'.log')
        with logpath.open('w') as log:
            result=run_bounded((['node',str(binary)] if wasm else [str(binary)])+[str(exe),str(checkpoint)],directory=out,timeout=60,check=target!='native-old-asan',stdout=log,stderr=subprocess.STDOUT)
        if target=='native-old-asan':
            trace=logpath.read_text()
            old_asan_rejected=result.returncode!=0 and 'AddressSanitizer: stack-buffer-overflow' in trace and 'Print' in trace and ('FUN_00420e20' in trace or 'FUN_004221fc' in trace)
        else:data[target]=checkpoint.read_bytes()
    results={target:compare(data['original-x86'],raw) for target,raw in data.items() if target!='original-x86'}
    passed=all(results[target]['pass_'] for target in ('native','native-asan','wasm'))
    if not old_asan_rejected or any(results[target]['pass_'] for target in ('native-old','wasm-old')):
        raise AssertionError('Incorrect scalar RGB layout accepted')
    damaged=bytearray(data['original-x86']);damaged[20+SNAPSHOT+28+512+96+703]^=1
    if compare(data['original-x86'],damaged)['pass_']:raise AssertionError('Damaged glyph guard accepted')
    try:compare(data['original-x86'],data['original-x86'][:-1])
    except ValueError:pass
    else:raise AssertionError('Truncated output accepted')
    report=dict(scope=__doc__.strip(),pass_=passed,original_exe_sha256=EXE_SHA,cases=CASES,record_bytes=RECORD,results=results,
        output_sha256={target:hashlib.sha256(raw).hexdigest() for target,raw in data.items()},
        engine_functions_sha256=hashlib.sha256(''.join(functions).encode()).hexdigest(),
        old_scalar_asan_rejected=old_asan_rejected,damaged_guard_rejected=True,truncated_output_rejected=True)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS' if passed else 'FAIL','Print RGB:',CASES,'original/native/ASan/WASM cases; old scalar layout rejected')
    if passed:
        opened=open_files()
        if any((p.stat().st_dev,p.stat().st_ino) in opened for p in out.glob('*.bin')):
            raise RuntimeError('Component output still in use')
        discard_frames(out)
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
