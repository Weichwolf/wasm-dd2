#!/usr/bin/env python3
"""Check complete prepared static scenes and cropped visibility on three targets."""
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
from rewrite.quality import ROOT
from assets.verify_render import image_compare


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'prepared-world-render')
    parser.add_argument('--sanitized-build', type=Path, required=True)
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents or WORK not in args.sanitized_build.resolve().parents:
        parser.error('Verification artifacts/builds must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    runtime = ROOT / 'assets/runtime'
    manifest = json.loads((runtime / 'reference/manifest.json').read_text())
    commands = {'native': [WORK / 'rewrite-native/dd2_prepared_preview'],
                'wasm': ['node', WORK / 'rewrite-wasm/dd2_prepared_preview.js'],
                'sanitized': [args.sanitized_build / 'dd2_prepared_preview']}
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1:exitcode=86',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1:exitcode=87')
    captures, comparisons = [], []
    for level in manifest['levels']:
        for mode in ('full', 'crop'):
            images, counters = {}, {}
            for platform, command in commands.items():
                name = platform + '-' + level['level'].lower() + '-' + mode
                path = output / (name + '.ppm')
                log = output / (name + '.log')
                with log.open('wb') as stream:
                    result = run_bounded([*map(str, command), str(runtime), level['compiled_scene'], str(path),
                                          *(['crop'] if mode == 'crop' else [])], directory=output,
                                         timeout=120, stdout=stream, stderr=subprocess.STDOUT, cwd=output, env=environment)
                data = log.read_bytes()
                if result.returncode != 0 or b'Sanitizer:' in data or b'runtime error:' in data:
                    raise RuntimeError(name + ': ' + data.decode(errors='replace'))
                stats = json.loads(data)
                if stats['objects'] != level['prepared_objects'] or stats['visible'] + stats['culled'] != stats['objects']:
                    raise ValueError('Invalid visibility accounting')
                if not (0 < stats['triangles'] <= level['prepared_triangles']):
                    raise ValueError('Invalid submitted geometry count')
                with Image.open(path) as image:
                    if image.size != (640, 360):
                        raise ValueError('Unexpected scene capture extent')
                captures.append(dict(level=level['level'], mode=mode, platform=platform, stats=stats,
                                      sha256=digest(path.read_bytes()), log_sha256=digest(data)))
                counters[platform] = stats
                images[platform] = path
                log.unlink()
            if any(row != counters['native'] for row in counters.values()):
                raise ValueError('Cross-target visibility/submission accounting differs')
            for platform in ('wasm', 'sanitized'):
                comparisons.append(dict(level=level['level'], mode=mode, platform=platform,
                                         **image_compare(images['native'], images[platform])))
            # Retain only the first circuit and first arena for direct review.
            if level['level'] in ('1', '8'):
                with Image.open(images['native']) as image:
                    image.save(output / (level['level'].lower() + '-' + mode + '.png'))
            for path in images.values():
                path.unlink()
        print(json.dumps(dict(level=level['level'], pass_=True)), flush=True)
    source_names = ['src/assets/world.c', 'src/assets/world.h', 'src/render/frustum.c', 'src/render/frustum.h',
                    'src/render/world_draw.c', 'src/render/world_draw.h', 'src/render/model_draw.c',
                    'src/render/model_draw.h', 'tests/prepared_preview.c', 'tools/assets/verify_world_render.py']
    report = dict(pass_=True, verified_at=datetime.now(timezone.utc).isoformat(),
                  scope='Prepared static world rendering, cutouts, shared uploads and visibility; actual gameplay/browser world UI/audio/60 FPS remain open.',
                  original_inputs_required=False, profile=dict(width=640, height=360, samples=4),
                  captures=captures, comparisons=comparisons, review_pngs=[path.name for path in output.glob('*.png')],
                  source_sha256={name: digest((ROOT / name).read_bytes()) for name in source_names})
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    check_space(output)
    print(json.dumps(dict(pass_=True, captures=len(captures), comparisons=len(comparisons), report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
