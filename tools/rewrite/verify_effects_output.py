#!/usr/bin/env python3
"""Check source-bank gameplay effects through Native SDL and real WebAudio.

The Native device sink compares an actual application's idle engine PCM; a
sanitized application checks bank/device lifetime. Browser callbacks compare the
complete four-voice sum against an independent WAVE reader/resampling oracle.
This is scoped rewrite playback evidence, not original audio-engine parity.
"""
import argparse
from datetime import datetime, timezone
from functools import partial
from http.server import ThreadingHTTPServer
import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import sys
import threading
import time
import wave

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, open_files, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.serve import BUILD, Handler
from rewrite.verify_levels import assets
from rewrite.verify_window import NativeWindow, build_sanitized


def digest(path):
    with Path(path).open('rb') as stream: return hashlib.file_digest(stream, 'sha256').hexdigest()


def motor_prefix(archive):
    bank = assets(archive.read_bytes())['VAGS\\BANK1.SBK']
    start, size = struct.unpack_from('<II', bank, 16)
    with wave.open(io.BytesIO(bank[start:start + size]), 'rb') as source:
        assert source.getnchannels() == 1 and source.getsampwidth() == 1
        samples = source.readframes(source.getnframes())
    output = bytearray()
    for tick in range(2400):
        frame, fraction = divmod(tick * 6192, 48000)
        first, second = (samples[frame] - 128) * 256, (samples[frame + 1] - 128) * 256
        value = first * (48000 - fraction) + second * fraction
        value = (-1 if value < 0 else 1) * ((abs(value) + 24000) // 48000)
        value *= 80
        value = (-1 if value < 0 else 1) * ((abs(value) + 128) // 256)
        output.extend(struct.pack('<hh', value, value))
    return bytes(output)


def native_check(output, archive, binary, label):
    raw = output / (label + '.pcm')
    if raw.exists():
        stat = raw.stat()
        if (stat.st_dev, stat.st_ino) in open_files(): raise ValueError('Capture is in use')
        raw.unlink()
    ui = NativeWindow(output, archive, binary, label)
    ui.env.update(SDL_AUDIODRIVER='disk', SDL_DISKAUDIOFILE=str(raw), SDL_DISKAUDIODELAY='21')
    try:
        ui.start()
        time.sleep(.6)
        ui.command('key', 'F10') # pause optional Native autoplay music
        for _ in range(7):
            ui.command('key', 'PageUp'); time.sleep(.6)
        time.sleep(.2)
        start = raw.stat().st_size
        ui.command('key', 'Return')
        expected = motor_prefix(archive)
        ui.wait(lambda: expected in raw.read_bytes()[start:], timeout=10)
        ui.command('key', 'p'); time.sleep(.15)
        paused_start = raw.stat().st_size; time.sleep(.2)
        if any(raw.read_bytes()[paused_start:]): raise ValueError('Paused Native effects emitted PCM')
        ui.command('key', 'r')
        ui.command('key', 'Escape')
        if ui.process.wait(timeout=10) != 0: raise ValueError('Application failed on audio close')
        return dict(pass_=True, idle_frames=2400, frequency=6192, output_rate=48000, gain=80,
                    original_engine_pcm_sha256=hashlib.sha256(expected).hexdigest(),
                    capture_sha256=digest(raw), controls=['real Enter starts engine', 'pause silences device', 'reset/close'])
    finally: ui.close()


def units(output):
    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT / 'src'), '-I', str(ROOT / 'tests'),
             '-Wall', '-Wextra', '-Wpedantic', '-Wno-unused-parameter', '-Wno-unused-function',
             '-fno-strict-aliasing', '-ffast-math', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    libraries = [WORK / ('rewrite-native/libdd2_' + name + '.a') for name in ('game', 'ai', 'physics', 'assets')]
    results = {}
    for name, sources in {'effects': ['src/audio/effects.c', 'src/audio/mixer.c', 'src/assets/audio.c'],
                           'game_sound': ['src/game/sound_events.c']}.items():
        binary = output / (name + '-sanitized')
        with (output / (name + '-sanitizer.log')).open('wb') as log:
            run_bounded([tool('clang'), *flags, str(ROOT / ('tests/' + name + '_test.c')),
                         *[str(ROOT / source) for source in sources], *map(str, libraries), '-lm', '-o', str(binary)],
                        directory=output, timeout=60, stdout=log, stderr=subprocess.STDOUT, check=True)
            run_bounded([str(binary)], directory=output, timeout=30, stdout=log, stderr=subprocess.STDOUT, check=True)
        results[name] = dict(pass_=True, binary_sha256=digest(binary))
        binary.unlink()
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-effects-output-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents: parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True); check_space(output)
    paths = [path for path in (ROOT / 'src').rglob('*') if path.is_file()]
    paths += [Path(__file__).resolve(), ROOT / 'tools/rewrite/verify_effects_browser.js', ROOT / 'tools/rewrite/verify_window.py', ROOT / 'CMakeLists.txt']
    hashes = {str(path.relative_to(ROOT)): digest(path) for path in paths}
    report = dict(pass_=False, scope=__doc__.strip(), source_sha256=hashes, verified_at=datetime.now(timezone.utc).isoformat())
    archive = (ROOT / 'DestructionDerby2/Dirinfo').resolve()
    server = thread = sanitized = None
    try:
        report['archive_sha256'] = digest(archive)
        report['sanitized_units'] = units(output)
        report['native'] = native_check(output, archive, WORK / 'rewrite-native/dd2_app', 'native-effects')
        sanitized = build_sanitized(output)
        report['sanitized'] = native_check(output, archive, sanitized, 'sanitized-effects')
        report['sanitizer_scope'] = 'All application rewrite C units instrumented; system SDL2/pinned release SoftGL uninstrumented. Standalone effects/events also instrumented.'
        server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(BUILD)))
        thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
        report['browser'] = {}
        for rate in ('default', '48000'):
            command = ['node', str(ROOT / 'tools/rewrite/verify_effects_browser.js'),
                       f'http://127.0.0.1:{server.server_port}/', str(archive), str(output)]
            if rate != 'default': command.append(rate)
            with (output / ('browser-' + rate + '.log')).open('wb') as log:
                run_bounded(command, directory=output, timeout=120, stdout=log, stderr=subprocess.STDOUT, check=True)
            report['browser'][rate] = json.loads((output / ('effects-browser-' + rate + '.json')).read_text())
            if not report['browser'][rate]['pass_']: raise ValueError('Browser effects verification failed')
        for log in output.glob('*.log'):
            text = log.read_text(errors='replace')
            if 'Sanitizer:' in text or 'runtime error:' in text: raise ValueError('Sanitizer finding: ' + log.name)
        if hashes != {str(path.relative_to(ROOT)): digest(path) for path in paths}: raise ValueError('Sources changed during verification')
        report['binary_sha256'] = {str(path): digest(path) for path in (WORK / 'rewrite-native/dd2_app', sanitized, BUILD / 'dd2_app.js', BUILD / 'dd2_app.wasm')}
        report['pass_'] = True
    finally:
        if server is not None: server.shutdown(); server.server_close(); thread.join(timeout=5)
        report['log_sha256'] = {path.name: digest(path) for path in output.glob('*.log')}
        (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        if report['pass_']:
            if sanitized is not None: sanitized.unlink()
            opened = open_files()
            for path in output.iterdir():
                if path.suffix in ('.pcm', '.log'):
                    stat = path.stat()
                    if (stat.st_dev, stat.st_ino) not in opened: path.unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, report=str(output / 'report.json'))))


if __name__ == '__main__': main()
