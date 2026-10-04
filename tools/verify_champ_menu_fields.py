#!/usr/bin/env python3
"""Compare championship menu packed coordinates, line lengths and adjacent bytes.

504 explicit cases cover six race types, all human league/rank positions and
four neighboring-byte patterns. The reference executes the unchanged x86
season-result helper and observes the bounded Front_End cross-coordinate
instruction branch with CPU single stepping. Ports use the actual patched
helper and the corresponding extracted Front_End branch. This field-level
comparison does not establish live menu frames or complete championship parity.
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
CASES = 504
SNAPSHOT = 152
RECORD = 20 + 2 * SNAPSHOT


def compare(reference, actual):
    if len(reference)!=CASES*RECORD or len(actual)!=len(reference):
        raise ValueError('Incomplete championship menu field output')
    failures=[]
    for case in range(CASES):
        a=reference[case*RECORD:(case+1)*RECORD];b=actual[case*RECORD:(case+1)*RECORD]
        if a[:20+SNAPSHOT]!=b[:20+SNAPSHOT]:raise ValueError('Explicit menu fields differ')
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
    for name,address in [('FUN_004549c4','004549c4'),('Check_League_Standing','0044c92c')]:
        matches=re.findall(r'/\* ===== '+name+' @ '+address+r' ===== \*/(.*?)(?=/\* =====|\Z)',engine,re.S)
        if len(matches)!=1:raise ValueError('Actual engine function required: '+name)
        functions.append(matches[0])
    front=re.findall(r'/\* ===== Front_End @ 004502a8 ===== \*/(.*?)(?=/\* =====|\Z)',engine,re.S)
    if len(front)!=1:raise ValueError('Actual frontend required')
    branch=re.search(r'      if \(\(race_type == 4\) \|\| \(race_type == 3\)\) \{\n.*?\n      \}\n      else \{\n.*?\n      \}',front[0],re.S)
    if branch is None:raise ValueError('Actual locked-track coordinate branch required')
    functions.append('void DD2_Cross_Fields(void) {\n'+branch[0]+'\n}\n')
    symbols=(ROOT/'build/dd2_symbols.h').read_text()
    for name in ('dd2_symbols.h','ghidra_compat.h'):shutil.copyfile(ROOT/'build'/name,out/name)
    header='#define DD2_NO_FOPEN_WRAP\n#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n'
    body='unsigned Check_League_Standing(void);\n'+''.join(functions)
    (out/'engine.c').write_text(header+body)
    mutations={}
    for kind,names in [('cross',('_DAT_0046968c','_DAT_0046968e')),('line',('_DAT_0046ae26','_DAT_0046ae32'))]:
        defines=''
        for name in names:
            address=name[len('_DAT_'):]
            expected=f'#define {name} (*(unsigned short*)GIMG(0x{address}))'
            if expected not in symbols:raise ValueError('Actual original WORD field required: '+name)
            defines+=f'#undef {name}\n#define {name} (*(int*)GIMG(0x{address}))\n'
        mutations[kind]=header+defines+body;(out/(kind+'.c')).write_text(mutations[kind])
    data={}
    for target in ('original-x86','native','native-asan','wasm','native-cross','wasm-cross','native-line','wasm-line'):
        wasm=target.startswith('wasm');binary=out/(target+'.js' if wasm else target)
        compiler=['emcc','-mllvm','-fast-isel=false','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760'] if wasm else ['gcc','-m32','-no-pie']
        command=compiler+['-O0','-std=gnu99','-w','-Wno-incompatible-pointer-types','-Wno-int-conversion','-fno-strict-aliasing','-I'+str(out),str(ROOT/'tools/reference/champ_menu_fields_fixture.c')]
        if target=='native-asan':command+=['-fsanitize=address']
        command+=['-DDD2_ORIGINAL_CHAMP_MENU_FIELDS'] if target=='original-x86' else [str(out/('engine.c' if target in ('native','native-asan','wasm') else target.split('-',1)[1]+'.c'))]
        command+=['-o',str(binary)]
        with (out/(target+'-build.log')).open('w') as log:run_bounded(command,directory=out,timeout=120,check=True,stdout=log,stderr=subprocess.STDOUT)
        checkpoint=out/(target+'.bin')
        with (out/(target+'.log')).open('w') as log:run_bounded((['node',str(binary)] if wasm else [str(binary)])+[str(exe),str(checkpoint)],directory=out,timeout=60,check=True,stdout=log,stderr=subprocess.STDOUT)
        data[target]=checkpoint.read_bytes()
    results={target:compare(data['original-x86'],raw) for target,raw in data.items() if target!='original-x86'}
    passed=all(results[target]['pass_'] for target in ('native','native-asan','wasm'))
    if any(results[target]['pass_'] for target in ('native-cross','wasm-cross','native-line','wasm-line')):raise AssertionError('Incorrect DWORD fields accepted')
    damaged=bytearray(data['original-x86']);damaged[20+SNAPSHOT+SNAPSHOT-1]^=1
    if compare(data['original-x86'],damaged)['pass_']:raise AssertionError('Damaged neighboring field accepted')
    try:compare(data['original-x86'],data['original-x86'][:-1])
    except ValueError:pass
    else:raise AssertionError('Truncated output accepted')
    report=dict(scope=__doc__.strip(),pass_=passed,original_exe_sha256=EXE_SHA,cases=CASES,record_bytes=RECORD,results=results,
        output_sha256={target:hashlib.sha256(raw).hexdigest() for target,raw in data.items()},
        engine_functions_sha256=hashlib.sha256(''.join(functions).encode()).hexdigest(),damaged_neighbor_rejected=True,truncated_output_rejected=True)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS' if passed else 'FAIL','championship menu fields:',CASES,'original/native/ASan/WASM cases')
    if passed:
        opened=open_files()
        if any((p.stat().st_dev,p.stat().st_ino) in opened for p in out.glob('*.bin')):raise RuntimeError('Component output still in use')
        discard_frames(out)
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
