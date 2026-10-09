#!/usr/bin/env python3
"""Independently validate authored mesh bounds, material maps and PCM exports."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import wave

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
HEADER = struct.Struct('<8s5I')
TEXTURE = struct.Struct('<64s64s64s')
MATERIAL = struct.Struct('<32s4f2fI')
PART = struct.Struct('<64s4I3f')


def string(value):
    if b'\0' not in value:
        raise ValueError('Unterminated asset string')
    content, padding = value.split(b'\0', 1)
    if not content or any(padding):
        raise ValueError('Empty string or nonzero padding')
    if any(value < 32 or value > 126 for value in content):
        raise ValueError('Non-printable asset string')
    try:
        return content.decode('ascii')
    except UnicodeDecodeError as error:
        raise ValueError('Non-ASCII asset string') from error


def resource_name(value):
    name = string(value)
    path = Path(name)
    components = name.split('/')
    if (path.is_absolute() or path.suffix != '.png' or
            any(value in ('', '.', '..') or re.fullmatch(r'[A-Za-z0-9_.-]+', value) is None
                for value in components)):
        raise ValueError('Unsafe texture resource name')
    return name


def decode_model(data):
    if len(data) < HEADER.size:
        raise ValueError('Truncated mesh header')
    magic, textures, materials, parts, vertices, indices = HEADER.unpack_from(data)
    if magic != b'DD2MESH2':
        raise ValueError('Unsupported mesh magic/version')
    if not (textures <= 32 and 1 <= materials <= 64 and 1 <= parts <= 4096
            and 3 <= vertices <= 1000000 and 3 <= indices <= 3000000 and indices % 3 == 0):
        raise ValueError('Invalid or excessive mesh counts')
    expected = HEADER.size + textures * TEXTURE.size + materials * MATERIAL.size + parts * PART.size + vertices * 32 + indices * 4
    if expected != len(data):
        raise ValueError('Mesh byte extent differs from declared counts')
    cursor = HEADER.size
    texture_rows = []
    for _ in range(textures):
        texture_rows.append([resource_name(value) for value in TEXTURE.unpack_from(data, cursor)])
        cursor += TEXTURE.size
    material_rows = []
    names = set()
    for _ in range(materials):
        row = MATERIAL.unpack_from(data, cursor)
        name = string(row[0])
        if name in names or any(not math.isfinite(value) or value < 0 or value > 1 for value in row[1:7]):
            raise ValueError('Duplicate material or invalid material scalar')
        if row[7] != 0xffffffff and row[7] >= textures:
            raise ValueError('Invalid material texture index')
        names.add(name)
        material_rows.append({'name': name, 'rgba': list(row[1:5]), 'metallic': row[5],
                              'roughness': row[6], 'texture': row[7]})
        cursor += MATERIAL.size
    part_rows = []
    names = set()
    previous_end = 0
    for _ in range(parts):
        row = PART.unpack_from(data, cursor)
        name = string(row[0])
        if name in names or row[1] >= materials or row[2] != previous_end or row[3] < 3 or row[3] % 3:
            raise ValueError('Invalid part name/material/index partition')
        if row[2] + row[3] > indices or row[4] > 6 or any(not math.isfinite(value) or abs(value) > 65536 for value in row[5:]):
            raise ValueError('Invalid part range/role/pivot')
        previous_end = row[2] + row[3]
        names.add(name)
        part_rows.append({'name': name, 'material': row[1], 'first': row[2],
                          'count': row[3], 'role': row[4], 'pivot': list(row[5:])})
        cursor += PART.size
    if previous_end != indices:
        raise ValueError('Unowned index suffix')
    vectors = np.frombuffer(data, dtype='<f4', count=vertices * 8, offset=cursor).reshape(vertices, 8)
    if not np.all(np.isfinite(vectors)) or np.max(np.abs(vectors)) > 65536:
        raise ValueError('Nonfinite or out-of-bound vertex/normal/UV')
    lengths = np.linalg.norm(vectors[:, 3:6].astype(np.float64), axis=1)
    if np.max(np.abs(lengths - 1)) > .0001:
        raise ValueError('Non-unit mesh normal')
    cursor += vertices * 32
    elements = np.frombuffer(data, dtype='<u4', count=indices, offset=cursor)
    if np.max(elements) >= vertices:
        raise ValueError('Index outside vertex array')
    triangles = vectors[elements.reshape(-1, 3), :3].astype(np.float64)
    faces = np.cross(triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0])
    areas = np.sum(faces * faces, axis=1)
    if np.any(areas < 1e-20):
        raise ValueError('Degenerate exported triangle')
    normals = np.mean(vectors[elements.reshape(-1, 3), 3:6], axis=1)
    if np.any(np.sum(faces * normals, axis=1) < -1e-10):
        raise ValueError('Triangle winding disagrees with exported normals')
    return {'textures': texture_rows, 'materials': material_rows, 'parts': part_rows,
            'vertices': vertices, 'triangles': indices // 3, 'bytes': len(data),
            'bounds': [vectors[:, :3].min(axis=0).tolist(), vectors[:, :3].max(axis=0).tolist()]}


def rejected_corruption(data):
    _, textures, materials, parts, vertices, indices = HEADER.unpack_from(data)
    material_offset = HEADER.size + textures * TEXTURE.size
    part_offset = material_offset + materials * MATERIAL.size
    vertex_offset = part_offset + parts * PART.size
    index_offset = vertex_offset + vertices * 32
    cases = {'empty': b'', 'truncated-header': data[:20], 'truncated-tail': data[:-1], 'trailing-byte': data + b'\0'}

    def changed(name, offset, format_, value):
        altered = bytearray(data)
        struct.pack_into(format_, altered, offset, value)
        cases[name] = bytes(altered)

    changed('bad-version', 7, 'B', ord('9'))
    changed('excessive-count', 20, 'I', 1000001)
    changed('invalid-index', index_offset, 'I', vertices)
    changed('invalid-material-reference', part_offset + 64, 'I', materials)
    changed('invalid-texture-reference', material_offset + 56, 'I', textures)
    changed('nonfinite-material', material_offset + 32, 'f', math.nan)
    changed('nonfinite-position', vertex_offset, 'f', math.nan)
    changed('non-unit-normal', vertex_offset + 12, 'f', 10.0)
    changed('invalid-part-role', part_offset + 76, 'I', 7)
    changed('invalid-part-partition', part_offset + 68, 'I', 3)
    changed('nonfinite-pivot', part_offset + 80, 'f', math.inf)
    altered = bytearray(data)
    altered[HEADER.size:HEADER.size + 64] = b'../outside.png\0'.ljust(64, b'\0')
    cases['unsafe-resource'] = bytes(altered)
    for label, damaged in cases.items():
        try:
            decode_model(damaged)
        except ValueError:
            continue
        raise ValueError('Corrupt mesh was accepted: ' + label)
    return list(cases)


def verify(output):
    runtime = output / 'runtime'
    texture_catalog = json.loads((runtime / 'textures/materials.json').read_text())
    textures = []
    known_maps = set()
    for item in texture_catalog['textures']:
        for kind, source in item['maps'].items():
            path = runtime / source['path']
            if not path.is_file() or path.is_symlink() or runtime.resolve() not in path.resolve().parents:
                raise ValueError('Texture resource escapes authored runtime content')
            content = path.read_bytes()
            if hashlib.sha256(content).hexdigest() != source['sha256']:
                raise ValueError('Texture digest differs')
            with Image.open(path) as image:
                if image.size != tuple(item['size']) or image.mode != ('L' if kind == 'roughness' else 'RGB'):
                    raise ValueError('Texture dimensions/mode differ')
                values = np.asarray(image).astype(np.float64)
            variation = np.max(np.std(values, axis=(0, 1)))
            if variation < .1:
                raise ValueError('Procedural material map has no variation')
            if kind == 'normal':
                length = np.linalg.norm(values / 127.5 - 1.0, axis=-1)
                if np.max(np.abs(length - 1)) > .008 or np.min(values[..., 2]) < 128:
                    raise ValueError('Invalid tangent-space normal map')
            # Boundary discontinuity must be comparable to ordinary adjacent
            # samples, including the deliberately granular material recipes.
            dx = np.mean(np.abs(np.diff(values, axis=1)))
            dy = np.mean(np.abs(np.diff(values, axis=0)))
            seam = [float(np.mean(np.abs(values[:, 0] - values[:, -1]))),
                    float(np.mean(np.abs(values[0] - values[-1])))]
            if seam[0] > max(dx * 3.0, 1.0) or seam[1] > max(dy * 3.0, 1.0):
                raise ValueError('Procedural texture has an anomalous tile seam')
            known_maps.add(source['path'])
            textures.append({'path': source['path'], 'bytes': len(content), 'sha256': source['sha256'],
                             'boundary_mean_delta': seam})
    models = []
    for path in sorted((runtime / 'models').glob('*.dd2mesh')):
        data = path.read_bytes()
        parsed = decode_model(data)
        for row in parsed['textures']:
            if any(name not in known_maps for name in row):
                raise ValueError('Mesh material refers to a missing authored map')
        metadata = json.loads(path.with_suffix('.json').read_text())
        if any(parsed[key] != metadata[key] for key in ('vertices', 'triangles', 'bytes')):
            raise ValueError('Mesh metadata differs from decoded container')
        if len(parsed['parts']) != metadata['part_count'] or len(parsed['materials']) != metadata['materials']:
            raise ValueError('Mesh metadata count differs')
        for role in range(1, 5):
            wheel = [part for part in parsed['parts'] if part['role'] == role]
            if not wheel or any(part['pivot'] != wheel[0]['pivot'] for part in wheel):
                raise ValueError('Wheel geometry/pivot ownership is incomplete')
        if metadata['lod'] == 'full':
            names = {part['name'] for part in parsed['parts']}
            for component in ('Cockpit.SeatBase', 'Cockpit.Dash', 'Cockpit.Tachometer', 'Cockpit.Floor', 'Steering.Wheel'):
                if component not in names:
                    raise ValueError('Missing full cockpit component')
        models.append({'path': str(path.relative_to(output)), 'sha256': hashlib.sha256(data).hexdigest(),
                       **{key: parsed[key] for key in ('vertices', 'triangles', 'bytes', 'bounds')},
                       'parts': len(parsed['parts']), 'materials': len(parsed['materials']),
                       'rejected_corruptions': rejected_corruption(data)})
    if len(models) != 3:
        raise ValueError('Expected full, exterior and NPC mesh exports')
    audio_catalog = json.loads((runtime / 'audio/audio.json').read_text())
    audio = []
    for item in audio_catalog['clips']:
        path = runtime / item['path']
        if hashlib.sha256(path.read_bytes()).hexdigest() != item['sha256']:
            raise ValueError('Audio digest differs')
        with wave.open(str(path), 'rb') as stream:
            if (stream.getnchannels(), stream.getsampwidth(), stream.getframerate(), stream.getnframes()) != (2, 2, 48000, item['frames']):
                raise ValueError('Unexpected PCM format/extent')
            samples = np.frombuffer(stream.readframes(stream.getnframes()), dtype='<i2').reshape(-1, 2).astype(np.float64) / 32767
        peak = float(np.max(np.abs(samples)))
        rms = float(np.sqrt(np.mean(samples * samples)))
        if peak > item['peak_limit'] + 1 / 32767 or peak >= .99 or rms < .001:
            raise ValueError('Clipped, silent or incorrectly normalized PCM')
        if np.max(np.abs(np.mean(samples, axis=0))) > .02:
            raise ValueError('Excessive audio DC offset')
        differences = np.abs(np.diff(samples, axis=0))
        seam = float(np.max(np.abs(samples[-1] - samples[0])))
        threshold = max(float(np.percentile(differences, 99)) * 4, .003)
        if item['loop'] and seam > threshold:
            raise ValueError('Loop seam has an anomalous PCM step')
        if not item['loop'] and max(np.max(np.abs(samples[0])), np.max(np.abs(samples[-1]))) > .0001:
            raise ValueError('One-shot audio boundary is not silent')
        spectrum = np.abs(np.fft.rfft(samples[:, 0])) ** 2
        frequencies = np.fft.rfftfreq(len(samples), 1 / 48000)
        audio.append({'path': item['path'], 'sha256': item['sha256'], 'frames': len(samples),
                      'peak': peak, 'rms': rms, 'loop': item['loop'], 'loop_boundary_step': seam,
                      'spectral_centroid_hz': float(np.sum(frequencies * spectrum) / np.sum(spectrum))})
    return {'pass_': True, 'verified_at': datetime.now(timezone.utc).isoformat(),
            'scope': 'Authored offline exports, bounds/corruptions, numeric material/PCM checks; not runtime visual/audio acceptance',
            'models': models, 'material_maps': textures, 'audio': audio, 'full_goal_acceptance': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assets', type=Path, default=ROOT / 'assets')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if args.report and Path('/tmp/wasm-dd2') not in args.report.resolve().parents:
        parser.error('Verification reports belong under /tmp/wasm-dd2/')
    report = verify(args.assets.resolve())
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'pass_': True, 'models': len(report['models']), 'maps': len(report['material_maps']),
                      'audio': len(report['audio']), 'full_goal_acceptance': False}))


if __name__ == '__main__':
    main()
