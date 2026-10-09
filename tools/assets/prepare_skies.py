#!/usr/bin/env python3
"""Bind authored panoramas to committed scenes; preserve reference sky evidence."""
import argparse
import copy
import json
from pathlib import Path

from prepare_scenes import compile_scene, prepare

ROOT = Path(__file__).resolve().parents[2]
PATCHES = [f'sky-{band}-{quadrant}' for band in ('lower','upper') for quadrant in range(4)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT/'assets/runtime')
    args = parser.parse_args()
    manifest_path = args.runtime/'reference/manifest.json'
    manifest = json.loads(manifest_path.read_text())
    skies = json.loads((args.runtime/'skies/manifest.json').read_text())
    if skies['format'] != 'DD2SKIES1' or not skies['authored'] or skies['original_inputs_required']:
        raise ValueError('Expected owned authored sky inventory')
    profiles = {item['level']:item for item in skies['profiles']}
    if set(profiles) != set('123456789AB'):
        raise ValueError('Missing authored sky level')
    scenes = []
    for level in manifest['levels']:
        scene_path = args.runtime/level['scene']
        scene = json.loads(scene_path.read_text())
        reference = scene.setdefault('reference_templates', {})
        for name in PATCHES:
            if name not in reference:
                if not scene['templates'][name]['model'].startswith('reference/'):
                    raise ValueError('Missing preserved reference sky template')
                reference[name] = copy.deepcopy(scene['templates'][name])
            replacement = profiles[level['level']]['templates'][name]
            if not replacement['model'].startswith('skies/') or replacement['model'] not in skies['models']:
                raise ValueError('Invalid authored sky model binding')
            scene['templates'][name] = copy.deepcopy(replacement)
        scene['authored_sky'] = dict(manifest='skies/manifest.json', profile=level['level'])
        compile_scene(scene)
        scenes.append((scene_path, scene))
    for path, scene in scenes:
        path.write_text(json.dumps(scene, indent=2)+'\n')
    prepare(args.runtime, manifest)
    manifest['authored_skies'] = 'skies/manifest.json'
    manifest_path.write_text(json.dumps(manifest, indent=2)+'\n')
    print(json.dumps(dict(levels=len(scenes), patches=88, original_inputs=False,
                          reference_resource_bytes_changed=False)))


if __name__ == '__main__':
    main()
