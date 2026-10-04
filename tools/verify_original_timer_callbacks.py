#!/usr/bin/env python3
"""Verify the forwarding observer against actual Wine and original callbacks.

This checks timer IDs, callback arguments, independent cancellation, complete
inline ordering, unchanged backend code and bounded read-only original counter
observations. Logging changes timing. Full-game port PCM/video remains pending.
"""
import argparse
import copy
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys

from artifacts import WORK,check_space,prepare_output,run_bounded

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference'))
from timer_callbacks import ROW,original_report,read
from timer_observer import SYSTEM_WINMM,build,build_probe,observer_digest,pe_exports,verify_forwarders
from capture import original_pid


def digest(path):
    with Path(path).open('rb') as source:return hashlib.file_digest(source,'sha256').hexdigest()


def reject(name,operation,cases):
    try:operation()
    except (ValueError,KeyError):cases.append(dict(case=name,rejected=True))
    else:raise AssertionError('Damaged timer evidence accepted: '+name)


def probe(binary,output,env):
    with output.open('wb') as log:
        try:
            result=run_bounded(['wine',str(binary)],directory=output.parent,timeout=20,
                               cwd=binary.parent,env=env,stdout=log,stderr=subprocess.STDOUT)
        finally:
            # This caller owns the dedicated prefix lock. A server already
            # gone can return 1; never kill a user's ordinary Wine prefix.
            subprocess.run(['wineserver','-k'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=10)
            subprocess.run(['wineserver','-w'],env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,timeout=10)
    return result.returncode


def verify(capture,output):
    output=prepare_output(output)
    if WORK not in output.parents:raise ValueError('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=False)
    source=original_report(capture,ROOT/'DestructionDerby2/dd2h.exe',allow_terminal=True)
    metadata=json.loads((capture/'timer-observer-build.json').read_text())
    exports,sections,_=pe_exports(SYSTEM_WINMM.read_bytes())
    if metadata['backend_sha256']!=digest(SYSTEM_WINMM) or metadata['unchanged_sections']!=sections or metadata['exports']!=exports:
        raise ValueError('Capture backend metadata differs from installed Wine')
    verify_forwarders(metadata['observer_exports'],exports)
    for name,sha in metadata['source_sha256'].items():
        if digest(ROOT/'tools/reference'/name)!=sha:raise ValueError('Captured observer source differs')
    current=output/'current';build_metadata=build(current);build_probe(current)
    captured_dll=capture/'timer-observer.dll'
    if digest(captured_dll)!=metadata['observer_sha256']:
        raise ValueError('Captured observer DLL hash differs from metadata')
    if observer_digest(captured_dll.read_bytes())!=build_metadata['observer_normalized_sha256']:
        raise ValueError('Captured observer differs from verified probe binary')
    mutant=output/'wrong-user';mutant.mkdir()
    observer=ROOT/'tools/reference/winmm_timer_observer.c'
    anchor='context->callback(id,message,context->user,a,b);'
    text=observer.read_text()
    if text.count(anchor)!=1:raise ValueError('Observer callback dispatch anchor differs')
    (mutant/observer.name).write_text(text.replace(anchor,'context->callback(id,message,context->user+1,a,b);'))
    shutil.copyfile(observer.with_suffix('.h'),mutant/observer.with_suffix('.h').name)
    build(mutant,observer_source=mutant/observer.name);build_probe(mutant)
    negatives=[];targets=[]
    prefix=WORK/'wine-reference/prefix'
    with (prefix.parent/'capture.lock').open('w') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        if original_pid(prefix) is not None:raise RuntimeError('Original still running in reference prefix')
        env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
        env.update(WINEPREFIX=str(prefix),WINEARCH='win32',WINEDEBUG='-all,+debugstr')
        with (output/'xvfb.log').open('wb') as xlog:
            server=subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','640x480x16'],
                                     stdout=subprocess.PIPE,stderr=xlog,start_new_session=True)
            try:
                display=server.stdout.readline().decode().strip()
                if not display:raise RuntimeError('Xvfb did not start')
                env['DISPLAY']=':'+display
                expected=dict(ids=[16,33],callback_arguments=True,independent_cancel=True)
                for mode in ['builtin','observer','wrong-user']:
                    directory=mutant if mode=='wrong-user' else current
                    journal=directory/(mode+'.bin')
                    test_env=dict(env,WINEDLLOVERRIDES='winmm=b' if mode=='builtin' else 'winmm=n;_winmm_real=n')
                    if mode!='builtin':test_env['DD2_TIMER_CAPTURE']='Z:'+str(journal).replace('/','\\')
                    log=output/(mode+'.log')
                    code=probe(directory/'probe.exe',log,test_env)
                    if mode=='wrong-user':
                        if code!=12:raise AssertionError('Wrong callback user was not rejected by the actual probe')
                        negatives.append(dict(case='wrong-callback-user',rejected=True));continue
                    if code:raise AssertionError('Wine timer probe failed: '+mode)
                    lines=[line for line in log.read_text().splitlines() if line.startswith('{')]
                    if len(lines)!=1 or json.loads(lines[0])!=expected:raise AssertionError('Probe result differs: '+mode)
                    result=dict(mode=mode,pass_=True,observed=expected)
                    if mode=='observer':
                        callbacks=read(journal)
                        if len(callbacks['registrations'])!=2 or callbacks['counts'].get(5)!=3 or callbacks['counts'].get(6)!=3:
                            raise AssertionError('Probe API journal incomplete')
                        result.update(callback_count=len(callbacks['completed_callbacks']),
                                      journal_sha256=callbacks['journal_sha256'],tick_offset_ms=callbacks['tick_offset_ms'])
                    targets.append(result)
            finally:
                if server.poll() is None:os.killpg(server.pid,signal.SIGTERM)
                server.wait(timeout=5)
    # Literal original records keep the same API/callback identities. This
    # bounded excerpt tests the decoder without copying the large sound trace.
    excerpt=output/'excerpt';(excerpt/'audio').mkdir(parents=True)
    journal=(capture/'timer-callbacks.bin').read_bytes()
    (excerpt/'timer-callbacks.bin').write_bytes(journal)
    markers=[]
    with (capture/'wine.log').open('rb') as lines:
        for line in lines:
            if b'DD2_TIMER record=' in line:markers.append(line)
    literal=b''.join(markers)
    (excerpt/'wine.log').write_bytes(literal)
    observations=(capture/'audio/engine.jsonl').read_bytes()
    (excerpt/'audio/engine.jsonl').write_bytes(observations)
    positive=original_report(excerpt,ROOT/'DestructionDerby2/dd2h.exe',allow_terminal=True)
    if positive['journal_sha256']!=source['journal_sha256'] or positive['counter_checks']!=source['counter_checks']:
        raise AssertionError('Literal excerpt changed callback/counter evidence')
    rows=[list(row) for row in ROW.iter_unpack(journal)]
    first_callback=next(i for i,r in enumerate(rows) if r[2]==3)
    def damage(index,column,value):
        data=copy.deepcopy(rows);data[index][column]=value
        return b''.join(ROW.pack(*row) for row in data)
    mutations=[('serial-gap',damage(first_callback,1,9000)),
               ('wrong-magic',damage(0,0,0)),('wrong-user',damage(first_callback,8,1)),
               ('wrong-frequency',damage(first_callback,13,rows[first_callback][13]*2)),
               ('clock-domain',damage(first_callback,14,(rows[first_callback][14]+3000)&0xffffffff)),
               ('unknown-id',damage(first_callback,3,4000)),
               ('wrong-callback',damage(first_callback,7,0x41345d)),
               ('unmatched-end',damage(first_callback,2,4)),('truncated-record',journal[:-1])]
    for name,data in mutations:
        (excerpt/'timer-callbacks.bin').write_bytes(data)
        reject('journal-'+name,lambda:read(excerpt/'timer-callbacks.bin',allow_terminal=True),negatives)
    (excerpt/'timer-callbacks.bin').write_bytes(journal)
    for name,data in [('missing',b''.join(markers[1:])),('duplicate',literal+markers[-1]),
                      ('wrong-order',b''.join([markers[1],markers[0],*markers[2:]])),
                      ('wrong-id',literal.replace(b'id=16',b'id=17',1))]:
        (excerpt/'wine.log').write_bytes(data)
        reject('inline-'+name,lambda:original_report(excerpt,ROOT/'DestructionDerby2/dd2h.exe',allow_terminal=True),negatives)
    (excerpt/'wine.log').write_bytes(literal)
    samples=[json.loads(line) for line in observations.splitlines()]
    damaged=copy.deepcopy(samples);damaged[-1]['timer']['fires']+=1
    (excerpt/'audio/engine.jsonl').write_text(''.join(json.dumps(x)+'\n' for x in damaged))
    reject('wrong-original-fire-counter',lambda:original_report(excerpt,ROOT/'DestructionDerby2/dd2h.exe',allow_terminal=True),negatives)
    damaged=copy.deepcopy(samples);damaged[0].pop('qpc_clock')
    (excerpt/'audio/engine.jsonl').write_text(''.join(json.dumps(x)+'\n' for x in damaged))
    reject('missing-counter-clock-domain',lambda:original_report(excerpt,ROOT/'DestructionDerby2/dd2h.exe',allow_terminal=True),negatives)
    wrong=copy.deepcopy(build_metadata['observer_exports'])
    next(e for e in wrong if e['name']=='mciSendCommandA')['forwarder']='winmm_real.mciSendCommandA'
    reject('wrong-export-backend-alias',lambda:verify_forwarders(wrong,exports),negatives)
    report=dict(scope=__doc__.strip(),pass_=True,original=source,backend=metadata,targets=targets,negative_cases=negatives)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    # The source capture remains a required input for the full audio scheduler.
    # Successful standalone raw probes and mutation excerpts are disposable.
    shutil.rmtree(excerpt);shutil.rmtree(mutant)
    for path in current.glob('*.bin'):path.unlink()
    for path in output.glob('*.log'):path.unlink()
    check_space(output)
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();report=verify(args.capture,args.output)
    print('Original callback observer: PASS;',len(report['original']['completed_callbacks']),'callbacks,',
          len(report['original']['counter_checks']),'read-only counter checks,',len(report['negative_cases']),'rejected regressions')
