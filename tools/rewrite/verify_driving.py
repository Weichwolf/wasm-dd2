#!/usr/bin/env python3
"""Check original grid starts and deterministic free-driving presentation.

Native, Node/WASM and ASan/UBSan snapshots cover eleven tracks/arenas before
and after three seconds of acceleration. This is not a race or original parity.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256
from rewrite.verify_levels import assets
from rewrite.verify_roads import reference
from rewrite.verify_scene_render import compare_images

CODES = '123456789AB'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def original_starts(files):
    # Read independent raw graph/quad geometry. The original preprocessing assigns
    # main IDs first, then branch IDs; source_number is lap equivalence, not this ID.
    image = (ROOT / 'DestructionDerby2/dd2_image.bin').read_bytes()
    rows = list(struct.iter_unpack('<2i', image[0x63dcc:0x63e2c]))
    starts = {}
    for code, number, lane in zip('1234567', (266, 656, 535, 614, 22, 242, 449), (2, 1, 3, 2, 2, 2, 1)):
        decoded = reference(files[f'LEV{code}\\LEVEL.DAT'], code, rows)
        strips = decoded['strips']
        numbered = {s[4]: s for s in strips.values() if s[4] != 0xffffffff}
        counter, current = len(numbered), 0
        for _ in range(len(strips)):
            strip = strips[current]
            if strip[5] == 8:
                current = strip[3]
                for _ in range(len(strips)):
                    if strips[current][5] == 9: break
                    numbered[counter] = strips[current]
                    counter += 1
                    current = strips[current][1]
            current = strips[current][1]
            if current == 0: break
        strip = numbered[number]
        cell = decoded['cells'][(strip[0], lane)]
        vertices = [decoded['vertices'][index] for index in cell[5:]]
        position = [sum(vertex[axis] for vertex in vertices) / 4 for axis in range(3)]
        position[1] += 190
        starts[code] = [*position, ((192 - strip[8]) % 256) * math.tau / 256]
    for code in '89AB':
        level = files[f'LEV{code}\\LEVEL.DAT']
        offsets = struct.unpack_from('<29I', level)
        vertices = list(struct.iter_unpack('<3i', level[offsets[2]:offsets[3]]))
        xpos, zpos = (-12000, 0) if code == 'B' else (0, -12000)
        height = next(y for x, y, z in vertices if x == xpos and z == zpos)
        starts[code] = [xpos, height + 190, zpos, math.pi / 2 if code == 'B' else 0]
    return starts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-driving-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents: parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (ROOT / 'DestructionDerby2/Dirinfo').resolve()
    data = archive.read_bytes()
    if digest(data) != ORIGINAL_SHA256: raise ValueError('Provision supported unmodified Dirinfo')
    expected = original_starts(assets(data))
    calls = []

    def run(command, label):
        log = output / (label + '.log')
        with log.open('wb') as stream:
            result = run_bounded(command, directory=output, timeout=60, stdout=stream,
                                 stderr=subprocess.STDOUT, cwd=ROOT)
        content = log.read_text(errors='replace')
        calls.append(dict(label=label, returncode=result.returncode, log_sha256=digest(log.read_bytes())))
        if result.returncode != 0 or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label + ' failed: ' + content[-4000:])
        return [json.loads(line) for line in content.splitlines() if line.startswith('{')]

    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT / 'src'),
             '-I', str(ROOT / 'deps/softgl/libsoftgl/include'),
             '-Wall', '-Wextra', '-Wpedantic', '-Wno-unused-parameter', '-Wno-unused-function',
             '-fno-strict-aliasing', '-ffast-math', '-Werror', '-Wshadow', '-Wconversion',
             '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wformat=2',
             '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    assets_units = [ROOT / f'src/assets/{name}.c' for name in
                    ('archive', 'car', 'level', 'textures', 'lz', 'mesh', 'scene', 'track', 'road', 'barriers', 'save_card', 'save_profile')]
    physics_units = [ROOT / f'src/physics/{name}.c' for name in ('road_contact', 'road_surface', 'body_surface', 'vehicle', 'barrier_world', 'car_contact', 'contact_group', 'vehicle_collision', 'damage')]
    render_units = [ROOT / f'src/render/{name}.c' for name in ('renderer', 'mesh_draw', 'driving_draw', 'damage_draw', 'score_draw', 'race_draw')]
    game_units = [ROOT / f'src/game/{name}.c' for name in ('driving', 'starting_grid', 'accidents', 'course', 'laps', 'race', 'recovery', 'sound_events', 'drivers', 'league', 'championship', 'configuration', 'profile_menu')]
    game_units += [ROOT / f'src/ai/{name}.c' for name in ('path', 'driver')]
    preview, test = output / 'preview-sanitized', output / 'timing-sanitized'
    run([tool('clang'), *flags, *map(str, assets_units + physics_units + render_units + game_units),
         str(ROOT / 'tests/driving_preview.c'), str(WORK / 'rewrite-native/softgl/libsoftgl.a'),
         '-lm', '-o', str(preview)], 'preview-build')
    sound_fixture = output / 'sound-fixture.o'
    run([tool('clang'), *flags, '-Ddd2_sound_events_step=dd2_drive_fixture_sound_step',
         '-c', str(ROOT / 'src/game/sound_events.c'), '-o', str(sound_fixture)], 'sound-fixture-build')
    test_game_units = [p for p in game_units if p.name != 'sound_events.c']
    run([tool('clang'), *flags, *map(str, assets_units + physics_units + test_game_units),
         str(sound_fixture),
         str(ROOT / 'tests/driving_test.c'), '-lm', '-o', str(test)], 'timing-build')
    run([str(test)], 'timing-sanitized')
    commands = {'native': [str(WORK / 'rewrite-native/dd2_driving_preview')],
                'wasm': ['node', str(WORK / 'rewrite-wasm/dd2_driving_preview.js')],
                'sanitized': [str(preview)]}
    comparisons = []
    for code in CODES:
        for mode in ('start', 'drive'):
            results = {}
            for platform, command in commands.items():
                path = output / (platform + '.ppm')
                spawn, state, coverage = run([*command, str(archive), str(path), code, mode],
                                             f'{code}-{mode}-{platform}')
                if not all(math.isclose(a, b, abs_tol=1e-9, rel_tol=1e-12)
                           for a, b in zip(spawn['spawn'], expected[code])):
                    raise ValueError('Original grid spawn differs: ' + code)
                if state['steps'] != (200 if mode == 'start' else 800):
                    raise ValueError('Fixed step count differs')
                if mode == 'start' and state['contacts'] != 4:
                    raise ValueError('Start has unsupported wheels: ' + str(state))
                if platform == 'native':
                    baseline = state
                else:
                    for field in ('position', 'velocity', 'rotation'):
                        if not all(math.isclose(a, b, abs_tol=1e-5, rel_tol=1e-8)
                                   for a, b in zip(baseline[field], state[field])):
                            raise ValueError('Simulation state differs: ' + field)
                    if baseline['contacts'] != state['contacts'] or baseline['collisions'] != state['collisions']:
                        raise ValueError('Wheel contacts differ')
                    results[platform] = compare_images(output / 'native.ppm', path)
                    if platform == 'sanitized' and results[platform]['changed_pixels'] != 0:
                        raise ValueError('Sanitized native image differs')
            comparisons.append(dict(level=code, mode=mode, state=baseline, comparisons=results))
            print(json.dumps(dict(level=code, mode=mode, pass_=True)), flush=True)
    sources = [path for path in (ROOT / 'src').rglob('*') if path.suffix in ('.c', '.h')]
    sources += [ROOT / 'tests/driving_preview.c', ROOT / 'tests/driving_test.c',
                ROOT / 'tests/driving_sound_fixture.h', Path(__file__).resolve()]
    binaries = [WORK / 'rewrite-native/dd2_driving_preview', WORK / 'rewrite-wasm/dd2_driving_preview.wasm', preview, test, sound_fixture]
    report = dict(pass_=True, scope=__doc__.strip(), verified_at=datetime.now(timezone.utc).isoformat(),
                  original_sha256=ORIGINAL_SHA256, starts=expected, comparisons=comparisons, calls=calls,
                  sanitizer_scope='Rewrite C instrumented; pinned release SoftGL uninstrumented',
                  source_sha256={str(p.relative_to(ROOT)): digest(p.read_bytes()) for p in sources},
                  binary_sha256={str(p): digest(p.read_bytes()) for p in binaries})
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    preview.unlink(); test.unlink(); sound_fixture.unlink()
    for path in output.iterdir():
        if path.suffix in ('.ppm', '.log'): path.unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, previews=len(comparisons), report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
