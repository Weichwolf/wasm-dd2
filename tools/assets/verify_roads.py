#!/usr/bin/env python3
"""Verify prepared roads and gameplay consumers; originals are optional comparisons."""
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
from rewrite.quality import ROOT
from rewrite.verify_roads import compare
from assets.prepare_roads import compile_road, HEADER, VERTEX, STRIP, CELL, NONE


def digest(data):
    return hashlib.sha256(data).hexdigest()


def expected_fields(road):
    return dict(vertices=road['vertices'], main_count=road['main_count'],
                strips={index: [index, row['next'], row['previous'], row['branch'], row['main_order'],
                                 row['kind'], row['lanes'], 0, row['heading'], 0, 0, row['flags']]
                        for index, row in enumerate(road['strips'])},
                cells={(row['strip'], row['lane']): [row['strip'], row['lane'], row['surface_flags'],
                        row['heading'], row['triangle_mask'], *row['vertices']] for row in road['cells']})


def mutations(data):
    _, _, _, vertices, strips, _, cells = HEADER.unpack_from(data)
    first_strip = HEADER.size + vertices * VERTEX.size
    first_cell = first_strip + strips * STRIP.size
    result = dict(empty=b'', truncated_header=data[:31], truncated_tail=data[:-1], trailing_byte=data+b'\0')

    def change(name, offset, format_, value):
        altered = bytearray(data)
        struct.pack_into(format_, altered, offset, value)
        result[name] = bytes(altered)

    for name, offset, value in (('magic', 0, 0), ('layout', 8, 2), ('scale', 12, 1),
                                ('vertices', 16, NONE), ('strips', 20, NONE),
                                ('main', 24, NONE), ('cells', 28, NONE),
                                ('coordinate', HEADER.size, 0x7fffffff),
                                ('next', first_strip, strips), ('previous', first_strip+4, strips),
                                ('nonjunction_branch', first_strip+8, 0),
                                ('first_cell', first_strip+12, 1), ('main_order', first_strip+16, 1),
                                ('bad_corner', first_cell, vertices),
                                ('cell_owner', first_cell+16, strips), ('cell_lane', first_cell+20, NONE)):
        change(name, offset, '<I', value)
    for name, offset, value in (('kind', first_strip+22, 0), ('lanes_zero', first_strip+23, 0),
                                ('lanes_limit', first_strip+23, 128), ('strip_padding', first_strip+25, 1),
                                ('cell_padding', first_cell+27, 1), ('triangle_zero', first_cell+26, 0),
                                ('triangle_limit', first_cell+26, 4)):
        change(name, offset, '<B', value)
    # A formerly valid branch must not hide a self-contained next/previous cycle.
    records = [STRIP.unpack_from(data, first_strip + index*STRIP.size) for index in range(strips)]
    branch = next(index for index, row in enumerate(records) if row[4] == NONE)
    change('disconnected_next_cycle', first_strip+branch*STRIP.size, '<I', branch)
    change('disconnected_previous_cycle', first_strip+branch*STRIP.size+4, '<I', branch)
    change('duplicate_main_order', first_strip+STRIP.size+16, '<I', records[0][4])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT / 'assets/runtime')
    parser.add_argument('--output', type=Path, default=WORK / 'prepared-road-verification')
    parser.add_argument('--sanitized-build', type=Path, required=True)
    parser.add_argument('--reference-archive', type=Path)
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents or WORK not in args.sanitized_build.resolve().parents:
        parser.error('Verification/build output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    runtime = args.runtime.resolve()
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1:exitcode=86',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1:exitcode=87')
    calls = []

    def run(command, name, expected=0, export=False):
        log = output / (name + '.log')
        with log.open('wb') as stream:
            result = run_bounded(list(map(str, command)), directory=output, timeout=120,
                                 stdout=stream, stderr=subprocess.STDOUT, cwd=output, env=environment)
        data = log.read_bytes()
        calls.append(dict(name=name, returncode=result.returncode, sha256=digest(data)))
        if result.returncode != expected or b'Sanitizer:' in data or b'runtime error:' in data:
            raise RuntimeError(name + ': ' + data.decode(errors='replace'))
        if expected != 0 and data:
            raise ValueError('Rejected road emitted partial output')
        parsed = json.loads(data) if export else None
        log.unlink()
        return parsed

    commands = {'native': [WORK / 'rewrite-native/dd2_road_export'],
                'wasm': ['node', WORK / 'rewrite-wasm/dd2_road_export.js'],
                'sanitized': [args.sanitized_build / 'dd2_road_export']}
    gameplay = {'native': [WORK / 'rewrite-native/dd2_road_gameplay_export'],
                'wasm': ['node', WORK / 'rewrite-wasm/dd2_road_gameplay_export.js'],
                'sanitized': [args.sanitized_build / 'dd2_road_gameplay_export']}
    run([args.sanitized_build / 'dd2_road_content_test'], 'sanitized-ownership-rejections')
    run(['valgrind', '--error-exitcode=86', '--leak-check=full', WORK / 'rewrite-native/dd2_road_content_test'], 'memcheck-ownership')
    manifest = json.loads((runtime / 'roads/manifest.json').read_text())
    levels = []
    for level in manifest['levels']:
        path = runtime / level['resource']
        data = path.read_bytes()
        road = json.loads((runtime / level['editable']).read_text())
        if compile_road(road) != data or len(data) != level['bytes'] or digest(data) != level['sha256']:
            raise ValueError('Prepared road differs from editable JSON/inventory')
        results, gameplay_results = {}, {}
        for platform, command in commands.items():
            actual = run([*command, '--prepared', path], platform+'-'+level['level'], export=True)
            results[platform] = compare(actual, expected_fields(road))
            gameplay_results[platform] = run([*gameplay[platform], 'prepared', path, level['level']],
                                             platform+'-gameplay-'+level['level'], export=True)
        reference = gameplay_results['native']
        # Cross-target floating-point variations retain the established physics
        # comparison scale; integer IDs/flags/order are always exact.
        def equivalent(left, right):
            if isinstance(left, dict):
                return left.keys() == right.keys() and all(equivalent(left[key], right[key]) for key in left)
            if isinstance(left, list):
                return len(left) == len(right) and all(equivalent(a, b) for a, b in zip(left, right))
            if type(left) is int and type(right) is int:
                return left == right
            return abs(left-right) <= 1e-8 + 1e-12 * max(abs(left), abs(right))
        for row in gameplay_results.values():
            if not equivalent(reference, row):
                raise ValueError('Gameplay consumer output differs across targets')
        original_comparison = None
        if args.reference_archive:
            original = run([*gameplay['native'], 'reference', args.reference_archive.resolve(), level['level']],
                           'optional-reference-gameplay-'+level['level'], export=True)
            if reference != original:
                raise ValueError('Original-decoded and prepared gameplay consumer outputs differ')
            original_comparison = True
        levels.append(dict(level=level['level'], platforms=results, grid_starts=len(reference['starts']),
                           barrier_segments=len(reference['barriers']), course_length=reference['course']['length'],
                           ai_queries=len(reference['paths']), valid_ai_queries=sum(row[0] for row in reference['paths']),
                           gameplay_sha256={name:digest(json.dumps(row, sort_keys=True).encode()) for name,row in gameplay_results.items()},
                           optional_original_consumer_comparison=original_comparison))
        print(json.dumps(dict(level=level['level'], pass_=True)), flush=True)
    variants = mutations((runtime / manifest['levels'][0]['resource']).read_bytes())
    altered = output / 'mutated.dd2road'
    for name, data in variants.items():
        altered.write_bytes(data)
        for platform, command in commands.items():
            run([*command, '--prepared', altered], platform+'-reject-'+name, expected=1)
            run([*gameplay[platform], 'prepared', altered, '1'], platform+'-gameplay-reject-'+name, expected=1)
    altered.unlink()
    sources = ['src/assets/road.c','src/assets/road.h','tests/road_export.c','tests/road_content_test.c',
               'tests/road_gameplay_export.c','tools/assets/prepare_roads.py','tools/assets/convert_roads.py',
               'tools/assets/verify_roads.py','src/game/starting_grid.c','src/game/course.c','src/ai/path.c']
    report = dict(pass_=True, verified_at=datetime.now(timezone.utc).isoformat(),
                  scope='Owned road fields/ownership/topology, rational triangle contacts and current grid/barrier/course/AI consumers; no original-executable/full-game/standalone-startup/60-FPS acceptance',
                  original_inputs_required=False, optional_reference_requested=bool(args.reference_archive),
                  levels=levels, rejection_variants=list(variants), calls=calls,
                  source_sha256={name:digest((ROOT/name).read_bytes()) for name in sources})
    (output / 'report.json').write_text(json.dumps(report,indent=2)+'\n')
    check_space(output)
    print(json.dumps(dict(pass_=True, levels=len(levels), rejection_variants=len(variants), report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
