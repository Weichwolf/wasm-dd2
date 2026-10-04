#!/usr/bin/env python3
"""Verify production startup activation and elapsed-time multimedia timers.

An independent original relay establishes registration order, sites and IDs.
Native, ASan and WASM then exercise window activation, cancellation, wrap and
fixed-menu-frame sample progress. The callback body and whole-game audio/video
scheduling remain outside this transport comparison.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import shutil
import subprocess
import sys

from artifacts import WORK,check_space,prepare_output,run_bounded

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference'))
from multimedia_timer import observe


def verify(original,output):
    source=observe(original/'wine.log',ROOT/'DestructionDerby2/dd2h.exe')
    gap=source['registration_gap_ms']
    output=prepare_output(output)
    if WORK not in output.parents:raise ValueError('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=False)
    (output/'original-timers.json').write_text(json.dumps(source,indent=2)+'\n')
    current=ROOT/'build/dd2h_stubs.c'
    window=ROOT/'build/dd2_win32.c'
    baseline=output/'old-stubs.c'
    # Build the actual pre-863 unit from the ordered series. Reversing 863 on
    # today's unit fails when later patches legitimately edit timer methods.
    old_directory=output/'before-863';old_directory.mkdir()
    for pattern in ('*.c','*.h'):
        for file in (ROOT/'re_out').glob(pattern):shutil.copyfile(file,old_directory/file.name)
    for file in sorted((ROOT/'patches').glob('*.diff')):
        if int(file.name.split('-',1)[0])>=863:break
        with file.open('rb') as patch:
            run_bounded(['patch','-p1','-s','-F0','--fuzz=0','-d',str(old_directory)],
                        directory=output,timeout=10,check=True,stdin=patch,stdout=subprocess.DEVNULL)
    shutil.copyfile(old_directory/'dd2h_stubs.c',baseline)
    shutil.rmtree(old_directory)
    mutants={'old-timers':baseline}
    skip=output/'skip-replay.c'
    text=current.read_text()
    anchor='    polling=1;now=dd2_mmtimer_ms();'
    if text.count(anchor)!=1:raise ValueError('Corrected timer poll required')
    skip.write_text(text.replace(anchor,'    if(!getenv("DD2_REALTIME"))return;\n'+anchor))
    mutants['skip-replay']=skip
    old_window=output/'old-window.c'
    window_diff=output/'old-window.diff'
    window_diff.write_text('--- a/dd2_win32.c'+
                          (ROOT/'patches/863-independent-multimedia-timers.diff').read_text().split('--- a/dd2_win32.c')[1])
    run_bounded(['patch','-R','-p1','--fuzz=0','--output='+str(old_window),str(window),str(window_diff)],
                directory=output,timeout=10,check=True,stdout=subprocess.DEVNULL)
    mutants['old-window']=current
    clock=output/'clock.bin'
    clock.write_bytes(struct.pack('<8sIIII',b'DD2AC01\0',44100,1,5,0)+
                      struct.pack('<5Q',17639,17640,35279,35280,52920))
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    env.update(DD2_SOUND='1',ASAN_OPTIONS='detect_leaks=0')
    common=['-std=gnu99','-w','-O2','-fno-strict-aliasing','-DDD2_NO_FOPEN_WRAP',
            '-ffunction-sections','-fdata-sections','-I'+str(ROOT/'build'),
            str(ROOT/'tools/multimedia_timer_test.c'),'-Wl,--gc-sections']
    compiled={};results=[]
    for target,unit in [('native',current),('native-asan',current),('wasm',current),*mutants.items()]:
        binary=output/(target+('.js' if target=='wasm' else ''))
        window_source=old_window if target=='old-window' else window
        if target=='wasm':
            command=['emcc',*common,str(window_source),str(unit),'-sNODERAWFS=1','-sEXIT_RUNTIME=1',
                     '-sGLOBAL_BASE=10485760','--pre-js',str(ROOT/'tools/node_env.js'),'-o',str(binary)]
        else:
            command=['gcc','-m32','-no-pie',*common,str(window_source),str(unit),
                     *(['-fsanitize=address','-g'] if target=='native-asan' else []),'-o',str(binary)]
        with (output/(target+'-build.log')).open('w') as log:
            run_bounded(command,directory=output,timeout=120,check=True,stdout=log,stderr=subprocess.STDOUT)
        compiled[target]=['node',str(binary)] if target=='wasm' else [str(binary)]
    expected={
        'realtime':dict(startup_ids=[16,33],registration_gap_ms=gap,callback_count=gap//400+5,
                        wrap=True,cancel_independent=True,reentrant=True,id_word_wrap=True),
        'observed':dict(startup_ids=[16,33],callback_counts=[0,2,2,4,5],elapsed_ms=1200,engine_cf=0)}
    for target in ['native','native-asan','wasm']:
        for mode in expected:
            clock_env=dict(env)
            clock_env.update({'DD2_REALTIME':'1'} if mode=='realtime' else {'DD2_AUDIO_FRAME_CLOCK':str(clock)})
            run=subprocess.run([*compiled[target],mode,str(gap)],env=clock_env,capture_output=True,text=True,timeout=20)
            (output/(target+'-'+mode+'.log')).write_text(run.stdout+run.stderr)
            if run.returncode or json.loads(run.stdout)!=expected[mode]:
                raise AssertionError('Timer transport failed: '+target+'/'+mode)
            results.append(dict(target=target,mode=mode,pass_=True,observed=json.loads(run.stdout)))
    negatives=[]
    for target,mode,reason in [('old-timers','realtime','original independent startup IDs'),
                             ('skip-replay','observed','fixed-cf device time'),
                             ('old-window','realtime','activation before explicit timer')]:
        clock_env=dict(env)
        clock_env.update({'DD2_REALTIME':'1'} if mode=='realtime' else {'DD2_AUDIO_FRAME_CLOCK':str(clock)})
        run=subprocess.run([*compiled[target],mode,str(gap)],env=clock_env,capture_output=True,text=True,timeout=20)
        (output/(target+'.log')).write_text(run.stdout+run.stderr)
        if run.returncode!=2 or reason not in run.stderr:raise AssertionError('Timer regression was not rejected: '+target)
        negatives.append(dict(case=target,rejected=True))
    # Extract bounded literal original records for malformed-input checks.
    excerpt=[]
    with (original/'wine.log').open(errors='replace') as lines:
        for index,line in enumerate(lines,1):
            if any(record['entry_line']==index or record['return_line']==index for record in source['platform_calls']):
                excerpt.append(line)
    literal=''.join(excerpt)
    positive=output/'excerpt.trace';positive.write_text(literal)
    if observe(positive,ROOT/'DestructionDerby2/dd2h.exe')['timers']!=source['timers']:
        # Line numbers differ in the literal bounded excerpt; compare semantic fields.
        a=observe(positive,ROOT/'DestructionDerby2/dd2h.exe')['timers']
        for x,y in zip(a,source['timers']):
            if {k:v for k,v in x.items() if not k.endswith('_line')}!={k:v for k,v in y.items() if not k.endswith('_line')}:
                raise AssertionError('Original excerpt changed registration evidence')
    mutations=[('same-id',literal.replace('retval=00000021','retval=00000010')),
               ('unknown-site',literal.replace('ret=004133d8','ret=004133dc')),
               ('wrong-period',literal.replace('00000190,0000000a','00000191,0000000a',1)),
               ('failed-timer',literal.replace('retval=00000021','retval=00000000')),
               ('unreturned',literal.replace(next(line for line in excerpt if 'Ret  winmm.timeSetEvent' in line),'')),
               ('malformed',literal.replace('retval=00000021','retval=garbage'))]
    for name,data in mutations:
        path=output/(name+'.trace');path.write_text(data)
        try:observe(path,ROOT/'DestructionDerby2/dd2h.exe')
        except ValueError:negatives.append(dict(case='relay-'+name,rejected=True))
        else:raise AssertionError('Damaged original timer evidence accepted: '+name)
        path.unlink()
    report=dict(scope=__doc__.strip(),pass_=True,original=source,targets=results,negative_cases=negatives,
                source_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [current,window,ROOT/'tools/multimedia_timer_test.c']})
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    for path in [baseline,skip,old_window,window_diff,clock,positive]:path.unlink()
    for path in [output/'clock.bin.json']:path.unlink(missing_ok=True)
    check_space(output)
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    report=verify(args.original,args.output)
    print('Multimedia timer transport: PASS;',len(report['targets']),'target cases,',len(report['negative_cases']),'rejected regressions')
