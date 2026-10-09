#!/usr/bin/env python3
"""Compile editable owned road JSON without original input or geometry subdivision."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
NONE = 0xffffffff
HEADER = struct.Struct('<8s6I')
VERTEX = struct.Struct('<3i')
STRIP = struct.Struct('<5IH3B3x')
CELL = struct.Struct('<6I3Bx')


def integer(value, low, high):
    if type(value) is not int or not low <= value <= high:
        raise ValueError('Invalid owned road integer')
    return value


def compile_road(road):
    if (road['format'] != 'DD2ROAD1' or type(road['units_per_meter']) is not int
            or road['units_per_meter'] != 160 or road['up'] != 'Y'):
        raise ValueError('Unsupported owned road format or scale')
    if road['layout'] not in ('racing', 'arena'):
        raise ValueError('Unsupported road layout')
    vertices, strips, cells = road['vertices'], road['strips'], road['cells']
    if not 1 <= len(vertices) <= 65536 or not 1 <= len(cells) <= 1000000 or len(strips) > 65536:
        raise ValueError('Owned road count exceeds runtime limits')
    main = integer(road['main_count'], 0, len(strips))
    arena = road['layout'] == 'arena'
    if (arena and (strips or main)) or (not arena and (not strips or not main)):
        raise ValueError('Road topology conflicts with layout')
    output = bytearray(HEADER.pack(b'DD2ROAD1', int(arena), 160, len(vertices), len(strips), main, len(cells)))
    for point in vertices:
        if len(point) != 3:
            raise ValueError('Invalid road vertex extent')
        output.extend(VERTEX.pack(*(integer(value, -160000000, 160000000) for value in point)))
    first = 0
    for strip in strips:
        next_ = integer(strip['next'], 0, len(strips) - 1)
        previous = integer(strip['previous'], 0, len(strips) - 1)
        kind = integer(strip['kind'], 1, 11)
        lanes = integer(strip['lanes'], 1, 127)
        branch = strip['branch']
        if kind in (8, 9):
            integer(branch, 0, len(strips) - 1)
        elif branch != NONE:
            raise ValueError('Unexpected non-junction branch')
        if strip['main_order'] != NONE:
            integer(strip['main_order'], 0, main - 1)
        first_cell = integer(strip['first_cell'], 0, len(cells))
        if first_cell != first or first + lanes > len(cells):
            raise ValueError('Noncontiguous lane ownership')
        first += lanes
        output.extend(STRIP.pack(next_, previous, branch, strip['first_cell'], strip['main_order'],
                                 integer(strip['flags'], 0, 65535), kind, lanes,
                                 integer(strip['heading'], 0, 255)))
    if strips:
        if first != len(cells):
            raise ValueError('Unowned lane suffix')
        current, visited = 0, set()
        for order in range(main):
            if current in visited or strips[current]['main_order'] != order:
                raise ValueError('Invalid main-loop progress')
            visited.add(current)
            current = strips[current]['next']
        if current != 0 or any((index in visited) != (strip['main_order'] != NONE)
                                for index, strip in enumerate(strips)):
            raise ValueError('Invalid main-loop ownership')
        for direction in ('next', 'previous'):
            reaches_start = {0}
            for start in range(1, len(strips)):
                current, path = start, set()
                while current not in path and current not in reaches_start:
                    path.add(current)
                    current = strips[current][direction]
                if current not in reaches_start:
                    raise ValueError('Disconnected road cycle')
                reaches_start.update(path)
    for index, cell in enumerate(cells):
        if len(cell['vertices']) != 4:
            raise ValueError('Invalid road cell extent')
        corners = [integer(value, 0, len(vertices) - 1) for value in cell['vertices']]
        strip = cell['strip']
        lane = integer(cell['lane'], 0, NONE)
        if arena:
            if strip != NONE:
                raise ValueError('Arena cell refers to a strip')
        else:
            integer(strip, 0, len(strips) - 1)
            if lane >= strips[strip]['lanes'] or strips[strip]['first_cell'] + lane != index:
                raise ValueError('Invalid lane owner')
        output.extend(CELL.pack(*corners, strip, lane, integer(cell['surface_flags'], 0, 255),
                                integer(cell['heading'], 0, 255), integer(cell['triangle_mask'], 1, 3)))
    return bytes(output)


def prepare(runtime, manifest):
    for level in manifest['levels']:
        source = runtime / level['editable']
        data = compile_road(json.loads(source.read_text()))
        target = source.with_suffix('.dd2road')
        target.write_bytes(data)
        level.update(resource=str(target.relative_to(runtime)), bytes=len(data),
                     sha256=hashlib.sha256(data).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT / 'assets/runtime')
    args = parser.parse_args()
    path = args.runtime / 'roads/manifest.json'
    manifest = json.loads(path.read_text())
    prepare(args.runtime, manifest)
    path.write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(dict(levels=len(manifest['levels']), original_inputs_required=False)))


if __name__ == '__main__':
    main()
