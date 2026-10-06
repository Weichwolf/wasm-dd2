#!/usr/bin/env python3
"""Enforce LLVM 19 formatting and analysis for the readable rewrite sources."""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def tool(name):
    path = shutil.which(name + '-19') or shutil.which(name)
    if path is None:
        raise RuntimeError(name + ' 19 is required')
    version = subprocess.check_output([path, '--version'], text=True)
    if not re.search(r'version 19\.', version):
        raise RuntimeError('Expected LLVM 19: ' + version.strip())
    return path


def sources():
    names = subprocess.check_output(
        ['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=ROOT)
    paths = {ROOT / name.decode() for name in names.split(b'\0') if name and
             name.startswith((b'src/', b'tests/rewrite/')) and (ROOT / name.decode()).is_file()}
    if any(p.suffix in ('.cc', '.cpp', '.cxx', '.hpp') for p in paths):
        raise RuntimeError('Rewrite modules must be C11; C++ belongs only to separate offline tools')
    return sorted(p for p in paths if p.suffix in ('.c', '.h'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('format', 'format-check', 'tidy'))
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    files = sources()
    if not files:
        parser.error('No rewrite C sources found')
    if args.action in ('format', 'format-check'):
        flags = ['-i'] if args.action == 'format' else ['--dry-run', '--Werror']
        subprocess.run([tool('clang-format'), *flags, *map(str, files)], check=True, cwd=ROOT)
    else:
        if args.build_dir is None:
            parser.error('--build-dir with a native compile_commands.json is required')
        database = args.build_dir.resolve() / 'compile_commands.json'
        commands = json.loads(database.read_text())
        compiled = {Path(row['file']).resolve() for row in commands}
        units = [p for p in files if p.suffix == '.c']
        missing = set(units) - compiled
        if missing:
            raise RuntimeError('Rewrite translation units missing from compile database: ' +
                               ', '.join(map(str, sorted(missing))))
        for unit in units:
            subprocess.run([tool('clang-tidy'), '--warnings-as-errors=*',
                            '-p', str(args.build_dir.resolve()), str(unit)], check=True, cwd=ROOT)
    print(args.action + ': PASS (' + str(len(files)) + ' rewrite C/header files)')


if __name__ == '__main__':
    main()
