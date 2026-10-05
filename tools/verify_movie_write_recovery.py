#!/usr/bin/env python3
"""Compare recoverable source writes in real Wine WaveOut and native transport.

Inject one EPIPE, EINTR or EAGAIN before accepting caller sample zero. Require
complete accepted PCM at offset zero and an actual unchanged retry. Require
complete native playback and the exact independently observed Wine playback
prefix; WaveOut's single-header completion can truncate its final samples.
Wine and native recovery calls must agree; the old native header must fail
the EPIPE/EINTR cases. This is an API/device fault test, not full original-game
audio, silence-tail, browser or synchronized A/V acceptance.
"""
import argparse
import errno
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess

from artifacts import WORK, check_space, open_files, prepare_output
from reference.audio import build_audio, build_write_fault, summarize_audio
from verify_configuration_persistence import ROOT, require
from verify_native_sdl import config
from verify_sound_cursor import wine_probe


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_source(actual, source, complete=True):
    if complete:
        require(len(actual) >= len(source) and actual[:len(source)] == source,
                'Complete unchanged caller source required at offset zero')
    else:
        require(len(actual) >= len(source)//2 and actual[:len(source)] == source[:len(actual)],
                'Unchanged observed Wine playback prefix required at offset zero')
    require(not any(actual[len(source):]), 'Nonzero post-source output')


def verify_fault(events, code):
    # The production worker can later recover an independent natural XRUN.
    # Preserve every observation, but verify the injected write through its
    # first real retry separately from the rest of the device lifetime.
    retries = [i for i,e in enumerate(events) if e['event'] == 'retry']
    require(len(retries) == 1, 'Actual source retry required')
    relevant = events[:retries[0]+1]
    names = ['fault', 'retry'] if code == errno.EAGAIN else [
        'fault', 'recover-error', 'recover-result', 'retry']
    require([e['event'] for e in relevant] == names, 'Fault/recovery/retry sequence differs')
    require(relevant[0]['result'] == -code and relevant[-1]['result'] > 0 and
            relevant[0]['frames'] == relevant[-1]['frames'], 'Caller source retry differs')
    if code != errno.EAGAIN:
        require(relevant[1]['result'] == -code and relevant[1]['silent'] == 0 and
                relevant[2]['result'] == 0 and relevant[2]['silent'] == 0,
                'Wine best-effort recovery arguments/result differ')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--mingw', default='i686-w64-mingw32-gcc')
    parser.add_argument('--before-header', type=Path,
                        help='old production header; default reverses patch899 in a private snapshot')
    args = parser.parse_args()
    output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    fixture = output/'movie_write_recovery_test.c'
    fixture.write_bytes((ROOT/'tools/movie_write_recovery_test.c').read_bytes())
    fault_source = output/'movie_write_fault.c'
    fault_source.write_bytes((ROOT/'tools/reference/movie_write_fault.c').read_bytes())
    libraries = build_audio(output/'audio-libraries')
    build_write_fault(libraries, fault_source)
    executable = output/'recover.exe'
    subprocess.run([args.mingw, '-O2', '-Wall', '-Wextra', '-Werror', str(fixture),
                    '-lwinmm', '-o', str(executable)], check=True)
    binaries = {}
    for label, header_source in [('native', ROOT/'build/dd2_native_movie_alsa.h'),
                                 ('before', args.before_header or ROOT/'build/dd2_native_movie_alsa.h')]:
        directory = output/(label+'-source'); directory.mkdir()
        (directory/'dd2_native_movie_alsa.h').write_bytes(header_source.read_bytes())
        if label == 'before' and not args.before_header:
            subprocess.run(['patch','-R','-p1','-F0','--fuzz=0','-s','-d',str(directory)],
                           input=(ROOT/'patches/899-native-movie-write-recovery.diff').read_bytes(), check=True)
        binary = output/(label+'-recover')
        subprocess.run(['gcc', '-m32', '-O2', '-no-pie', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        *config('cflags'), str(fixture), '-I'+str(directory),
                        *config('libs'), '-ldl', '-o', str(binary)], check=True)
        binaries[label] = binary
    asan = subprocess.check_output(['gcc', '-m32', '-print-file-name=libasan.so'], text=True).strip()
    environment = {k:v for k,v in os.environ.items() if not k.startswith('DD2_') and k != 'LD_PRELOAD'}
    environment['LD_LIBRARY_PATH'] = ':'.join(map(str, libraries))
    configuration = (f'pcm_type.dd2clock {{ lib "{output}/audio-libraries/$LIB/dd2_clock.so" }}\n'
                     'pcm.!default { type dd2clock }\n')
    expected = b''.join(struct.pack('<hh', 12345+i, -23456+i) for i in range(2205))
    cases = []; negative = []
    for code in (errno.EPIPE, errno.EINTR, errno.EAGAIN):
        for target in ('wine', 'native', 'before'):
            directory = output/f'{target}-{code}'; directory.mkdir(); (directory/'audio').mkdir()
            binary = executable if target == 'wine' else binaries[target]
            env = {**environment, 'DD2_AUDIO_CAPTURE':str(directory/'audio'),
                   'DD2_AUDIO_PROCESS':binary.name[:15], 'DD2_AUDIO_RATE':'22050',
                   'DD2_RECOVERY_ERRNO':str(code), 'DD2_RECOVERY_LOG':str(directory/'fault.jsonl'),
                   'LD_PRELOAD':'dd2_write_fault.so dd2_audio.so'}
            if target == 'wine':
                result = wine_probe(executable, directory, env, alsa_config=configuration)
                shutil.rmtree(directory/'wine-prefix')
                require(result['done'] == 1 and result['waveout_position'] >= 2205,
                        'Wine did not complete caller source')
            else:
                (directory/'asound.conf').write_text(configuration)
                env.update(ALSA_CONFIG_PATH=str(directory/'asound.conf'),
                           LD_PRELOAD=asan+' '+env['LD_PRELOAD'],
                           ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1')
                run = subprocess.run([str(binary)], env=env, capture_output=True, text=True, timeout=5)
                (directory/'run.log').write_text(run.stdout+run.stderr)
                require(run.returncode == 0 and 'runtime error:' not in run.stderr,
                        'Sanitized source-write fixture failed: '+run.stderr)
                result = json.loads(run.stdout)
                require(result['closed'], 'Native transport did not close')
            events = [json.loads(s) for s in (directory/'fault.jsonl').read_text().splitlines()]
            if target == 'before' and code != errno.EAGAIN:
                require(result['start_result'] == -1 and result['done'] == 0 and
                        [e['event'] for e in events] == ['fault'], 'Old source write unexpectedly recovered')
                for path in (directory/'audio').glob('*.pcm'):
                    require(path.stat().st_size == 0, 'Rejected initial source write produced samples')
                cases.append(dict(target=target, errno=code, old_failure_rejected=True, result=result,
                                  fault_events=events, binary_sha256=sha(binary)))
                continue
            if target != 'wine':
                require(result['start_result'] == 0 and result['done'] == 1,
                        'Native did not recover and complete caller source')
            verify_fault(events, code)
            audio = summarize_audio(directory/'audio', require_played=True)
            for kind in ('streams', 'played_streams'):
                require(len(audio[kind]) == 1, 'Unique complete source device required')
                stream = audio[kind][0]
                require(stream['closed'] and stream['format'] == 'S16_LE' and
                        stream['rate'] == 22050 and stream['channels'] == 2, 'Source device format differs')
                actual = (directory/'audio'/stream['file']).read_bytes()
                count = stream['accepted_frames'] if kind == 'streams' else stream['played_frames']
                require(len(actual) == count*4 and hashlib.sha256(actual).hexdigest() == stream['sha256'],
                        'PCM extent/journal binding differs')
                complete = kind == 'streams' or target != 'wine'
                verify_source(actual, expected, complete)
                for label, altered in [('source-bit', bytes([actual[0]^1])+actual[1:]),
                                       ('leading-zero', b'\0'*4+actual),
                                       ('deleted-source-frame', actual[:4]+actual[8:])]:
                    try: verify_source(altered, expected, complete)
                    except RuntimeError: negative.append(dict(target=target, errno=code, kind=kind, mutation=label))
                    else: raise RuntimeError('Accepted altered source: '+label)
            for label, altered in [('lost-retry', [e for e in events if e['event'] != 'retry']),
                                   ('wrong-error', [{**events[0], 'result':0}, *events[1:]])]:
                try: verify_fault(altered, code)
                except RuntimeError: negative.append(dict(target=target, errno=code, mutation=label))
                else: raise RuntimeError('Accepted changed recovery evidence: '+label)
            cases.append(dict(target=target, errno=code, result=result, audio=audio,
                              fault_events=events, binary_sha256=sha(binary)))
            check_space(output)
    report = dict(scope=__doc__, pass_=True, original_game_av_parity='unproven',
                  source_sha256=sha(fixture), fault_source_sha256=sha(fault_source),
                  current_header_sha256=sha(output/'native-source/dd2_native_movie_alsa.h'),
                  before_header_sha256=sha(output/'before-source/dd2_native_movie_alsa.h'),
                  caller_pcm_sha256=hashlib.sha256(expected).hexdigest(), cases=cases,
                  verifier_sha256=sha(Path(__file__)),
                  negative_controls=negative)
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    opened = open_files()
    for pcm in output.glob('*/audio/*.pcm'):
        stat = pcm.stat();require((stat.st_dev,stat.st_ino) not in opened, 'Completed raw PCM still open');pcm.unlink()
    print('PASS Wine/native source-write recovery, old failures rejected;', len(negative), 'negative controls')


if __name__ == '__main__':
    main()
