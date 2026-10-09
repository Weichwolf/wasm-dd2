#!/usr/bin/env python3
"""Correct prepared scene roles without original files or geometry conversion.

The original sky initializer uses sections 7..14; detached wheel/hood/trunk
initialization uses 18..20. Section 21 has level-specific auxiliary roles.
This migration only relabels existing committed references and compiles scenes.
"""
import argparse
import json
from pathlib import Path

from prepare_scenes import compile_scene, prepare

ROOT = Path(__file__).resolve().parents[2]
RENAMES = {**{f'prop-{index:02}': f'sky-lower-{index-7}' for index in range(7,11)},
           **{f'prop-{index:02}': f'sky-upper-{index-11}' for index in range(11,15)},
           'flag': 'detached-wheel', 'prop-19': 'detached-hood', 'sky': 'detached-trunk'}


def relabel(table):
    result = {RENAMES.get(name, name): item for name, item in table.items()}
    if len(result) != len(table):
        raise ValueError('Conflicting prepared template identities')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT/'assets/runtime')
    args = parser.parse_args()
    path = args.runtime/'reference/manifest.json'
    manifest = json.loads(path.read_text())
    manifest['limitations'] = [item.replace('cockpit/sky policy', 'cockpit policy')
                               for item in manifest['limitations']]
    scenes = []
    for level in manifest['levels']:
        scene_path = args.runtime/level['scene']
        scene = json.loads(scene_path.read_text())
        scene['templates'] = relabel(scene['templates'])
        level['templates'] = relabel(level['templates'])
        for band in ('lower','upper'):
            for quadrant in range(4):
                if f'sky-{band}-{quadrant}' not in scene['templates']:
                    raise ValueError('Missing prepared sky patch')
        if any(name not in scene['templates'] for name in ('detached-wheel','detached-hood','detached-trunk')):
            raise ValueError('Missing prepared detached vehicle template')
        compile_scene(scene)
        scenes.append((scene_path,scene))
    for scene_path,scene in scenes:
        scene_path.write_text(json.dumps(scene,indent=2)+'\n')
    prepare(args.runtime,manifest)
    path.write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(dict(levels=len(scenes),sky_patches=8,original_inputs_required=False,geometry_converted=False)))


if __name__=='__main__':main()
