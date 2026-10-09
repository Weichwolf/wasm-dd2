#!/usr/bin/env python3
"""Generate owned continuous sky/land panoramas from a seed and authored palette."""
import argparse
import json
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def smooth(value):
    clipped = np.clip(value, 0, 1)
    return clipped * clipped * (3 - 2 * clipped)


def directional_noise(x, y, z, seed):
    rng = np.random.default_rng(seed)
    field = np.zeros(np.broadcast_shapes(x.shape, y.shape, z.shape), dtype=np.float32)
    total = 0
    for octave in range(5):
        amplitude = 0.52 ** octave
        for _ in range(6):
            direction = rng.normal(size=3)
            direction /= np.linalg.norm(direction)
            frequency = 4.5 * 2 ** octave
            phase = rng.uniform(0, 2 * np.pi)
            field += amplitude * np.sin(frequency * (direction[0] * x + direction[1] * y + direction[2] * z) + phase) / 6
        total += amplitude
    return np.clip(0.5 + field / total, 0, 1)


def landscape(profile, longitude, elevation, pixels, noise):
    rng = np.random.default_rng(profile['seed'] + 17)
    width = pixels.shape[1]
    angle = longitude.reshape(width)
    silhouette = np.full(width, -0.025, dtype=np.float32)
    for frequency in range(1, 10):
        silhouette += (0.09 / frequency ** 1.3) * np.sin(frequency * angle + rng.uniform(0, 2 * np.pi))
    if profile['scenery'] == 'mountains':
        silhouette = 0.01 + 1.4 * np.abs(silhouette)
    elif profile['scenery'] == 'city':
        blocks = np.floor(np.arange(width) * 80 / (width - 1)).astype(int) % 80
        heights = rng.uniform(.025, .22, 80)
        silhouette = heights[blocks]
    mask = smooth((silhouette[None, :] - elevation) * 250)
    if profile['scenery'] == 'forest':
        trees = np.arange(width) * 120 / (width - 1)
        slots = np.floor(trees).astype(int) % 120
        heights = rng.uniform(.035, .15, 120)[slots]
        relative = (elevation - silhouette[None, :]) / heights[None, :]
        branch_width = (1 - relative) * (.72 + .18 * np.cos(relative * 24))
        crown = smooth((branch_width - np.abs(trees % 1 - .5)[None, :]) * 45)
        crown *= smooth((1 - relative) * 50) * smooth(relative * 70)
        mask = np.maximum(mask, crown)
    land = np.asarray(profile['land'], dtype=np.float32)
    relief = 0.7 + 0.4 * noise
    fog = np.exp(-np.maximum(silhouette[None, :] - elevation, 0) * 18) * .22
    color = land * relief[..., None] + np.asarray(profile['horizon']) * fog[..., None]
    if profile['scenery'] == 'city':
        rows = np.floor(elevation * 850).astype(int)
        columns = np.floor(longitude * 260).astype(int)
        windows = ((rows % 6 == 0) & (columns % 5 == 0) & (elevation > .005))
        window_mix = windows[..., None] * .85
        color = color * (1 - window_mix) + np.array([235, 187, 104]) * window_mix
    pixels[:] = pixels * (1 - mask[..., None]) + color * mask[..., None]


def panorama(recipe, profile):
    width, height, guard = (recipe[k] for k in ('width', 'height', 'vertical_guard'))
    longitude = np.linspace(0, 2 * np.pi, width, dtype=np.float32)[None, :]
    elevation = np.linspace(np.pi / 2, -np.pi / 2, height - 2 * guard, dtype=np.float32)[:, None]
    x, y, z = np.cos(elevation) * np.cos(longitude), np.sin(elevation), np.cos(elevation) * np.sin(longitude)
    field = directional_noise(x, y, z, profile['seed'])
    blend = smooth(np.maximum(y, 0) ** .65)
    base = np.asarray(profile['horizon']) * (1 - blend[..., None]) + np.asarray(profile['zenith']) * blend[..., None]
    pixels = np.broadcast_to(base, (height - 2 * guard, width, 3)).copy()
    opacity = smooth((field - profile['coverage']) * 4.5)
    shaded = np.asarray(profile['cloud']) * (0.70 + .30 * smooth(field))[..., None]
    pixels = pixels * (1 - opacity[..., None]) + shaded * opacity[..., None]
    sun_angle = profile['sun_yaw']
    sun_y = profile['sun_height']
    sun_xz = np.sqrt(1 - sun_y * sun_y)
    facing = np.clip(x * sun_xz * np.cos(sun_angle) + y * sun_y + z * sun_xz * np.sin(sun_angle), -1, 1)
    halo = np.exp((facing - 1) * 32) * .18 * (1 - opacity)
    pixels += halo[..., None] * np.asarray(profile['cloud'])
    if profile['scenery'] == 'city':
        moon = smooth((facing - .9995) * 4000) * (1 - opacity)
        pixels = pixels * (1 - moon[..., None]) + np.array([221, 224, 226]) * moon[..., None]
        rng = np.random.default_rng(profile['seed'] + 99)
        for _ in range(350):
            row = int(rng.uniform(.08, .43) * pixels.shape[0])
            column = int(rng.integers(0, width))
            brightness = rng.uniform(80, 190) * (1 - opacity[row, column])
            pixels[row, column] = np.maximum(pixels[row, column], brightness)
    landscape(profile, longitude, elevation, pixels, field)
    pixels = np.rint(np.clip(pixels, 0, 255)).astype(np.uint8)
    # Longitude repeats; each pole collapses to one direction. Guard rows keep
    # filtered latitude endpoints away from vertical wrap in the shared sampler.
    pixels[:, -1] = pixels[:, 0]
    pixels[0] = pixels[0, 0]
    pixels[-1] = pixels[-1, 0]
    padded = np.pad(pixels, ((guard, guard), (0, 0), (0, 0)), mode='edge')
    alpha = np.full((height, width, 1), 255, dtype=np.uint8)
    return Image.fromarray(np.concatenate((padded, alpha), axis=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--recipe', type=Path, default=ROOT/'assets/recipes/skies.json')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    recipe = json.loads(args.recipe.read_text())
    if recipe['format'] != 'DD2SKYRECIPE1':
        raise ValueError('Unsupported sky recipe')
    output = args.output/'runtime/skies'
    output.mkdir(parents=True, exist_ok=True)
    for profile in recipe['profiles']:
        path = output/f"level-{profile['level'].lower()}.png"
        panorama(recipe, profile).save(path, compress_level=9)
        print(json.dumps(dict(level=profile['level'], texture=str(path), original_inputs=False)), flush=True)
    Image.new('RGBA', (1, 1), (255, 255, 255, 255)).save(output/'roughness.png')
    Image.new('RGBA', (1, 1), (128, 128, 255, 255)).save(output/'normal.png')


if __name__ == '__main__':
    main()
