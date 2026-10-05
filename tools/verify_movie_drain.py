#!/usr/bin/env python3
"""Compare actual Wine short-AVI drain behavior with production MCI components.

The fixture retains two original Cinepak packets and one complete original
ADPCM block. Wine uses the real-time virtual ALSA device. The port clock is
controlled; this does not prove full original/live-port A/V timing or PCM tails.
Wine must close its device before MCI_PLAY returns; native and WASM must reset
audio before notifying success. A previous production source must fail that
same notification ordering invariant when supplied as a negative control.
Optional scoped ALSA reset faults additionally compare real Wine MCI success
with the production native MCI/device notification and its old-header failure.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess

from artifacts import WORK, check_space, prepare_output
from reference.audio import build_audio, build_reset_fault, summarize_audio
from verify_configuration_persistence import ROOT, require
from verify_movie_codec import riff_chunks
from verify_sound_cursor import wine_probe
from verify_native_sdl import config as sdl_config


def chunk(kind, data):
    return kind+struct.pack('<I', len(data))+data+(b'\0' if len(data)&1 else b'')


def short_avi(source):
    rows = list(riff_chunks(source))
    main = bytearray(next(d for k, d in rows if k == b'avih'))
    struct.pack_into('<I', main, 16, 2)
    streams = []
    for kind in (b'vids', b'auds'):
        header = bytearray(next(d for k, d in rows if k == b'strh' and d[:4] == kind))
        struct.pack_into('<I', header, 16, 0)
        struct.pack_into('<I', header, 32, 2 if kind == b'vids' else 1)
        format_ = next(d for k, d in rows if k == b'strf' and
                       (len(d) >= 40 and d[16:20] == b'cvid' if kind == b'vids' else d[:2] == b'\x02\0'))
        streams.append(chunk(b'LIST', b'strl'+chunk(b'strh', header)+chunk(b'strf', format_)))
    video = [d for k, d in rows if k == b'00dc'][:2]
    audio = next(d for k, d in rows if k == b'01wb')[:1024]
    packets = [(b'00dc', video[0]), (b'01wb', audio), (b'00dc', video[1])]
    index = bytearray();offset = 4
    for kind, data in packets:
        index.extend(struct.pack('<4sIII', kind, 0x10, offset, len(data)))
        offset += len(chunk(kind, data))
    body = (b'AVI '+chunk(b'LIST', b'hdrl'+chunk(b'avih', main)+b''.join(streams))+
            chunk(b'LIST', b'movi'+b''.join(chunk(k, d) for k, d in packets))+chunk(b'idx1', index))
    return chunk(b'RIFF', body)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--reference-only', action='store_true')
    parser.add_argument('--mingw', default='i686-w64-mingw32-gcc')
    parser.add_argument('--before-native', type=Path)
    parser.add_argument('--before-movie-source', type=Path)
    parser.add_argument('--reset-errors', action='store_true', help='observe actual Wine MCI completion despite injected ALSA reset errors')
    parser.add_argument('--before-reset-error-header', type=Path, help='require an old ALSA header to notify MCI failure after completed-source reset warnings')
    args = parser.parse_args();output = prepare_output(args.output)
    if args.before_reset_error_header and (not args.reset_errors or args.reference_only):
        parser.error('--before-reset-error-header requires --reset-errors without --reference-only')
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    movie = output/'short.avi';movie.write_bytes(short_avi(ROOT/'DestructionDerby2/Intro.avi'))
    snapshot=output/'production';snapshot.mkdir()
    source=snapshot/'movie_drain_test.c';source.write_bytes((ROOT/'tools/movie_drain_test.c').read_bytes())
    names=('movie','movie_surface','avi','cinepak','msadpcm')
    if not args.reference_only:
        for path in [*(ROOT/'build').glob('*.h'), *(ROOT/'build'/f'dd2_{name}.c' for name in names)]:
            (snapshot/path.name).write_bytes(path.read_bytes())
    exe = output/'movie-drain.exe'
    subprocess.run([args.mingw, '-O2', '-Wall', '-Wextra', '-Werror',
                    str(source), '-lwinmm', '-o', str(exe)], check=True)
    libraries = build_audio(output/'audio-libraries')
    env = {k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    reference = output/'wine';reference.mkdir();(reference/'audio').mkdir()
    env.update(DD2_AUDIO_CAPTURE=str(reference/'audio'), DD2_AUDIO_PROCESS='movie-drain.exe',
               DD2_AUDIO_RATE='22050', LD_PRELOAD='dd2_audio.so',
               LD_LIBRARY_PATH=':'.join(map(str, libraries)))
    config = f'pcm_type.dd2clock {{ lib "{output}/audio-libraries/$LIB/dd2_clock.so" }}\npcm.!default {{ type dd2clock }}\n'
    actual = wine_probe(exe, reference, env, alsa_config=config,
                        arguments=tuple('Z:'+str(p).replace('/', '\\') for p in (movie, reference/'audio')),
                        wine_debug='-all,+mciavi')
    audio = summarize_audio(reference/'audio', require_played=True)
    trace = (reference/'wine.log').read_text(errors='replace')
    marker = re.search(r'MCIAVI_mciPlay Playing from frame=0 to frame=1', trace)
    require(marker is not None, 'actual short-AVI exclusive play endpoint differs')
    painted = [int(n) for n in re.findall(r'MCIAVI_PaintFrame Painting frame (\d+)', trace[marker.end():])]
    require(painted == [0] and 80 <= actual['elapsed_ms'] < 300, 'actual Wine short drain behavior differs')
    require(actual['device_closed_before_play_return'] is True, 'Wine device not closed before play returned')
    require(len(audio['streams']) == len(audio['played_streams']) == 1 and
            audio['streams'][0]['format'] == 'S16_LE' and audio['played_streams'][0]['played_frames'] >= 1012,
            'real-time PCM16 movie device observation required')
    report = dict(scope=__doc__, original_port_live_parity='unproven', fixture_source_sha256=sha(source),
                  original_avi_sha256=sha(ROOT/'DestructionDerby2/Intro.avi'), short_avi_sha256=sha(movie),
                  wine=actual, wine_frames=painted, wine_audio=audio, targets=[])
    shutil.rmtree(reference/'wine-prefix')
    if args.reset_errors:
        build_reset_fault(libraries)
        failed_reset = output/'wine-reset-errors';failed_reset.mkdir();(failed_reset/'audio').mkdir()
        fault_log = failed_reset/'reset-fault.jsonl'
        reset_env = {**env, 'DD2_AUDIO_CAPTURE':str(failed_reset/'audio'),
                     'DD2_RESET_FAULT_LOG':str(fault_log), 'LD_PRELOAD':'dd2_reset_fault.so dd2_audio.so'}
        reset_result = wine_probe(exe, failed_reset, reset_env, alsa_config=config,
                                 arguments=tuple('Z:'+str(p).replace('/', '\\') for p in (movie, failed_reset/'audio')),
                                 wine_debug='-all,+mciavi,+alsa')
        reset_audio = summarize_audio(failed_reset/'audio', require_played=True)
        faults = [json.loads(s) for s in fault_log.read_text().splitlines()]
        require(len(faults)>=2 and all(e['result']==-5 for e in faults), 'actual Wine reset faults not observed')
        require(reset_result['device_closed_before_play_return'] is True, 'reset failure prevented actual MCI completion/close')
        reset_trace = (failed_reset/'wine.log').read_text(errors='replace')
        require('snd_pcm_reset failed' in reset_trace, 'actual Wine ALSA reset failure warning missing')
        for kind, count_key in (('streams','accepted_frames'), ('played_streams','played_frames')):
            require(len(reset_audio[kind])==1 and reset_audio[kind][0]['closed'] and
                    reset_audio[kind][0][count_key]>=1012, 'reset failure truncated original movie source')
            expected=(reference/'audio'/audio[kind][0]['file']).read_bytes()[:1012*4]
            observed=(failed_reset/'audio'/reset_audio[kind][0]['file']).read_bytes()[:1012*4]
            require(observed==expected, 'reset failure changed original source bytes')
        require(all(e['pid']==reset_audio['streams'][0]['pid'] for e in faults), 'reset faults affected a different process')
        report['wine_reset_errors'] = dict(mci_completed=True, actual=reset_result, faults=faults, audio=reset_audio,
                                          fault_source_sha256=sha(ROOT/'tools/reference/alsa_reset_fault.c'),
                                          trace_sha256=sha(failed_reset/'wine.log'))
        shutil.rmtree(failed_reset/'wine-prefix')
    unavailable = output/'wine-audio-unavailable';unavailable.mkdir()
    silent_env = {k:v for k,v in env.items() if k not in ('LD_PRELOAD', 'DD2_AUDIO_CAPTURE')}
    silent = wine_probe(exe, unavailable, silent_env,
                        alsa_config='pcm.!default { type hw card "DD2_NONEXISTENT" }\n',
                        arguments=('Z:'+str(movie).replace('/', '\\'),), wine_debug='-all,+mciavi')
    silent_trace = (unavailable/'wine.log').read_text(errors='replace')
    play_at = re.search(r'MCIAVI_mciPlay Playing from frame=0 to frame=1', silent_trace)
    require(play_at is not None and 'Can\'t open low level audio device' in silent_trace and
            [int(n) for n in re.findall(r'MCIAVI_PaintFrame Painting frame (\d+)',
                                       silent_trace[play_at.end():])] == [0] and
            silent['elapsed_ms'] < 40, 'actual unavailable-audio final-draw completion differs')
    report['wine_audio_unavailable'] = silent
    shutil.rmtree(unavailable/'wine-prefix')
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('Actual real-time Wine short-AVI play:', actual, flush=True)
    if args.reference_only:
        check_space(output);return
    units = [snapshot/f'dd2_{name}.c' for name in names]
    common = ['-O2', '-std=gnu99', '-ffunction-sections', '-fdata-sections', '-I'+str(snapshot),
              *map(str, units), str(source), '-Wl,--gc-sections']
    if args.reset_errors:
        asan=subprocess.check_output(['gcc','-m32','-print-file-name=libasan.so'],text=True).strip()
        live=[]
        for before in ([False,True] if args.before_reset_error_header else [False]):
            case=output/('before-reset-errors' if before else 'device-reset-errors');case.mkdir();(case/'audio').mkdir()
            header=snapshot/'dd2_native_movie_alsa.h'
            if before:
                header=case/'dd2_native_movie_alsa.h';header.write_bytes(args.before_reset_error_header.read_bytes())
            # Quoted includes search the fixture's directory before -I paths.
            # Keep each fixture next to its intended production/old header.
            fixture=case/source.name;fixture.write_bytes(source.read_bytes())
            live_common=[str(fixture) if arg==str(source) else arg for arg in common]
            binary=case/('before-drain' if before else 'device-drain')
            subprocess.run(['gcc','-m32','-no-pie','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                            '-DDD2_MOVIE_DRAIN_DEVICE','-I'+str(case),*sdl_config('cflags'),*live_common,
                            *sdl_config('libs'),'-ldl','-o',str(binary)],check=True)
            (case/'asound.conf').write_text(config)
            live_env={**env,'ALSA_CONFIG_PATH':str(case/'asound.conf'),
                      'DD2_AUDIO_CAPTURE':str(case/'audio'),'DD2_AUDIO_PROCESS':binary.name,
                      'DD2_RESET_FAULT_LOG':str(case/'reset-fault.jsonl'),
                      'LD_PRELOAD':asan+' dd2_reset_fault.so dd2_audio.so',
                      'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
            run=subprocess.run([str(binary),str(movie)],env=live_env,capture_output=True,text=True,timeout=10)
            (case/'run.log').write_text(run.stdout+run.stderr)
            require(run.returncode==0 and 'runtime error:' not in run.stderr,'actual native MCI/device fixture failed: '+run.stderr)
            result=json.loads(run.stdout)
            live_audio=summarize_audio(case/'audio',require_played=True)
            live_faults=[json.loads(s) for s in (case/'reset-fault.jsonl').read_text().splitlines()]
            require(len(live_faults)>=2 and all(e['result']==-5 for e in live_faults),'actual native reset faults missing')
            for kind,key in (('streams','accepted_frames'),('played_streams','played_frames')):
                require(len(live_audio[kind])==1 and live_audio[kind][0]['closed'] and live_audio[kind][0][key]>=1012,
                        'actual native reset warning truncated source')
                expected=(reference/'audio'/audio[kind][0]['file']).read_bytes()[:1012*4]
                require((case/'audio'/live_audio[kind][0]['file']).read_bytes()[:1012*4]==expected,
                        'actual native reset warning changed original source bytes')
            require(result['frames']==1 and result['notifications']==1 and result['device_closed_before_notify'],
                    'actual native reset warning changed MCI lifetime')
            try:require(result['notify_result']==1,'reset warning changed successful MCI notification')
            except RuntimeError as error:
                require(before and 'reset warning changed successful MCI notification' in str(error) and result['notify_result']==8,
                        'unexpected reset-warning notification failure: '+str(error))
            else:require(not before,'old header unexpectedly preserved successful MCI notification')
            live.append(dict(before=before,result=result,faults=live_faults,audio=live_audio,
                             header_sha256=sha(header),binary_sha256=sha(binary),
                             failed_notification_rejected=before))
        report['native_device_reset_errors']=live
    native = output/'native';wasm = output/'wasm.js'
    subprocess.run(['gcc', '-m32', '-no-pie', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', *common, '-o', str(native)], check=True)
    subprocess.run(['emcc', *common, '-sNODERAWFS=1', '-sEXIT_RUNTIME=1', '-o', str(wasm)], check=True)
    cases = [(0, 0), (1, 0), (46, 0), (100, 0), (101, 0), (250, 0), (0, 1), (0, 2), (101, 2)]
    for name, command in [('native-asan-ubsan', [str(native)]), ('wasm', ['node', str(wasm)])]:
        rows = []
        for done, failed in cases:
            run = subprocess.run([*command, str(movie), str(done), str(failed)],
                                 capture_output=True, text=True, check=True, timeout=15)
            observed = json.loads(run.stdout)
            elapsed = 0 if failed == 1 else (done+99)//100*100
            expected = dict(frames=1, elapsed_ms=elapsed,
                            audio_queries=[] if failed == 1 else list(range(0, elapsed+1, 100)),
                            notifications=1, closed=1 if failed == 1 else 2,
                            closed_at_notify=0 if failed == 1 else 1)
            require(observed == expected, f'{name} actual drain queries/completion differ: {observed}')
            rows.append(dict(done_ms=done, audio_unavailable=failed == 1,
                             output_error=failed == 2, actual=observed))
        report['targets'].append(dict(target=name, cases=rows))
    if args.before_native:
        before = subprocess.run([str(args.before_native.resolve()), str(movie), '46', '0'],
                                capture_output=True, text=True, check=True, timeout=15)
        observed = json.loads(before.stdout)
        require(observed == dict(frames=1, elapsed_ms=46, audio_queries=list(range(40, 47)),
                                 notifications=1, closed=1), 'before binary failed for another reason')
        report['before_native'] = dict(rejected=True, binary_sha256=sha(args.before_native), actual=observed)
    if args.before_movie_source:
        before_source=snapshot/'before_movie.c';before_source.write_bytes(args.before_movie_source.read_bytes())
        before=output/'before-native'
        before_common=[str(before_source) if token==str(units[0]) else token for token in common]
        subprocess.run(['gcc','-m32','-no-pie','-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all',*before_common,'-o',str(before)],check=True)
        rejected=[]
        for done in (0,46,101,250):
            result=subprocess.run([str(before),str(movie),str(done),'0'],capture_output=True,text=True,timeout=15)
            (output/f'before-{done}.log').write_text(result.stdout+result.stderr)
            require(result.returncode!=0 and 'audio reset before notification' in result.stderr and
                    'AddressSanitizer' not in result.stderr and 'runtime error:' not in result.stderr,
                    'old production movie source failed for another reason')
            rejected.append(dict(done_ms=done,reset_before_notify_rejected=True))
        report['before_movie_source']=dict(source_sha256=sha(before_source),cases=rejected)
    report['sources'] = {p.name:sha(p) for p in units}
    report['pass_'] = True
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    for pcm in output.glob('*/audio/*.pcm'): pcm.unlink()
    check_space(output)
    print('Production native/ASan/UBSan and WASM: final draw, 100-ms drain retries, reset before notify, wrap and audio failure pass', flush=True)


if __name__ == '__main__':
    main()
