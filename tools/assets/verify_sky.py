#!/usr/bin/env python3
"""Verify prepared sky roles, coverage and renderer semantics without originals."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from assets.prepare_scenes import compile_scene
from assets.prepare_templates import relabel
from assets.verify_render import image_compare
from rewrite.quality import ROOT

CODES = '123456789AB'
ANGLES = '01234567UD'
PATCHES = [f'sky-{band}-{quadrant}' for band in ('lower', 'upper') for quadrant in range(4)]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT/'assets/runtime')
    parser.add_argument('--sanitized-build', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=WORK/'prepared-sky-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents or WORK not in args.sanitized_build.resolve().parents:
        parser.error('Verification output/builds must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    runtime = args.runtime.resolve()
    environment = dict(os.environ, SDL_VIDEODRIVER='dummy',
                       ASAN_OPTIONS='detect_leaks=1:halt_on_error=1:exitcode=86',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1:exitcode=87')
    report = dict(pass_=False, started=datetime.now(timezone.utc).isoformat(),
                  scope='Eight prepared sky patches: semantic bindings, ten viewing directions, '
                        'Native/Node-WASM/sanitized images, translation/depth/culling unit checks',
                  original_inputs_required=False, runtime_subdivision=False,
                  limitations=['No original/full-game parity, authored-quality or 60-FPS claim'],
                  levels=[], cases=[], comparisons=[], calls=[])

    def run(command, label):
        check_space(output)
        log = output/(label+'.log')
        with log.open('wb') as stream:
            result = run_bounded(list(map(str, command)), directory=output, timeout=120,
                                 cwd=output, stdout=stream, stderr=subprocess.STDOUT, env=environment)
        raw = log.read_bytes()
        report['calls'].append(dict(label=label, returncode=result.returncode, sha256=digest(raw)))
        if result.returncode or b'Sanitizer:' in raw or b'runtime error:' in raw:
            raise ValueError(label+': '+raw.decode(errors='replace'))
        log.unlink()
        return raw

    builds = {'native': WORK/'rewrite-native', 'wasm': WORK/'rewrite-wasm',
              'sanitized': args.sanitized_build}
    try:
        for target, build in builds.items():
            command = ['node', build/'dd2_sky_draw_test.js'] if target == 'wasm' else [build/'dd2_sky_draw_test']
            run(command, target+'-depth-translation-rotation-culling')
        run(['valgrind', '--error-exitcode=86', '--leak-check=full',
             builds['native']/'dd2_sky_draw_test'], 'memcheck-sky')
        manifest = json.loads((runtime/'reference/manifest.json').read_text())
        if len(manifest['levels']) != len(CODES):
            raise ValueError('Expected all eleven prepared levels')
        for code, level in zip(CODES, manifest['levels']):
            scene = json.loads((runtime/level['scene']).read_text())
            if relabel(scene['templates']) != scene['templates'] or relabel(level['templates']) != level['templates']:
                raise ValueError('Template migration is not idempotent')
            compiled = compile_scene(scene)
            if compiled != (runtime/level['compiled_scene']).read_bytes():
                raise ValueError('Compiled scene differs from owned JSON')
            models = [scene['templates'][name]['model'] for name in PATCHES]
            if len(set(models)) != len(PATCHES) or scene['templates']['detached-trunk']['model'] in models:
                raise ValueError('Sky identities alias a patch or detached trunk')
            report['levels'].append(dict(level=code, models=models, compiled_sha256=digest(compiled)))
            for angle in ANGLES:
                stats = {}
                for target, build in builds.items():
                    name = f'{target}-{code}-{angle}'
                    command = ['node', build/'dd2_prepared_preview.js'] if target == 'wasm' else [build/'dd2_prepared_preview']
                    raw = run([*command, runtime, f'sky-{code}-{angle}', output/(name+'.ppm')], name)
                    stats[target] = json.loads(raw)
                    row = stats[target]
                    if row['tested'] != 8 or row['visible']+row['culled'] != 8 or row['triangles'] == 0 or row['batches'] == 0:
                        raise ValueError('Empty or inconsistent prepared sky submission')
                if any(row != stats['native'] for row in stats.values()):
                    raise ValueError('Cross-target sky submission differs')
                image = Image.open(output/f'native-{code}-{angle}.ppm').convert('RGB')
                visible = sum(any(pixel) for pixel in image.getdata())
                # Some prepared bottom caps intentionally contain flat black material.
                # Submission and unit depth/rotation checks provide geometric evidence;
                # visible colors are reported rather than inventing replacement content.
                report['cases'].append(dict(level=code, angle=angle, stats=stats['native'],
                                            nonblack_pixels=visible, pixels_sha256=digest(image.tobytes())))
                for target in ('wasm', 'sanitized'):
                    report['comparisons'].append(dict(level=code, angle=angle, target=target,
                        **image_compare(output/f'native-{code}-{angle}.ppm', output/f'{target}-{code}-{angle}.ppm')))
        report['pass_'] = True
    finally:
        report['finished'] = datetime.now(timezone.utc).isoformat()
        (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(pass_=report['pass_'], levels=len(report['levels']),
                         images=len(report['cases'])*3, comparisons=len(report['comparisons']))))


if __name__ == '__main__':
    main()
