#!/usr/bin/env python3
"""Check MCI drain waits independently of the movie clock.

Actual Wine MCI plays a short original-packet AVI normally and with a declared
frozen QPC/RAW clock. Forwarded OS wait brackets and complete source playback
must still occur. Native ASan/UBSan checks production MONOTONIC timer boundaries;
native/WASM MCI uses declared frozen, normal, fast and slow movie clocks with
an independent wait clock. Old production MCI must fail the same wait contract.
This is a timer/API comparison, not full original movie A/V or PCM-tail parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

from artifacts import WORK, check_space, prepare_output
from reference.audio import build_audio, summarize_audio
from verify_configuration_persistence import ROOT, require
from verify_movie_drain import short_avi
from verify_sound_cursor import wine_probe


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_timer(rows):
    epochs=(123456789,999999999,4294967295999999)
    probes=(0,1,99999999,100000000,100000001)
    require(len(rows)==15,'complete timer boundaries required')
    for index,row in enumerate(rows):
        require(row['epoch_ns']==epochs[index//5] and row['elapsed_ns']==probes[index%5],
                'timer input identity differs')
        require(row['ready']==int(probes[index%5]>=100000000),'timer expires before relative 100-ms wait')


def validate_mci(row):
    require(row['mode'] in range(4) and row['completed']==1 and row['result']==1 and
            row['elapsed_ns']==100000000 and row['queries_ns']==[0,100000000] and
            row['timer_armed_after_close']==0,'MCI drain depends on movie clock')


def observed_wait(rows,begin_ns,end_ns):
    main=[r for r in rows if r['pid']==r['tid'] and begin_ns<=r['begin_ns']<=r['end_ns']<=end_ns]
    for index,row in enumerate(main):
        if not 90000<=row['requested_us']<=100000:
            continue
        for last in main[index:]:
            require(last['result'] in (0,-1),'unexpected forwarded select result')
            if last['result']==0:
                elapsed=last['end_ns']-row['begin_ns']
                require(elapsed>=row['requested_us']*1000,'forwarded OS wait returned before requested timeout')
                return dict(begin_ns=row['begin_ns'],end_ns=last['end_ns'],
                            initial_requested_us=row['requested_us'],observed_elapsed_ns=elapsed)
    raise RuntimeError('actual main-thread MCI relative OS wait missing')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--before-movie',type=Path,required=True)
    parser.add_argument('--mingw',default='i686-w64-mingw32-gcc')
    args=parser.parse_args();output=prepare_output(args.output)
    require(WORK in output.parents,'Use /tmp/wasm-dd2/');output.mkdir(parents=True,exist_ok=False)
    snapshot=output/'production';snapshot.mkdir()
    for p in [*(ROOT/'build').glob('*.h'), *(ROOT/'build'/f'dd2_{n}.c' for n in ('movie','movie_platform','avi','cinepak','msadpcm'))]:
        (snapshot/p.name).write_bytes(p.read_bytes())
    source=snapshot/'movie_wait_test.c';source.write_bytes((ROOT/'tools/movie_wait_test.c').read_bytes())
    before=output/'before-movie.c';before.write_bytes(args.before_movie.read_bytes())
    movie=output/'short.avi';movie.write_bytes(short_avi(ROOT/'DestructionDerby2/Intro.avi'))
    libraries=build_audio(output/'audio-libraries')
    observer=ROOT/'tools/reference/movie_wait_observer.c'
    for bits,library in zip((32,64),libraries):
        subprocess.run(['gcc',f'-m{bits}','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror',
                        str(observer),'-ldl','-pthread','-o',str(library/'dd2_wait.so')],check=True)
    executable=output/'movie-drain.exe'
    subprocess.run([args.mingw,'-O2','-Wall','-Wextra','-Werror',str(ROOT/'tools/movie_drain_test.c'),
                    '-lwinmm','-o',str(executable)],check=True)
    config=f'pcm_type.dd2clock {{ lib "{output}/audio-libraries/$LIB/dd2_clock.so" }}\npcm.!default {{ type dd2clock }}\n'
    environment={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    wine_cases=[];original_source=None
    for frozen in (False,True):
        directory=output/('wine-frozen' if frozen else 'wine-normal');directory.mkdir();(directory/'audio').mkdir()
        env={**environment,'LD_LIBRARY_PATH':':'.join(map(str,libraries)),
             'LD_PRELOAD':'dd2_wait.so dd2_audio.so','DD2_AUDIO_CAPTURE':str(directory/'audio'),
             'DD2_AUDIO_PROCESS':executable.name,'DD2_AUDIO_RATE':'22050',
             'DD2_WAIT_PROCESS':executable.name,'DD2_WAIT_CAPTURE':str(directory/'waits.jsonl')}
        if frozen:env['DD2_WAIT_FREEZE_RAW']='1'
        actual=wine_probe(executable,directory,env,alsa_config=config,
                          arguments=tuple('Z:'+str(p).replace('/','\\') for p in (movie,directory/'audio')),
                          wine_debug='-all,+mciavi')
        shutil.rmtree(directory/'wine-prefix')
        require(actual['qpc_frequency']==10000000 and actual['device_closed_before_play_return'],
                'actual Wine MCI did not close its device')
        require(actual['elapsed_ms']==0 if frozen else 80<=actual['elapsed_ms']<300,
                'declared frozen/normal Wine QPC differs')
        audio=summarize_audio(directory/'audio',require_played=True)
        require(len(audio['streams'])==len(audio['played_streams'])==1,'unique actual movie device required')
        accepted=audio['streams'][0];played=audio['played_streams'][0]
        require(accepted['format']==played['format']=='S16_LE' and accepted['closed'] and played['closed'] and
                accepted['accepted_frames']>=1012 and played['played_frames']>=1012,'full actual short-source playback required')
        events=[json.loads(s) for s in (directory/'audio'/accepted['events']).read_text().splitlines()]
        closed=next(e['time_ns'] for e in reversed(events) if e['event']=='close')
        waits=[json.loads(s) for s in (directory/'waits.jsonl').read_text().splitlines()]
        selected=observed_wait(waits,accepted['time_ns'],closed)
        trace=(directory/'wine.log').read_text(errors='replace')
        require('Playing from frame=0 to frame=1' in trace and 'Painting frame 0 ' in trace and
                'Painting frame 1 ' not in trace,'short MCI exclusive final draw differs')
        for stream in (accepted,played):
            pcm=(directory/'audio'/stream['file']).read_bytes()[:1012*4]
            require(len(pcm)==1012*4,'complete literal original source required')
            if original_source is None:original_source=pcm
            require(pcm==original_source,'declared QPC freeze changed original source PCM')
        wine_cases.append(dict(frozen_raw=frozen,actual=actual,os_wait=selected,audio=audio,
                               journal_sha256=sha(directory/'waits.jsonl'),trace_sha256=sha(directory/'wine.log')))
    base=['-O2','-std=gnu99','-ffunction-sections','-fdata-sections','-I'+str(snapshot),'-Wl,--gc-sections']
    sanitizer=['-m32','-no-pie','-fsanitize=address,undefined','-fno-sanitize-recover=all']
    def run(command):
        result=subprocess.run(command,capture_output=True,text=True,check=True,timeout=15)
        require('runtime error:' not in result.stderr,'timer sanitizer error')
        return [json.loads(s) for s in result.stdout.splitlines()]
    timer=output/'timer'
    subprocess.run(['gcc',*sanitizer,*base,'-DDD2_WAIT_TIMER',str(source),str(snapshot/'dd2_movie_platform.c'),
                    '-Wl,--wrap=clock_gettime','-o',str(timer)],check=True)
    timer_rows=run([str(timer)]);validate_timer(timer_rows)
    units=[str(snapshot/f'dd2_{n}.c') for n in ('avi','cinepak','msadpcm')];targets=[]
    for label,selected in (('production',snapshot/'dd2_movie.c'),('before',before)):
        for target in ('native','wasm'):
            binary=output/(label+'-'+target+('.js' if target=='wasm' else ''))
            command=['emcc',*base,'-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sINITIAL_MEMORY=67108864'] if target=='wasm' else ['gcc',*sanitizer,*base]
            subprocess.run([*command,*units,str(selected),str(source),'-o',str(binary)],check=True)
            rows=[]
            for mode in range(4):
                row=run((['node',str(binary)] if target=='wasm' else [str(binary)])+[str(movie),str(mode)])[0]
                if label=='before':
                    try:validate_mci(row)
                    except RuntimeError as error:require(str(error)=='MCI drain depends on movie clock','unexpected old-source failure')
                    else:raise RuntimeError('old movie-clock drain accepted independent wait contract')
                else:validate_mci(row)
                rows.append(row)
            targets.append(dict(source=label,target=target,binary_sha256=sha(binary),rows=rows,old_source_rejected=label=='before'))
    controls=[]
    for label in ('early-timer','missing-timer','frozen-drain','early-drain','armed-after-close'):
        rows=json.loads(json.dumps(timer_rows if label.endswith('timer') else targets[0]['rows']))
        if label=='early-timer':rows[2]['ready']=1
        elif label=='missing-timer':rows.pop()
        elif label=='frozen-drain':rows[0]['completed']=0
        elif label=='early-drain':rows[0]['queries_ns'][1]-=100000
        else:rows[0]['timer_armed_after_close']=1
        try:
            if label.endswith('timer'):validate_timer(rows)
            else:
                for row in rows:validate_mci(row)
        except RuntimeError:controls.append(label)
        else:raise RuntimeError('accepted changed wait evidence: '+label)
    report=dict(scope=__doc__,pass_=True,original_port_av_parity='unproven',wine_cases=wine_cases,
                fixture_sha256=sha(source),reference_fixture_sha256=sha(ROOT/'tools/movie_drain_test.c'),
                reference_executable_sha256=sha(executable),wine_version=subprocess.check_output(['wine','--version'],text=True).strip(),
                observer_source_sha256=sha(observer),observer_library_sha256=[sha(p/'dd2_wait.so') for p in libraries],
                verifier_sha256=sha(Path(__file__)),
                sources={p.name:sha(p) for p in snapshot.glob('*.c')},original_avi_sha256=sha(ROOT/'DestructionDerby2/Intro.avi'),
                short_avi_sha256=sha(movie),timer_cases=timer_rows,targets=targets,negative_controls_rejected=controls)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    for pcm in output.glob('wine-*/audio/*.pcm'):pcm.unlink()
    check_space(output)
    print('PASS actual Wine relative wait with frozen QPC; 15 native timer boundaries and native/WASM independent MCI drain; old sources and five controls rejected')


if __name__=='__main__':
    main()
