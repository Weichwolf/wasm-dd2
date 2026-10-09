#!/usr/bin/env python3
"""Validate committed livery bindings; optionally compare original paint code.

Standalone checks read owned scenes/models only. --original adds a bounded,
optional offline component oracle, never a required build or runtime input.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from assets.prepare_scenes import compile_scene

EXE_SHA256 = '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
LODS = ('close', 'medium', 'distant')
NUMBERS = [1, 0, 7, 13, 17, 35, 37, 40, 42, 47, 50, 52, 53, 64, 66, 69, 77, 82, 88, 99]
from rewrite.verify_archive import ORIGINAL_SHA256
from rewrite.verify_levels import assets
from rewrite.quality import ROOT


def digest(data):
    return hashlib.sha256(data).hexdigest()


def bindings(runtime):
    manifest = json.loads((runtime / 'reference/manifest.json').read_text())
    if len(manifest['liveries']['levels']) != 11:
        raise ValueError('Incomplete livery level inventory')
    count = 0
    for level in manifest['liveries']['levels']:
        source = next(row for row in manifest['levels'] if row['level'] == level['level'])
        scene = json.loads((runtime / source['scene']).read_text())
        if len(level['variants']) != 22:
            raise ValueError('Incomplete livery/class inventory')
        for variant, item in enumerate(level['variants']):
            driver = variant if variant < 20 else 0
            car_class = variant - 19 if variant >= 20 else 0
            if (item['variant'], item['driver'], item['car_class'], item['number']) != (
                    variant, driver, car_class, NUMBERS[driver]):
                raise ValueError('Unstable driver/class/number mapping')
            for lod in LODS:
                model = item['models'][lod]
                binding = scene['templates'][f'car-{lod}-{variant:02}']
                base = source['templates']['car-' + lod]
                evidence = source['templates'][f'car-{lod}-{variant:02}']
                if (binding != dict(model=model['model'], bounds=model['bounds']) or
                        model['triangles'] != base['triangles'] or evidence != base or
                        manifest['models'][model['model']]['bounds'] != model['bounds']):
                    raise ValueError('Livery geometry/count/binding differs')
                count += 1
    item = next(iter(scene['templates'].values()))
    scene['templates'] = {f'limit-{index:03}': item for index in range(128)}
    compile_scene(scene)
    scene['templates']['overflow'] = item
    try:
        compile_scene(scene)
    except ValueError:
        pass
    else:
        raise ValueError('Scene compiler accepted too many templates')
    return manifest, count


def original_checks(manifest, archive, executable, output):
    data = archive.read_bytes()
    if digest(data) != ORIGINAL_SHA256 or digest(executable.read_bytes()) != EXE_SHA256:
        raise ValueError('Unsupported original comparison inputs')
    files = assets(data)
    calls = []
    def run(command, label):
        log = output / (label + '.log')
        with log.open('wb') as stream:
            result = run_bounded(list(map(str, command)), directory=output, timeout=120,
                                 stdout=stream, stderr=subprocess.STDOUT, cwd=output)
        raw = log.read_bytes()
        calls.append(dict(label=label, returncode=result.returncode, sha256=digest(raw)))
        if result.returncode:
            raise ValueError(label + ': ' + raw.decode(errors='replace'))
        log.unlink()
    original = output / 'original-livery-oracle'
    run(['gcc', '-m32', '-no-pie', '-O0', '-std=gnu99', '-w',
         ROOT / 'tools/reference/car_livery_fixture.c', '-o', original], 'original-build')
    cases = []
    for level in manifest['liveries']['levels']:
        code = level['level']
        level_data = files[f'LEV{code}\\LEVEL.DAT']
        offsets = struct.unpack_from('<29I', level_data)
        sections = [level_data[a:b] for a, b in zip(offsets, (*offsets[1:], len(level_data))) ]
        for name, index in (('sprites', 3), ('definitions', 4)):
            (output / (name + '.bin')).write_bytes(sections[index])
        for detail, lod in enumerate(LODS):
            (output / 'mesh.bin').write_bytes(sections[17 - detail])
            raw_path = output / 'original.bin'
            run([original, executable, output / 'sprites.bin', output / 'mesh.bin',
                 output / 'definitions.bin', raw_path, detail], code + '-' + lod)
            raw = raw_path.read_bytes()
            selected_faces = [[face for face in item['models'][lod]['surfaces']
                               if face['opcode'] & 253 in (25, 29)] for item in level['variants']]
            face_count = len(selected_faces[0])
            words = 18 + 10 * face_count
            if len(raw) != 60 * words * 4 or any(len(faces) != face_count for faces in selected_faces):
                raise ValueError('Incomplete original material records')
            values = struct.unpack('<' + str(len(raw) // 4) + 'I', raw)
            for item, faces in zip(level['variants'], selected_faces):
                index = item['car_class'] * 20 + item['driver']
                expected = values[index * words:(index + 1) * words]
                actual = tuple(value for face in faces for value in (face['bank'], face['page'], *face['uv']))
                if actual != expected[18:]:
                    mismatch = next(i for i, (a,b) in enumerate(zip(actual, expected[18:])) if a != b)
                    raise ValueError(f'{code}/{lod}/variant {item["variant"]}: material word {mismatch}: '
                                     f'{actual[mismatch]} != {expected[18 + mismatch]}')
            cases.append(dict(level=code, lod=lod, variants=22, faces_per_variant=face_count,
                              source_sha256=digest(raw)))
            raw_path.unlink()
        check_space(output)
    for name in ('original-livery-oracle', 'sprites.bin', 'definitions.bin', 'mesh.bin'):
        (output / name).unlink()
    return dict(pass_=True, cases=cases, calls=calls,
                scope='Original palette/page/UV fields for close, medium and distant bodies; no full-game parity claim')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT / 'assets/runtime')
    parser.add_argument('--output', type=Path, default=WORK / 'prepared-livery-verification')
    parser.add_argument('--original', action='store_true')
    parser.add_argument('--archive', type=Path, default=ROOT / 'DestructionDerby2/Dirinfo')
    parser.add_argument('--executable', type=Path, default=ROOT / 'DestructionDerby2/dd2h.exe')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    manifest, count = bindings(args.runtime)
    report = dict(pass_=True, checked=datetime.now(timezone.utc).isoformat(), bindings=count,
                  original_inputs_required=False, original_comparison_requested=args.original,
                  runtime=str(args.runtime.resolve()), source_sha256={name:digest((ROOT/name).read_bytes()) for name in
                  ('tools/assets/convert_liveries.py','tools/assets/verify_liveries.py','tools/reference/car_livery_fixture.c')})
    if args.original:
        report['original'] = original_checks(manifest, args.archive.resolve(), args.executable.resolve(), output)
    check_space(output)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(pass_=True, bindings=count, original_compared=args.original)))


if __name__ == '__main__':
    main()
