#!/usr/bin/env python3
"""Verify actual production ALSA start/play/close with nonzero caller PCM.

Accepted and virtual-device consumed bytes must start at caller sample zero.
Full runs check complete source playback; delayed constructors must become
ready before the device sample clock starts. Failed constructors/initial
writes close without producing samples. Cancel/error runs check literal
prefixes and closed journals. This is a device boundary test, not original
game output, whole movie tails, physical DAC or A/V timing parity.
Delayed joins must reset the device before cleanup and consume no samples
during that wait; the old stop header must fail those same invariants.
Source samples must drain before driver silence is submitted; each subsequent
silence write is limited by the independently observed negotiated period.
The completed source queue must drop/reset/prepare independently of later
movie close; the initial driver lead-in is discarded while caller PCM remains
complete. A previous queue-reset header must fail that same transition.
Injected reset errors must surface as failures after complete source playback
and still close and join safely in both repeated device lifetimes.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

from artifacts import WORK, check_space, prepare_output
from reference.audio import build_audio, summarize_audio
from verify_configuration_persistence import ROOT, require
from verify_native_sdl import config


def verify_pcm(actual, source, full):
    require(actual and len(actual)%4 == 0, 'complete nonempty stereo frames')
    if full:
        require(len(actual) >= len(source), 'complete caller PCM required')
        require(actual[:len(source)] == source, 'source PCM differs at offset zero')
        require(not any(actual[len(source):]), 'device tail contains nonzero samples')
    else:
        require(len(actual) < len(source) and actual == source[:len(actual)],
                'cancel/error output differs from caller prefix')


def verify_driver_tail(directory, accepted, played, source_frames, *, require_tail=False):
    period=played['period_frames']
    end=next(s['begin_ns']+((source_frames-s['offset_frames'])*1_000_000_000+played['rate']-1)//played['rate']
             for s in played['segments'] if s['offset_frames'] < source_frames <= s['offset_frames']+s['frames'])
    events=[json.loads(s) for s in (directory/accepted['events']).read_text().splitlines()]
    zeros=[e for e in events if e['event']=='write' and e['accepted']>0 and
           e['offset_frames']+e['accepted']>source_frames]
    require(not require_tail or zeros, 'missing observed post-drain driver silence')
    require(all(e['offset_frames']>=source_frames and e['call_begin_ns']>=end for e in zeros),
            'driver silence submitted before source playback completed')
    require(all(e['requested']<=period for e in zeros), 'driver silence write exceeds negotiated period')
    return dict(source_played_end_ns=end,negotiated_period_frames=period,
                driver_silence_writes=len(zeros),first_silence_write_ns=zeros[0]['call_begin_ns'] if zeros else None)


def verify_queue_reset(directory, accepted, played, source_frames, before_close_ns):
    boundary=verify_driver_tail(directory,accepted,played,source_frames,require_tail=True)
    end=boundary['source_played_end_ns']
    events=[json.loads(s) for s in (directory/accepted['events']).read_text().splitlines()]
    resets=[e for e in events if e['event']=='snd_pcm_reset' and end<=e['time_ns']<before_close_ns]
    require(len(resets)==1 and resets[0]['result']==0, 'missing independent completed-queue reset')
    reset=events.index(resets[0]);drop=events[reset-1];prepared=events[reset+1]
    require(drop['event']=='snd_pcm_drop' and prepared['event']=='snd_pcm_prepare' and
            drop['result']==prepared['result']==0 and
            end<=drop['time_ns']<=resets[0]['time_ns']<=prepared['time_ns']<before_close_ns,
            'completed queue reset sequence differs')
    return dict(drop_ns=drop['time_ns'],reset_ns=resets[0]['time_ns'],prepare_ns=prepared['time_ns'],
                before_close_ns=before_close_ns,queue_reset_before_close=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--before-header', type=Path, help='require delayed startup using an old production header to start audio too early')
    parser.add_argument('--before-stop-header', type=Path, help='require an old production header to keep playback running during a delayed join')
    parser.add_argument('--before-tail-header', type=Path, help='require an old production header to submit silence while source samples remain pending')
    parser.add_argument('--before-queue-header', type=Path, help='require an old production header to omit the independent completed-queue reset')
    args = parser.parse_args();output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    source = ROOT/'tools/native_movie_device_test.c'
    snapshot = output/'production';snapshot.mkdir()
    header = snapshot/'dd2_native_movie_alsa.h'
    header.write_bytes((ROOT/'build/dd2_native_movie_alsa.h').read_bytes())
    binary = output/'movie-device';fault = output/'fault.so';delay = output/'delay.so';join = output/'join.so'
    common = ['gcc', '-m32', '-O2', '-Wall', '-Wextra', '-Werror', *config('cflags'), str(source)]
    subprocess.run([*common, '-no-pie', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    '-I'+str(snapshot), *config('libs'), '-ldl', '-o', str(binary)], check=True)
    subprocess.run([*common, '-shared', '-fPIC', '-DDD2_MOVIE_DEVICE_FAULT', '-ldl',
                    '-o', str(fault)], check=True)
    subprocess.run([*common, '-shared', '-fPIC', '-DDD2_MOVIE_DEVICE_DELAY', *config('libs'), '-ldl',
                    '-o', str(delay)], check=True)
    subprocess.run([*common, '-shared', '-fPIC', '-DDD2_MOVIE_DEVICE_JOIN', *config('libs'), '-ldl',
                    '-o', str(join)], check=True)
    old_binary = None
    if args.before_header:
        old_source = output/'before-source';old_source.mkdir()
        (old_source/'dd2_native_movie_alsa.h').write_bytes(args.before_header.read_bytes())
        old_binary = output/'before-device'
        subprocess.run([*common, '-no-pie', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-I'+str(old_source), *config('libs'), '-ldl', '-o', str(old_binary)], check=True)
    before_stop = None
    if args.before_stop_header:
        old_source = output/'before-stop-source';old_source.mkdir()
        (old_source/'dd2_native_movie_alsa.h').write_bytes(args.before_stop_header.read_bytes())
        before_stop = output/'before-stop'
        subprocess.run([*common, '-no-pie', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-I'+str(old_source), *config('libs'), '-ldl', '-o', str(before_stop)], check=True)
    before_tail = None
    if args.before_tail_header:
        old_source = output/'before-tail-source';old_source.mkdir()
        (old_source/'dd2_native_movie_alsa.h').write_bytes(args.before_tail_header.read_bytes())
        before_tail = output/'before-tail-device'
        subprocess.run([*common, '-no-pie', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-I'+str(old_source), *config('libs'), '-ldl', '-o', str(before_tail)], check=True)
    before_queue = None
    if args.before_queue_header:
        old_source = output/'before-queue-source';old_source.mkdir()
        (old_source/'dd2_native_movie_alsa.h').write_bytes(args.before_queue_header.read_bytes())
        before_queue = output/'before-queue-device'
        subprocess.run([*common, '-no-pie', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-I'+str(old_source), *config('libs'), '-ldl', '-o', str(before_queue)], check=True)
    asan = subprocess.check_output(['gcc', '-m32', '-print-file-name=libasan.so'], text=True).strip()
    libraries = build_audio(output/'audio-libraries')
    base = {k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    base.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1',
                LD_LIBRARY_PATH=':'.join(map(str, libraries)))
    cases = [];negative = []
    for mode in ('full', 'delay', 'tail', 'reset-error', 'cancel', 'join', 'error', 'initial-error', 'thread-error', 'unavailable',
                 *(['before-delay'] if old_binary else []), *(['before-join'] if before_stop else []),
                 *(['before-tail'] if before_tail else []), *(['before-queue'] if before_queue else [])):
        case = output/mode;case.mkdir();(case/'audio').mkdir()
        asound = case/'asound.conf'
        asound.write_text('pcm.!default { type hw card "DD2_NONEXISTENT" }\n' if mode == 'unavailable' else
                          f'pcm_type.dd2clock {{ lib "{output}/audio-libraries/$LIB/dd2_clock.so" }}\n'
                          'pcm.!default { type dd2clock }\n')
        selected = old_binary if mode == 'before-delay' else before_stop if mode == 'before-join' else before_tail if mode == 'before-tail' else before_queue if mode == 'before-queue' else binary
        preloader = str(fault)+' ' if mode in ('error','initial-error','reset-error') else str(delay)+' ' if mode in ('delay','before-delay','thread-error') else str(join)+' ' if mode in ('join','before-join') else ''
        env = {**base, 'ALSA_CONFIG_PATH':str(asound), 'DD2_AUDIO_CAPTURE':str(case/'audio'),
               'DD2_AUDIO_PROCESS':selected.name[:15], 'DD2_AUDIO_RATE':'22050',
               'DD2_MOVIE_DEVICE_DELAY_LOG':str(case/'thread-ready.jsonl'),
               'DD2_MOVIE_DEVICE_JOIN_LOG':str(case/'join.jsonl'),
               'LD_PRELOAD':asan+' '+preloader+'dd2_audio.so'}
        if mode == 'initial-error': env['DD2_MOVIE_DEVICE_FAIL_START'] = '1'
        if mode == 'thread-error': env['DD2_MOVIE_DEVICE_FAIL_THREAD'] = '1'
        if mode == 'reset-error': env['DD2_MOVIE_DEVICE_FAIL_RESET_LOG'] = str(case/'reset-fault.jsonl')
        fixture_mode = 'full' if mode in ('delay','before-delay') else 'cancel' if mode in ('join','before-join') else 'tail' if mode in ('before-tail','before-queue') else mode
        run = subprocess.run([str(selected), fixture_mode], env=env, capture_output=True, text=True, timeout=10)
        (case/'run.log').write_text(run.stdout+run.stderr)
        require(run.returncode == 0 and 'runtime error:' not in run.stderr,
                'sanitized device test failed: '+run.stderr)
        rows = [json.loads(line) for line in run.stdout.splitlines()]
        if mode == 'unavailable':
            require(rows == [dict(unavailable_rejected=True)] and not list((case/'audio').glob('*.pcm')),
                    'unavailable device produced PCM')
            cases.append(dict(mode=mode, results=rows));continue
        if mode in ('initial-error','thread-error'):
            require(rows == [{mode.replace('-','_')+'_rejected':True}], 'initial fill/thread failure not rejected')
            for prefix in ('stream','played'):
                journals=list((case/'audio').glob(prefix+'-*.jsonl'))
                require(len(journals)==1, 'unique failed-start device journal required')
                events=[json.loads(s) for s in journals[0].read_text().splitlines()]
                require(events[-1]['event']=='close' and journals[0].with_suffix('.pcm').stat().st_size==0,
                        'failed startup did not close without samples')
            cases.append(dict(mode=mode, results=rows));continue
        audio = summarize_audio(case/'audio', require_played=True)
        full = fixture_mode in ('full','tail','reset-error');expected_runs = 2 if full else 1
        require(len(rows) == len(audio['streams']) == len(audio['played_streams']) == expected_runs,
                'incorrect repeated device lifetimes')
        tails=[];queue_resets=[]
        for round_, row in enumerate(rows):
            require(row['round'] == round_ and row['mode'] == fixture_mode, 'unexpected test lifetime')
            count = 2205 if full else 22050
            expected = b''.join(struct.pack('<hh', 12345+(i%2205)+round_*4000, -23456+(i%2205))
                                for i in range(count))
            for kind in ('streams', 'played_streams'):
                stream = audio[kind][round_]
                require(stream['closed'] and stream['format'] == 'S16_LE' and
                        stream['rate'] == 22050 and stream['channels'] == 2, 'closed original format required')
                actual = (case/'audio'/stream['file']).read_bytes()
                verify_pcm(actual, expected, full)
                if full:
                    for label, altered in [('changed-first-sample', bytes([actual[0]^1])+actual[1:]),
                                           ('leading-silence', b'\0'*4+actual),
                                           ('truncated-source', actual[:len(expected)-4])]:
                        try: verify_pcm(altered, expected, True)
                        except (RuntimeError, AssertionError): negative.append(dict(round=round_, kind=kind, mutation=label))
                        else: raise RuntimeError('accepted altered device output: '+label)
            if mode in ('full','delay','tail','before-tail'):
                try:
                    tail=verify_driver_tail(case/'audio',audio['streams'][round_],audio['played_streams'][round_],count,
                                            require_tail=fixture_mode=='tail')
                except RuntimeError as error:
                    require(mode=='before-tail' and 'before source playback completed' in str(error),
                            'unexpected driver silence boundary failure: '+str(error))
                    tail=dict(premature_driver_silence_rejected=True)
                else: require(mode!='before-tail', 'old tail header unexpectedly passes source drain boundary')
                tails.append(tail)
            if mode in ('tail','before-queue'):
                try:
                    reset=verify_queue_reset(case/'audio',audio['streams'][round_],audio['played_streams'][round_],
                                             count,row['before_close_ns'])
                except RuntimeError as error:
                    require(mode=='before-queue' and 'missing independent completed-queue reset' in str(error),
                            'unexpected queue reset failure: '+str(error))
                    reset=dict(missing_queue_reset_rejected=True)
                else: require(mode!='before-queue', 'old queue header unexpectedly resets before close')
                queue_resets.append(reset)
        reset_faults = None
        if mode == 'reset-error':
            reset_faults = [json.loads(s) for s in (case/'reset-fault.jsonl').read_text().splitlines()]
            require(len(reset_faults) == len(rows) and all(r['done'] == -1 for r in rows) and
                    all(e['result'] == -5 for e in reset_faults), 'reset failure was not reported')
            for fault_, row, stream in zip(reset_faults, rows, audio['played_streams']):
                require(stream['segments'][-1]['end_ns'] <= fault_['time_ns'] < row['before_close_ns'],
                        'reset fault must follow complete source playback and precede close')
        startup = None
        if mode in ('delay','before-delay'):
            ready = [json.loads(s)['ready_ns'] for s in (case/'thread-ready.jsonl').read_text().splitlines()]
            require(len(ready) == expected_runs, 'every delayed thread readiness required')
            startup = []
            for index, stream in enumerate(audio['played_streams']):
                first = stream['segments'][0]['begin_ns']
                if mode == 'delay': require(first >= ready[index], 'device playback precedes producer readiness')
                else: require(first < ready[index], 'old producer ordering did not fail device readiness')
                startup.append(dict(ready_ns=ready[index], first_played_sample_ns=first,
                                    ready_before_playback=first >= ready[index]))
        shutdown = None
        if mode in ('join','before-join'):
            joins=[json.loads(s) for s in (case/'join.jsonl').read_text().splitlines()]
            require(len(joins)==1, 'one delayed producer join required')
            stream=audio['played_streams'][0]
            events=[json.loads(s) for s in (case/'audio'/stream['events']).read_text().splitlines()]
            started=next(e['time_ns'] for e in events if e['event']=='start')
            stopped=next(e['time_ns'] for e in events if e['event']=='stop' and e['time_ns']>=started)
            boundary=joins[0]['join_begin_ns']
            played_during_join=any(s['end_ns']>boundary for s in stream['segments'])
            if mode=='join':
                require(stopped<=boundary and not played_during_join, 'playback continued during producer join')
                require(not any(e['event']=='start' and e['time_ns']>stopped for e in events),
                        'producer restarted the reset device')
            else:
                require(stopped>boundary and played_during_join,
                        'old stop did not fail reset-before-join invariant')
            shutdown=dict(device_stop_ns=stopped,**joins[0],reset_before_join=stopped<=boundary,
                          played_during_join=played_during_join)
        cases.append(dict(mode=mode, results=rows, startup=startup, shutdown=shutdown, tails=tails,
                          queue_resets=queue_resets,reset_faults=reset_faults,audio=audio));check_space(output)
    report = dict(scope=__doc__, pass_=True, original_port_parity='unproven',
                  source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                  production_header_sha256=hashlib.sha256(header.read_bytes()).hexdigest(),
                  before_header_sha256=hashlib.sha256(args.before_header.read_bytes()).hexdigest() if args.before_header else None,
                  before_stop_header_sha256=hashlib.sha256(args.before_stop_header.read_bytes()).hexdigest() if args.before_stop_header else None,
                  before_tail_header_sha256=hashlib.sha256(args.before_tail_header.read_bytes()).hexdigest() if args.before_tail_header else None,
                  before_queue_header_sha256=hashlib.sha256(args.before_queue_header.read_bytes()).hexdigest() if args.before_queue_header else None,
                  cases=cases, negative_controls=negative)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    # These successful boundary diagnoses retain reports and journals only.
    for pcm in output.glob('*/audio/*.pcm'): pcm.unlink()
    check_space(output)
    print('PASS sanitized actual ALSA device: repeat, delayed readiness/join, source/driver silence, completed queue reset, cancel, errors, unavailable;',
          len(negative), 'changed PCM controls rejected', flush=True)


if __name__ == '__main__':
    main()
