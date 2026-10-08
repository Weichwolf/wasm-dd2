#!/usr/bin/env python3
"""Check assigned stable-ID grids and physical mode/reset ownership on original data.

Four fixture league divisions exercise every original level. Fixture promotion
points initialize standings; they are not simulated championship race results.
The export checks nominal starts against the existing original-grid module and
then advances real physics. This is component integration evidence, not original
bitidentity, completed natural races or complete championship acceptance.
"""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256


def digest(path):
    hasher = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            hasher.update(block)
    return hasher.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-grid-verification')
    parser.add_argument('--native-build-dir', type=Path, default=WORK / 'rewrite-native')
    parser.add_argument('--wasm-build-dir', type=Path, default=WORK / 'rewrite-wasm')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (ROOT / 'DestructionDerby2/Dirinfo').resolve()
    if digest(archive) != ORIGINAL_SHA256:
        raise ValueError('Provision the supported unmodified original Dirinfo')

    units = [ROOT / f'src/assets/{name}.c' for name in ('archive', 'level', 'road', 'barriers')]
    units += [ROOT / f'src/physics/{name}.c' for name in
              ('road_contact', 'road_surface', 'body_surface', 'vehicle', 'barrier_world', 'car_contact',
               'contact_group', 'vehicle_collision', 'damage')]
    units += [ROOT / f'src/game/{name}.c' for name in
              ('starting_grid', 'driving', 'accidents', 'course', 'laps', 'race', 'recovery',
               'sound_events', 'league')]
    units += [ROOT / f'src/ai/{name}.c' for name in ('path', 'driver')]
    units += [ROOT / 'src/platform/file.c', ROOT / 'tests/grid_export.c']
    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT / 'src'), '-Wall', '-Wextra', '-Wpedantic',
             '-Wno-unused-parameter', '-Wno-unused-function', '-fno-strict-aliasing', '-ffast-math',
             '-Werror', '-Wshadow', '-Wconversion', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wformat=2', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    sanitized = output / 'export-sanitized'
    build = [tool('clang'), *flags, *map(str, units), '-lm', '-o', str(sanitized)]
    build_log = output / 'build.log'
    with build_log.open('wb') as stream:
        result = run_bounded(build, directory=output, timeout=180, cwd=ROOT,
                             stdout=stream, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError('Sanitized grid build failed; see ' + str(build_log))

    commands = {'native': [str(args.native_build_dir.resolve() / 'dd2_grid_export')],
                'wasm': ['node', str(args.wasm_build_dir.resolve() / 'dd2_grid_export.js')],
                'sanitized': [str(sanitized)]}
    sources = [p for folder in ('src', 'tests') for p in (ROOT / folder).rglob('*')
               if p.suffix in ('.c', '.h')]
    sources += [Path(__file__).resolve(), ROOT / 'CMakeLists.txt']
    binaries = [args.native_build_dir.resolve() / 'dd2_grid_export',
                args.wasm_build_dir.resolve() / 'dd2_grid_export.wasm', sanitized]
    identity = {'scope': __doc__.strip(), 'original_sha256': digest(archive),
                'source_sha256': {str(p.relative_to(ROOT)): digest(p) for p in sources},
                'binary_sha256': {str(p): digest(p) for p in binaries},
                'sanitized_build': build, 'build_log_sha256': digest(build_log)}
    (output / 'identity.json').write_text(json.dumps(identity, indent=2) + '\n')
    build_log.unlink()

    def check_target(item):
        target, command = item
        path = output / (target + '.log')
        with path.open('wb') as stream:
            result = run_bounded([*command, str(archive)], directory=output, timeout=900,
                                 cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT)
        content = path.read_text(errors='replace')
        rows = [json.loads(line) for line in content.splitlines() if line.startswith('{')]
        expected = {(level, division) for level in range(1, 12) for division in range(4)}
        human_slots = {3: 0, 2: 9, 1: 14, 0: 19}
        passed = result.returncode == 0 and len(rows) == len(expected) and all(
            row['valid'] and row['human_grid_slot'] == human_slots[row['division']] for row in rows)
        passed = passed and {(r['level'], r['division']) for r in rows} == expected
        passed = passed and 'Sanitizer:' not in content and 'runtime error:' not in content
        receipt = {'identity': 'identity.json', 'target': target, 'pass_': passed,
                   'returncode': result.returncode, 'log_sha256': digest(path), 'rows': rows}
        (output / (target + '.json')).write_text(json.dumps(receipt, indent=2) + '\n')
        if passed:
            path.unlink()
        print(json.dumps({'target': target, 'pass_': passed, 'fields': len(rows)}), flush=True)
        return receipt

    with ThreadPoolExecutor(max_workers=len(commands)) as executor:
        results = list(executor.map(check_target, commands.items()))
    source_unchanged = all(digest(ROOT / name) == value for name, value in identity['source_sha256'].items())
    binary_unchanged = all(digest(Path(name)) == value for name, value in identity['binary_sha256'].items())
    report = {'identity': 'identity.json', 'scope': __doc__.strip(),
              'verified_at': datetime.now(timezone.utc).isoformat(),
              'pass_': source_unchanged and binary_unchanged and all(r['pass_'] for r in results),
              'source_unchanged': source_unchanged, 'binary_unchanged': binary_unchanged,
              'assigned_starts_per_target': 880, 'results': results}
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    if not report['pass_']:
        raise RuntimeError('Assigned grid verification failed; see ' + str(output / 'report.json'))
    sanitized.unlink()
    check_space(output)
    print(json.dumps({'pass_': True, 'report': str(output / 'report.json')}), flush=True)


if __name__ == '__main__':
    main()
