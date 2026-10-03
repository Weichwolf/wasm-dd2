#!/usr/bin/env python3
"""Compare all league classifications with actual original x86 and reject old DWORD loads.

Explicit packed inputs cover every valid division/rank, signed-word boundaries
and nonzero adjacent points. This is a function component check, not full UI/A/V.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess

from artifacts import WORK, prepare_output, run_bounded, discard_frames

ROOT = Path(__file__).resolve().parents[1]
EXE_SHA = '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=prepare_output(args.output)
    if WORK not in out.parents or out.exists():
        parser.error('use a fresh directory under /tmp/wasm-dd2/')
    out.mkdir(parents=True)
    exe=ROOT/'DestructionDerby2/dd2h.exe'
    if hashlib.sha256(exe.read_bytes()).hexdigest()!=EXE_SHA:
        raise ValueError('Supported unmodified original required')
    engine=(ROOT/'build/dd2.c').read_text()
    match=re.findall(r'/\* ===== Check_League_Standing @ 0044c92c ===== \*/(.*?)(?=/\* =====)',engine,re.S)
    if len(match)!=1:raise ValueError('Actual engine function missing')
    sources={}
    for variant in ('fixed','old'):
        directory=out/variant;directory.mkdir()
        for name in ('dd2_symbols.h','ghidra_compat.h'):
            shutil.copyfile(ROOT/'build'/name,directory/name)
        if variant=='old':
            with (ROOT/'patches/851-league-standing-word-loads.diff').open() as patch:
                subprocess.run(['patch','-R','-F0','-p1'],cwd=directory,stdin=patch,check=True,stdout=subprocess.DEVNULL)
        (directory/'engine.c').write_text('#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n'+match[0])
        sources[variant]=directory
    results={};data={}
    for target in ('original-x86','native','wasm','native-old','wasm-old'):
        directory=sources['old' if target.endswith('-old') else 'fixed']
        wasm=target.startswith('wasm');binary=out/(target+'.js' if wasm else target)
        compiler=['emcc','-mllvm','-fast-isel=false','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760'] if wasm else ['gcc','-m32','-no-pie']
        command=compiler+['-O0','-std=gnu99','-w','-I'+str(directory),str(ROOT/'tools/reference/league_standing_fixture.c')]
        if target=='original-x86':command+=['-DDD2_ORIGINAL_STANDING']
        else:command+=[str(directory/'engine.c')]
        command+=['-o',str(binary)]
        with (out/(target+'-build.log')).open('w') as log:
            run_bounded(command,directory=out,timeout=120,check=True,stdout=log,stderr=subprocess.STDOUT)
        checkpoint=out/(target+'.bin');command=(['node',str(binary)] if wasm else [str(binary)])+[str(exe),str(checkpoint)]
        with (out/(target+'.log')).open('w') as log:
            run_bounded(command,directory=out,timeout=30,check=True,stdout=log,stderr=subprocess.STDOUT)
        raw=checkpoint.read_bytes()
        if len(raw)!=210*20:raise ValueError('Incomplete classification output')
        if any(raw[i:i+8]!=raw[i+12:i+20] for i in range(0,len(raw),20)):
            raise ValueError('Classifier changed packed input words')
        data[target]=raw
        if target=='original-x86':continue
        failures=[]
        for offset in range(0,len(raw),20):
            if raw[offset:offset+20]!=data['original-x86'][offset:offset+20]:
                failures.append(dict(case=offset//20,original=struct.unpack_from('<I',data['original-x86'],offset+8)[0],
                    port=struct.unpack_from('<I',raw,offset+8)[0]))
        results[target]=dict(pass_=not failures,cases=210,failures=failures)
    if any(not results[t]['pass_'] for t in ('native','wasm')):
        raise RuntimeError('Fixed classifier differs from original')
    for target in ('native-old','wasm-old'):
        if results[target]['pass_']:raise RuntimeError('Old DWORD loads were not rejected')
        bottom=next((f for f in results[target]['failures'] if f['case']==(3*7+4)*5),None)
        if bottom!=dict(case=125,original=5,port=3):raise RuntimeError('Bottom-place elimination regression not reproduced')
    report=dict(scope=__doc__.strip(),original_exe_sha256=EXE_SHA,results=results,
        output_sha256={t:hashlib.sha256(v).hexdigest() for t,v in data.items()})
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');discard_frames(out)
    print('PASS 210 original-x86 classifications on Native/WASM; old DWORD classification rejected on both')
    return 0


if __name__=='__main__':raise SystemExit(main())
