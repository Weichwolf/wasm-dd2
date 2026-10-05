#!/usr/bin/env python3
"""Compare actual dd2h.exe MCIAVI source/window RGB with production components.

Every observed compressed packet must be literal original AVI input. Native
and WASM independently decode that AVI and render its observed frame/rectangle
inputs. Every source RGB and opaque window ARGB byte is compared without pixel
fitting. This does not prove actual port presentation schedules, audio clocks,
physical display timing or full original/port movie parity.
"""
import argparse
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import zlib

from artifacts import WORK, check_space, prepare_output
from reference.movie_video_observer import RECORD_BYTES, parse, digest
from verify_configuration_persistence import EXE_SHA256, ROOT, require
from verify_movie_codec import packets

RGB_BYTES = 320*192*3
ARGB_BYTES = 640*480*4


def read_exact(file, count):
    parts = []
    while count:
        part = file.read(count)
        if not part:
            raise ValueError('Incomplete production movie component output')
        parts.append(part)
        count -= len(part)
    return b''.join(parts)


def unpack_record(capture, entry):
    packed = (capture/'movie-video-archive'/entry['file']).read_bytes()
    require(len(packed) == entry['compressed_bytes'] and digest(packed) == entry['compressed_sha256'],
            'compressed original movie readback differs')
    inflater = zlib.decompressobj()
    raw = inflater.decompress(packed, RECORD_BYTES+1)
    require(inflater.eof and not inflater.unused_data and not inflater.unconsumed_tail and
            len(raw) == RECORD_BYTES and digest(raw) == entry['raw_sha256'],
            'literal original movie readback differs')
    return raw, parse(raw, entry['serial'])


def equal(expected, actual, label):
    if expected != actual:
        offset = next((i for i, (a, b) in enumerate(zip(expected, actual)) if a != b),
                      min(len(expected), len(actual)))
        raise ValueError(f'{label} differs at byte {offset}')


@contextmanager
def component(command, movie, output, label, frames):
    env = {k: v for k, v in os.environ.items() if not k.startswith('DD2_')}
    with (output/f'{label}.log').open('wb') as log:
        process = subprocess.Popen([*command, str(movie)], env=env, stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE, stderr=log)
        try:
            require(struct.unpack('<4I', read_exact(process.stdout, 16)) == (0x50324444, 320, 192, frames),
                    'production AVI metadata differs')
            yield process
            process.stdin.close()
            require(not process.stdout.read(1), 'extra production movie output')
            require(process.wait(timeout=30) == 0, 'production movie component failed')
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=10)
            process.stdout.close()
            if not process.stdin.closed:
                process.stdin.close()


def render(process, frame, rectangle):
    process.stdin.write(struct.pack('<I4i', frame, *rectangle))
    process.stdin.flush()
    return read_exact(process.stdout, RGB_BYTES+ARGB_BYTES)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--negative-controls', action='store_true')
    parser.add_argument('--before-native',type=Path,help='pre-875 production movie component: source RGB must match, actual window must differ')
    args = parser.parse_args()
    output = prepare_output(args.output)
    require(WORK in output.parents and not output.exists(), 'fresh /tmp/wasm-dd2 output required')
    output.mkdir(parents=True)
    capture = args.capture.resolve()
    checkpoint = json.loads((capture/'checkpoint.json').read_text())
    require(checkpoint['exe_sha256'] == EXE_SHA256 and checkpoint['exe_modified'] is False,
            'supported unmodified original game required')
    metadata = json.loads((capture/'movie-video-observer-build.json').read_text())
    observer = capture/'movie-video-observer.dll'
    require(digest(observer.read_bytes()) == metadata['observer_sha256'] and
            digest((capture/'movie-gdi-observer.dll').read_bytes()) == metadata['gdi']['observer_sha256'] and
            metadata['record_bytes'] == RECORD_BYTES, 'original movie observer binding differs')
    for name, expected in metadata['source_sha256'].items():
        require(digest((ROOT/'tools/reference'/name).read_bytes()) == expected,
                'movie observer source differs')
    manifest = json.loads((capture/'movie-video-archive/manifest.json').read_text())
    require(manifest['pass_'] is True and manifest['record_bytes'] == RECORD_BYTES and
            manifest['frames'] == len(manifest['records']) and 0 < manifest['frames'] < 60000,
            'complete bounded movie observation manifest required')
    movie = ROOT/'DestructionDerby2/Intro.avi'
    dimensions, compressed = packets(movie)
    require(dimensions == (320, 192) and all(compressed), 'supported original intro required')
    # Hold/repeat frames can have identical compressed packets. Bind ordinal
    # identity to the actual PaintFrame trace, then compare the literal packet;
    # never choose a matching frame by its pixels or compressed hash.
    painted = [int(n) for n in re.findall(r'MCIAVI_PaintFrame Painting frame (\d+)',
                                        (capture/'wine.log').read_text(errors='replace'))]
    require(painted, 'missing actual original movie paint trace')
    fixture = ROOT/'tools/movie_window_test.c'
    units = [ROOT/'build'/f'dd2_{name}.c' for name in ('avi', 'cinepak', 'msadpcm', 'movie_surface')]
    native, wasm = output/'native', output/'wasm.js'
    common = ['-std=gnu99', '-O2', '-Wall', '-Wextra', '-Werror', f'-I{ROOT/"build"}',
              *map(str, units), str(fixture)]
    subprocess.run(['gcc', '-m32', '-no-pie', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', *common, '-o', str(native)], check=True)
    subprocess.run(['emcc', *common, '-sNODERAWFS=1', '-sINITIAL_MEMORY=67108864',
                    '-sALLOW_MEMORY_GROWTH=1', '-sEXIT_RUNTIME=1', '-o', str(wasm)], check=True)
    report = dict(scope=__doc__, pass_=False, original_exe_sha256=EXE_SHA256,
                  avi_sha256=digest(movie.read_bytes()), observer_sha256=metadata['observer_sha256'],
                  manifest_sha256=digest((capture/'movie-video-archive/manifest.json').read_bytes()),
                  sources={str(p.relative_to(ROOT)): digest(p.read_bytes()) for p in [fixture, *units]},
                  frames=manifest['frames'], source_frames=len(compressed), targets={}, negative_controls=[])
    rows = []
    last_frame, last_decode = -1, 0
    skipped=None
    for serial, entry in enumerate(manifest['records']):
        require(entry['serial'] == serial and entry['file'] == f'{serial:06d}.zlib',
                'missing or reordered movie record')
        _, row = unpack_record(capture, entry)
        if skipped is None:skipped=row['private_mci_window_draws']
        require(row['private_mci_window_draws']==skipped and skipped+serial<len(painted),
                'movie window observation/trace count differs')
        frame=painted[skipped+serial]
        require(0<=frame<len(compressed) and row['packet']==compressed[frame],
                'observed compressed packet differs from original AVI at its actual traced ordinal')
        require(frame >= last_frame and row['decode_serial'] >= last_decode,
                'original movie decoding moved backwards')
        if row['decode_serial'] == last_decode:
            require(frame == last_frame, 'cached original movie packet identity changed')
        rows.append(dict(serial=serial, source_frame=frame, decode_serial=row['decode_serial'],
                         rectangle=row['rectangle'], observed_ms=row['observed_ms'],
                         private_mci_window_draws=row['private_mci_window_draws']))
        last_frame, last_decode = frame, row['decode_serial']
    skipped = rows[0]['private_mci_window_draws']
    require(all(row['private_mci_window_draws'] == skipped for row in rows) and
            painted and [row['source_frame'] for row in rows] == painted[skipped:],
            'actual DrawDib readbacks differ from the original PaintFrame trace')
    original_frames=set(painted[skipped:])
    require(original_frames and min(original_frames)==0 and max(original_frames)>=len(compressed)-2,
            'original intro observation did not reach its final presentations')
    if args.before_native:
        _,observed=unpack_record(capture,manifest['records'][0])
        with component([str(args.before_native.resolve())],movie,output,'before-native',len(compressed)) as process:
            actual=render(process,rows[0]['source_frame'],rows[0]['rectangle'])
            equal(observed['source_rgb'],actual[:RGB_BYTES],'pre-875 source RGB')
            try:equal(observed['window_argb'],actual[RGB_BYTES:],'pre-875 actual window')
            except ValueError as error:
                report['before_native']=dict(rejected=True,binary_sha256=digest(args.before_native.read_bytes()),
                    error=str(error),source_rgb_exact=True,
                    changed_window_bytes=sum(a!=b for a,b in zip(observed['window_argb'],actual[RGB_BYTES:])))
            else:raise ValueError('Accepted the pre-875 nearest-neighbor window output')
    for label, command in [('native-asan-ubsan', [str(native)]), ('wasm', ['node', str(wasm)])]:
        target_sha = hashlib.sha256()
        with component(command, movie, output, label, len(compressed)) as process:
            for entry, row in zip(manifest['records'], rows):
                _, observed = unpack_record(capture, entry)
                actual = render(process, row['source_frame'], row['rectangle'])
                equal(observed['source_rgb'], actual[:RGB_BYTES], f'{label} source frame {row["source_frame"]}')
                equal(observed['window_argb'], actual[RGB_BYTES:], f'{label} actual original window frame {row["source_frame"]}')
                target_sha.update(actual)
                if args.negative_controls and entry['serial'] == 0:
                    for name, data, offset in [('source-bit', observed['source_rgb'], 1000),
                                               ('window-bit', observed['window_argb'], 640*48*4+1000),
                                               ('last-window-bit', observed['window_argb'], ARGB_BYTES-4),
                                               ('truncated-window', observed['window_argb'][:-4], None)]:
                        changed = bytearray(data)
                        if offset is not None:
                            changed[offset] ^= 1
                        expected = actual[:RGB_BYTES] if name == 'source-bit' else actual[RGB_BYTES:]
                        try:
                            equal(bytes(changed), expected, name)
                        except ValueError:
                            report['negative_controls'].append(dict(target=label, case=name, rejected=True))
                        else:
                            raise ValueError('Accepted changed original movie pixels')
        report['targets'][label] = dict(pass_=True, observations=len(rows),
            source_rgb_bytes=RGB_BYTES*len(rows), actual_window_argb_bytes=ARGB_BYTES*len(rows),
            source_and_window_sha256=target_sha.hexdigest())
        print('Exact original movie source/window bytes:', label, len(rows), flush=True)
    report.update(pass_=True, observed_presentations=rows,
                  original_unpresented_source_frames=sorted(set(range(len(compressed)))-original_frames),
                  original_port_presentation_timing='pending', actual_port_sinks='separate existing tests')
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    check_space(output)


if __name__ == '__main__':
    main()
