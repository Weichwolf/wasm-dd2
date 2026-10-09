#!/usr/bin/env python3
"""Verify prepared reference-derived assets without opening original files."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from assets.verify_assets import decode_model
from assets.verify_content import FLAGS
from assets.prepare_scenes import compile_scene


def digest(data):
    return hashlib.sha256(data).hexdigest()


def resource(root, name):
    path = root / name
    if (not name.startswith('reference/') or any(part in ('', '.', '..') for part in name.split('/'))
            or path.is_symlink() or not path.is_file()
            or root.resolve() not in path.resolve().parents):
        raise ValueError('Prepared resource escapes its standalone root: ' + name)
    return path


def inventory(runtime):
    manifest = json.loads((runtime / 'reference/manifest.json').read_text())
    if manifest['format'] != 'DD2REFERENCE1' or manifest['subdivision_steps'] != 2:
        raise ValueError('Unsupported prepared manifest')
    models = {}
    for name, record in manifest['files'].items():
        path = resource(runtime, name)
        data = path.read_bytes()
        if record != dict(sha256=digest(data), bytes=len(data)):
            raise ValueError('Prepared resource digest/extent differs: ' + name)
        if path.suffix == '.dd2mesh':
            models[name] = decode_model(data)
            if models[name] != manifest['models'][name]:
                raise ValueError('Independent model decode differs: ' + name)
            for maps in models[name]['textures']:
                for image in maps:
                    if image not in manifest['files']:
                        raise ValueError('Missing converted texture')
                    resource(runtime, image)
        elif path.suffix == '.png':
            with Image.open(path) as image:
                image.verify()
        else:
            raise ValueError('Unexpected prepared resource type')
    if set(models) != set(manifest['models']):
        raise ValueError('Prepared model inventory differs')
    used = set()
    for level in manifest['levels']:
        scene = json.loads(resource(runtime, level['scene']).read_text())
        compiled = resource(runtime, level['compiled_scene']).read_bytes()
        if compiled != compile_scene(scene):
            raise ValueError('Compiled runtime scene differs from prepared placement JSON')
        if manifest['compiled_scenes'][level['compiled_scene']] != dict(sha256=digest(compiled), bytes=len(compiled)):
            raise ValueError('Compiled scene digest/extent differs')
        if (scene['format'] != 'DD2SCENE1' or scene['level'] != level['level']
                or scene['units'] != 'meters' or scene['up'] != 'Y'
                or len(scene['objects']) != level['prepared_objects']):
            raise ValueError('Prepared scene identity differs')
        ids = [obj['id'] for obj in scene['objects']]
        if len(set(ids)) != len(ids):
            raise ValueError('Duplicate prepared instance identity')
        for obj in scene['objects']:
            if len(obj['position']) != 3 or not np.all(np.isfinite(obj['position'])):
                raise ValueError('Invalid prepared placement')
        for item in [*scene['objects'], *scene['templates'].values()]:
            model = models[item['model']]
            if item['bounds'] != model['bounds']:
                raise ValueError('Prepared culling bounds differ')
            used.add(item['model'])
        triangles = sum(models[obj['model']]['triangles'] for obj in scene['objects'])
        if triangles != level['prepared_triangles'] or triangles != 16 * (level['source_triangles'] - level['removed_triangles']):
            raise ValueError('Prepared scene subdivision ratio differs')
        if level['removed_triangles'] != sum(len(row['removed']) for row in level['omissions']):
            raise ValueError('Uninventoried discarded source triangle')
        for name, record in level['templates'].items():
            expected = 16 * (record['source_triangles'] - len(record['removed']))
            actual = models[scene['templates'][name]['model']]['triangles'] if name in scene['templates'] else 0
            if actual != expected or record['triangles'] != expected:
                raise ValueError('Prepared template subdivision ratio differs')
    if set(models) != used:
        raise ValueError('Unreferenced prepared model')
    expected_files = {*manifest['files'], *manifest['compiled_scenes'], *(level['scene'] for level in manifest['levels']),
                      'reference/manifest.json'}
    actual_files = {str(path.relative_to(runtime)) for path in (runtime / 'reference').rglob('*')
                    if path.is_file()}
    if actual_files != expected_files:
        raise ValueError('Uninventoried or missing prepared file')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT / 'assets/runtime')
    parser.add_argument('--output', type=Path, default=WORK / 'prepared-reference-verification')
    args = parser.parse_args()
    runtime = args.runtime.resolve()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    manifest = inventory(runtime)
    calls = []
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1:exitcode=86',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1:exitcode=87')

    def run(command, label, expected=0):
        log = output / (label + '.log')
        with log.open('wb') as stream:
            result = run_bounded(list(map(str, command)), directory=output, timeout=120,
                                 stdout=stream, stderr=subprocess.STDOUT, cwd=output, env=environment)
        data = log.read_bytes()
        calls.append(dict(label=label, returncode=result.returncode, output_sha256=digest(data)))
        if result.returncode != expected or b'Sanitizer:' in data or b'runtime error:' in data:
            raise RuntimeError(label + ': ' + data.decode(errors='replace'))

    sanitized = output / 'dd2_content_sanitized'
    sources = ['src/assets/model.c', 'src/assets/image.c', 'src/platform/file.c', 'tests/content_export.c']
    run([tool('clang'), *FLAGS, *[ROOT / name for name in sources], '-lz', '-lm', '-o', sanitized], 'sanitizer-build')
    commands = {'native': [WORK / 'rewrite-native/dd2_content_export'],
                'wasm': ['node', WORK / 'rewrite-wasm/dd2_content_export.js'], 'sanitized': [sanitized]}
    comparisons = {}
    raw = output / 'decoded.bin'
    listing = output / 'inputs.txt'
    for kind, suffix in (('model', '.dd2mesh'), ('image', '.png')):
        names = sorted(name for name in manifest['files'] if name.endswith(suffix))
        listing.write_text(''.join(str(resource(runtime, name)) + '\n' for name in names))
        for platform, command in commands.items():
            run([*command, kind + '-list', listing, raw], platform + '-' + kind + '-list')
            expected_hash = hashlib.sha256()
            size = 0
            with raw.open('rb') as stream:
                for name in names:
                    path = resource(runtime, name)
                    if kind == 'model':
                        expected = path.read_bytes()
                    else:
                        with Image.open(path) as image:
                            expected = image.convert('RGBA').tobytes()
                    if stream.read(len(expected)) != expected:
                        raise ValueError(platform + ' typed export differs: ' + name)
                    expected_hash.update(expected)
                    size += len(expected)
                if stream.read(1):
                    raise ValueError('Unexpected batch export suffix')
            comparisons[platform + '-' + kind] = dict(entries=len(names), bytes=size, sha256=expected_hash.hexdigest())
            raw.unlink()
        print(json.dumps(dict(kind=kind, entries=len(names), platforms=list(commands))), flush=True)
    # Exercise bounded list parsing and failed resource lookup on both targets.
    invalid = {'empty': b'', 'blank': b'\n', 'unterminated': b'missing',
               'long-path': b'x' * 1024 + b'\n', 'missing': b'missing.dd2mesh\n',
               'carriage-return': b'missing.dd2mesh\r\n', 'embedded-nul': b'x\0missing\n'}
    first_model = next(name for name in manifest['files'] if name.endswith('.dd2mesh'))
    invalid['failure-after-valid-entry'] = (str(resource(runtime, first_model)) + '\nmissing.dd2mesh\n').encode()
    for label, data in invalid.items():
        listing.write_bytes(data)
        for platform, command in commands.items():
            run([*command, 'model-list', listing, raw], platform + '-reject-' + label, 1)
            if raw.exists():
                raise ValueError('Failed batch left partial comparison output')
    listing.unlink()
    report = dict(pass_=True, verified_at=datetime.now(timezone.utc).isoformat(),
                  scope='All converted mesh fields/indices, PNG pixels, placements, bounds and offline 16x counts; gameplay/visual parity/audio/60 FPS remain open.',
                  original_inputs_required=False, runtime=str(runtime), models=len(manifest['models']),
                  files=len(manifest['files']), levels=len(manifest['levels']), comparisons=comparisons,
                  source_sha256={name: digest((ROOT / name).read_bytes()) for name in sources},
                  calls=calls, negative_list_cases=list(invalid))
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    sanitized.unlink()
    for call in calls:
        (output / (call['label'] + '.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, models=report['models'], files=report['files'], report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
