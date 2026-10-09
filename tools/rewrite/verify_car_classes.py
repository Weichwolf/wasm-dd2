#!/usr/bin/env python3
"""Verify original class coefficients and bounded actual class driving/UI ownership."""
import argparse
from datetime import datetime, timezone
from functools import partial
import hashlib
from http.server import ThreadingHTTPServer
import json
from pathlib import Path
import subprocess
import sys
import threading

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, open_files, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.serve import BUILD, Handler
from rewrite.verify_window import NativeWindow, build_sanitized
from rewrite.verify_car_liveries import EXE_SHA, ORIGINAL_SHA256


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def original_comparison(text, catalog):
    checked = 0
    for line in text.splitlines():
        values = line.split(',')
        if values[0] == 'rating':
            car, *ratings = map(int, values[1:])
            if catalog[car]['ratings'] != ratings:
                raise ValueError('Original class ratings differ')
        else:
            car, engine, vx, friction, front_factor, result_x, result_z, yaw, speed = map(int, values)
            info = catalog[car]
            front, rear = info['negative_drive'] if engine < 0 else info['coasting'] if engine == 0 else [2048, 2048]
            rear_force = friction * info['rear_grip'] // 4096 * rear // 4096
            front_force = friction * front // 4096 if front_factor == 2 else 0
            expected = (vx - vx // 16 * 50 // 256 - rear_force - front_force,
                        engine * info['traction'] // 4096, rear_force - front_force)
            if (result_x, result_z, yaw) != expected:
                raise ValueError(f'Original class force mismatch: {line}, expected {expected}')
            if speed != (16 if vx else 1):
                raise ValueError('Unexpected original bounded speed')
        checked += 1
    if checked != 30:
        raise ValueError('Incomplete original class inventory')
    return {'cases': checked, 'exact': True,
            'scope': 'Original ratings and isolated drive/rear/front class coefficients, not complete rigid-body force parity'}


def native_selection(output, archive, binary, label):
    ui = NativeWindow(output, archive, binary, label)
    try:
        ui.start()
        ui.stable()
        ui.command('key', 'Tab')
        images = [ui.stable()]
        for _ in range(2):
            ui.command('key', 'F1')
            images.append(ui.stable())
        ui.command('key', 'F1')
        if ui.stable() != images[0] or len(set(images)) != 3:
            raise ValueError('Native class previews/cycle differ')
        ui.command('key', 'F2')
        modal = ui.stable()
        ui.command('key', 'F1')
        if ui.stable() != modal:
            raise ValueError('Native class changed in dialog')
        ui.command('key', 'Escape')
        if ui.stable() != images[0]:
            raise ValueError('Native dialog lost class')
        ui.command('key', 'Return')
        ui.command('key', 'p')
        paused = ui.stable()
        ui.command('key', 'F1')
        if ui.stable() != paused:
            raise ValueError('Native class changed while driving')
        return {'class_preview_sha256': [hashlib.sha256(data).hexdigest() for data in images],
                'real_f1_cycle': True, 'modal_and_driving_lock': True}
    finally:
        ui.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-car-classes-0005')
    parser.add_argument('--keep-images', action='store_true', help='Retain images until visual review finishes')
    args = parser.parse_args()
    output = prepare_output(args.output).resolve()
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    archive = (ROOT / 'DestructionDerby2/Dirinfo').resolve()
    executable = (ROOT / 'DestructionDerby2/dd2h.exe').resolve()
    if digest(archive) != ORIGINAL_SHA256 or digest(executable) != EXE_SHA:
        raise ValueError('Unsupported original assets')
    report = {'pass_': False, 'verified_at': datetime.now(timezone.utc).isoformat(),
              'scope': __doc__, 'archive_sha256': digest(archive), 'original_exe_sha256': digest(executable)}
    server = thread = None

    def run(command, label, timeout=180):
        log = output / (label + '.log')
        with log.open('wb') as stream:
            result = run_bounded(list(map(str, command)), directory=output, timeout=timeout,
                                 stdout=stream, stderr=subprocess.STDOUT)
        text = log.read_text(errors='replace')
        if result.returncode or 'Sanitizer:' in text or 'runtime error:' in text:
            raise RuntimeError(f'{label} failed: {text[-4000:]}')
        return text

    try:
        flags = ['-std=c11', '-O1', '-g', '-I', ROOT / 'src', '-I', ROOT / 'tests',
                 '-Wall', '-Wextra', '-Wpedantic', '-Wno-unused-parameter', '-Wno-unused-function',
                 '-fno-strict-aliasing', '-ffast-math', '-Werror', '-Wshadow', '-Wconversion',
                 '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wformat=2',
                 '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        units = sorted((ROOT / 'src/assets').glob('*.c')) + sorted((ROOT / 'src/physics').glob('*.c'))
        units += sorted((ROOT / 'src/ai').glob('*.c'))
        units += [path for path in sorted((ROOT / 'src/game').glob('*.c'))
                  if path.name not in ('application.c', 'main.c', 'audio.c')]
        units += [ROOT / 'src/platform/file.c']
        sanitized = {}
        for name in ('car_class_test', 'car_class_export'):
            binary = output / (name + '-sanitized')
            run([tool('clang'), *flags, *units, ROOT / f'tests/{name}.c', '-lm', '-o', binary], name+'-build')
            sanitized[name] = binary
        catalogs = {}
        for target, command in [('native', [WORK / 'rewrite-native/dd2_car_class_test']),
                                ('wasm', ['node', WORK / 'rewrite-wasm/dd2_car_class_test.js']),
                                ('sanitized', [sanitized['car_class_test']])]:
            catalogs[target] = [json.loads(line) for line in run(command, target+'-handling').splitlines() if line.startswith('{')]
        if len(catalogs['native']) != 3 or len({json.dumps(value, sort_keys=True) for value in catalogs.values()}) != 1:
            raise ValueError('Cross-target class catalog differs')
        original = output / 'original-car-classes'
        run(['gcc', '-m32', '-no-pie', '-O0', '-std=gnu99', '-w', ROOT / 'tools/reference/car_class_fixture.c', '-o', original], 'original-build')
        report['original'] = original_comparison(run([original, executable], 'original'), catalogs['native'])
        report['catalog'] = catalogs['native']
        report['prefixes'] = {}
        for target, command in [('native', [WORK / 'rewrite-native/dd2_car_class_export']),
                                ('wasm', ['node', WORK / 'rewrite-wasm/dd2_car_class_export.js']),
                                ('sanitized', [sanitized['car_class_export']])]:
            rows = [json.loads(line) for line in run([*command, archive], target+'-fields', 300).splitlines() if line.startswith('{')]
            keys = {(row['level'], row['class'], row['mode']) for row in rows}
            expected = {(level, car, mode) for level in range(1,12) for car in range(3)
                        for mode in ([0,1,2,3] if level <= 7 else [0,1,4])}
            if keys != expected or len(rows) != len(expected) or any(row['steps'] != (700 if row['mode'] == 0 else 300) for row in rows):
                raise ValueError('Incomplete class physical prefix inventory')
            report['prefixes'][target] = rows
        report['memcheck'] = run(['valgrind', '--error-exitcode=99', '--leak-check=full', '--show-leak-kinds=all',
                                  '--errors-for-leak-kinds=all', WORK / 'rewrite-native/dd2_car_class_test'], 'memcheck')
        report['native_ui'] = native_selection(output, archive, WORK / 'rewrite-native/dd2_app', 'native-ui')
        sanitized_app = build_sanitized(output)
        report['sanitized_ui'] = native_selection(output, archive, sanitized_app, 'sanitized-ui')
        if report['native_ui']['class_preview_sha256'] != report['sanitized_ui']['class_preview_sha256']:
            raise ValueError('Native/sanitized setup images differ')
        server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(BUILD)))
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        run(['node', ROOT / 'tools/rewrite/verify_car_classes_browser.js',
             f'http://127.0.0.1:{server.server_port}/', archive, output], 'browser', 180)
        report['browser'] = json.loads((output / 'browser.json').read_text())
        if not report['browser']['pass_']:
            raise ValueError('Browser class selection failed')
        report['source_sha256'] = {str(path.relative_to(ROOT)): digest(path) for root in ('src','tests')
                                  for path in (ROOT / root).rglob('*') if path.suffix in ('.c','.h','.js','.html')}
        for name in ('CMakeLists.txt', 'Makefile', 'tools/reference/car_class_fixture.c',
                     'tools/rewrite/verify_car_classes.py', 'tools/rewrite/verify_car_classes_browser.js'):
            report['source_sha256'][name] = digest(ROOT / name)
        report['softgl_revision'] = subprocess.check_output(['git', '-C', str(ROOT/'deps/softgl'), 'rev-parse', 'HEAD'], text=True).strip()
        report['binary_sha256'] = {str(path): digest(path) for path in [original, *sanitized.values(), sanitized_app,
            WORK / 'rewrite-native/dd2_car_class_export', WORK / 'rewrite-wasm/dd2_car_class_export.wasm',
            WORK / 'rewrite-native/dd2_app', BUILD / 'dd2_app.wasm']}
        report['pass_'] = True
    finally:
        if server is not None:
            server.shutdown(); server.server_close(); thread.join(timeout=5)
        report['log_sha256'] = {path.name: digest(path) for path in output.glob('*.log')}
        (output / 'verification-report.json').write_text(json.dumps(report, indent=2)+'\n')
        if report['pass_']:
            opened = open_files()
            for path in output.iterdir():
                if path.suffix == '.json' or (args.keep_images and path.suffix == '.png'):
                    continue
                stat = path.stat()
                if (stat.st_dev, stat.st_ino) not in opened:
                    path.unlink()
        check_space(output)
    print(json.dumps({'pass_': report['pass_'], 'report': str(output/'verification-report.json')}))


if __name__ == '__main__':
    main()
