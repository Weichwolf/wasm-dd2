#!/usr/bin/env python3
"""Compare actual lap-record update logic with original x86 under a UI contract.

All seven saved/runtime indices, equal/better/worse times, accepted/cancelled
name-entry results and 1/2/8/9-character names use identical component inputs.
Original machine code executes comparison, record history shifts, time stores
and name copies; its actual configuration packer then serializes the payload.
A hardware breakpoint supplies only Enter_Driver_Names(0,1)'s
declared return; this does not exercise or verify that UI. Complete targeted
records/name/payload regions and guards, and callback decisions, must agree on native,
ASan and WASM. No live menu, persisted card, ordinary race or original A/V parity
is accepted. Verify those separately through real original/port input.
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
from artifacts import WORK,discard_frames,open_files,prepare_output,run_bounded

ROOT=Path(__file__).resolve().parents[1]
EXE_SHA='0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
REGIONS=((0x466e20,0x90),(0x4680a0,0x270),(0x93e790,0x40),(0x93a470,0x19c0))
SNAPSHOT=sum(size for _,size in REGIONS)
RECORD=24+2*SNAPSHOT
CASES=3528
PATCH=ROOT/'patches/892-saved-lap-record-layout.diff'


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def compare(reference,actual):
    if len(reference)!=CASES*RECORD or len(actual)!=len(reference):raise ValueError('Incomplete record update output')
    failures=[];count=0;bad_prompts=0
    for case in range(CASES):
        a=reference[case*RECORD:(case+1)*RECORD];b=actual[case*RECORD:(case+1)*RECORD]
        saved,level,shape,response,name=struct.unpack_from('<5I',a)
        if a[:20+SNAPSHOT]!=b[:20+SNAPSHOT]:raise ValueError('Record inputs differ')
        if a==b:continue
        count+=1
        original_calls=struct.unpack_from('<I',a,20+SNAPSHOT)[0]
        port_calls=struct.unpack_from('<I',b,20+SNAPSHOT)[0]
        bad_prompts+=original_calls!=port_calls
        if len(failures)>=16:continue
        first=next(i for i,(x,y) in enumerate(zip(a,b)) if x!=y)
        position=first-(24+SNAPSHOT);address=None
        for start,size in REGIONS:
            if 0<=position<size:address=hex(start+position);break
            position-=size
        failures.append(dict(case=case,saved=saved,level=level,shape=shape,response=response,name=name,
            original_calls=original_calls,port_calls=port_calls,first_difference=dict(offset=first,address=address,original=a[first],port=b[first])))
    return dict(pass_=count==0,cases=CASES,failing_cases=count,wrong_prompt_cases=bad_prompts,first_failures=failures)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=prepare_output(args.output)
    if WORK not in out.parents or out.exists():parser.error('Fresh output under /tmp/wasm-dd2 required')
    out.mkdir(parents=True);exe=ROOT/'DestructionDerby2/dd2h.exe'
    if digest(exe)!=EXE_SHA:raise ValueError('Supported unmodified original required')
    fixture_dir=out/'fixture';fixture_dir.mkdir()
    for name in ('lap_record_update_fixture.c','lap_record_callback_gdb.py','pe_fixture.h'):
        shutil.copyfile(ROOT/'tools/reference'/name,fixture_dir/name)
    sources={}
    for variant in (('fixed','old') if PATCH.exists() else ('fixed',)):
        directory=out/variant;directory.mkdir()
        for name in ('dd2.c','dd2_symbols.h','ghidra_compat.h'):shutil.copyfile(ROOT/'build'/name,directory/name)
        if variant=='old':
            with PATCH.open() as patch:subprocess.run(['patch','-R','-F0','-p1'],cwd=directory,stdin=patch,stdout=subprocess.DEVNULL,check=True)
        units=[];engine=(directory/'dd2.c').read_text()
        for name in ('Update_Jimmy_Spunk_Times','FUN_0044ae0c'):
            matches=re.findall(r'/\* ===== '+name+r' @ [0-9a-f]+ ===== \*/(.*?)(?=/\* =====|\Z)',engine,re.S)
            if len(matches)!=1:raise ValueError('Actual engine function missing: '+name)
            units.append(matches[0])
        (directory/'engine.c').write_text('#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n'+'\n'.join(units))
        (directory/'dd2.c').unlink();sources[variant]=directory
    data,hashes={},{}
    targets=['original-x86','native','native-asan','wasm']+(['native-old','wasm-old'] if 'old' in sources else [])
    for target in targets:
        directory=sources['old' if target.endswith('-old') else 'fixed'];wasm=target.startswith('wasm')
        binary=out/(target+'.js' if wasm else target)
        compiler=['emcc','-mllvm','-fast-isel=false','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760'] if wasm else ['gcc','-m32','-no-pie']
        command=compiler+['-O0','-g','-std=gnu99','-w','-Wno-implicit-function-declaration','-Wno-incompatible-pointer-types','-fno-strict-aliasing','-I'+str(directory),str(fixture_dir/'lap_record_update_fixture.c')]
        if target=='native-asan':command+=['-fsanitize=address']
        command+=['-DDD2_ORIGINAL_RECORD_UPDATE'] if target=='original-x86' else [str(directory/'engine.c')]
        command+=['-o',str(binary)]
        with (out/(target+'-build.log')).open('w') as log:run_bounded(command,directory=out,timeout=120,check=True,stdout=log,stderr=subprocess.STDOUT)
        checkpoint=out/(target+'.bin');code=out/(target+'-code.bin')
        if target=='original-x86':
            script=out/'original.gdb'
            script.write_text('set pagination off\nset confirm off\nset auto-solib-add off\nfile '+str(binary)+'\nset args '+str(exe)+' '+str(checkpoint)+' '+str(code)+'\npython\nimport sys,gdb\nsys.path.insert(0,'+repr(str(fixture_dir))+')\nfrom lap_record_callback_gdb import run\ntry:\n    run('+repr(str(out/'original-callbacks.json'))+')\nexcept Exception:\n    import traceback\n    traceback.print_exc()\n    gdb.execute("quit 1")\nend\nquit\n')
            command=['gdb','--nx','-q','-batch','-x',str(script)]
        else:command=(['node',str(binary)] if wasm else [str(binary)])+[str(exe),str(checkpoint),str(code)]
        with (out/(target+'.log')).open('w') as log:run_bounded(command,directory=out,timeout=240,check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1'),stdout=log,stderr=subprocess.STDOUT)
        data[target]=checkpoint.read_bytes()
        hashes[target]=dict(binary=digest(binary),output=digest(checkpoint),unchanged_original_code=digest(code),**({'wasm':digest(binary.with_suffix('.wasm'))} if wasm else {}))
    events=json.loads((out/'original-callbacks.json').read_text())['events']
    expected_events=[]
    for case in range(CASES):
        record=data['original-x86'][case*RECORD:(case+1)*RECORD]
        saved,level,shape,response,name=struct.unpack_from('<5I',record)
        calls=struct.unpack_from('<I',record,20+SNAPSHOT)[0]
        if calls!=int(shape in (2,4,6,7)):raise AssertionError('Original callback gate differs')
        if calls:expected_events.append(dict(case=case,caller=0x44d8dc,arguments=[0,1],response=response))
    if events!=expected_events:raise AssertionError('Controlled original callback events differ')
    if len({item['unchanged_original_code'] for item in hashes.values()})!=1:raise AssertionError('Original machine code differs')
    results={target:compare(data['original-x86'],raw) for target,raw in data.items() if target!='original-x86'}
    passed=all(results[target]['pass_'] for target in ('native','native-asan','wasm'))
    if 'old' in sources and any(results[t]['pass_'] or results[t]['wrong_prompt_cases']==0 for t in ('native-old','wasm-old')):raise AssertionError('Old record defects not rejected')
    damaged=bytearray(data['original-x86']);damaged[24+SNAPSHOT]^=1
    if compare(data['original-x86'],damaged)['pass_']:raise AssertionError('Damaged record guard accepted')
    try:compare(data['original-x86'],data['original-x86'][:-1])
    except ValueError:pass
    else:raise AssertionError('Truncated record output accepted')
    report=dict(scope=__doc__.strip(),pass_=passed,original_port_parity='component under controlled UI return only',original_exe_sha256=EXE_SHA,cases=CASES,record_bytes=RECORD,regions=REGIONS,controlled_original_callbacks=len(events),results=results,hashes=hashes,
        sources={v:{p.name:digest(p) for p in d.iterdir() if p.is_file()} for v,d in sources.items()},fixture_sources={p.name:digest(p) for p in fixture_dir.iterdir() if p.is_file()},verifier_sha256=digest(Path(__file__)),patch_sha256=digest(PATCH) if PATCH.exists() else None,damaged_guard_rejected=True,truncated_output_rejected=True)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(pass_=passed,results=results),indent=2))
    if passed:
        opened=open_files()
        if any((p.stat().st_dev,p.stat().st_ino) in opened for p in out.glob('*.bin')):raise RuntimeError('Completed output still in use')
        discard_frames(out)
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
