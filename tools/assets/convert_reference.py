#!/usr/bin/env python3
"""Once-only conversion of reference geometry into standalone DD2MESH2 assets.

This is an offline development tool. Game builds and launches must consume the
committed results, never call this tool or load its original archive input.
"""
import argparse
from collections import defaultdict
import hashlib
import io
import json
from pathlib import Path
import struct
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from rewrite.verify_archive import ORIGINAL_SHA256
from rewrite.verify_levels import assets, assemble_atlas, page_rgba
from rewrite.verify_meshes import LEVELS, level_expected
from assets.verify_assets import HEADER, TEXTURE, MATERIAL, PART, decode_model
from assets.prepare_scenes import prepare

ROOT = Path(__file__).resolve().parents[2]
UNITS_PER_METER = 160.0
TEMPLATES = dict(zip(range(5, 22), (
    'wheel-primary', 'wheel-secondary', 'prop-07', 'prop-08', 'prop-09',
    'prop-10', 'prop-11', 'prop-12', 'prop-13', 'prop-14', 'car-distant',
    'car-medium', 'car-close', 'flag', 'prop-19', 'sky', 'prop-21')))


def digest(data):
    return hashlib.sha256(data).hexdigest()


def signed(word):
    return word - 0x100000000 if word & 0x80000000 else word


class Stream:
    """Read the independently verified reference mesh-field stream offline."""
    def __init__(self, data):
        self.data = data
        self.cursor = 1

    def words(self, count):
        values = struct.unpack_from(f'<{count}I', self.data, self.cursor)
        self.cursor += count * 4
        return values

    def mesh(self):
        flags, vertices, normals, faces = self.words(4)
        vectors = self.words((vertices + normals) * 4)
        return dict(flags=flags, positions=np.array([
            [signed(value) / UNITS_PER_METER for value in vectors[index:index + 3]]
            for index in range(0, vertices * 4, 4)], dtype=np.float64),
            faces=[self.words(21) for _ in range(faces)])


def subdivide(triangles):
    """Two linear midpoint splits; preserve the source surface and UV seams."""
    result = np.asarray(triangles, dtype=np.float64)
    for _ in range(2):
        a, b, c = (result[:, index] for index in range(3))
        ab, bc, ca = (a + b) * .5, (b + c) * .5, (c + a) * .5
        result = np.stack((np.stack((a, ab, ca), axis=1),
                           np.stack((ab, b, bc), axis=1),
                           np.stack((ca, bc, c), axis=1),
                           np.stack((ab, bc, ca), axis=1)), axis=1).reshape(-1, 3, 8)
    return result


def srgb_to_linear(color):
    value = np.asarray(color, dtype=np.float64) / 255
    return np.where(value <= .04045, value / 12.92, ((value + .055) / 1.055) ** 2.4)


def fixed(name, size):
    data = name.encode('ascii')
    if not data or len(data) >= size:
        raise ValueError('Invalid export name: ' + name)
    return data.ljust(size, b'\0')


class Resources:
    def __init__(self, runtime):
        self.runtime = runtime
        self.files = {}
        self.models = {}
        self.normal = self.png(Image.new('RGB', (8, 8), (128, 128, 255)))
        self.roughness = self.png(Image.new('L', (8, 8), 255))

    def store(self, category, extension, data):
        name = f'reference/{category}/{digest(data)[:24]}.{extension}'
        path = self.runtime / name
        path.parent.mkdir(parents=True, exist_ok=True)
        if name in self.files and self.files[name]['sha256'] != digest(data):
            raise ValueError('Truncated content-hash collision')
        if name not in self.files:
            path.write_bytes(data)
            self.files[name] = dict(sha256=digest(data), bytes=len(data))
        return name

    def png(self, image):
        stream = io.BytesIO()
        image.save(stream, format='PNG', compress_level=9)
        return self.store('textures', 'png', stream.getvalue())


class Materials:
    def __init__(self, files, code, resources):
        self.resources = resources
        self.atlas, _, _ = assemble_atlas(files, code)
        self.palette = files[f'LEV{code}\\LEVEL.PAL']
        self.cluts = files[f'LEV{code}\\LEVEL.CLT'] + files.get(f'LEV{code}\\LEVEL.ECL', b'')
        level = files[f'LEV{code}\\LEVEL.DAT']
        start, end = struct.unpack_from('<II', level, 16)
        self.definitions = list(struct.iter_unpack('<HH8B', level[start + 4:end]))
        self.pages = {}
        self.gradients = {}

    def textured(self, face):
        page_flags, _, *uvs = self.definitions[face[12]]
        page, bank = page_flags & 31, face[13]
        indices = self.atlas[page * 65536:(page + 1) * 65536]
        lookup = self.cluts[bank * 4096 + 8 * 256:bank * 4096 + 9 * 256]
        # Preserve the existing reference renderer's first-zero-texel policy.
        us, vs = uvs[0::2], uvs[1::2]
        candidate = next((indices[v * 256 + u] for v in range(min(vs), max(vs) + 1)
                          for u in range(min(us), max(us) + 1)
                          if indices[v * 256 + u] & 15 == 0), None)
        cutout = candidate is not None and lookup[candidate] == 0
        key = page, bank, cutout
        if key not in self.pages:
            rgba = page_rgba(indices, self.palette, lookup, cutout)
            self.pages[key] = self.resources.png(Image.frombytes('RGBA', (256, 256), rgba))
        coordinates = [((u + .5) / 256, 1 - (v + .5) / 256) for u, v in zip(us, vs)]
        return (self.pages[key], (1., 1., 1., 1.)), coordinates

    def untextured(self, face):
        colors = [tuple((face[corner * 3 + 2] >> shift) & 255 for shift in (0, 8, 16))
                  for corner in range(face[17])]
        if len(set(colors)) == 1:
            return (None, (*srgb_to_linear(colors[0]), 1.)), [(0., 0.)] * 4
        if len(colors) == 3:
            colors.append(colors[1])
        key = tuple(colors)
        if key not in self.gradients:
            # Bake the two PSX triangle color gradients into a small owned map.
            side = 32
            y, x = np.mgrid[0:side, 0:side].astype(np.float64) / (side - 1)
            a, b, c, d = np.asarray(colors, dtype=np.float64)
            first = a + x[..., None] * (b - a) + y[..., None] * (c - a)
            second = d + (1 - x[..., None]) * (c - d) + (1 - y[..., None]) * (b - d)
            pixels = np.where((x + y <= 1)[..., None], first, second)
            self.gradients[key] = self.resources.png(Image.fromarray(np.rint(pixels).clip(0, 255).astype('uint8')))
        edge, high = .5 / 32, 1 - .5 / 32
        return (self.gradients[key], (1., 1., 1., 1.)), [(edge, high), (high, high), (edge, edge), (high, edge)]

    def face(self, face):
        return self.textured(face) if face[18] else self.untextured(face)


def convert_mesh(mesh, materials, resources):
    groups = defaultdict(list)
    removed = []
    source_triangles = 0
    for ordinal, face in enumerate(mesh['faces']):
        surface, uvs = materials.face(face)
        orders = ((0, 1, 2), (2, 1, 3)) if face[17] == 4 else ((0, 1, 2),)
        for order in orders:
            source_triangles += 1
            points = mesh['positions'][[face[corner * 3] for corner in order]]
            normal = np.cross(points[1] - points[0], points[2] - points[0])
            length = np.linalg.norm(normal)
            if length < 1e-10:
                removed.append(dict(face=ordinal, corners=list(order), sprite=face[15] >= 32))
                continue
            normal /= length
            groups[surface].append(np.concatenate((points, np.tile(normal, (3, 1)),
                                                   np.asarray([uvs[corner] for corner in order])), axis=1))
    if not groups:
        return None, dict(source_triangles=source_triangles, removed=removed, triangles=0)
    textures, material_rows, part_rows, indices, vertices = [], [], [], [], []
    vertex_lookup = {}
    for number, ((image, tint), triangles) in enumerate(groups.items()):
        texture = 0xffffffff
        if image is not None:
            texture = len(textures)
            textures.append(TEXTURE.pack(*(fixed(path, 64) for path in (image, resources.roughness, resources.normal))))
        material_rows.append(MATERIAL.pack(fixed(f'surface-{number:02}', 32), *tint, 0., 1., texture))
        first = len(indices)
        for vertex in subdivide(triangles).astype('<f4').reshape(-1, 8):
            data = vertex.tobytes()
            index = vertex_lookup.get(data)
            if index is None:
                index = len(vertices)
                vertex_lookup[data] = index
                vertices.append(data)
            indices.append(index)
        part_rows.append(PART.pack(fixed(f'surface-{number:02}', 64), number, first,
                                   len(indices) - first, 0, 0., 0., 0.))
    data = b''.join((HEADER.pack(b'DD2MESH2', len(textures), len(material_rows), len(part_rows), len(vertices), len(indices)),
                     *textures, *material_rows, *part_rows, *vertices, struct.pack(f'<{len(indices)}I', *indices)))
    decoded = decode_model(data)
    expected = (source_triangles - len(removed)) * 16
    if decoded['triangles'] != expected:
        raise ValueError('Offline subdivision count differs')
    name = resources.store('models', 'dd2mesh', data)
    resources.models[name] = decoded
    return dict(model=name, bounds=decoded['bounds']), dict(source_triangles=source_triangles,
                                                          removed=removed, triangles=expected)


def convert(archive, runtime):
    original = archive.read_bytes()
    if digest(original) != ORIGINAL_SHA256:
        raise ValueError('Expected the supported, unmodified provisioned Dirinfo')
    files = assets(original)
    resources = Resources(runtime)
    levels = []
    for code in LEVELS:
        binary, _ = level_expected(files, code)
        stream = Stream(binary)
        _, count = stream.words(2)
        materials = Materials(files, code, resources)
        placements, templates, omissions = [], {}, []
        triangles = source_triangles = removed_triangles = 0
        for index in range(count):
            coordinates = stream.words(6)
            origin = [signed(value) / UNITS_PER_METER for value in coordinates[3:]]
            item, evidence = convert_mesh(stream.mesh(), materials, resources)
            source_triangles += evidence['source_triangles']
            removed_triangles += len(evidence['removed'])
            triangles += evidence['triangles']
            if evidence['removed']:
                omissions.append(dict(object=index, **evidence))
            if item:
                placements.append(dict(id=f'object-{index:04}', position=origin, **item))
        template_counts = {}
        section, = stream.words(1)
        while section != 22:
            item, evidence = convert_mesh(stream.mesh(), materials, resources)
            name = TEMPLATES[section]
            template_counts[name] = evidence
            if item:
                templates[name] = item
            section, = stream.words(1)
        if stream.cursor != len(binary):
            raise ValueError('Unconsumed reference geometry')
        scene = dict(format='DD2SCENE1', level=code, units='meters', up='Y',
                     objects=placements, templates=templates)
        scene_path = runtime / f'reference/scenes/level-{code.lower()}.json'
        scene_path.parent.mkdir(parents=True, exist_ok=True)
        scene_path.write_text(json.dumps(scene, indent=2) + '\n')
        levels.append(dict(level=code, scene=str(scene_path.relative_to(runtime)),
                           source_objects=count, prepared_objects=len(placements),
                           source_triangles=source_triangles, removed_triangles=removed_triangles,
                           prepared_triangles=triangles, omissions=omissions, templates=template_counts))
        print(json.dumps(dict(level=code, objects=len(placements), triangles=triangles)), flush=True)
    manifest = dict(format='DD2REFERENCE1', intermediate=True, source_sha256=ORIGINAL_SHA256,
                    recipe='tools/assets/convert_reference.py', subdivision_steps=2,
                    triangles_per_retained_source_triangle=16, source_units_per_meter=UNITS_PER_METER,
                    limitations=['Intermediate reference-derived geometry and decoded colors, not final remodeled content.',
                                 'Linear subdivision preserves shape; it does not smooth or add authored detail.',
                                 'Zero-area source faces are inventoried; billboard animation remains pending.',
                                 'Normals are per source triangle; original lighting is not reconstructed.',
                                 'Driving metadata, livery variants, UI and audio migration remain pending.',
                                 'The default game still uses original inputs until provider integration.'],
                    files=resources.files, models=resources.models, levels=levels)
    prepare(runtime, manifest)
    (runtime / 'reference/manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, default=ROOT / 'DestructionDerby2/Dirinfo')
    parser.add_argument('--output', type=Path, default=ROOT / 'assets/runtime')
    args = parser.parse_args()
    manifest = convert(args.archive, args.output)
    print(json.dumps(dict(models=len(manifest['models']), resources=len(manifest['files']),
                          bytes=sum(item['bytes'] for item in manifest['files'].values()),
                          manifest=str(args.output / 'reference/manifest.json'))))


if __name__ == '__main__':
    main()
