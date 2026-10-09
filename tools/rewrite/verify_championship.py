#!/usr/bin/env python3
"""Verify single-player championship progression and its physical result owner.

Synthetic rule fixtures cover all outcomes/history/rejection boundaries. Actual
original-data sessions drive the first scheduled circuit with ordinary AI input,
consume natural results once, hold results and prepare the next assigned field.
A copied archive with the next track removed checks preparation rollback. This
is component integration, not full-season physical, menu/save/multiplayer
acceptance or original physics/graphics parity.
"""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256

IMAGE_SHA256 = '5c7a6c013a64faac0e3bcde437e01d696de3d79eee655e5fe091c9532465d9d9'


def digest(path):
    hasher = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            hasher.update(block)
    return hasher.hexdigest()


def write_json(path, data):
    path.write_text(json.dumps(data, indent=2) + '\n')


def check_rules(content, schedule, names):
    rows = [json.loads(line) for line in content.splitlines() if line.startswith('{')]
    name_rows = [r for r in rows if r.get('kind') == 'name']
    if [r.get('name') for r in name_rows] != names or [r.get('driver') for r in name_rows] != list(range(20)):
        raise ValueError('Driver names differ from the original NPC roster/default human')
    rows = [r for r in rows if 'tracks' in r]
    expected = {(mode, difficulty) for mode in (0, 1) for difficulty in range(4)}
    if {(r['mode'], r['difficulty']) for r in rows} != expected or len(rows) != len(expected):
        raise ValueError('Missing/duplicate rule schedule receipts')
    for row in rows:
        count = 5 if row['mode'] == 0 else 4
        if row['tracks'] != schedule[row['difficulty']][:count]:
            raise ValueError('Schedule differs from independently decoded original image')
    if 'Championship restart: PASS' not in content or 'stock=1 wreck=1 history=1 losses=1 rejection=1 overflow=1' not in content:
        raise ValueError('Synthetic rule fixture failed')
    return rows


def check_session(content, mode, missing, tables):
    rows = [json.loads(line) for line in content.splitlines() if line.startswith('{')]
    results = [r for r in rows if r['kind'] == 'result']
    summaries = [r for r in rows if r['kind'] == 'summary']
    if len(results) != 1 or len(summaries) != 1 or not summaries[0]['valid']:
        raise ValueError('Physical owner did not complete its contract')
    result, summary = results[0], summaries[0]
    if result['mode'] != mode or summary['mode'] != mode or summary['missing_next_track_fixture'] != missing:
        raise ValueError('Wrong physical fixture/result mode')
    if result['steps'] != 400 + result['elapsed'] or result['end'] not in (1, 2):
        raise ValueError('No natural circuit result with source-timed countdown')
    drivers = result['drivers']
    if len(drivers) != 20 or sorted(d[0] for d in drivers) != list(range(1, 21)):
        raise ValueError('Result places are not a twenty-driver permutation')
    for place, accidents, finish, total, laps in drivers:
        expected_finish = tables[mode][place - 1]
        expected_total = min(999, expected_finish + (accidents if mode == 0 else 0))
        if not 0 <= accidents <= 999 or finish != expected_finish or total != expected_total or laps > 11:
            raise ValueError('Results disagree with original placement points/score cap')
    if result['end'] == 1 and drivers[0][4] != 11:
        raise ValueError('Finished first circuit without ten credited full laps')
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-championship-verification')
    parser.add_argument('--native-build-dir', type=Path, default=WORK / 'rewrite-native')
    parser.add_argument('--wasm-build-dir', type=Path, default=WORK / 'rewrite-wasm')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (ROOT / 'DestructionDerby2/Dirinfo').resolve()
    image_path = ROOT / 'DestructionDerby2/dd2_image.bin'
    if digest(archive) != ORIGINAL_SHA256 or digest(image_path) != IMAGE_SHA256:
        raise ValueError('Provision supported unmodified original Dirinfo/image')
    image = image_path.read_bytes()
    schedule = [list(struct.unpack_from('<5I', image, 0x6758c + difficulty * 20)) for difficulty in range(4)]
    names = ['PLAYER'] + [image[0x67654 + i * 16:0x67654 + (i + 1) * 16].split(b'\0', 1)[0].decode('ascii') for i in range(1, 20)]
    tables = {0: list(struct.unpack_from('<20H', image, 0x6762c)),
              1: list(struct.unpack_from('<20H', image, 0x67604))}
    corrupted = bytearray(archive.read_bytes())
    directory = corrupted[:0x2808]
    row = directory.find(b'LEV2\\LEVEL.DAT\0')
    if row < 0 or row % 24:
        raise ValueError('Next-track fixture entry is not a directory row')
    corrupted[row] = ord('Z')
    missing_archive = output / 'missing-next-Dirinfo'
    missing_archive.write_bytes(corrupted)
    del corrupted

    units = [ROOT / f'src/assets/{name}.c' for name in
             ('car_class', 'archive', 'car', 'level', 'textures', 'lz', 'mesh', 'scene', 'model', 'world', 'track', 'road', 'barriers')]
    units += [ROOT / f'src/physics/{name}.c' for name in
              ('road_contact', 'road_surface', 'body_surface', 'vehicle', 'barrier_world', 'car_contact',
               'contact_group', 'vehicle_collision', 'damage')]
    units += [ROOT / f'src/game/{name}.c' for name in
              ('starting_grid', 'driving', 'accidents', 'course', 'laps', 'race', 'recovery',
               'sound_events', 'league', 'drivers', 'championship', 'championship_session')]
    units += [ROOT / f'src/ai/{name}.c' for name in ('path', 'driver')]
    units += [ROOT / 'src/platform/file.c']
    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT / 'src'), '-Wall', '-Wextra', '-Wpedantic',
             '-Wno-unused-parameter', '-Wno-unused-function', '-fno-strict-aliasing', '-ffast-math',
             '-Werror', '-Wshadow', '-Wconversion', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wformat=2', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    sanitized = {kind: output / ('sanitized-' + kind) for kind in ('test', 'export')}
    builds = {}
    for kind, binary in sanitized.items():
        command = [tool('clang'), *flags, *map(str, units),
                   str(ROOT / f'tests/championship_{kind}.c'), '-lm', '-o', str(binary)]
        log = output / ('build-' + kind + '.log')
        with log.open('wb') as stream:
            result = run_bounded(command, directory=output, timeout=180, cwd=ROOT,
                                 stdout=stream, stderr=subprocess.STDOUT)
        builds[kind] = {'command': command, 'returncode': result.returncode, 'log_sha256': digest(log)}
        if result.returncode:
            write_json(output / 'failed-build.json', builds)
            raise RuntimeError('Sanitized build failed; see ' + str(log))
        log.unlink()

    commands = {
        'native': {kind: [str(args.native_build_dir.resolve() / ('dd2_championship_' + kind))]
                   for kind in ('test', 'export')},
        'wasm': {kind: ['node', str(args.wasm_build_dir.resolve() / ('dd2_championship_' + kind + '.js'))]
                 for kind in ('test', 'export')},
        'sanitized': {kind: [str(binary)] for kind, binary in sanitized.items()}}
    sources = [p for folder in ('src', 'tests') for p in (ROOT / folder).rglob('*') if p.suffix in ('.c', '.h')]
    sources += [Path(__file__).resolve(), ROOT / 'CMakeLists.txt', ROOT / '.clang-format', ROOT / '.clang-tidy']
    binaries = [args.native_build_dir.resolve() / ('dd2_championship_' + kind) for kind in ('test', 'export')]
    binaries += [args.wasm_build_dir.resolve() / ('dd2_championship_' + kind + extension)
                 for kind in ('test', 'export') for extension in ('.js', '.wasm')]
    binaries += list(sanitized.values())
    identity = {'scope': __doc__.strip(), 'original_sha256': digest(archive),
                'image_sha256': digest(image_path), 'original_schedule': schedule,
                'original_placement_points': tables, 'original_npc_names': names[1:], 'missing_next_fixture_sha256': digest(missing_archive),
                'source_sha256': {str(p.relative_to(ROOT)): digest(p) for p in sources},
                'binary_sha256': {str(p): digest(p) for p in binaries}, 'sanitized_builds': builds}
    write_json(output / 'identity.json', identity)

    def check_target(item):
        target, programs = item
        receipts = []
        cases = [('rules', programs['test'], None, None)]
        cases += [(mode_name + '-' + suffix,
                   [*programs['export'], str(missing_archive if missing else archive), mode_name,
                    'missing' if missing else 'next'], mode, missing)
                  for mode_name, mode, missing, suffix in
                  [('stock', 1, False, 'next'), ('wreck', 0, False, 'next'), ('stock', 1, True, 'missing')]]
        for name, command, mode, missing in cases:
            log = output / (target + '-' + name + '.log')
            error = None
            rows = []
            returncode = None
            try:
                with log.open('wb') as stream:
                    result = run_bounded(command, directory=output, timeout=1200, cwd=ROOT,
                                         stdout=stream, stderr=subprocess.STDOUT)
                returncode = result.returncode
                content = log.read_text(errors='replace')
                if returncode or 'Sanitizer:' in content or 'runtime error:' in content:
                    raise ValueError('Export/rule process failed')
                rows = check_rules(content, schedule, names) if mode is None else check_session(content, mode, missing, tables)
            except (ValueError, subprocess.TimeoutExpired, RuntimeError) as exception:
                error = str(exception)
            receipt = {'identity': 'identity.json', 'target': target, 'case': name, 'command': command,
                       'pass_': error is None, 'returncode': returncode, 'error': error,
                       'log_sha256': digest(log), 'rows': rows}
            write_json(output / (target + '-' + name + '.json'), receipt)
            receipts.append(receipt)
            if receipt['pass_']:
                log.unlink()
            print(json.dumps({'target': target, 'case': name, 'pass_': receipt['pass_']}), flush=True)
            if error:
                break
        return receipts

    with ThreadPoolExecutor(max_workers=len(commands)) as executor:
        receipts = [receipt for group in executor.map(check_target, commands.items()) for receipt in group]
    source_unchanged = all(digest(ROOT / name) == value for name, value in identity['source_sha256'].items())
    binary_unchanged = all(digest(Path(name)) == value for name, value in identity['binary_sha256'].items())
    report = {'identity': 'identity.json', 'scope': __doc__.strip(),
              'verified_at': datetime.now(timezone.utc).isoformat(),
              'pass_': source_unchanged and binary_unchanged and len(receipts) == 12 and all(r['pass_'] for r in receipts),
              'source_unchanged': source_unchanged, 'binary_unchanged': binary_unchanged,
              'full_game_acceptance': False, 'results': receipts}
    write_json(output / 'report.json', report)
    # All processes above are terminal, including after a bounded timeout.
    missing_archive.unlink()
    for binary in sanitized.values():
        binary.unlink()
    check_space(output)
    if not report['pass_']:
        raise RuntimeError('Championship owner verification failed; see ' + str(output / 'report.json'))
    print(json.dumps({'pass_': True, 'report': str(output / 'report.json')}), flush=True)


if __name__ == '__main__':
    main()
