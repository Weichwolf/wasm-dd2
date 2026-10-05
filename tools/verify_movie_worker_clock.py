#!/usr/bin/env python3
"""Observe Wine ALSA producer QPC and check native worker wake correction.

Actual Wine plays declared caller PCM. Forwarded clock/ALSA calls identify its
producer thread and RAW QPC reads. Native ASan/UBSan runs the production worker
with declared RAW/MONOTONIC clocks and real mutexes; only wait/device inputs are
controlled. The oracle follows Wine's 100-ns QPC and signed 32-bit correction
before its half-period clamp. This is a worker boundary comparison, not whole
original dd2h.exe audio, silent tails or synchronized A/V parity.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess

from artifacts import WORK, check_space, prepare_output
from reference.audio import build_audio, summarize_audio
from verify_configuration_persistence import ROOT, require
from verify_native_sdl import config
from verify_sound_cursor import wine_probe


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def validate(rows):
    require(len(rows)==24,'complete worker clock cases required')
    for index,row in enumerate(rows):
        fallback=index//12
        require(row['case']==index%12 and row['fallback']==fallback,'worker input identity differs')
        require(row['clock_calls']==([4,1]*4 if fallback else [4]*4),'worker QPC clock selection differs')
        require(row['first_fill_samples']==1 and row['first_fill_sleeps']==0 and row['fills']==4,
                'worker initial fill order differs')
        clock=row['mono_ns'] if fallback else row['raw_ns']
        require(len(clock)==4,'complete declared worker clocks required')
        ticks=[ns//100 for ns in clock]
        deadline=ticks[0]+100000;expected=[10000000]
        for tick in ticks[1:]:
            adjust=(deadline-tick+2**31)%2**32-2**31
            expected.append((100000+min(50000,max(-50000,adjust)))*100)
            deadline+=100000
        require(row['sleep_ns']==expected,'worker QPC wake correction differs')


def identify_producer(journal):
    tids={r['tid'] for r in journal if r['event']=='avail'}
    counts={tid:sum(r['event']=='avail' and r['tid']==tid for r in journal) for tid in tids}
    require(counts,'actual producer queries missing')
    producer=max(counts,key=counts.get)
    require(counts[producer]>=3 and sum(n==counts[producer] for n in counts.values())==1,
            'actual producer thread ambiguous')
    rows=[r for r in journal if r['tid']==producer]
    raw=[r for r in rows if r['event']=='clock' and r['clock']==4]
    require(len(raw)>=3 and all(a['ns']<=b['ns'] for a,b in zip(raw,raw[1:])),
            'actual producer RAW clock reads missing')
    preceding=[];previous=None;retry=False;recovered_queries=0
    for row in rows:
        if row['event']=='clock':
            previous=row['clock']
            if previous==4:retry=False
        elif row['event']=='recover_return':retry=row['clock']>=0
        elif row['event']=='avail' and previous is not None:
            # A successful recovery can query availability again inside the
            # same producer tick; the virtual device reads MONOTONIC meanwhile.
            require(previous==4 or retry,'producer correction does not use RAW QPC')
            if previous!=4:recovered_queries+=1
            preceding.append(previous);retry=False
    require(preceding.count(4)>=3,'producer QPC correction cycles missing')
    first_avail=next(i for i,row in enumerate(rows) if row['event']=='avail')
    initial_raw=sum(row['event']=='clock' and row['clock']==4 for row in rows[:first_avail])
    return dict(producer_tid=producer,producer_queries=counts[producer],
                raw_clock_reads=len(raw),preceding_query_clocks=preceding,
                raw_reads_before_first_query=initial_raw,
                recovered_queries_without_new_correction=recovered_queries)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--mingw',default='i686-w64-mingw32-gcc')
    parser.add_argument('--before-header',type=Path,required=True)
    args=parser.parse_args();out=prepare_output(args.output)
    require(WORK in out.parents,'Use /tmp/wasm-dd2/');out.mkdir(parents=True,exist_ok=False)
    fixture=out/'movie_worker_clock_test.c'
    fixture.write_bytes((ROOT/'tools/movie_worker_clock_test.c').read_bytes())
    observer=ROOT/'tools/reference/movie_worker_clock_observer.c'
    libraries=build_audio(out/'audio-libraries')
    for bits,directory in zip((32,64),libraries):
        subprocess.run(['gcc','-m'+str(bits),'-shared','-fPIC','-O2','-Wall','-Wextra','-Werror',
                        str(observer),'-ldl','-o',str(directory/'dd2_worker_clock.so')],check=True)
    exe=out/'worker.exe'
    subprocess.run([args.mingw,'-O2','-Wall','-Wextra','-Werror',str(fixture),'-lwinmm','-o',str(exe)],check=True)
    wine=out/'wine';wine.mkdir();(wine/'audio').mkdir()
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    env.update(DD2_AUDIO_PROCESS=exe.name,DD2_AUDIO_CAPTURE=str(wine/'audio'),DD2_AUDIO_RATE='22050',
               DD2_WORKER_PROCESS=exe.name,DD2_WORKER_CAPTURE=str(wine/'worker-clock.jsonl'),
               LD_PRELOAD='dd2_worker_clock.so dd2_audio.so',LD_LIBRARY_PATH=':'.join(map(str,libraries)))
    asound=(f'pcm_type.dd2clock {{ lib "{out}/audio-libraries/$LIB/dd2_clock.so" }}\n'
            'pcm.!default { type dd2clock }\n')
    actual=wine_probe(exe,wine,env,alsa_config=asound)
    shutil.rmtree(wine/'wine-prefix')
    require(actual==dict(source_frames=2205,frequency=10000000,header_done=True),'actual Wine caller completion differs')
    journal=[json.loads(s) for s in (wine/'worker-clock.jsonl').read_text().splitlines()]
    producer=identify_producer(journal)
    require(producer['raw_reads_before_first_query']==1,'actual Wine initial fill order differs')
    audio=summarize_audio(wine/'audio',require_played=True)
    expected=b''.join(struct.pack('<hh',12345+i,-23456+i) for i in range(2205))
    for kind in ('streams','played_streams'):
        require(len(audio[kind])==1 and audio[kind][0]['closed'],'complete caller PCM lifetime required')
        pcm=(wine/'audio'/audio[kind][0]['file']).read_bytes()
        require(len(pcm)>=len(expected) and pcm[:len(expected)]==expected and not any(pcm[len(expected):]),
                'actual Wine caller source PCM differs')
    native=[]
    for label,header in [('production',ROOT/'build/dd2_native_movie_alsa.h'),('before',args.before_header)]:
        directory=out/label;directory.mkdir()
        saved=directory/'dd2_native_movie_alsa.h';saved.write_bytes(header.read_bytes())
        binary=directory/'native'
        subprocess.run(['gcc','-m32','-O2','-no-pie','-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all',*config('cflags'),str(fixture),'-I'+str(directory),
                        *config('libs'),'-ldl','-o',str(binary)],check=True)
        run=subprocess.run([str(binary)],capture_output=True,text=True,check=True,timeout=10)
        (directory/'run.log').write_text(run.stdout+run.stderr)
        require('runtime error:' not in run.stderr,'worker sanitizer error')
        result=[json.loads(s) for s in run.stdout.splitlines()]
        if label=='before':
            try:validate(result)
            except RuntimeError as error:
                require(str(error) in ('worker QPC clock selection differs','worker initial fill order differs'),
                        'old worker failed for another reason')
                rejection=str(error)
            else:raise RuntimeError('accepted old worker')
        else:validate(result)
        native.append(dict(source=label,header_sha256=sha(saved),binary_sha256=sha(binary),rows=result,
                           old_source_rejected=label=='before',
                           rejection=rejection if label=='before' else None))
    negative=[]
    for label in ('wrong-clock','missing-fallback','unquantized-wait','missing-case','wrong-wrap',
                  'late-first-clock','sleep-before-fill','missing-first-fill'):
        result=copy.deepcopy(native[0]['rows'])
        if label=='wrong-clock':result[0]['clock_calls']=[1]*4
        elif label=='missing-fallback':result[12]['clock_calls']=[1]*4
        elif label=='unquantized-wait':result[1]['sleep_ns'][1]+=30
        elif label=='missing-case':result.pop()
        elif label=='wrong-wrap':result[11]['sleep_ns'][1]=5000000
        elif label=='late-first-clock':result[0]['first_fill_samples']=2
        elif label=='sleep-before-fill':result[0]['first_fill_sleeps']=1
        else:result[0]['fills']=3
        try:validate(result)
        except RuntimeError:negative.append(dict(mutation=label,rejected=True))
        else:raise RuntimeError('accepted changed worker evidence: '+label)
    report=dict(scope=__doc__,pass_=True,original_game_av_parity='unproven',fixture_sha256=sha(fixture),
                verifier_sha256=sha(Path(__file__)),observer_source_sha256=sha(observer),
                observer_library_sha256=[sha(d/'dd2_worker_clock.so') for d in libraries],
                wine_version=subprocess.check_output(['wine','--version'],text=True).strip(),
                wine_executable_sha256=sha(exe),wine=dict(result=actual,**producer,
                journal_sha256=sha(wine/'worker-clock.jsonl'),audio=audio),native=native,negative_controls=negative)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    for pcm in (wine/'audio').glob('*.pcm'):pcm.unlink()
    check_space(out)
    print('PASS actual Wine producer first fill and RAW clock, 24 sanitized worker schedules, old worker and eight controls rejected')


if __name__=='__main__':main()
