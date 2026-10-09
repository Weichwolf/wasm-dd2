#!/usr/bin/env python3
"""Generate tileable material maps entirely from deterministic numeric recipes."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
NAMES = ('asphalt', 'paint-flake', 'carbon-twill', 'seat-fabric', 'tire-rubber', 'brushed-alloy')


def hash_grid(x, y, seed):
    """Coordinate hash with explicit uint32 wraparound; no random global state."""
    with np.errstate(over='ignore'):
        value = np.asarray(x, dtype=np.uint32) * np.uint32(0x45D9F3B)
        value = value ^ (np.asarray(y, dtype=np.uint32) * np.uint32(0x119DE1F3)) ^ np.uint32(seed)
        value = (value ^ (value >> 16)) * np.uint32(0x45D9F3B)
        value = (value ^ (value >> 16)) * np.uint32(0x45D9F3B)
        value = value ^ (value >> 16)
    return (value & np.uint32(0xFFFFFF)).astype(np.float64) / 16777215.0


def noise(size, cells, seed):
    # Periodic control points and smooth interpolation make the sampled signal
    # tileable; the final pixel is one sample before the periodic boundary.
    coord = np.arange(size, dtype=np.float64) * cells / size
    integer = np.floor(coord).astype(np.int32)
    fraction = coord - integer
    weight = fraction * fraction * (3.0 - 2.0 * fraction)
    ix, iy = integer[None, :], integer[:, None]
    wx, wy = weight[None, :], weight[:, None]
    low = hash_grid(ix % cells, iy % cells, seed) * (1.0 - wx)
    low += hash_grid((ix + 1) % cells, iy % cells, seed) * wx
    high = hash_grid(ix % cells, (iy + 1) % cells, seed) * (1.0 - wx)
    high += hash_grid((ix + 1) % cells, (iy + 1) % cells, seed) * wx
    return low * (1.0 - wy) + high * wy


def recipes(size):
    y, x = np.indices((size, size), dtype=np.int32)
    fine = hash_grid(x, y, 6217)
    macro = noise(size, 8, 2051)
    medium = noise(size, 64, 3923)
    grains = noise(size, 256, 2027)
    # Albedo maps are factors multiplied by each material's authored base tint.
    aggregate = 0.14 + 0.08 * macro + 0.05 * medium + 0.06 * fine
    cracks = np.exp(-np.square((medium - 0.48) * 125.0)) * (macro > 0.63)
    aggregate -= cracks * 0.07
    yield 'asphalt', aggregate, 0.81 + fine * 0.15, grains * 0.5 - cracks * 0.2, 9.0
    flakes = np.where(fine > 0.978, (fine - 0.978) * 4.0, 0.0)
    yield 'paint-flake', 0.86 + 0.055 * medium + flakes, 0.28 + fine * 0.09, flakes * 0.16, 3.0
    # 2/2 diagonal twill: raised warp/weft alternates every two bundles.
    bundle = max(4, size // 128)
    warp = ((x // bundle + y // bundle) % 4) < 2
    fiber_x = np.square(np.sin(np.pi * (x % bundle) / bundle))
    fiber_y = np.square(np.sin(np.pi * (y % bundle) / bundle))
    weave = np.where(warp, fiber_x, fiber_y)
    twill = 0.35 + weave * 0.34 + medium * 0.035 + fine * 0.025
    yield 'carbon-twill', twill, 0.37 + weave * 0.10, weave * 0.02, 2.0
    thread = max(2, size // 256)
    fabric = (np.sin(np.pi * x / thread) ** 2 + np.sin(np.pi * y / thread) ** 2) * 0.5
    yield 'seat-fabric', 0.68 + fabric * 0.14 + fine * 0.07, 0.83 + fine * 0.14, fabric * 0.02, 3.0
    grooves = np.square(np.sin(np.pi * y * 32 / size))
    yield 'tire-rubber', 0.70 + medium * 0.10 + fine * 0.025, 0.76 + fine * 0.16, grooves * 0.05 + grains * 0.04, 5.0
    brushed = hash_grid(np.zeros_like(x), y, 1087)
    yield 'brushed-alloy', 0.73 + brushed * 0.18 + fine * 0.035, 0.30 + brushed * 0.12, brushed * 0.006, 2.0


def normal_map(height, strength):
    dx = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) * strength
    dy = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) * strength
    # PNG rows increase downward while the model's standard UV V increases up.
    normals = np.stack((-dx, dy, np.ones_like(dx)), axis=-1)
    normals /= np.sqrt(np.sum(normals * normals, axis=-1))[..., None]
    return np.rint((normals + 1.0) * 127.5).astype(np.uint8)


def generate(output, size=1024):
    if size < 512 or size > 2048 or size & (size - 1):
        raise ValueError('Texture size must be a power of two from 512 to 2048')
    folder = output / 'runtime/textures'
    folder.mkdir(parents=True, exist_ok=True)
    entries = []
    for name, color, roughness, height, strength in recipes(size):
        maps = {
            'albedo': np.repeat(np.rint(np.clip(color, 0, 1)[..., None] * 255).astype(np.uint8), 3, axis=2),
            'roughness': np.rint(np.clip(roughness, 0, 1) * 255).astype(np.uint8),
            'normal': normal_map(height, strength),
        }
        files = {}
        for kind, values in maps.items():
            path = folder / f'{name}-{kind}.png'
            Image.fromarray(values).save(path, optimize=False, compress_level=9)
            files[kind] = {'path': str(path.relative_to(output / 'runtime')),
                           'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                           'bytes': path.stat().st_size,
                           'color_space': 'sRGB' if kind == 'albedo' else 'linear'}
        entries.append({'name': name, 'size': [size, size], 'tileable': True,
                        'albedo_usage': 'Multiply the authored base tint', 'maps': files})
    metadata = {'schema': 1, 'generator': 'tools/assets/generate_textures.py',
                'authorship': 'Entirely procedural coordinate hashes and authored material recipes; no input images',
                'pixel_origin': 'top-left; tangent normals use bottom-left UV with +Y increasing V',
                'textures': entries}
    (folder / 'materials.json').write_text(json.dumps(metadata, indent=2) + '\n')
    return metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'assets')
    parser.add_argument('--size', type=int, default=1024)
    args = parser.parse_args()
    metadata = generate(args.output.resolve(), args.size)
    print(json.dumps({'generated_materials': len(metadata['textures']), 'maps_per_material': 3,
                      'size': args.size, 'original_inputs': False}))


if __name__ == '__main__':
    main()
