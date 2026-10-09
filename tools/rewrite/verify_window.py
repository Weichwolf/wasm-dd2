#!/usr/bin/env python3
"""Actual rewrite native/window and browser/canvas verification with real input.

Compare all eleven scene/car views with the rewrite's headless reference. Check
keyboard motion/release, focus loss, reset, track/view switching, resize and
browser invalid-file recovery. This is not original game/framebuffer parity.
"""
import argparse
from datetime import datetime, timezone
from functools import partial
import hashlib
from http.server import ThreadingHTTPServer
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import threading
import time

from PIL import Image, ImageGrab

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from artifacts import WORK, check_space, open_files, prepare_output, run_bounded
from rewrite.serve import BUILD, Handler
from rewrite.quality import tool

CODES = '123456789AB'


def digest(data):
    return hashlib.sha256(data).hexdigest()


class NativeWindow:
    def __init__(self, output, archive, binary, label='native', arguments=None, input_pipe=False,
                 title='Destruction Derby 2 - Track viewer'):
        self.output, self.archive, self.binary = output, archive, binary
        self.arguments = arguments if arguments is not None else [str(archive), '1']
        self.input_pipe = input_pipe
        self.title = title
        self.env = os.environ.copy()
        # Private Xvfb has no authentication file; discard the caller's display
        # credentials and never interact with an existing desktop.
        self.env.pop('XAUTHORITY', None)
        self.log = (output / (label + '.log')).open('wb')
        self.display = self.process = None
        self.window = None

    def command(self, *arguments):
        return subprocess.check_output(['xdotool', *map(str, arguments)], env=self.env,
                                       text=True, stderr=subprocess.DEVNULL, timeout=5)

    def start(self):
        self.display = subprocess.Popen(['Xvfb', '-displayfd', '1', '-screen', '0', '1280x1024x24', '-ac'],
                                        stdout=subprocess.PIPE, stderr=self.log)
        number = self.display.stdout.readline().decode().strip()
        if not number:
            raise RuntimeError('Private Xvfb failed to start')
        self.env['DISPLAY'] = ':' + number
        self.process = subprocess.Popen([str(self.binary), *self.arguments], cwd=ROOT,
                                        env=self.env, stdout=self.log, stderr=self.log,
                                        stdin=subprocess.PIPE if self.input_pipe else None)
        def ready():
            try:
                self.window = self.command('search', '--name', '^' + re.escape(self.title) + '$').splitlines()[-1]
                return True
            except (subprocess.CalledProcessError, IndexError):
                return False
        self.wait(ready)
        self.command('windowfocus', self.window)

    def close(self):
        for process in (self.process, self.display):
            if process is not None and process.poll() is None:
                process.terminate()
                try: process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill(); process.wait(timeout=5)
        self.log.close()

    def wait(self, predicate, timeout=15):
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            if self.process is not None and self.process.poll() is not None:
                raise RuntimeError('Native application terminated unexpectedly')
            check_space(self.output)
            result = predicate()
            if result:
                return result
            time.sleep(.05)
        raise TimeoutError('Actual native window condition timed out')

    def image(self):
        values = dict(line.split('=', 1) for line in self.command('getwindowgeometry', '--shell', self.window).splitlines())
        xpos, ypos, width, height = [int(values[key]) for key in ('X', 'Y', 'WIDTH', 'HEIGHT')]
        return ImageGrab.grab(xdisplay=self.env['DISPLAY']).crop((xpos, ypos, xpos + width, ypos + height)).convert('RGB')

    def match(self, expected, label):
        target = expected.tobytes()
        try:
            self.wait(lambda: self.image().tobytes() == target)
        except BaseException:
            actual = self.image()
            actual.save(self.output / ('failed-' + label + '-actual.png'))
            expected.save(self.output / ('failed-' + label + '-expected.png'))
            data = actual.tobytes()
            changed = sum(data[index:index+3] != target[index:index+3]
                          for index in range(0, min(len(target), len(data)), 3))
            print('Native mismatch:', label, actual.size, expected.size, 'changed pixels', changed, flush=True)
            raise
        return dict(label=label, size=expected.size, actual_sha256=digest(target), exact=True)

    def stable(self):
        previous = None
        since = time.monotonic()
        def settled():
            nonlocal previous, since
            current = self.image().tobytes()
            if current != previous:
                since = time.monotonic()
            previous = current
            # A render can take longer than four capture intervals. Require a
            # full second without changes, not repeated samples of an old frame.
            return current if time.monotonic() - since >= 1 else None
        return self.wait(settled)


def native_driving_checks(ui, references):
    ui.command('key', 'Return')
    ui.command('key', 'p')
    ui.command('key', 'r')
    comparisons = []
    for code in CODES:
        expected = Image.open(references[(code, 'driving')]).convert('RGB')
        comparisons.append(ui.match(expected, code + '-driving-start'))
        ui.command('key', 'Prior')
    expected = Image.open(references[('1', 'driving')]).convert('RGB')
    ui.match(expected, 'driving-wrap')
    baseline = ui.image().tobytes()
    ui.command('keydown', 'w')
    try:
        if ui.stable() != baseline:
            raise ValueError('Paused native vehicle moves')
    finally: ui.command('keyup', 'w')
    ui.command('key', 'p')
    ui.command('keydown', 'w')
    try:
        ui.wait(lambda: ui.image().tobytes() != baseline)
        time.sleep(1)
    finally: ui.command('keyup', 'w')
    ui.command('key', 'p')
    moved = ui.stable()
    if moved == baseline: raise ValueError('Native driving did not move vehicle')
    ui.command('key', 'r')
    ui.match(expected, 'driving-reset')
    ui.command('key', 'p')
    ui.command('keydown', 'w')
    ui.wait(lambda: ui.image().tobytes() != baseline)
    ui.command('windowfocus', '0')
    ui.command('keyup', 'w')
    ui.stable()  # Must freeze the simulation, including coasting and suspension.
    ui.command('windowfocus', ui.window)
    ui.command('key', 'p')
    ui.command('key', 'r')
    ui.match(expected, 'driving-focus-reset')
    ui.command('key', 'Return')
    return dict(pass_=True, comparisons=comparisons, real_throttle=True,
                pause_freezes=True, focus_loss_freezes=True, deterministic_reset=True,
                track_wrap=True)


def native_race_checks(ui, references):
    ui.command('key', 'F5')
    ui.command('key', 'p')
    ui.command('key', 'r')
    comparisons = []
    for code in CODES:
        expected = Image.open(references[(code, 'race-start')]).convert('RGB')
        comparisons.append(ui.match(expected, code + '-race-start'))
        baseline = ui.image().tobytes()
        ui.command('keydown', 'w')
        try:
            if ui.stable() != baseline:
                raise ValueError('Paused race/countdown advances')
        finally: ui.command('keyup', 'w')
        ui.command('key', 'F7')
        result = Image.open(references[(code, 'race-results')]).convert('RGB')
        comparisons.append(ui.match(result, code + '-race-withdrawn'))
        ui.command('key', 'p')  # Results freeze even with simulation unpaused.
        ui.command('keydown', 'w')
        try:
            if ui.stable() != result.tobytes():
                raise ValueError('Race results change after publication')
        finally: ui.command('keyup', 'w')
        ui.command('key', 'p')
        ui.command('key', 'r')
        ui.match(expected, code + '-race-restart')
        ui.command('key', 'Prior')
    ui.command('key', 'F6')  # Stockcar, native keyboard path.
    ui.command('key', 'p')
    ui.command('key', 'r')
    grid = Image.open(references[('1', 'race-start')]).convert('RGB')
    ui.match(grid, 'stockcar-grid')
    # This crop excludes countdown/position/score overlays. It can change only
    # after the held throttle begins moving the actual body/world at GO.
    body_region = (160, 340, 480, 456)
    baseline = grid.crop(body_region).tobytes()
    ui.command('key', 'p')
    ui.command('keydown', 'w')
    try:
        ui.wait(lambda: ui.image().crop(body_region).tobytes() != baseline)
    finally: ui.command('keyup', 'w')
    ui.command('key', 'p')
    ui.stable()
    ui.command('key', 'r')
    ui.match(grid, 'stockcar-after-go-reset')
    ui.command('key', 'F7')
    ui.stable()
    ui.command('key', 'Return')
    return dict(pass_=True, comparisons=comparisons, modes=True,
                countdown_pause=True, real_throttle_after_go=True, frozen_results=True, reset=True)


def native_trial_checks(ui, references):
    ui.command('key', 'F8')
    ui.command('key', 'p')
    ui.command('key', 'r')
    comparisons = []
    for code in '1234567':
        start = Image.open(references[(code, 'trial-start')]).convert('RGB')
        comparisons.append(ui.match(start, code + '-trial-start'))
        ui.command('key', 'F7')
        result = Image.open(references[(code, 'trial-results')]).convert('RGB')
        comparisons.append(ui.match(result, code + '-trial-results'))
        ui.command('key', 'p')
        ui.command('keydown', 'w')
        try:
            if ui.stable() != result.tobytes(): raise ValueError('Time Trial result advances')
        finally: ui.command('keyup', 'w')
        ui.command('key', 'p')
        ui.command('key', 'r')
        ui.match(start, code + '-trial-reset')
        if code != '7': ui.command('key', 'Prior')
    ui.command('key', 'Prior')
    arena = Image.open(references[('8', 'race-start')]).convert('RGB')
    ui.match(arena, 'trial-to-arena')
    ui.command('key', 'F8')
    ui.match(arena, 'arena-trial-rejected')
    for code in '7654321':
        ui.command('key', 'Next')
        ui.match(Image.open(references[(code, 'race-start')]).convert('RGB'), 'trial-return-' + code)
    ui.command('key', 'F8')
    ui.command('key', 'p')
    ui.command('key', 'r')
    start = Image.open(references[('1', 'trial-start')]).convert('RGB')
    ui.match(start, 'trial-before-throttle')
    body_region = (160, 340, 480, 456)
    baseline = start.crop(body_region).tobytes()
    ui.command('key', 'p')
    ui.command('keydown', 'w')
    try: ui.wait(lambda: ui.image().crop(body_region).tobytes() != baseline)
    finally: ui.command('keyup', 'w')
    ui.command('key', 'p')
    ui.stable()
    ui.command('key', 'r')
    ui.match(start, 'trial-after-go-reset')
    ui.command('key', 'Return')
    return dict(pass_=True, comparisons=comparisons, all_circuits=True,
                real_keyboard_throttle=True, reset=True, frozen_results=True, arena_rejected=True)


def native_total_checks(ui, references):
    # Start from scene 1 after the Time Trial lifecycle. Match each change so
    # edge-triggered page keys cannot collapse into one application frame.
    for code in '2345678':
        ui.command('key', 'Prior')
        ui.match(Image.open(references[(code,'scene')]).convert('RGB'), 'total-select-'+code)
    ui.command('key', 'F9')
    ui.command('key', 'p')
    ui.command('key', 'r')
    comparisons = []
    for code in '89AB':
        start = Image.open(references[(code,'total-start')]).convert('RGB')
        comparisons.append(ui.match(start, code+'-total-start'))
        ui.command('key', 'F7')
        result = Image.open(references[(code,'total-results')]).convert('RGB')
        comparisons.append(ui.match(result, code+'-total-results'))
        ui.command('key', 'p')
        if ui.stable() != result.tobytes(): raise ValueError('Total Destruction results advance')
        ui.command('key', 'p')
        ui.command('key', 'r')
        ui.match(start, code+'-total-reset')
        if code != 'B': ui.command('key', 'Prior')
    ui.command('key', 'Prior')
    circuit = Image.open(references[('1','race-start')]).convert('RGB')
    ui.match(circuit, 'total-to-circuit')
    ui.command('key', 'F9')
    ui.match(circuit, 'circuit-total-rejected')
    ui.command('key', 'Return')
    return dict(pass_=True, comparisons=comparisons, all_arenas=True,
        real_keyboard_mode=True, results=True, reset=True, circuit_rejected=True)


def native_checks(output, archive, references, binary, label='native'):
    ui = NativeWindow(output, archive, binary, label)
    results = []
    try:
        ui.start()
        for code in CODES:
            scene = Image.open(references[(code, 'scene')]).convert('RGB')
            car = Image.open(references[(code, 'car')]).convert('RGB')
            results.append(ui.match(scene, code + '-scene'))
            ui.command('key', 'Tab')
            results.append(ui.match(car, code + '-car'))
            ui.command('key', 'Tab')
            ui.match(scene, code + '-scene-restored')
            ui.command('key', 'Prior')
        expected = Image.open(references[('1', 'scene')]).convert('RGB')
        ui.match(expected, 'track-wrap')
        baseline = ui.image().tobytes()
        for key in ('Right', 'Up', 'a', 'w', 'plus', 'minus'):
            ui.command('keydown', key)
            try:
                ui.wait(lambda: ui.image().tobytes() != baseline)
            finally: ui.command('keyup', key)
            settled = ui.stable()
            if settled == baseline:
                raise ValueError('Actual native camera did not move: ' + key)
            ui.command('key', 'r')
            results.append(ui.match(expected, 'keyboard-reset-' + key))
        ui.command('mousemove', '--window', ui.window, 320, 240)
        ui.command('click', '4')
        ui.wait(lambda: ui.image().tobytes() != baseline)
        ui.command('key', 'r')
        ui.match(expected, 'wheel-reset')
        ui.command('keydown', 'Right')
        ui.wait(lambda: ui.image().tobytes() != baseline)
        ui.command('windowfocus', '0')
        ui.command('keyup', 'Right')
        # Require actual presentation after the queued focus loss. A long
        # render can leave the old screenshot unchanged for a second while
        # input events still await polling. Letterbox margins acknowledge the
        # resize/focus event batch without assuming a wall-clock render speed.
        ui.command('windowsize', ui.window, 800, 480)
        def acknowledged():
            image = ui.image()
            return image.size == (800, 480) and not any(image.crop((0, 0, 80, 480)).tobytes()) and not any(image.crop((720, 0, 800, 480)).tobytes())
        ui.wait(acknowledged)
        unfocused = ui.stable()
        ui.command('windowfocus', ui.window)
        refocused = ui.stable()
        if refocused != unfocused:
            # X11 can resize the outer window before SDL presents its centered
            # surface. A naturally black scene border can satisfy the margin
            # predicate while the old 640px image is still left aligned.
            # Accept only its exact 80px centering translation; camera motion,
            # rescaling or any changed content must still fail.
            pending = Image.frombytes('RGB', (800, 480), unfocused).crop((0, 0, 640, 480))
            centered = Image.new('RGB', (800, 480))
            centered.paste(pending, (80, 0))
            if refocused != centered.tobytes():
                raise ValueError('Native camera changes after focus loss/release')
        if ui.stable() != refocused:
            raise ValueError('Native camera keeps moving after focus loss/release')
        ui.command('windowsize', ui.window, 640, 480)
        ui.command('key', 'r')
        ui.match(expected, 'focus-reset')
        driving = native_driving_checks(ui, references)
        ui.match(expected, 'inspection-after-driving')
        race = native_race_checks(ui, references)
        ui.match(expected, 'inspection-after-race')
        trial = native_trial_checks(ui, references)
        ui.match(expected, 'inspection-after-trial')
        total = native_total_checks(ui, references)
        ui.match(expected, 'inspection-after-total')
        championship = native_championship_checks(ui, expected)
        ui.command('windowmove', ui.window, 0, 0)
        for size, offset in (((800, 480), (80, 0)), ((640, 600), (0, 60))):
            ui.command('windowsize', ui.window, *size)
            padded = Image.new('RGB', size); padded.paste(expected, offset)
            results.append(ui.match(padded, 'letterbox-' + str(size)))
        ui.command('windowsize', ui.window, 1280, 960)
        results.append(ui.match(expected.resize((1280, 960), Image.Resampling.NEAREST), 'integer-scale'))
        ui.command('key', 'Escape')
        if ui.process.wait(timeout=10) != 0:
            raise ValueError('Native application did not shut down cleanly')
    finally: ui.close()
    return dict(pass_=True, comparisons=results, real_x11_keys=True,
                camera_motion_release=True, focus_loss_release=True, track_wrap=True,
                wheel=True, resize_letterbox_scale=True, clean_exit=True, driving=driving, race=race, time_trial=trial, total_destruction=total, championship=championship)


def native_championship_checks(ui, expected):
    evidence = []
    for key, leave in [('c', 'Escape'), ('n', 'F7')]:
        ui.command('key', key)
        # Preparing the owned track/field can exceed the stable-frame interval.
        # A changed presentation acknowledges entry before testing its pause;
        # repeated captures of the previous inspection image cannot do that.
        ui.wait(lambda: ui.image().tobytes() != expected.tobytes())
        ui.command('key', 'p')
        baseline = ui.stable()
        if baseline == expected.tobytes():
            raise ValueError('Championship entry did not change the actual window')
        ui.command('key', 'Prior')
        ui.command('keydown', 'w')
        try:
            if ui.stable() != baseline:
                raise ValueError('Paused championship or scheduled field changed')
        finally:
            ui.command('keyup', 'w')
        ui.command('key', 'r')
        ui.wait(lambda: ui.image().tobytes() != baseline)
        ui.command('key', 'p')
        restarted = ui.stable()
        if restarted == expected.tobytes():
            raise ValueError('Championship restart exited the field')
        ui.command('key', leave)
        ui.match(expected, 'championship-leave-' + key)
        evidence.append({'mode_key': key, 'leave_key': leave,
                         'paused_sha256': digest(baseline),
                         'restarted_sha256': digest(restarted)})
    return {'pass_': True, 'real_x11_keys': True, 'schedule_locked': True,
            'entry_presentation_acknowledged': True,
            'pause_and_restart': True, 'unscored_exit': True, 'cases': evidence}


def build_sanitized(output, entry=None, link_flags=()):
    binary = output / 'dd2_app_sanitized'
    units = [ROOT / f'src/assets/{name}.c' for name in
             ('car_class', 'archive', 'audio', 'car', 'level', 'textures', 'lz', 'mesh', 'scene', 'image', 'model', 'world', 'track', 'road', 'barriers', 'save_card', 'save_profile')]
    units += [ROOT / 'src/audio/mixer.c', ROOT / 'src/audio/effects.c']
    units += [ROOT / f'src/render/{name}.c' for name in ('renderer', 'color', 'mesh_draw', 'camera', 'track_draw', 'sky_draw', 'model_draw', 'world_draw', 'frustum', 'driving_draw', 'damage_draw', 'score_draw', 'race_draw')]
    units += [ROOT / f'src/platform/{name}.c' for name in ('file', 'content', 'window', 'audio_device', 'save_store', 'save_backend', 'save_location')]
    units += [ROOT / f'src/physics/{name}.c' for name in ('road_contact', 'road_surface', 'body_surface', 'vehicle', 'barrier_world', 'car_contact', 'contact_group', 'vehicle_collision', 'damage')]
    units += [ROOT / f'src/game/{name}.c' for name in ('profile_menu', 'configuration', 'application', 'audio', 'driving', 'starting_grid', 'accidents', 'course', 'laps', 'race', 'recovery', 'sound_events', 'league', 'drivers', 'championship', 'championship_session')]
    units += [ROOT / f'src/ai/{name}.c' for name in ('path', 'driver')]
    units += [entry if entry is not None else ROOT / 'src/game/main.c']
    flags = ['-std=c11', '-O1', '-g', '-D_POSIX_C_SOURCE=200809L', '-I', str(ROOT / 'src'),
             '-I', str(ROOT / 'deps/softgl/libsoftgl/include'),
             '-Wall', '-Wextra', '-Wpedantic', '-Wno-unused-parameter', '-Wno-unused-function',
             '-fno-strict-aliasing', '-ffast-math', '-Werror', '-Wshadow', '-Wconversion',
             '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wformat=2',
             '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    sdl = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'sdl2'], text=True))
    with (output / 'sanitizer-build.log').open('wb') as log:
        command = [tool('clang'), *flags, *map(str, units),
                   str(WORK / 'rewrite-native/softgl/libsoftgl.a'), *sdl,
                   '-lm', '-lz', *link_flags, '-o', str(binary)]
        (output / 'sanitizer-build.json').write_text(json.dumps({'command': command,
            'scope': 'All reached rewrite units instrumented; SDL2 and pinned release SoftGL uninstrumented'}, indent=2) + '\n')
        run_bounded(command, directory=output, timeout=180,
                    stdout=log, stderr=subprocess.STDOUT, check=True)
    return binary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-window-verification')
    parser.add_argument('--native-binary', type=Path, default=WORK / 'rewrite-native/dd2_app')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    archive = (ROOT / 'DestructionDerby2/Dirinfo').resolve()
    report = dict(pass_=False, scope=__doc__.strip(), verified_at=datetime.now(timezone.utc).isoformat(),
                  original_sha256=digest(archive.read_bytes()))
    references = {}
    sanitized = None
    server = thread = None
    try:
        for code in CODES:
            for mode in ('scene', 'car'):
                path = output / f'{code}-{mode}.ppm'
                run_bounded([str(WORK / 'rewrite-native/dd2_scene_preview'), str(archive), str(path), code, mode],
                            directory=output, timeout=30, stdout=subprocess.DEVNULL, check=True)
                references[(code, mode)] = path
        for code in CODES:
            for mode in (('driving', 'race-start', 'race-results', 'trial-start', 'trial-results')
                         if code in '1234567' else ('driving', 'race-start', 'race-results', 'total-start', 'total-results')):
                path = output / f'{code}-{mode}.ppm'
                run_bounded([str(WORK / 'rewrite-native/dd2_driving_preview'), str(archive), str(path), code,
                             'start' if mode == 'driving' else mode],
                            directory=output, timeout=30, stdout=subprocess.DEVNULL, check=True)
                references[(code, mode)] = path
        report['native'] = native_checks(output, archive, references, args.native_binary.resolve())
        sanitized = build_sanitized(output)
        report['sanitized'] = native_checks(output, archive, references, sanitized, 'sanitized')
        report['sanitizer_scope'] = 'Rewrite C units instrumented with ASan/UBSan; SDL2 and pinned release SoftGL uninstrumented'
        for log in output.glob('*.log'):
            content = log.read_text(errors='replace')
            if 'Sanitizer:' in content or 'runtime error:' in content:
                raise ValueError('Sanitizer finding: ' + log.name)
        server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(BUILD)))
        thread = threading.Thread(target=server.serve_forever, daemon=True); thread.start()
        command = ['node', str(ROOT / 'tools/rewrite/verify_browser.js'),
                   f'http://127.0.0.1:{server.server_port}/', str(archive), str(output)]
        with (output / 'browser.log').open('wb') as log:
            run_bounded(command, directory=output, timeout=420, stdout=log,
                        stderr=subprocess.STDOUT, cwd=ROOT, check=True)
        report['browser'] = json.loads((output / 'browser.json').read_text())
        if not report['browser']['pass_']:
            raise ValueError('Browser checks did not pass')
        sources = [str(path.relative_to(ROOT)) for path in (ROOT / 'src').rglob('*')
                   if path.is_file() and path.suffix in ('.c', '.h', '.html', '.js')]
        sources += ['CMakeLists.txt', 'tools/rewrite/build_wasm.py', 'tools/rewrite/serve.py',
                    'tools/rewrite/verify_window.py', 'tools/rewrite/verify_browser.js']
        report['source_sha256'] = {name: digest((ROOT / name).read_bytes()) for name in sources}
        report['binary_sha256'] = {str(path): digest(path.read_bytes()) for path in
                                  (args.native_binary.resolve(), BUILD / 'dd2_app.js', BUILD / 'dd2_app.wasm')}
        report['binary_sha256'][str(sanitized)] = digest(sanitized.read_bytes())
        report['pass_'] = True
    finally:
        if server is not None:
            server.shutdown(); server.server_close(); thread.join(timeout=5)
        report['log_sha256'] = {path.name: digest(path.read_bytes()) for path in output.glob('*.log')}
        (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        if report['pass_']:
            sanitized.unlink()
            opened = open_files()
            for path in output.iterdir():
                if path.suffix in ('.ppm', '.png', '.log'):
                    stat = path.stat()
                    if (stat.st_dev, stat.st_ino) not in opened: path.unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, native_and_browser=True, levels=len(CODES), report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
