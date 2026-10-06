#!/usr/bin/env python3
"""Build with a private writable ports cache, including Debian's frozen SDK.

Reuse system headers/libraries through read-only file links, rather than copying
the complete SDK or writing package-manager files. SDL headers and libraries are
built privately. These are reusable dependencies, not verification captures.
"""
import hashlib
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
WORK = Path('/tmp/wasm-dd2')


def cache_environment():
    env = os.environ.copy()
    if env.get('EM_CACHE'):
        return env
    system = Path(subprocess.check_output(['em-config', 'CACHE'], text=True).strip()).resolve()
    version = subprocess.check_output(['emcc', '--version'], text=True)
    key = hashlib.sha256((str(system) + version).encode()).hexdigest()[:12]
    private = WORK / 'emscripten-cache' / key
    private.mkdir(parents=True, exist_ok=True)
    marker = private / 'dd2-system-links.json'
    if not marker.exists():
        for parent, directories, names in os.walk(system, followlinks=False):
            relative = Path(parent).relative_to(system)
            if relative == Path('.'):
                directories[:] = [name for name in directories if name not in ('ports', 'ports-builds')]
            if relative == Path('sysroot/include'):
                directories[:] = [name for name in directories if name != 'SDL2']
            destination = private / relative
            destination.mkdir(parents=True, exist_ok=True)
            for name in names:
                if name.endswith('.lock') or 'SDL2' in name:
                    continue
                target = destination / name
                if not target.exists() and not target.is_symlink():
                    target.symlink_to(Path(parent) / name)
        # The marker is written only after all reusable system links exist.
        import json
        marker.write_text(json.dumps(dict(system=str(system), compiler=version), indent=2) + '\n')
    env.update(EM_CACHE=str(private), EM_FROZEN_CACHE='0')
    print('Rewrite Emscripten dependency cache:', private, flush=True)
    return env


def main():
    env = cache_environment()
    for command in (['emcmake', 'cmake', '--preset', 'rewrite-wasm'],
                    ['cmake', '--build', '--preset', 'rewrite-wasm']):
        subprocess.run(command, cwd=ROOT, env=env, check=True)


if __name__ == '__main__':
    main()
