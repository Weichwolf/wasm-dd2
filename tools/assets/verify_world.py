#!/usr/bin/env python3
"""Verify owned scene loading against prepared files, without original input."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from assets.prepare_scenes import compile_scene, HEADER, RESOURCE, INSTANCE
from assets.verify_content import FLAGS


def digest(data):
    return hashlib.sha256(data).hexdigest()


def mutations(data):
    _, models, instances, templates = HEADER.unpack_from(data)
    first_instance = HEADER.size + models * RESOURCE.size
    first_template = first_instance + instances * INSTANCE.size
    cases = {'empty': b'', 'truncated-header': data[:19], 'truncated-tail': data[:-1],
             'trailing-byte': data + b'\0'}

    def change(label, offset, format_, value):
        altered = bytearray(data)
        struct.pack_into(format_, altered, offset, value)
        cases[label] = bytes(altered)

    for offset, label in ((8, 'model-count'), (12, 'instance-count'), (16, 'template-count')):
        change(label, offset, '<I', 0xffffffff)
    change('magic', 0, '<I', 0)
    change('nonfinite-bounds', HEADER.size + 64, '<I', 0x7fc12345)
    change('nonfinite-placement', first_instance + 36, '<I', 0x7f800000)
    change('invalid-model-reference', first_instance + 32, '<I', models)
    change('invalid-template-reference', first_template + 32, '<I', models)
    change('nonzero-string-padding', HEADER.size + 63, '<B', ord('x'))
    for label, name in (('parent-resource', '../escape.dd2mesh'), ('absolute-resource', '/escape.dd2mesh'),
                         ('empty-component', 'a//b.dd2mesh'), ('wrong-resource-type', 'a.png')):
        altered = bytearray(data)
        altered[HEADER.size:HEADER.size + 64] = name.encode().ljust(64, b'\0')
        cases[label] = bytes(altered)
    for label, first, size in (('duplicate-resource', HEADER.size, 64),
                                ('duplicate-instance', first_instance, 32),
                                ('duplicate-template', first_template, 32)):
        record_size = RESOURCE.size if label == 'duplicate-resource' else INSTANCE.size if label == 'duplicate-instance' else 36
        altered = bytearray(data)
        altered[first + record_size:first + record_size + size] = altered[first:first + size]
        cases[label] = bytes(altered)
    low_high = RESOURCE.unpack_from(data, HEADER.size)[1:]
    axis = next(axis for axis in range(3) if low_high[axis] < low_high[axis + 3])
    change('incorrect-finite-culling-bounds', HEADER.size + 64 + axis * 4, '<f',
           (low_high[axis] + low_high[axis + 3]) / 2)
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT / 'assets/runtime')
    parser.add_argument('--output', type=Path, default=WORK / 'prepared-world-verification')
    args = parser.parse_args()
    runtime = args.runtime.resolve()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1:exitcode=86',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1:exitcode=87')
    calls = []

    def run(command, label, expected=0):
        log = output / (label + '.log')
        with log.open('wb') as stream:
            result = run_bounded(list(map(str, command)), directory=output, timeout=120,
                                 stdout=stream, stderr=subprocess.STDOUT, cwd=output, env=environment)
        data = log.read_bytes()
        calls.append(dict(label=label, returncode=result.returncode, output_sha256=digest(data)))
        if result.returncode != expected or b'Sanitizer:' in data or b'runtime error:' in data:
            raise RuntimeError(label + ': ' + data.decode(errors='replace'))

    sanitized = output / 'dd2_world_export_sanitized'
    core = ['src/assets/world.c', 'src/assets/model.c', 'src/platform/file.c']
    run([tool('clang'), *FLAGS, *[ROOT / name for name in [*core, 'tests/world_export.c']],
         '-lm', '-o', sanitized], 'sanitized-export-build')
    sanitized_test = output / 'dd2_world_test_sanitized'
    run([tool('clang'), *FLAGS, *[ROOT / name for name in [*core, 'tests/world_test.c']],
         '-lm', '-o', sanitized_test], 'sanitized-test-build')
    run([sanitized_test], 'sanitized-ownership-bounds')
    commands = {'native': [WORK / 'rewrite-native/dd2_world_export'],
                'wasm': ['node', WORK / 'rewrite-wasm/dd2_world_export.js'], 'sanitized': [sanitized]}
    manifest = json.loads((runtime / 'reference/manifest.json').read_text())
    entries = []
    raw = output / 'decoded.dd2scene'
    for level in manifest['levels']:
        path = runtime / level['compiled_scene']
        source = path.read_bytes()
        expected = compile_scene(json.loads((runtime / level['scene']).read_text()))
        if source != expected:
            raise ValueError('Prepared scene binary differs from JSON')
        for platform, command in commands.items():
            run([*command, runtime, path, raw], platform + '-level-' + level['level'])
            if raw.read_bytes() != expected:
                raise ValueError(platform + ' typed scene re-encoding differs')
            raw.unlink()
        _, models, instances, templates = HEADER.unpack(source[:HEADER.size])
        entries.append(dict(level=level['level'], sha256=digest(source), bytes=len(source),
                            resources=models, instances=instances, templates=templates))
        print(json.dumps(dict(level=level['level'], platforms=list(commands))), flush=True)
    variants = mutations((runtime / manifest['levels'][0]['compiled_scene']).read_bytes())
    altered = output / 'mutated.dd2scene'
    for label, data in variants.items():
        altered.write_bytes(data)
        for platform, command in commands.items():
            run([*command, runtime, altered, raw], platform + '-reject-' + label, 1)
            if raw.exists():
                raise ValueError('Rejected scene created comparison output')
    altered.unlink()
    sources = [*core, 'tests/world_export.c', 'tests/world_test.c', 'tests/world_fixture.h',
               'tests/content_file_fixture.h', 'tools/assets/prepare_scenes.py', 'tools/assets/verify_world.py']
    report = dict(pass_=True, verified_at=datetime.now(timezone.utc).isoformat(),
                  scope='All prepared scene fields after source release, owned model loading/bounds and malformed-scene rollback; no full-game/original parity/60-FPS claim.',
                  original_inputs_required=False, entries=entries, platforms=list(commands),
                  rejection_variants=list(variants), calls=calls,
                  source_sha256={name: digest((ROOT / name).read_bytes()) for name in sources})
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    sanitized.unlink()
    sanitized_test.unlink()
    for call in calls:
        (output / (call['label'] + '.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, levels=len(entries), variants=len(variants), report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
