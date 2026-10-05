#!/usr/bin/env python3
"""Compare worker-published completion with real Wine WaveOut notifications.

Wine gates its first WOM_DONE callback while the device consumes two headers;
the second header must remain pending until the notification worker resumes.
Native gates its producer before its first period: fully consumed source PCM
must remain pending until that producer publishes completion. Both probes use
the same nonzero caller PCM. This API/device boundary test does not establish
original dd2h.exe playback, whole movie tails or chronological A/V parity.
"""
import argparse
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


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_publication(result, wine=False):
    require(result['source_frames'] == 882, 'caller source frame count')
    if wine:
        require(result['waveout_position_frames'] >= 882 and result['pending_before_publication'] is True and
                result['callbacks'] == 2, 'Wine worker publication differs')
    else:
        require(result['submitted_frames'] == 882 and result['pending_frames'] == 0 and
                result['done_before_publication'] == 0 and result['done_after_publication'] == 1,
                'native completion precedes producer publication')


def verify_pcm(actual, expected):
    require(len(actual) >= len(expected) and actual[:len(expected)] == expected,
            'source PCM differs at offset zero')
    require(not any(actual[len(expected):]), 'nonzero post-source PCM')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--mingw', default='i686-w64-mingw32-gcc')
    parser.add_argument('--before-header', type=Path)
    args = parser.parse_args()
    output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    fixture = output/'movie_completion_test.c'
    fixture.write_bytes((ROOT/'tools/movie_completion_test.c').read_bytes())
    current = output/'current';current.mkdir()
    header = current/'dd2_native_movie_alsa.h';header.write_bytes((ROOT/'build'/header.name).read_bytes())
    libraries = build_audio(output/'audio-libraries')
    executable = output/'completion.exe'
    subprocess.run([args.mingw, '-O2', '-Wall', '-Wextra', '-Werror', str(fixture),
                    '-lwinmm', '-o', str(executable)], check=True)
    environment = {k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    environment['LD_LIBRARY_PATH'] = ':'.join(map(str, libraries))
    configuration = (f'pcm_type.dd2clock {{ lib "{output}/audio-libraries/$LIB/dd2_clock.so" }}\n'
                     'pcm.!default { type dd2clock }\n')
    expected = b''.join(struct.pack('<hh', 12345+i, -23456+i) for i in range(882))
    results = [];negative = []
    for case in ('wine', 'native', *(['before'] if args.before_header else [])):
        directory = output/case;directory.mkdir();(directory/'audio').mkdir()
        env = {**environment, 'DD2_AUDIO_CAPTURE':str(directory/'audio'), 'DD2_AUDIO_RATE':'22050',
               'LD_PRELOAD':'dd2_audio.so'}
        if case == 'wine':
            env['DD2_AUDIO_PROCESS'] = executable.name
            result = wine_probe(executable, directory, env, alsa_config=configuration)
            shutil.rmtree(directory/'wine-prefix')
            verify_publication(result, wine=True)
            source_header = None
        else:
            source_header = header
            if case == 'before':
                source_header = directory/header.name;source_header.write_bytes(args.before_header.read_bytes())
            binary = directory/('before-complete' if case == 'before' else 'native-complete')
            subprocess.run(['gcc', '-m32', '-O2', '-no-pie', '-fsanitize=address,undefined',
                            '-fno-sanitize-recover=all', *config('cflags'), str(fixture),
                            '-I'+str(source_header.parent), *config('libs'), '-ldl', '-o', str(binary)], check=True)
            (directory/'asound.conf').write_text(configuration)
            asan = subprocess.check_output(['gcc','-m32','-print-file-name=libasan.so'],text=True).strip()
            env.update(ALSA_CONFIG_PATH=str(directory/'asound.conf'), DD2_AUDIO_PROCESS=binary.name,
                       LD_PRELOAD=asan+' dd2_audio.so', ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1')
            run = subprocess.run([str(binary), '1' if case == 'before' else '0'], env=env,
                                 capture_output=True, text=True, timeout=8)
            (directory/'run.log').write_text(run.stdout+run.stderr)
            require(run.returncode == 0 and 'runtime error:' not in run.stderr,
                    'sanitized completion fixture failed: '+run.stderr)
            result = json.loads(run.stdout)
            if case == 'before':
                try: verify_publication(result)
                except RuntimeError as error:
                    require(str(error) == 'native completion precedes producer publication', 'unexpected old-header failure')
                else: raise RuntimeError('old header unexpectedly waits for producer publication')
            else: verify_publication(result)
        audio = summarize_audio(directory/'audio', require_played=True)
        for kind in ('streams', 'played_streams'):
            streams = audio[kind]
            require(len(streams) == 1 and streams[0]['closed'] and streams[0]['format'] == 'S16_LE' and
                    streams[0]['rate'] == 22050 and streams[0]['channels'] == 2, 'closed caller-format device required')
            actual = (directory/'audio'/streams[0]['file']).read_bytes()
            verify_pcm(actual, expected)
            for label, mutated in [('first-sample', bytes([actual[0]^1])+actual[1:]),
                                   ('truncated-source', actual[:len(expected)-4])]:
                try: verify_pcm(mutated, expected)
                except RuntimeError: negative.append(dict(case=case, kind=kind, mutation=label))
                else: raise RuntimeError('accepted altered source PCM')
        results.append(dict(case=case, result=result, audio=audio, executable_sha256=sha(executable if case=='wine' else binary),
                            header_sha256=sha(source_header) if source_header else None,
                            old_publication_rejected=case=='before'))
        check_space(output)
    for wine, result in ((True, results[0]['result']), (False, results[1]['result'])):
        mutations = ({'pending_before_publication':False,'callbacks':1,'waveout_position_frames':881} if wine else
                     {'done_before_publication':1,'done_after_publication':0,'pending_frames':1})
        for key, value in mutations.items():
            try: verify_publication({**result, key:value}, wine=wine)
            except RuntimeError: negative.append(dict(case='wine' if wine else 'native', mutation=key))
            else: raise RuntimeError('accepted changed completion state')
    report = dict(scope=__doc__, pass_=True, original_game_av_parity='unproven',
                  source_sha256=sha(fixture), verifier_sha256=sha(Path(__file__)),
                  current_header_sha256=sha(header), wine_version=subprocess.check_output(['wine','--version'],text=True).strip(),
                  caller_pcm_sha256=hashlib.sha256(expected).hexdigest(), results=results, negative_controls=negative)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    for pcm in output.glob('*/audio/*.pcm'): pcm.unlink()
    check_space(output)
    print('PASS real Wine and sanitized native completion publication;', len(negative), 'negative controls')


if __name__ == '__main__':
    main()
