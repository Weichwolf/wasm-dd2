#!/usr/bin/env python3
"""Check actual movie producer cadence under a declared 3-ms fill cost.

Two one-second nonzero-source lifetimes use the production ALSA header and
real-time virtual device. Waits and processing are observed in CLOCK_MONOTONIC;
neither source bytes nor clock data are shifted. The old header must accumulate
period drift under that same load. This is cadence/source/device evidence,
not synchronized original/port A/V, whole tails or physical output parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import statistics
import struct
import subprocess

from artifacts import WORK, check_space, prepare_output
from reference.audio import build_audio, summarize_audio
from verify_configuration_persistence import ROOT, require
from verify_native_movie_device import verify_pcm
from verify_native_sdl import config


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--before-header',type=Path,required=True)
    args=parser.parse_args();output=prepare_output(args.output)
    require(WORK in output.parents,'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=False)
    source=output/'device_test.c';source.write_bytes((ROOT/'tools/native_movie_device_test.c').read_bytes())
    observer_source=output/'schedule_observer.c';observer_source.write_bytes((ROOT/'tools/native_movie_schedule_observer.c').read_bytes())
    observer=output/'schedule.so'
    subprocess.run(['gcc','-m32','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror',
                    *config('cflags'),str(observer_source),*config('libs'),'-ldl','-o',str(observer)],check=True)
    libraries=build_audio(output/'audio-libraries')
    asan=subprocess.check_output(['gcc','-m32','-print-file-name=libasan.so'],text=True).strip()
    cases=[];negative=[]
    base={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    for before in (False,True):
        case=output/('before' if before else 'production');case.mkdir();(case/'audio').mkdir()
        header=case/'dd2_native_movie_alsa.h'
        header.write_bytes((args.before_header if before else ROOT/'build/dd2_native_movie_alsa.h').read_bytes())
        binary=case/('before-paced' if before else 'movie-paced')
        subprocess.run(['gcc','-m32','-no-pie','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all',*config('cflags'),'-I'+str(case),str(source),
                        *config('libs'),'-ldl','-o',str(binary)],check=True)
        asound=case/'asound.conf'
        asound.write_text(f'pcm_type.dd2clock {{ lib "{output}/audio-libraries/$LIB/dd2_clock.so" }}\n'
                          'pcm.!default { type dd2clock }\n')
        env={**base,'ALSA_CONFIG_PATH':str(asound),'DD2_AUDIO_CAPTURE':str(case/'audio'),
             'DD2_AUDIO_PROCESS':binary.name,'DD2_AUDIO_RATE':'22050','DD2_SCHEDULE_LOG':str(case/'schedule.jsonl'),
             'LD_PRELOAD':asan+' '+str(observer)+' dd2_audio.so','LD_LIBRARY_PATH':':'.join(map(str,libraries)),
             'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
        run=subprocess.run([str(binary),'schedule'],env=env,capture_output=True,text=True,timeout=10)
        (case/'run.log').write_text(run.stdout+run.stderr)
        require(run.returncode==0 and 'runtime error:' not in run.stderr,'sanitized schedule fixture failed: '+run.stderr)
        rows=[json.loads(s) for s in run.stdout.splitlines()]
        require(len(rows)==2 and all(r['done']==1 and r['mode']=='schedule' for r in rows),'two completed source lifetimes required')
        audio=summarize_audio(case/'audio',require_played=True)
        require(len(audio['streams'])==len(audio['played_streams'])==2,'repeated actual device lifetimes missing')
        events=[json.loads(s) for s in (case/'schedule.jsonl').read_text().splitlines()]
        periods=[]
        for lifetime in (0,1):
            expected=b''.join(struct.pack('<hh',12345+i%2205+lifetime*4000,-23456+i%2205) for i in range(22050))
            for kind in ('streams','played_streams'):
                stream=audio[kind][lifetime];actual=(case/'audio'/stream['file']).read_bytes()
                require(stream['closed'] and stream['rate']==22050 and stream['channels']==2,'closed source device format required')
                verify_pcm(actual,expected,True)
                for label,changed in [('first-sample',bytes([actual[0]^1])+actual[1:]),
                                      ('leading-silence',b'\0'*4+actual),('truncated-source',actual[:len(expected)-4])]:
                    try:verify_pcm(changed,expected,True)
                    except RuntimeError:negative.append(dict(before=before,lifetime=lifetime,kind=kind,mutation=label))
                    else:raise RuntimeError('accepted changed scheduled source: '+label)
            trace=[e for e in events if e['lifetime']==lifetime]
            require(trace[0]['event']=='thread-start' and trace[-1]['event']=='thread-end','complete producer observation required')
            waits=[e for e in trace if e['event']=='wait'];work=[e for e in trace if e['event']=='work']
            require(len(waits)>=40 and work and all(e['end_ns']-e['begin_ns']>=3000000 for e in work),
                    'declared per-fill load not observed')
            require(all(e['result'] in (0,None) and 5000000<=e['requested_ns']<=15000000 for e in waits),
                    'producer wait exceeds half-period correction range')
            phase=[e['end_ns']-trace[0]['time_ns']-(index+1)*10000000 for index,e in enumerate(waits)]
            drift=statistics.mean(phase[-10:])-statistics.mean(phase[:10])
            require((drift>=20000000) if before else abs(drift)<20000000,
                    'old cadence did not drift' if before else 'producer accumulated processing time into its periods')
            periods.append(dict(lifetime=lifetime,waits=len(waits),phase_drift_ns=drift,
                                first_wake_phase_ns=phase[0],last_wake_phase_ns=phase[-1],
                                minimum_wait_ns=min(e['requested_ns'] for e in waits),
                                maximum_wait_ns=max(e['requested_ns'] for e in waits),old_drift_rejected=before))
        cases.append(dict(before=before,header_sha256=sha(header),binary_sha256=sha(binary),periods=periods,audio=audio))
        check_space(output)
    report=dict(scope=__doc__,pass_=True,original_port_parity='unproven',source_sha256=sha(source),
                observer_source_sha256=sha(observer_source),injected_fill_cost_ns=3000000,period_ns=10000000,
                drift_limit_ns=20000000,cases=cases,negative_controls=negative)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    for pcm in output.glob('*/audio/*.pcm'):pcm.unlink()
    print('PASS actual sanitized producer cadence and complete source: old header drifts in both lifetimes;',
          len(negative),'changed PCM controls rejected',flush=True)


if __name__=='__main__':main()
