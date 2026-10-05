#!/usr/bin/env python3
"""Compare actual Wine short-AVI drain behavior with production MCI components.

The fixture retains two original Cinepak packets and one complete original
ADPCM block. Wine uses the real-time virtual ALSA device. The port clock is
controlled; this does not prove full original/live-port A/V timing or PCM tails.
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
from reference.audio import build_audio, summarize_audio
from verify_configuration_persistence import ROOT, require
from verify_movie_codec import riff_chunks
from verify_sound_cursor import wine_probe


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
    args = parser.parse_args();output = prepare_output(args.output)
    require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    movie = output/'short.avi';movie.write_bytes(short_avi(ROOT/'DestructionDerby2/Intro.avi'))
    source = ROOT/'tools/movie_drain_test.c';exe = output/'movie-drain.exe'
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
                        arguments=('Z:'+str(movie).replace('/', '\\'),), wine_debug='-all,+mciavi')
    audio = summarize_audio(reference/'audio', require_played=True)
    trace = (reference/'wine.log').read_text(errors='replace')
    marker = re.search(r'MCIAVI_mciPlay Playing from frame=0 to frame=1', trace)
    require(marker is not None, 'actual short-AVI exclusive play endpoint differs')
    painted = [int(n) for n in re.findall(r'MCIAVI_PaintFrame Painting frame (\d+)', trace[marker.end():])]
    require(painted == [0] and 80 <= actual['elapsed_ms'] < 300, 'actual Wine short drain behavior differs')
    require(len(audio['streams']) == len(audio['played_streams']) == 1 and
            audio['streams'][0]['format'] == 'S16_LE' and audio['played_streams'][0]['played_frames'] >= 1012,
            'real-time PCM16 movie device observation required')
    report = dict(scope=__doc__, original_port_live_parity='unproven', fixture_source_sha256=sha(source),
                  original_avi_sha256=sha(ROOT/'DestructionDerby2/Intro.avi'), short_avi_sha256=sha(movie),
                  wine=actual, wine_frames=painted, wine_audio=audio, targets=[])
    shutil.rmtree(reference/'wine-prefix')
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
    units = [ROOT/'build'/f'dd2_{name}.c' for name in ('movie', 'movie_surface', 'avi', 'cinepak', 'msadpcm')]
    common = ['-O2', '-std=gnu99', '-ffunction-sections', '-fdata-sections', '-I'+str(ROOT/'build'),
              *map(str, units), str(source), '-Wl,--gc-sections']
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
                            notifications=1, closed=2 if failed == 2 else 1)
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
    report['sources'] = {str(p.relative_to(ROOT)):sha(p) for p in units}
    report['pass_'] = True
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    check_space(output)
    print('Production native/ASan/UBSan and WASM: immediate final draw, 100-ms drain retries, wrap and audio failure pass', flush=True)


if __name__ == '__main__':
    main()
