#!/usr/bin/env python3
"""Rebuild the checked-in authored content without any original game input."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'assets')
    parser.add_argument('--preview', type=Path)
    parser.add_argument('--blender', default='blender')
    args = parser.parse_args()
    if args.preview and Path('/tmp/wasm-dd2') not in args.preview.resolve().parents:
        parser.error('Preview renders belong under /tmp/wasm-dd2/')
    blender = shutil.which(args.blender)
    if blender is None:
        parser.error('Blender 4.3 is required for offline source regeneration')
    environment = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
    for tool in ('generate_textures.py', 'generate_audio.py'):
        subprocess.run([sys.executable, str(ROOT / 'tools/assets' / tool),
                        '--output', str(args.output.resolve())], env=environment, check=True)
    command = [blender, '--background', '--factory-startup', '-t', '4', '--python-exit-code', '1',
               '--python', str(ROOT / 'tools/assets/build_vehicle.py'), '--',
               '--output', str(args.output.resolve())]
    if args.preview:
        command += ['--preview', str(args.preview.resolve())]
    subprocess.run(command, env=environment, check=True)
    subprocess.run([sys.executable, str(ROOT / 'tools/assets/verify_assets.py'),
                    '--assets', str(args.output.resolve())], env=environment, check=True)
    print(json.dumps({'authored_content_generated_and_verified': True, 'original_inputs': False,
                      'runtime_integrated': False, 'output': str(args.output.resolve())}))


if __name__ == '__main__':
    main()
