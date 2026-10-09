#!/usr/bin/env python3
"""Verify actual Native SDL and browser WebAudio Redbook output/ownership.

Native SDL's disk sink preserves the device's callback PCM for independent
integer resampling comparison. The real browser output node supplies floating
stereo buffers, compared independently with the selected CDDA bytes. This is
rewrite playback evidence, not original audio engine parity or speaker hardware.
"""
import argparse
from datetime import datetime, timezone
from functools import partial
from http.server import ThreadingHTTPServer
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import threading
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, open_files, prepare_output, run_bounded
from rewrite.quality import ROOT
from rewrite.serve import BUILD, Handler
from rewrite.verify_window import NativeWindow, build_sanitized


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def expected_prefix(source, frames, rate=48000):
    output = bytearray(frames * 4)
    for index in range(frames):
        frame, fraction = divmod(index * 44100, rate)
        for channel in range(2):
            first = struct.unpack_from('<h', source, frame * 4 + channel * 2)[0]
            second = struct.unpack_from('<h', source, (frame + 1) * 4 + channel * 2)[0]
            numerator = first * (rate - fraction) + second * fraction
            value = (abs(numerator) + rate // 2) // rate
            value = -value if numerator < 0 else value
            struct.pack_into('<h', output, index * 4 + channel * 2, value)
    return bytes(output)


def native_check(output, archive, binary, label):
    raw = output / (label + '.pcm')
    if raw.exists():
        stat = raw.stat()
        if (stat.st_dev, stat.st_ino) in open_files():
            raise ValueError('Capture is in use: ' + str(raw))
        raw.unlink()
    ui = NativeWindow(output, archive, binary, label)
    ui.env.update(SDL_AUDIODRIVER='disk', SDL_DISKAUDIOFILE=str(raw), SDL_DISKAUDIODELAY='21')
    try:
        ui.start()
        ui.wait(lambda: raw.exists() and raw.stat().st_size >= 48000 * 4 and any(raw.read_bytes()))
        # Capture a complete first second before testing real transport keys.
        expected = expected_prefix((archive.parent / 'Redbook/track02.cdda').read_bytes(), 48000)
        captured = raw.read_bytes()
        first_expected = next(index for index in range(0, len(expected), 4) if any(expected[index:index + 4]))
        first_actual = next(index for index in range(0, len(captured), 4) if any(captured[index:index + 4]))
        startup = first_actual - first_expected
        if startup < 0 or startup > 48000 * 2 * 4 or any(captured[:startup]):
            raise ValueError(label + ': invalid SDL device startup silence')
        ui.wait(lambda: raw.stat().st_size >= startup + len(expected))
        captured = raw.read_bytes()
        prefix = captured[startup:startup + len(expected)]
        if prefix != expected or not any(prefix):
            raise ValueError(label + ': actual SDL output differs from original track 2 resampling')
        ui.command('key', 'F10')
        time.sleep(.2)
        start = raw.stat().st_size
        time.sleep(.2)
        paused = raw.read_bytes()[start:]
        if len(paused) < 4096 or any(paused):
            raise ValueError(label + ': paused music emitted nonzero PCM')
        ui.command('key', 'F10')
        time.sleep(.3)
        if not any(raw.read_bytes()[-4096:]):
            raise ValueError(label + ': resume stayed silent')
        replacements = []
        for key, track in (('F12', 3), ('F11', 2)):
            source = archive.parent / f'Redbook/track{track:02d}.cdda'
            selected = expected_prefix(source.read_bytes(), 12000)
            start = raw.stat().st_size
            ui.command('key', key)
            ui.wait(lambda: selected in raw.read_bytes()[start:])
            replacements.append(dict(key=key, track=track, compared_frames=12000,
                                     source_sha256=digest(source),
                                     expected_pcm_sha256=hashlib.sha256(selected).hexdigest()))
        ui.command('key', 'Escape')
        if ui.process.wait(timeout=10) != 0:
            raise ValueError(label + ': application failed on audio close')
        return dict(pass_=True, compared_frames=48000, stereo=True, rate=48000,
                    device_startup_silent_frames=startup // 4,
                    original_source_sha256=digest(archive.parent / 'Redbook/track02.cdda'),
                    expected_pcm_sha256=hashlib.sha256(expected).hexdigest(),
                    device_pcm_sha256=hashlib.sha256(prefix).hexdigest(),
                    full_capture_sha256=digest(raw), replacements=replacements,
                    controls=['F10 pause/resume', 'F12/F11 replace', 'Escape close'])
    finally:
        ui.close()


def preparation_check(output, archive, binary, label):
    pattern = output / (label + '.cdda')
    malformed = output / (label + '-malformed.cdda')
    raw = output / (label + '.pcm')
    log_path = output / (label + '.log')
    pattern.write_bytes(struct.pack('<hh', 12000, -6000) * 4093)
    malformed.write_bytes(b'bad')
    env = dict(os.environ, SDL_AUDIODRIVER='disk', SDL_DISKAUDIOFILE=str(raw),
               SDL_DISKAUDIODELAY='21', ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1')
    with log_path.open('wb') as log:
        run_bounded([str(binary), str(archive), str(pattern), str(malformed)],
                    directory=output, timeout=15, env=env, stdout=log,
                    stderr=subprocess.STDOUT, check=True)
    rows = [json.loads(line) for line in log_path.read_text().splitlines() if line.startswith('{')]
    if len(rows) != 1 or not rows[0]['pass_']:
        raise ValueError(label + ': preparation checks failed')
    captured = raw.read_bytes()
    frames = list(struct.iter_unpack('<hh', captured))
    first = next((index for index, frame in enumerate(frames) if frame != (0, 0)), None)
    if first is None or first < 1024 or not any(frame == (0, 0) for frame in frames[first:]):
        raise ValueError(label + ': missing READY/start/pause output transitions')
    if any(frame not in ((0, 0), (6000, -3000)) for frame in frames):
        raise ValueError(label + ': actual preparation/start PCM or gain differs')
    return dict(pass_=True, state_checks=rows[0]['checks'], binary_sha256=digest(binary),
                source_pcm_sha256=digest(pattern), capture_sha256=digest(raw),
                startup_ready_silent_frames=first, captured_frames=len(frames),
                contract='READY stays at zero through callbacks; explicit start, replacement, '
                         'gain, pause, failed READY/PLAYING/PAUSED loads and joined close')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-music-output-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    source_names = [str(path.relative_to(ROOT)) for path in (ROOT / 'src').rglob('*') if path.is_file()]
    source_names += ['CMakeLists.txt', 'tests/audio_device_test.c', 'tests/music_prepare_export.c',
                     'tools/rewrite/verify_music_output.py', 'tools/rewrite/verify_music_browser.js',
                     'tools/rewrite/verify_window.py']
    source_hashes = {name: digest(ROOT / name) for name in source_names}
    archive = (ROOT / 'DestructionDerby2/Dirinfo').resolve()
    report = dict(pass_=False, scope=__doc__.strip(), source_sha256=source_hashes,
                  verified_at=datetime.now(timezone.utc).isoformat(), archive_sha256=digest(archive))
    server = thread = sanitized = prepared_sanitized = None
    try:
        report['native'] = native_check(output, archive, WORK / 'rewrite-native/dd2_app', 'native-music')
        sanitized = build_sanitized(output)
        report['sanitized'] = native_check(output, archive, sanitized, 'sanitized-music')
        report['preparation'] = {
            'native': preparation_check(output, archive, WORK / 'rewrite-native/dd2_music_prepare_export',
                                        'native-preparation')}
        preparation_output = output / 'sanitized-preparation'
        preparation_output.mkdir()
        prepared_sanitized = build_sanitized(preparation_output, ROOT / 'tests/music_prepare_export.c')
        report['preparation']['sanitized'] = preparation_check(
            preparation_output, archive, prepared_sanitized, 'sanitized-preparation')
        report['sanitizer_scope'] = 'All rewrite C application units instrumented; SDL2 and pinned release SoftGL uninstrumented'
        server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(BUILD)))
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        report['browser'] = {}
        for rate in ('default', '48000'):
            with (output / ('browser-' + rate + '.log')).open('wb') as log:
                command = ['node', str(ROOT / 'tools/rewrite/verify_music_browser.js'),
                           f'http://127.0.0.1:{server.server_port}/', str(archive),
                           str(archive.parent / 'Redbook'), str(output)]
                if rate != 'default': command.append(rate)
                run_bounded(command, directory=output, timeout=120, stdout=log,
                            stderr=subprocess.STDOUT, check=True)
            report['browser'][rate] = json.loads((output / ('browser-music-' + rate + '.json')).read_text())
            if not report['browser'][rate]['pass_']:
                raise ValueError('Browser playback not verified: ' + rate)
        for log in output.rglob('*.log'):
            content = log.read_text(errors='replace')
            if 'Sanitizer:' in content or 'runtime error:' in content:
                raise ValueError('Sanitizer finding: ' + log.name)
        if source_hashes != {name: digest(ROOT / name) for name in source_names}:
            raise ValueError('Sources changed during verification')
        report['binary_sha256'] = {str(path): digest(path) for path in
                                  (WORK / 'rewrite-native/dd2_app', sanitized, BUILD / 'dd2_app.js', BUILD / 'dd2_app.wasm')}
        report['pass_'] = True
    finally:
        if server is not None:
            server.shutdown(); server.server_close(); thread.join(timeout=5)
        report['log_sha256'] = {str(path.relative_to(output)): digest(path) for path in output.rglob('*.log')}
        (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        if report['pass_']:
            if sanitized is not None: sanitized.unlink()
            if prepared_sanitized is not None: prepared_sanitized.unlink()
            opened = open_files()
            for path in output.rglob('*'):
                if path.suffix in ('.pcm', '.log', '.cdda'):
                    stat = path.stat()
                    if (stat.st_dev, stat.st_ino) not in opened: path.unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
