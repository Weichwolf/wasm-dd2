#!/usr/bin/env python3
"""Rebuild eleven authored sky panoramas and Blender domes without game files."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from assets.verify_assets import decode_model

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'assets')
    parser.add_argument('--recipe', type=Path, default=ROOT/'assets/recipes/skies.json')
    parser.add_argument('--blender', default='blender')
    args = parser.parse_args()
    output = args.output.resolve()
    recipe = json.loads(args.recipe.read_text())
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
    subprocess.run([sys.executable, ROOT/'tools/assets/generate_skies.py', '--recipe', args.recipe,
                    '--output', output], env=environment, check=True)
    blender = shutil.which(args.blender)
    if blender is None:
        raise RuntimeError('Blender is required for authored sky geometry')
    subprocess.run([blender, '--background', '--factory-startup', '-t', '4', '--python-exit-code', '1',
                    '--python', ROOT/'tools/assets/build_sky_meshes.py', '--', '--recipe', args.recipe,
                    '--output', output], env=environment, check=True)
    files, models, profiles = {}, {}, []
    for path in sorted((output/'runtime/skies').iterdir()):
        if path.suffix not in ('.png', '.dd2mesh'):
            continue
        data = path.read_bytes()
        name = str(path.relative_to(output/'runtime'))
        files[name] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
        if path.suffix == '.dd2mesh':
            models[name] = decode_model(data)
    for profile in recipe['profiles']:
        bindings = {}
        for band in ('lower', 'upper'):
            for quadrant in range(4):
                name = f"skies/level-{profile['level'].lower()}-{band}-{quadrant}.dd2mesh"
                bindings[f'sky-{band}-{quadrant}'] = dict(model=name, bounds=models[name]['bounds'])
        profiles.append(dict(level=profile['level'], theme=profile['theme'], templates=bindings))
    manifest = dict(format='DD2SKIES1', authored=True, original_inputs_required=False,
                    recipe='assets/recipes/skies.json', editable_blender='assets/blender/skies.blend',
                    generator='tools/assets/build_skies.py', latitude_cells=recipe['latitude_cells'],
                    longitude_cells=recipe['longitude_cells'], triangles_per_dome=sum(v['triangles'] for k,v in models.items() if k.startswith('skies/level-1-')),
                    width=recipe['width'], height=recipe['height'], vertical_guard=recipe['vertical_guard'],
                    files=files, models=models, profiles=profiles)
    (output/'runtime/skies/manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps(dict(original_inputs=False, models=len(models), files=len(files), output=str(output))))


if __name__ == '__main__':
    main()
