#!/usr/bin/env python3
"""Verify actual production ALSA start/play/close with nonzero caller PCM.

Accepted and virtual-device consumed bytes must start at caller sample zero.
Full runs check complete source playback; delayed constructors must become
ready before the device sample clock starts. Failed constructors/initial
writes close without producing samples. Cancel/error runs check literal
prefixes and closed journals. This is a device boundary test, not original
game output, whole movie tails, physical DAC or A/V timing parity.
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--before-header', type=Path, help='require delayed startup using an old production header to start audio too early')
    args = parser.parse_args();output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    source = ROOT/'tools/native_movie_device_test.c'
    snapshot = output/'production';snapshot.mkdir()
    header = snapshot/'dd2_native_movie_alsa.h'
    header.write_bytes((ROOT/'build/dd2_native_movie_alsa.h').read_bytes())
    binary = output/'movie-device';fault = output/'fault.so';delay = output/'delay.so'
    common = ['gcc', '-m32', '-O2', '-Wall', '-Wextra', '-Werror', *config('cflags'), str(source)]
    subprocess.run([*common, '-no-pie', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    '-I'+str(snapshot), *config('libs'), '-ldl', '-o', str(binary)], check=True)
    subprocess.run([*common, '-shared', '-fPIC', '-DDD2_MOVIE_DEVICE_FAULT', '-ldl',
                    '-o', str(fault)], check=True)
    subprocess.run([*common, '-shared', '-fPIC', '-DDD2_MOVIE_DEVICE_DELAY', *config('libs'), '-ldl',
                    '-o', str(delay)], check=True)
    old_binary = None
    if args.before_header:
        old_source = output/'before-source';old_source.mkdir()
        (old_source/'dd2_native_movie_alsa.h').write_bytes(args.before_header.read_bytes())
        old_binary = output/'before-device'
        subprocess.run([*common, '-no-pie', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-I'+str(old_source), *config('libs'), '-ldl', '-o', str(old_binary)], check=True)
    asan = subprocess.check_output(['gcc', '-m32', '-print-file-name=libasan.so'], text=True).strip()
    libraries = build_audio(output/'audio-libraries')
    base = {k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    base.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1',
                LD_LIBRARY_PATH=':'.join(map(str, libraries)))
    cases = [];negative = []
    for mode in ('full', 'delay', 'cancel', 'error', 'initial-error', 'thread-error', 'unavailable', *(['before-delay'] if old_binary else [])):
        case = output/mode;case.mkdir();(case/'audio').mkdir()
        asound = case/'asound.conf'
        asound.write_text('pcm.!default { type hw card "DD2_NONEXISTENT" }\n' if mode == 'unavailable' else
                          f'pcm_type.dd2clock {{ lib "{output}/audio-libraries/$LIB/dd2_clock.so" }}\n'
                          'pcm.!default { type dd2clock }\n')
        selected = old_binary if mode == 'before-delay' else binary
        preloader = str(fault)+' ' if mode in ('error','initial-error') else str(delay)+' ' if mode in ('delay','before-delay','thread-error') else ''
        env = {**base, 'ALSA_CONFIG_PATH':str(asound), 'DD2_AUDIO_CAPTURE':str(case/'audio'),
               'DD2_AUDIO_PROCESS':selected.name, 'DD2_AUDIO_RATE':'22050',
               'DD2_MOVIE_DEVICE_DELAY_LOG':str(case/'thread-ready.jsonl'),
               'LD_PRELOAD':asan+' '+preloader+'dd2_audio.so'}
        if mode == 'initial-error': env['DD2_MOVIE_DEVICE_FAIL_START'] = '1'
        if mode == 'thread-error': env['DD2_MOVIE_DEVICE_FAIL_THREAD'] = '1'
        fixture_mode = 'full' if mode in ('delay','before-delay') else mode
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
        full = fixture_mode == 'full';expected_runs = 2 if full else 1
        require(len(rows) == len(audio['streams']) == len(audio['played_streams']) == expected_runs,
                'incorrect repeated device lifetimes')
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
        cases.append(dict(mode=mode, results=rows, startup=startup, audio=audio));check_space(output)
    report = dict(scope=__doc__, pass_=True, original_port_parity='unproven',
                  source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                  production_header_sha256=hashlib.sha256(header.read_bytes()).hexdigest(),
                  before_header_sha256=hashlib.sha256(args.before_header.read_bytes()).hexdigest() if args.before_header else None,
                  cases=cases, negative_controls=negative)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    # These successful boundary diagnoses retain reports and journals only.
    for pcm in output.glob('*/audio/*.pcm'): pcm.unlink()
    check_space(output)
    print('PASS sanitized actual ALSA device: repeat, delayed readiness, cancel, initial/live errors, unavailable;',
          len(negative), 'changed PCM controls rejected', flush=True)


if __name__ == '__main__':
    main()
