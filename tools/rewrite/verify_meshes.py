#!/usr/bin/env python3
"""Independently compare decoded original scene objects and polygon meshes."""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256
from rewrite.verify_levels import assets

LEVELS = '123456789AB'


def decompress(source):
    size, = struct.unpack_from('<I', source)
    assert size <= 16384
    result = bytearray()
    cursor = 4
    while len(result) < size:
        flags = source[cursor]
        cursor += 1
        for bit in range(8):
            if len(result) == size:
                break
            if flags & (1 << bit):
                result.append(source[cursor])
                cursor += 1
            else:
                token, = struct.unpack_from('<H', source, cursor)
                cursor += 2
                distance = 4096 - ((token & 255) | ((token >> 4) & 3840))
                length = ((token >> 8) & 15) + 3
                assert distance <= len(result) and len(result) + length <= size
                for _ in range(length):
                    result.append(result[-distance])
    return bytes(result)


def mesh_words(data, limits, counts):
    vertex_count, normal_count = struct.unpack_from('<HH', data, 10)
    vertices, normals, cursor = struct.unpack_from('<III', data, 32)
    vectors = []
    for start, count in ((vertices, vertex_count), (normals, normal_count)):
        assert 44 <= start <= start + count * 8 <= len(data)
        for offset in range(start, start + count * 8, 8):
            vectors.extend(struct.unpack_from('<hhhH', data, offset))
    faces = []
    face_count = 0
    while True:
        count, opcode, flags = struct.unpack_from('<HBB', data, cursor)
        cursor += 4
        if flags == 0:
            break
        assert opcode < 44
        family, within = divmod(opcode, 8)
        corners = 4 if within >= 4 or family >= 4 else 3
        textured = family in (1, 3, 4, 5)
        gouraud = family in (2, 3)
        lit = bool(opcode < 32 and opcode % 2)
        color_count = corners if gouraud and not lit else 1
        vertex_offset = 4 + color_count * 4 + (4 if textured else 0)
        size = (vertex_offset + corners * 2 + (corners * 2 if lit and gouraud else 0) + 3) & ~3
        if family == 0:
            size = 16
        for _ in range(count):
            record = data[cursor:cursor + size]
            assert len(record) == size
            indices = struct.unpack_from(f'<{corners}H', record, vertex_offset)
            assert max(indices) < vertex_count
            colors = struct.unpack_from(f'<{color_count}I', record, 4)
            colors = colors * corners if color_count == 1 else colors
            if lit:
                normal_offset = vertex_offset + corners * 2 if gouraud else 2
                references = struct.unpack_from(f'<{corners if gouraud else 1}H', record, normal_offset)
                references = references if gouraud else references * corners
                assert max(references) < normal_count
            else:
                references = (0,) * corners
            for corner in range(4):
                faces.extend((indices[corner], references[corner], colors[corner]) if corner < corners else (0, 0, 0))
            texture, bank = struct.unpack_from('<HH', record, vertex_offset - 4) if textured else (0, 0)
            assert not textured or (texture < limits[0] and bank < limits[1])
            attribute, = struct.unpack_from('<H', record)
            faces.extend((texture, bank, attribute, opcode, flags, corners, textured, lit, gouraud))
            counts['opcodes'][opcode] += 1
            face_count += 1
            cursor += size
    counts['meshes'] += 1
    counts['faces'] += face_count
    counts['vertices'] += vertex_count
    counts['normals'] += normal_count
    return [data[4], vertex_count, normal_count, face_count, *vectors, *faces]


def level_expected(files, code):
    data = files[f'LEV{code}\\LEVEL.DAT']
    offsets = struct.unpack_from('<29I', data)
    sections = [data[start:end] for start, end in zip(offsets, (*offsets[1:], len(data)))]
    texture_count, = struct.unpack_from('<I', sections[4])
    banks = len(files[f'LEV{code}\\LEVEL.CLT'] + files.get(f'LEV{code}\\LEVEL.ECL', b'')) // 4096
    limits = texture_count, banks
    scene = sections[0]
    first, = struct.unpack_from('<I', scene)
    assert first % 4 == 0
    block_count = first // 4
    blocks = struct.unpack_from(f'<{block_count}I', scene)
    objects = []
    world_vertices = set()
    counts = dict(meshes=0, faces=0, vertices=0, normals=0, opcodes=Counter())
    for start, end in zip(blocks, (*blocks[1:], len(scene))):
        block = decompress(scene[start:end]) if code in '1234567' else scene[start:end]
        count, = struct.unpack_from('<I', block)
        rows = [struct.unpack_from('<Iiii', block, 4 + index * 16) for index in range(count)]
        for mesh, x, y, z in rows:
            end = min((row[0] for row in rows if row[0] > mesh), default=len(block))
            assert mesh >= 4 + count * 16 and mesh < end
            origin = (x, y, z) if block[mesh + 4] & 128 else tuple(
                (coordinate // 32768) * 32768 + 16384 for coordinate in (x, y, z))
            vertices = struct.unpack_from('<I', block, mesh + 32)[0]
            vertex_count = struct.unpack_from('<H', block, mesh + 10)[0]
            for vector in range(vertex_count):
                local = struct.unpack_from('<3h', block, mesh + vertices + vector * 8)
                world_vertices.add(tuple(base + axis for base, axis in zip(origin, local)))
            objects.append([x, y, z, *origin, *mesh_words(block[mesh:end], limits, counts)])
    result = bytearray(code.encode('ascii'))
    def words(values):
        result.extend(struct.pack(f'<{len(values)}I', *(value & 0xffffffff for value in values)))
    words([block_count, len(objects)])
    for obj in objects:
        words(obj)
    for section in range(5, 22):
        if sections[section]:
            words([section, *mesh_words(sections[section], limits, counts)])
    words([22])
    road_vertices = set(struct.iter_unpack('<3i', sections[2]))
    if code in '1234567' and not road_vertices <= world_vertices:
        raise ValueError('Racing road vertices do not all align with scene vertex origins')
    return bytes(result), dict(level=code, blocks=block_count, objects=len(objects),
                               scene_world_vertices=len(world_vertices),
                               unique_road_vertices=len(road_vertices),
                               road_vertices_in_scene=len(road_vertices & world_vertices), **counts)


def original_mutations(original, files):
    locations = {}
    for name, sector, size in struct.iter_unpack('<18sHI', original[:0x2808]):
        if name[0] == 0:
            break
        locations[name.split(b'\0', 1)[0].decode('ascii')] = sector * 2048
    # Arena blocks are uncompressed; mutate a live object and its first polygon.
    level = files['LEV8\\LEVEL.DAT']
    section, = struct.unpack_from('<I', level)
    start = locations['LEV8\\LEVEL.DAT'] + section
    scene = level[section:struct.unpack_from('<I', level, 4)[0]]
    block, = struct.unpack_from('<I', scene)
    mesh, = struct.unpack_from('<I', scene, block + 4)
    shape = start + block + mesh
    polygons, = struct.unpack_from('<I', original, shape + 40)
    return (
        ('scene-table-overflow', start, struct.pack('<I', 0xffffffff)),
        ('scene-instance-overflow', start + block + 4, struct.pack('<I', 0xffffffff)),
        ('mesh-vertices-overflow', shape + 32, struct.pack('<I', 0xffffffff)),
        ('mesh-polygons-overflow', shape + 40, struct.pack('<I', 0xffffffff)),
        ('mesh-opcode-unknown', shape + polygons + 2, b'\xff'),
        ('mesh-face-count-overflow', shape + polygons, b'\xff\xff'),
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path, default=ROOT / 'DestructionDerby2')
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-mesh-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (args.game_dir / 'Dirinfo').resolve()
    original = archive.read_bytes()
    if hashlib.sha256(original).hexdigest() != ORIGINAL_SHA256:
        raise ValueError('Provision the supported unmodified original Dirinfo')
    files = assets(original)
    expected = bytearray()
    levels = []
    for code in LEVELS:
        binary, counts = level_expected(files, code)
        expected.extend(binary)
        levels.append(counts)
    calls = []
    def run(command, label, success=True):
        log = output / (label + '.log')
        with log.open('w') as stream:
            status = run_bounded(command, directory=output, timeout=60, stdout=stream,
                                 stderr=subprocess.STDOUT, cwd=ROOT)
        content = log.read_text()
        calls.append(dict(label=label, command=list(map(str, command)), returncode=status.returncode,
                          log_sha256=hashlib.sha256(log.read_bytes()).hexdigest()))
        if status.returncode != (0 if success else 1) or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label + ' failed: ' + content)
        if not success and content:
            raise RuntimeError(label + ' unexpected rejection diagnostics')
    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT / 'src'), '-Wall', '-Wextra', '-Wpedantic',
             '-Wno-unused-parameter', '-Wno-unused-function', '-fno-strict-aliasing', '-ffast-math',
             '-Werror', '-Wshadow', '-Wconversion', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wformat=2', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    units = [str(ROOT / f'src/assets/{name}.c') for name in ('archive', 'level', 'textures', 'lz', 'mesh', 'scene')]
    sanitized_test = output / 'dd2_mesh_sanitized'
    sanitized_export = output / 'dd2_mesh_export_sanitized'
    for name, binary in (('mesh_test', sanitized_test), ('mesh_export', sanitized_export)):
        run([tool('clang'), *flags, *units, str(ROOT / f'tests/rewrite/{name}.c'), '-o', str(binary)], name + '-sanitizer-build')
    run([str(sanitized_test)], 'sanitized-bounds')
    commands = {
        'native': [str(WORK / 'rewrite-native/dd2_mesh_export')],
        'wasm': ['node', str(WORK / 'rewrite-wasm/dd2_mesh_export.js')],
        'sanitized': [str(sanitized_export)],
    }
    comparisons = {}
    digest = hashlib.sha256(expected).hexdigest()
    for platform, command in commands.items():
        raw = output / (platform + '.bin')
        run([*command, str(archive), str(raw)], platform + '-original')
        actual = raw.read_bytes()
        if actual != expected:
            mismatch = next((i for i, (a, b) in enumerate(zip(actual, expected)) if a != b), min(len(actual), len(expected)))
            raise ValueError(f'{platform}: mesh field differs at byte {mismatch}; sizes {len(actual)}/{len(expected)}')
        comparisons[platform] = dict(bytes=len(actual), actual_sha256=hashlib.sha256(actual).hexdigest(), expected_sha256=digest)
        raw.unlink()
    mutations = original_mutations(original, files)
    mutated = output / 'mutated-archive.bin'
    raw = output / 'rejected-export.bin'
    for label, offset, replacement in mutations:
        changed = bytearray(original)
        changed[offset:offset + len(replacement)] = replacement
        mutated.write_bytes(changed)
        for platform, command in commands.items():
            run([*command, str(mutated), str(raw)], platform + '-' + label, success=False)
            raw.unlink(missing_ok=True)
    sources = ['src/assets/bytes.h', 'src/assets/lz.c', 'src/assets/lz.h', 'src/assets/mesh.c',
               'src/assets/mesh.h', 'src/assets/scene.c', 'src/assets/scene.h', 'tests/rewrite/mesh_test.c',
               'tests/rewrite/mesh_export.c', 'tests/rewrite/archive_fixture.h', 'tools/rewrite/verify_meshes.py',
               'CMakeLists.txt', 'Makefile']
    report = dict(pass_=True, scope='decoded scene placements, mesh vectors/normals and polygon fields; rendering/gameplay pending',
                  verified_at=datetime.now(timezone.utc).isoformat(), original_sha256=ORIGINAL_SHA256,
                  comparisons=comparisons, levels=levels, rejection_variants=len(mutations), calls=calls,
                  source_sha256={p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in sources})
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    mutated.unlink()
    sanitized_test.unlink()
    sanitized_export.unlink()
    for call in calls:
        (output / (call['label'] + '.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, levels=len(LEVELS), meshes=sum(x['meshes'] for x in levels),
                          faces=sum(x['faces'] for x in levels), platforms=list(commands), report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
