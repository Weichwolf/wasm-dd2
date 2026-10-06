#!/usr/bin/env python3
"""Check course equivalence, complete route laps and physical progress on three targets.

Read the seven original road graphs and finish/default-lap table independently.
All main-route branch combinations complete the required laps through the rules
layer; these are ordered source-cell traces, not physically driven full races.
Six-second twenty-car physical fields additionally check every progress/timer
state against an independent geometric contact and sequential-checkpoint oracle.
Frame partition, pause and reset run in the owner. Race modes/results/countdown
and championship standings are outside this check; no original physics parity.
"""
import argparse
from bisect import bisect_right
from collections import defaultdict
from datetime import datetime, timezone
from fractions import Fraction
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
from rewrite.verify_fleet import original_grid
from rewrite.verify_levels import assets
from rewrite.verify_roads import reference, CORNERS, NONE

CODES = '1234567'
STEPS = 1200
COUNT = 20
EPSILON = 1e-6


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def course_numbers(decoded):
    strips = decoded['strips']
    numbers, current, counter = {}, 0, 0

    def path(start):
        result = []
        while strips[start][5] != 9:
            if start in result:
                raise ValueError('Source path never reaches a merge')
            result.append(start)
            start = strips[start][1]
        return result, start

    while current not in numbers:
        split = current
        numbers[current] = counter
        counter += 1
        current = strips[current][1]
        if strips[split][5] != 8:
            continue
        first, merge = path(current)
        second, other_merge = path(strips[split][3])
        if merge != other_merge:
            raise ValueError('Visited split arms have different merges')
        short, long = (first, second) if len(first) < len(second) else (second, first)
        # Invert cumulative source rounding with rational boundaries. Each long
        # strip finds its progress bin, independent of C's cursor/group loops.
        increment = Fraction((len(long) * 65536 + 32768) // len(short), 65536)
        boundaries = [min(len(long), math.floor(Fraction(1, 2) + i * increment))
                      for i in range(1, len(short) + 1)]
        for index, strip in enumerate(short):
            numbers[strip] = counter + index
        for index, strip in enumerate(long):
            numbers[strip] = counter + bisect_right(boundaries, index)
        counter += len(short)
        current = merge
    if current != 0 or set(numbers) != set(strips):
        raise ValueError('Incomplete original course map')
    return numbers, counter


class LapOracle:
    def __init__(self, course, cell):
        self.course = course
        self.values = [0] * 12
        self.values[5] = cell
        self.values[6] = self.values[7] = course['relative'][cell]

    def tick(self, cells, retired=False):
        v = self.values
        v[0] += 1
        v[10] = int(retired)
        if v[11] or retired:
            return
        for cell in cells:
            if cell == NONE:
                continue
            relative = self.course['relative'][cell]
            last = self.course['length'] - 1
            if relative == 0 and v[8] == v[9] and v[7] == last:
                if v[8]:
                    duration = v[0] - v[1]
                    v[2] = duration
                    v[3] = min(v[3], duration) if v[3] else duration
                v[1] = v[0]
                v[7] = 0
                v[8] += 1
                v[9] = v[8]
                if self.course['laps'] and v[9] > self.course['laps']:
                    v[11], v[4] = 1, v[0]
            elif relative == 0 and v[6] == last and v[9] < v[8]:
                v[9] = v[8]
            if relative:
                if relative == v[7] + 1:
                    v[7] += 1
                if v[6] == 0 and relative == last and v[8] and v[9] == v[8]:
                    v[9] -= 1
            v[5], v[6] = cell, relative
            if v[11]:
                break

    def compare(self, state, label):
        if state != self.values:
            raise ValueError(f'{label}: {state} != {self.values}')


class SurfaceOracle:
    def __init__(self, decoded, course):
        self.buckets = defaultdict(list)
        self.triangles = {}
        self.queries = 0
        for cell, key in course['keys'].items():
            geometry = decoded['cells'][key]
            points = [decoded['vertices'][index] for index in geometry[5:]]
            xmin, xmax = min(p[0] for p in points), max(p[0] for p in points)
            zmin, zmax = min(p[2] for p in points), max(p[2] for p in points)
            for xpos in range(math.floor((xmin - EPSILON) / 1000), math.floor((xmax + EPSILON) / 1000) + 1):
                for zpos in range(math.floor((zmin - EPSILON) / 1000), math.floor((zmax + EPSILON) / 1000) + 1):
                    self.buckets[xpos, zpos].append(cell)
            records = []
            for triangle, corners in enumerate(CORNERS):
                if not geometry[4] & (1 << triangle):
                    continue
                a, b, c = vertices = [points[index] for index in corners]
                edge, other = [b[i] - a[i] for i in range(3)], [c[i] - a[i] for i in range(3)]
                normal = [edge[1]*other[2]-edge[2]*other[1], edge[2]*other[0]-edge[0]*other[2],
                          edge[0]*other[1]-edge[1]*other[0]]
                if normal[1] == 0:
                    continue
                sign = -1 if normal[1] > 0 else 1
                edges = [(sign*(end[0]-start[0]), sign*(end[2]-start[2]), start[0], start[2],
                          EPSILON * math.hypot(end[0]-start[0], end[2]-start[2]))
                         for start, end in zip(vertices, vertices[1:] + vertices[:1])]
                bounds = (min(p[0] for p in vertices)-EPSILON, max(p[0] for p in vertices)+EPSILON,
                          min(p[2] for p in vertices)-EPSILON, max(p[2] for p in vertices)+EPSILON)
                records.append((a, normal, edges, bounds))
            self.triangles[cell] = records

    def select(self, position, preferred):
        self.queries += 1
        xpos, height, zpos = position
        candidates = []
        for cell in self.buckets[math.floor(xpos/1000), math.floor(zpos/1000)]:
            for a, normal, edges, bounds in self.triangles[cell]:
                if not (bounds[0] <= xpos <= bounds[1] and bounds[2] <= zpos <= bounds[3]):
                    continue
                if any(dx*(zpos-sz)-dz*(xpos-sx) < -tolerance for dx,dz,sx,sz,tolerance in edges):
                    continue
                ground = a[1] - (normal[0]*(xpos-a[0]) + normal[2]*(zpos-a[2])) / normal[1]
                # Cell contact chooses its first containing triangle, before the
                # surface index applies the support-height window.
                if height - 380 <= ground <= height:
                    candidates.append((ground, cell))
                break
        if not candidates:
            return NONE
        highest = max(height for height, _ in candidates)
        eligible = [cell for height, cell in candidates if height >= highest - EPSILON]
        return preferred if preferred in eligible else min(eligible)

    def trace(self, before, after, preferred):
        distance = math.hypot(after[0] - before[0], after[2] - before[2])
        if distance > 32 * 64:
            return [NONE]
        count = max(1, math.ceil(distance/32))
        result = []
        for index in range(1, count + 1):
            position = [a + index/count*(b-a) for a,b in zip(before, after)]
            cell = self.select(position, preferred)
            result.append(cell)
            if cell != NONE:
                preferred = cell
        return result


def check(rows, decoded, image, code):
    metadata = rows[0]
    numbers, length = course_numbers(decoded)
    finish, nominal_length, laps = struct.unpack_from('<3H', image, 0x66df0 + int(code)*6)
    if metadata['kind'] != 'course' or (metadata['length'], metadata['finish'], metadata['laps']) != (length, finish, laps):
        raise ValueError('Source course rules differ')
    if {offset:number for offset,number,_ in metadata['strips']} != numbers:
        raise ValueError('Branch/main equivalents differ')
    course = dict(length=length, laps=laps, relative={}, keys={}, first={})
    for offset, number, first in metadata['strips']:
        course['first'][offset] = first
        for lane in range(decoded['strips'][offset][6]):
            cell = first + lane
            if cell in course['keys']:
                raise ValueError('Overlapping cell inventory')
            course['relative'][cell] = (number - finish) % length
            course['keys'][cell] = offset, lane
    if set(course['keys']) != set(range(len(decoded['cells']))):
        raise ValueError('Incomplete cell inventory')
    sca = [struct.unpack_from('<HBx', image, 0x66da0 + i*4) for i in range(COUNT)]
    grid = original_grid(decoded, code, sca)
    for slot, cell in enumerate(metadata['starts']):
        geometry = decoded['cells'][course['keys'][cell]]
        vertices = [decoded['vertices'][index] for index in geometry[5:]]
        center = [sum(p[i] for p in vertices)/4 for i in range(3)]
        if center != [grid[slot][0], grid[slot][1]-190, grid[slot][2]]:
            raise ValueError('Progress not initialized at source grid cell')
    splits = [offset for offset,_,_ in metadata['strips']
              if decoded['strips'][offset][5] == 8 and decoded['strips'][offset][4] != NONE]
    routes, route_steps = defaultdict(list), 0
    for row in rows[1:]:
        if row['kind'] == 'route':
            routes[row['variant']].append(row['state'])
    if set(routes) != set(range(1 << len(splits))):
        raise ValueError('Missing branch combination')
    for variant, states in routes.items():
        oracle = LapOracle(course, metadata['starts'][0])
        strip = course['keys'][metadata['starts'][0]][0]
        for state in states:
            branch = strip in splits and bool(variant & (1 << splits.index(strip)))
            strip = decoded['strips'][strip][3 if branch else 1]
            oracle.tick([course['first'][strip]])
            oracle.compare(state, f'route {variant}')
            route_steps += 1
        if not oracle.values[11] or oracle.values[9] != laps + 1:
            raise ValueError('Ordered source path never completes required laps')
    live = [row for row in rows if row['kind'] == 'live']
    if [row['step'] for row in live] != [0, *range(1, STEPS+1), 0]:
        raise ValueError('Incomplete physical/reset sequence')
    oracles = [LapOracle(course, cell) for cell in metadata['starts']]
    positions = [car['position'] for car in live[0]['cars']]
    surface = SurfaceOracle(decoded, course)
    for row in live:
        if len(row['cars']) != COUNT:
            raise ValueError('Incomplete physical field')
        if row['step'] == 0:
            oracles = [LapOracle(course, cell) for cell in metadata['starts']]
        for slot, car in enumerate(row['cars']):
            if row['step']:
                trace = surface.trace(positions[slot], car['position'], oracles[slot].values[5])
                oracles[slot].tick(trace, car['retired'])
            oracles[slot].compare(car['state'], f'live step {row["step"]} slot {slot}')
        positions = [car['position'] for car in row['cars']]
    if live[0] != live[-1]:
        raise ValueError('Reset does not restore complete field')
    started = sum(car['state'][8] > 0 for car in live[-2]['cars'])
    if started == 0:
        raise ValueError('Actual field never crosses start line')
    return dict(strips=len(numbers), length=length, nominal_length=nominal_length,
                finish=finish, required_laps=laps, route_variants=len(routes), route_steps=route_steps,
                complete_rules_traces=len(routes), physical_vehicle_steps=STEPS*COUNT,
                physical_starters=started, geometric_queries=surface.queries,
                partition_pause_reset=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK/'rewrite-laps-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (ROOT/'DestructionDerby2/Dirinfo').resolve()
    if digest(archive) != ORIGINAL_SHA256:
        raise ValueError('Provision supported unmodified Dirinfo')
    files = assets(archive.read_bytes())
    image = (ROOT/'DestructionDerby2/dd2_image.bin').read_bytes()
    row_table = list(struct.iter_unpack('<2i', image[0x63dcc:0x63e2c]))
    calls = []

    def run(command, label):
        path = output/(label+'.log')
        with path.open('wb') as log:
            result = run_bounded(command, directory=output, timeout=180, cwd=ROOT,
                                 stdout=log, stderr=subprocess.STDOUT)
        content = path.read_text(errors='replace')
        calls.append(dict(label=label, returncode=result.returncode, log_sha256=digest(path)))
        if result.returncode or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label+' failed: '+content[-3000:])
        return [json.loads(line) for line in content.splitlines() if line.startswith('{')]

    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT/'src'), '-Wall', '-Wextra', '-Wpedantic',
             '-Wno-unused-parameter', '-Wno-unused-function', '-fno-strict-aliasing', '-ffast-math',
             '-Werror', '-Wshadow', '-Wconversion', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wformat=2', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    units = [ROOT/f'src/assets/{name}.c' for name in ('archive', 'level', 'road', 'barriers')]
    units += [ROOT/f'src/physics/{name}.c' for name in
              ('road_contact', 'road_surface', 'vehicle', 'barrier_world', 'car_contact', 'vehicle_collision', 'damage')]
    units += [ROOT/f'src/game/{name}.c' for name in ('starting_grid', 'driving', 'accidents', 'course', 'laps', 'race','recovery')]
    units += [ROOT/f'src/ai/{name}.c' for name in ('path', 'driver')]
    units += [ROOT/'src/platform/file.c']
    sanitized, synthetic = output/'export-sanitized', output/'rules-sanitized'
    for source, binary in (('laps_export', sanitized), ('laps_test', synthetic)):
        run([tool('clang'), *flags, *map(str, units), str(ROOT/f'tests/rewrite/{source}.c'),
             '-lm', '-o', str(binary)], source+'-build')
    run([str(synthetic)], 'rules-sanitized')
    commands = {'native': [str(WORK/'rewrite-native/dd2_laps_export')],
                'wasm': ['node', str(WORK/'rewrite-wasm/dd2_laps_export.js')],
                'sanitized': [str(sanitized)]}
    levels = []
    for code in CODES:
        decoded = reference(files[f'LEV{code}\\LEVEL.DAT'], code, row_table)
        targets = {target:check(run([*command, str(archive), code], code+'-'+target), decoded, image, code)
                   for target, command in commands.items()}
        levels.append(dict(level=code, targets=targets))
        print(json.dumps(dict(level=code, targets=targets, pass_=True)), flush=True)
    sources = [path for path in (ROOT/'src').rglob('*') if path.suffix in ('.c', '.h')]
    sources += [ROOT/'tests/rewrite/laps_export.c', ROOT/'tests/rewrite/laps_test.c',
                ROOT/'tests/rewrite/mesh_render_test.c', ROOT/'CMakeLists.txt', Path(__file__).resolve()]
    report = dict(pass_=True, scope=__doc__.strip(), verified_at=datetime.now(timezone.utc).isoformat(),
                  original_sha256=digest(archive), image_sha256=hashlib.sha256(image).hexdigest(), levels=levels, calls=calls,
                  source_sha256={str(path.relative_to(ROOT)):digest(path) for path in sources},
                  binary_sha256={str(path):digest(path) for path in
                                 (WORK/'rewrite-native/dd2_laps_export', WORK/'rewrite-wasm/dd2_laps_export.wasm', sanitized, synthetic)})
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    for path in output.iterdir():
        if path.suffix == '.log' or path in (sanitized, synthetic):
            path.unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, levels=len(levels), physical_vehicle_steps_per_target=STEPS*COUNT*len(levels),
                         report=str(output/'report.json'))))


if __name__ == '__main__':
    main()
