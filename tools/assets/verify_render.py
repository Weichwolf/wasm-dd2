#!/usr/bin/env python3
"""Render every authored vehicle LOD/cockpit on Native, WASM and real windows."""
import argparse
from datetime import datetime, timezone
from functools import partial
import hashlib
from http.server import ThreadingHTTPServer
import json
import os
from pathlib import Path
import subprocess
import sys
import threading

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.serve import BUILD, Handler
from rewrite.quality import ROOT, tool
from rewrite.verify_window import NativeWindow
from assets.verify_assets import decode_model

VIEWS = [('full', 0, 'exterior'), ('exterior', 1, 'exterior'), ('npc', 2, 'exterior'), ('cockpit', 0, 'cockpit')]
HEADER = b'P6\n640 360\n255\n'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def image_compare(left, right):
    import numpy as np
    lhs = np.asarray(Image.open(left).convert('RGB')).astype(np.int16)
    rhs = np.asarray(Image.open(right).convert('RGB')).astype(np.int16)
    if lhs.shape != (360, 640, 3) or rhs.shape != lhs.shape:
        raise ValueError('Image extent differs')
    changed = int(np.count_nonzero(np.any(lhs != rhs, axis=2)))
    error = float(np.mean(np.abs(lhs - rhs)))
    if changed > 640 * 360 * .01 or error > .5:
        raise ValueError(f'Cross-target authored image differs: {changed} pixels, {error} error: {right}')
    return dict(changed_pixels=changed, mean_channel_error=error,
                expected_sha256=digest(lhs.astype(np.uint8).tobytes()),
                actual_sha256=digest(rhs.astype(np.uint8).tobytes()))


def native_checks(output, binary, label, root):
    ui = NativeWindow(output, root, binary, label=label, arguments=[str(root)],
                      title='Destruction Derby 2 - Racer R1 preview')
    comparisons = []
    reference = 'sanitized' if label.startswith('sanitized') else 'native'
    def match(name):
        comparisons.append(ui.match(Image.open(output / (reference + '-' + name + '.ppm')).convert('RGB'), name))
    try:
        ui.start()
        match('full-rest')
        ui.command('key', 'space'); match('full-steer')
        ui.command('key', 'space'); match('full-rest')
        ui.command('key', 'Tab'); match('cockpit-rest')
        ui.image().save(output / (label + '-cockpit.png'))
        ui.command('key', 'space'); match('cockpit-steer')
        ui.command('key', 'r'); match('cockpit-rest')
        ui.command('key', 'Tab'); match('full-rest')
        ui.command('key', 'Prior'); match('exterior-rest')
        ui.command('key', 'space'); match('exterior-steer')
        ui.command('key', 'r'); match('exterior-rest')
        ui.command('key', 'Prior'); match('npc-rest')
        ui.command('key', 'space'); match('npc-steer')
        ui.command('key', 'r'); match('npc-rest')
        ui.command('key', 'Prior'); match('full-rest')
        ui.image().save(output / (label + '-exterior.png'))
        baseline = ui.stable()
        ui.command('keydown', 'Right')
        import time
        time.sleep(.18)
        ui.command('keyup', 'Right')
        if ui.stable() == baseline:
            raise ValueError('Actual native camera did not change')
        ui.command('key', 'r'); match('full-rest')
        ui.command('key', 'Escape')
        if ui.process.wait(timeout=10) != 0:
            raise ValueError('Actual native close failed')
    finally:
        ui.close()
    return comparisons


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'authored-render-verification')
    parser.add_argument('--sanitized-build', type=Path, default=WORK / 'authored-render-sanitized')
    args = parser.parse_args()
    output = prepare_output(args.output)
    sanitized_build = args.sanitized_build.resolve()
    if WORK not in output.parents or WORK not in sanitized_build.parents:
        parser.error('Build and output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    calls = []
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1:exitcode=86',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1:exitcode=87', PYTHONDONTWRITEBYTECODE='1')
    def run(command, label, timeout=60):
        log = output / (label + '.log')
        with log.open('w') as stream:
            result = run_bounded(list(map(str, command)), directory=output, timeout=timeout,
                                 stdout=stream, stderr=subprocess.STDOUT, cwd=output, env=env)
        data = log.read_bytes()
        calls.append(dict(label=label, returncode=result.returncode, sha256=digest(data)))
        if result.returncode != 0 or b'Sanitizer:' in data or b'runtime error:' in data:
            raise RuntimeError(label + ': ' + log.read_text())
        return log
    run(['cmake', '-S', ROOT, '-B', sanitized_build, '-G', 'Ninja', '-DCMAKE_C_COMPILER=' + tool('clang'),
         '-DCMAKE_BUILD_TYPE=Debug', '-DDD2_ENABLE_CLANG_TIDY=OFF',
         '-DCMAKE_C_FLAGS=-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer'], 'sanitized-configure')
    run(['cmake', '--build', sanitized_build, '--target', 'dd2_content_preview', 'dd2_content_viewer',
         'dd2_model_draw_test', 'dd2_color_test', '-j4'], 'sanitized-build', 600)
    run([sanitized_build / 'dd2_model_draw_test'], 'sanitized-pixels')
    run([sanitized_build / 'dd2_color_test'], 'sanitized-color')
    run(['valgrind', '--error-exitcode=86', '--leak-check=full', WORK / 'rewrite-native/dd2_model_draw_test'], 'memcheck-pixels')
    run(['valgrind', '--error-exitcode=86', '--leak-check=full', WORK / 'rewrite-native/dd2_color_test'], 'memcheck-color')
    root = ROOT / 'assets/runtime'
    inputs = sorted((root / 'models').glob('*')) + sorted((root / 'textures').glob('*'))
    before = {str(path.relative_to(ROOT)): digest(path.read_bytes()) for path in inputs}
    commands = {'native': [WORK / 'rewrite-native/dd2_content_preview'],
                'wasm': ['node', BUILD / 'dd2_content_preview.js'],
                'sanitized': [sanitized_build / 'dd2_content_preview']}
    captures = {}
    comparisons = []
    counts = {}
    model_names = ('racer-r1', 'racer-r1-exterior', 'racer-r1-npc')
    models = [decode_model((root / 'models' / (name + '.dd2mesh')).read_bytes()) for name in model_names]
    for name, detail, camera in VIEWS:
        for pose in ('rest', 'steer'):
            label = name + '-' + pose
            for platform, command in commands.items():
                image = output / (platform + '-' + label + '.ppm')
                log = run([*command, root, image, detail, camera, pose], platform + '-' + label)
                record = json.loads(log.read_text())
                if record['samples'] != 4:
                    raise ValueError('Preview does not use four real samples')
                model = models[detail]
                triangle_limit = model['triangles']
                if camera == 'cockpit':
                    triangle_limit = sum(part['count'] // 3 for part in model['parts'] if part['role'] in (5, 6))
                    if record['role_filtered'] == 0:
                        raise ValueError('Cockpit did not reject exterior/wheel batches')
                elif record['role_filtered'] != 0:
                    raise ValueError('Exterior unexpectedly filters roles')
                if not (0 < record['triangles'] <= triangle_limit):
                    raise ValueError('Submitted geometry exceeds the selected role inventory')
                if record['tested'] != record['role_filtered'] + record['culled'] + record['batches']:
                    raise ValueError('Batch selection accounting differs')
                if platform != 'native' and record != counts['native-' + label]:
                    raise ValueError('Cross-target submitted geometry differs')
                if not image.read_bytes().startswith(HEADER):
                    raise ValueError('Malformed preview image')
                counts[platform + '-' + label] = record
                captures[platform + '-' + label] = digest(image.read_bytes())
                if platform != 'native':
                    comparisons.append(dict(label=platform + '-' + label,
                                            **image_compare(output / ('native-' + label + '.ppm'), image)))
    windows = {}
    for label, binary in (('native-window', WORK / 'rewrite-native/dd2_content_viewer'),
                          ('sanitized-window', sanitized_build / 'dd2_content_viewer')):
        windows[label] = native_checks(output, binary, label, root)
        log = output / (label + '.log')
        if b'Sanitizer:' in log.read_bytes() or b'runtime error:' in log.read_bytes():
            raise RuntimeError('Native window sanitizer diagnostic: ' + log.read_text())
        calls.append(dict(label=label, sha256=digest(log.read_bytes())))
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(BUILD)))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        run(['node', ROOT / 'tools/assets/verify_render_browser.js',
             f'http://127.0.0.1:{server.server_port}/content.html', output], 'browser', 180)
    finally:
        server.shutdown(); thread.join(); server.server_close()
    if before != {str(path.relative_to(ROOT)): digest(path.read_bytes()) for path in inputs}:
        raise ValueError('Rendering modified authored inputs')
    sources = ['CMakeLists.txt', 'src/render/renderer.c', 'src/render/renderer.h',
               'src/render/color.c', 'src/render/color.h', 'tests/color_test.c',
               'src/render/model_draw.c', 'src/render/model_draw.h', 'src/render/model_view.c', 'src/render/model_view.h',
               'src/game/content_viewer.c', 'src/game/content_viewer.h', 'src/game/content_main.c',
               'src/platform/window.c', 'src/platform/window.h', 'src/platform/web/content.html', 'src/platform/web/content.js',
               'tests/model_draw_test.c', 'tests/model_fixture.h', 'tests/render_smoke.c', 'tests/content_preview.c',
               'tools/assets/verify_render.py', 'tools/assets/verify_render_browser.js',
               'tools/rewrite/serve.py', 'tools/rewrite/verify_window.py']
    pngs = {str(path.name): digest(path.read_bytes()) for path in output.glob('*.png')}
    report = dict(pass_=True, verified_at=datetime.now(timezone.utc).isoformat(),
                  scope='Authored vehicle rendering, cockpit-only roles, posed culling, MSAA, camera/control and resource lifetime; complete standalone game, PBR/audio/60 FPS remain pending',
                  sanitizer_scope='Fresh O1 ASan/UBSan including reached SoftGL sources; separate strict LLVM19 gate remains mandatory',
                  inputs=before, captures=captures, review_pngs=pngs, counts=counts,
                  headless_comparisons=comparisons, windows=windows,
                  browser=json.loads((output / 'browser-report.json').read_text()), calls=calls,
                  source_sha256={name: digest((ROOT / name).read_bytes()) for name in sources})
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    # Leave only the small actual window/canvas PNGs until visual review is done.
    for path in output.glob('*.ppm'):
        path.unlink()
    for call in calls:
        (output / (call['label'] + '.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, captures=len(captures), windows={key: len(value) for key, value in windows.items()},
                          browser_comparisons=len(report['browser']['comparisons']), report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
