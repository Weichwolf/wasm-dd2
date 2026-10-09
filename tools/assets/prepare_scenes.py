#!/usr/bin/env python3
"""Compile committed scene placements into an owned runtime container.

This uses prepared JSON/models only. It never opens original game files and
does not subdivide any geometry.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
HEADER = struct.Struct('<8s3I')
RESOURCE = struct.Struct('<64s6f')
INSTANCE = struct.Struct('<32sI3f')
TEMPLATE = struct.Struct('<32sI')


def text(name, length):
    value = name.encode('ascii')
    if not value or len(value) >= length or any(byte < 32 or byte > 126 for byte in value):
        raise ValueError('Invalid scene string')
    return value.ljust(length, b'\0')


def compile_scene(scene):
    if scene['format'] != 'DD2SCENE1' or scene['units'] != 'meters' or scene['up'] != 'Y':
        raise ValueError('Unsupported prepared scene')
    resources = {}
    for item in [*scene['objects'], *scene['templates'].values()]:
        path, bounds = item['model'], item['bounds']
        if path in resources and resources[path] != bounds:
            raise ValueError('Conflicting model bounds')
        if len(bounds) != 2 or any(len(row) != 3 for row in bounds):
            raise ValueError('Invalid model bounds')
        if any(not math.isfinite(v) or abs(v) > 1000000 for row in bounds for v in row):
            raise ValueError('Invalid scene scalar')
        if any(low > high for low, high in zip(*bounds)):
            raise ValueError('Reversed model bounds')
        resources[path] = bounds
    names = sorted(resources)
    indices = {name: index for index, name in enumerate(names)}
    objects, templates = scene['objects'], scene['templates']
    if len(names) > 4096 or len(objects) > 65536 or len(templates) > 64:
        raise ValueError('Scene counts exceed runtime allocation caps')
    identifiers = [item['id'] for item in objects]
    if len(set(identifiers)) != len(identifiers):
        raise ValueError('Duplicate instance identity')
    data = bytearray(HEADER.pack(b'DD2SCN1\0', len(names), len(objects), len(templates)))
    for name in names:
        components = name.split('/')
        if (not name.endswith('.dd2mesh') or any(part in ('', '.', '..') for part in components)
                or any(not (v.isascii() and (v.isalnum() or v in '/._-')) for v in name)):
            raise ValueError('Unsafe scene resource path')
        data.extend(RESOURCE.pack(text(name, 64), *resources[name][0], *resources[name][1]))
    for item in objects:
        position = item['position']
        if len(position) != 3 or any(not math.isfinite(v) or abs(v) > 1000000 for v in position):
            raise ValueError('Invalid instance position')
        data.extend(INSTANCE.pack(text(item['id'], 32), indices[item['model']], *position))
    for name, item in sorted(templates.items()):
        data.extend(TEMPLATE.pack(text(name, 32), indices[item['model']]))
    return bytes(data)


def prepare(runtime, manifest):
    compiled = {}
    for level in manifest['levels']:
        scene_path = runtime / level['scene']
        data = compile_scene(json.loads(scene_path.read_text()))
        path = scene_path.with_suffix('.dd2scene')
        path.write_bytes(data)
        name = str(path.relative_to(runtime))
        compiled[name] = dict(sha256=hashlib.sha256(data).hexdigest(), bytes=len(data))
        level['compiled_scene'] = name
    manifest['compiled_scenes'] = compiled
    return compiled


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT / 'assets/runtime')
    args = parser.parse_args()
    path = args.runtime / 'reference/manifest.json'
    manifest = json.loads(path.read_text())
    compiled = prepare(args.runtime, manifest)
    path.write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(dict(scenes=len(compiled), bytes=sum(item['bytes'] for item in compiled.values()),
                          original_inputs_required=False)))


if __name__ == '__main__':
    main()
