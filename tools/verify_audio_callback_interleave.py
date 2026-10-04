#!/usr/bin/env python3
"""Check production suspended caller stacks and own playback-status assertions.

Controlled sequences include the three main controls inside a callback observed
in the real original replay, suspension between callback APIs, and stop/play
before callback completion. This verifies scheduling mechanics, not full live
original/port engine output, timing or PCM parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess

from artifacts import WORK,check_space,open_files,prepare_output,run_bounded

ROOT=Path(__file__).resolve().parents[1]
ROW=struct.Struct('<12I')


def services(cycles):
    rows=[]
    def add(kind,sid=0,args=(),flip=0,context=0):
        rows.append([kind,sid,*((list(args)+[0]*6)[:6]),flip,0,context,len(rows)+1])
    add(1,args=(1024,0xe2,22050,1,8,1))
    add(11,16,(400,10,1,0,0x41345c))
    add(8,args=(0,))
    for i in range(cycles):
        flip=i+1
        add(30,16,flip=flip)
        if i%4==0:
            for kind,value in [(5,0),(6,-1130),(7,8333)]:add(kind,args=(value,),flip=flip)
            add(10,args=(1,),flip=flip,context=16)
        elif i%4==1:
            add(10,args=(1,),flip=flip,context=16)
            add(6,args=(-900,),flip=flip)
            add(10,args=(1,),flip=flip,context=16)
        elif i%4==2:
            add(5,args=(-100,),flip=flip)
            add(10,args=(1,),flip=flip,context=16)
        else:
            add(10,args=(1,),flip=flip,context=16)
            add(9,flip=flip)
            add(10,args=(0,),flip=flip,context=16)
            add(8,args=(0,),flip=flip)
        add(31,16,flip=flip,context=16)
    add(20,args=(1,0),flip=cycles+1)
    add(22,args=(0,),flip=cycles+1)
    header=struct.pack('<8s8I',b'DD2AS01\0',44100,len(rows),1,1,1,0,1,cycles+1)
    return header+b''.join(ROW.pack(*(v&0xffffffff for v in row)) for row in rows),rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=prepare_output(args.output)
    if WORK not in out.parents or out.exists():parser.error('Fresh output under /tmp/wasm-dd2/ required')
    out.mkdir(parents=True)
    raw,rows=services(512);input_file=out/'services.bin';input_file.write_bytes(raw)
    common=['-O2','-std=gnu89','-w','-DDD2_NO_FOPEN_WRAP','-fno-strict-aliasing',
            '-ffunction-sections','-fdata-sections','-I'+str(ROOT/'build'),
            str(ROOT/'tools/audio_callback_interleave_test.c'),'-Wl,--gc-sections']
    targets={};commands={};env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    env['ASAN_OPTIONS']='detect_leaks=0:detect_stack_use_after_return=1:abort_on_error=1'
    for target in ['native','asan','wasm']:
        binary=out/(target+'.js' if target=='wasm' else target)
        if target=='wasm':
            compiler=['emcc','-mllvm','-fast-isel=false','-DDD2_AUDIO_FIBERS','-sASYNCIFY',
                '-sASYNCIFY_STACK_SIZE=131072','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760']
        else:compiler=['gcc','-m32','-no-pie']+(['-fsanitize=address'] if target=='asan' else [])
        with (out/(target+'-build.log')).open('w') as log:
            run_bounded(compiler+common+['-o',str(binary)],directory=out,timeout=120,check=True,
                        stdout=log,stderr=subprocess.STDOUT)
        command=(['node',str(binary)] if target=='wasm' else [str(binary)]);commands[target]=command
        with (out/(target+'.log')).open('w') as log:
            run_bounded(command+[str(input_file),str(out/(target+'-complete.json')),'512'],
                directory=out,timeout=60,check=True,env=env,stdout=log,stderr=subprocess.STDOUT)
        result=json.loads(next(line for line in (out/(target+'.log')).read_text().splitlines() if line.startswith('{')))
        assert result==dict(callbacks=512,patterns=4,stack_guards=True,own_status=True)
        complete=json.loads((out/(target+'-complete.json')).read_text())
        assert complete['complete'] and complete['callbacks']==512 and complete['events']==len(rows)
        targets[target]=dict(pass_=True,observed=result,completion=complete)
        print('PASS',target,'512 callback continuations, main/callback locals and own statuses',flush=True)
    # Each damaged field must reject before accepting independently calculated
    # results. In particular, a supplied playback status is never applied.
    negatives={}
    cases=[('callback-id',3,1,17,'callback has no live engine timer'),
           ('main-volume',5,2,(-1129)&0xffffffff,'engine-calculated API argument differs'),
           ('callback-status',7,2,0,'independently calculated playback status differs'),
           ('callback-end',8,1,17,'unmatched callback end'),
           ('unknown-context',7,10,33,'API outside its callback context')]
    for label,index,field,value,reason in cases:
        damaged=bytearray(raw);struct.pack_into('<I',damaged,40+index*48+field*4,value)
        if label=='callback-id':
            # Keep the whole callback context well-formed so the live timer
            # identity assertion, rather than the loader, rejects this ID.
            for row,field in [(7,10),(8,1),(8,10)]:
                struct.pack_into('<I',damaged,40+row*48+field*4,value)
        file=out/(label+'.bin');file.write_bytes(damaged)
        for target,command in commands.items():
            with (out/(label+'-'+target+'.log')).open('w') as log:
                run=run_bounded(command+[str(file),str(out/(label+'-'+target+'-complete.json')),'512'],
                    directory=out,timeout=30,check=False,env=env,stdout=log,stderr=subprocess.STDOUT)
            text=(out/(label+'-'+target+'.log')).read_text()
            if not run.returncode or reason not in text:raise AssertionError(label+' accepted or wrong rejection on '+target+': '+text)
            negatives[label+'-'+target]=dict(rejected=True,reason=reason)
    old=out/'inline-before-869';old.mkdir()
    for name in ['dd2_audio_service.h','dd2h_stubs.c']:
        shutil.copyfile(ROOT/'build'/name,old/name)
    with (ROOT/'patches/869-interleaved-observed-audio-callbacks.diff').open('rb') as patch:
        run_bounded(['patch','-R','-p1','--fuzz=0','-d',str(old)],directory=out,timeout=10,
                    check=True,stdin=patch,stdout=subprocess.DEVNULL)
    # The old scheduler has no active-continuation field. Drop only that
    # final test assertion so the unchanged caller sequence reaches its API
    # order failure rather than failing to compile.
    test=(ROOT/'tools/audio_callback_interleave_test.c').read_text()
    (old/'test.c').write_text(test.replace(' && !ds_service_callback_active',''))
    old_common=['-I'+str(old)]+[str(old/'test.c') if flag==str(ROOT/'tools/audio_callback_interleave_test.c') else flag for flag in common]
    for target in ['native','wasm']:
        binary=old/(target+'.js' if target=='wasm' else target)
        compiler=(['emcc','-mllvm','-fast-isel=false','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760']
                  if target=='wasm' else ['gcc','-m32','-no-pie'])
        with (out/('old-'+target+'-build.log')).open('w') as log:
            run_bounded(compiler+old_common+['-o',str(binary)],directory=out,timeout=120,check=True,
                        stdout=log,stderr=subprocess.STDOUT)
        command=['node',str(binary)] if target=='wasm' else [str(binary)]
        with (out/('old-'+target+'.log')).open('w') as log:
            run=run_bounded(command+[str(input_file),str(out/('old-'+target+'-complete.json')),'512'],
                directory=out,timeout=30,check=False,env=env,stdout=log,stderr=subprocess.STDOUT)
        text=(out/('old-'+target+'.log')).read_text()
        if not run.returncode or 'engine API order differs' not in text:
            raise AssertionError('Inline scheduler did not reject observed interleaving: '+target+': '+text)
        negatives['old-inline-'+target]=dict(rejected=True,reason='engine API order differs')
    report=dict(scope=__doc__,pass_=True,service_input_sha256=hashlib.sha256(raw).hexdigest(),
                production_patch_sha256=hashlib.sha256((ROOT/'patches/869-interleaved-observed-audio-callbacks.diff').read_bytes()).hexdigest(),
                targets=targets,negative_cases=negatives)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(out)
    opened=open_files()
    for file in out.iterdir():
        if file.name=='report.json':continue
        stat=file.stat()
        if (stat.st_dev,stat.st_ino) in opened:raise RuntimeError('Successful diagnostic still open: '+str(file))
        if file.is_dir():shutil.rmtree(file)
        else:file.unlink()


if __name__=='__main__':main()
