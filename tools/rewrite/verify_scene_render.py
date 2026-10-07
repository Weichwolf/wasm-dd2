#!/usr/bin/env python3
"""Render original scene/car geometry through SoftGL on native and Node/WASM."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256

LEVELS = '123456789AB'
HEADER = b'P6\n640 480\n255\n'
PIXELS = 640 * 480


def compare_images(reference, candidate):
    # Cross-target image consistency for this renderer. This is not a comparison
    # with the original game; billboard/lighting/gameplay policies remain pending.
    left, right = reference.read_bytes(), candidate.read_bytes()
    if not left.startswith(HEADER) or not right.startswith(HEADER) or len(left) != len(HEADER) + PIXELS * 3 or len(right) != len(left):
        raise ValueError('Invalid image extent')
    left, right = left[len(HEADER):], right[len(HEADER):]
    coverage = [sum(any(data[i:i + 3]) for i in range(0, len(data), 3)) for data in (left, right)]
    if min(coverage) < 100:
        raise ValueError('Empty scene/car image')
    changes = [sum(left[i:i + 3] != right[i:i + 3] for i in range(0, len(left), 3)),
               sum(abs(a - b) for a, b in zip(left, right))]
    # Floating point transform/raster edges can round differently under SIMD.
    # Require >=99% pixel agreement and mean RGB error <=0.5, recording exact
    # measurements so these bounds never substitute for inspecting differences.
    if changes[0] > PIXELS // 100 or changes[1] > len(left) // 2:
        raise ValueError(f'Cross-target renderer image differs: {changes}')
    return dict(visible_pixels=coverage, changed_pixels=changes[0], mean_channel_error=changes[1] / len(left),
                reference_sha256=hashlib.sha256(left).hexdigest(), candidate_sha256=hashlib.sha256(right).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path, default=ROOT / 'DestructionDerby2')
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-scene-render-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (args.game_dir / 'Dirinfo').resolve()
    if hashlib.sha256(archive.read_bytes()).hexdigest() != ORIGINAL_SHA256:
        raise ValueError('Provision the supported unmodified original Dirinfo')
    calls = []
    def run(command, label):
        log = output / (label + '.log')
        with log.open('w') as stream:
            result = run_bounded(command, directory=output, timeout=60, stdout=stream,
                                 stderr=subprocess.STDOUT, cwd=ROOT)
        content = log.read_text()
        calls.append(dict(label=label, command=list(map(str, command)), returncode=result.returncode,
                          log_sha256=hashlib.sha256(log.read_bytes()).hexdigest()))
        if result.returncode != 0 or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label + ' failed: ' + content)
    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT / 'src'), '-I', str(ROOT / 'deps/softgl/libsoftgl/include'),
             '-Wall', '-Wextra', '-Wpedantic', '-Wno-unused-parameter', '-Wno-unused-function',
             '-fno-strict-aliasing', '-ffast-math', '-Werror', '-Wshadow', '-Wconversion',
             '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wformat=2',
             '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    units = [str(ROOT / f'src/assets/{name}.c') for name in ('archive', 'level', 'textures', 'lz', 'mesh', 'scene')]
    units.append(str(ROOT / 'src/physics/damage.c'))
    units.extend(str(ROOT / f'src/render/{name}.c') for name in ('renderer', 'mesh_draw', 'camera'))
    sanitized_probe = output / 'dd2_mesh_render_sanitized'
    sanitized_preview = output / 'dd2_scene_preview_sanitized'
    softgl = WORK / 'rewrite-native/softgl/libsoftgl.a'
    for name, binary in (('mesh_render_test', sanitized_probe), ('scene_preview', sanitized_preview)):
        run([tool('clang'), *flags, *units, str(ROOT / f'tests/{name}.c'), str(softgl), '-lm', '-o', str(binary)], name + '-sanitizer-build')
    run([str(sanitized_probe)], 'sanitized-pixels')
    commands = {
        'native': [str(WORK / 'rewrite-native/dd2_scene_preview')],
        'wasm': ['node', str(WORK / 'rewrite-wasm/dd2_scene_preview.js')],
        'sanitized': [str(sanitized_preview)],
    }
    images = []
    for code in LEVELS:
        for mode in ('scene', 'car'):
            native = output / 'native.ppm'
            run([*commands['native'], str(archive), str(native), code, mode], f'{code}-{mode}-native')
            comparisons = {}
            for platform in ('wasm', 'sanitized'):
                candidate = output / (platform + '.ppm')
                run([*commands[platform], str(archive), str(candidate), code, mode], f'{code}-{mode}-{platform}')
                comparisons[platform] = compare_images(native, candidate)
                candidate.unlink()
            images.append(dict(level=code, mode=mode, comparisons=comparisons))
            native.unlink()
            print(json.dumps(dict(level=code, mode=mode, pass_=True)), flush=True)
    sources = ['src/render/mesh_draw.c', 'src/render/mesh_draw.h', 'src/render/camera.c',
               'src/render/camera.h', 'tests/mesh_render_test.c',
               'tests/scene_preview.c', 'tools/rewrite/verify_scene_render.py', 'CMakeLists.txt']
    binaries = [WORK / 'rewrite-native/dd2_scene_preview', WORK / 'rewrite-wasm/dd2_scene_preview.js',
                WORK / 'rewrite-wasm/dd2_scene_preview.wasm', sanitized_probe, sanitized_preview, softgl]
    report = dict(pass_=True, verified_at=datetime.now(timezone.utc).isoformat(), original_sha256=ORIGINAL_SHA256,
                  scope='static mesh rendering with neutral palette, stored sprite quads and UV/CLUT-selected opacity; lighting, billboards, blending, road contact/topology and gameplay pending',
                  comparisons=images, calls=calls,
                  sanitizer_scope='rewrite C units instrumented; pinned SoftGL linked from native release build',
                  source_sha256={p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in sources},
                  binary_sha256={str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in binaries})
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    sanitized_probe.unlink(); sanitized_preview.unlink()
    for call in calls:
        (output / (call['label'] + '.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, previews=len(images), platforms=list(commands), report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
