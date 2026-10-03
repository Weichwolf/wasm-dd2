#!/usr/bin/env python3
"""Compare first frontend presentations with unmodified original primary Flips.

Includes loading, transitions and a complete settled menu highlight cycle.
Reads full indexed images and palettes in chronological order without alignment.
Intro video/audio, race output, live timing and physical display remain open.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re

from artifacts import WORK, prepare_output, check_space, open_files

EXE_SHA256 = '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
FIELDS = ('cf', 'level', 'poly_list', 'restart_cd_audio', 'cd_playing', 'highlight_phase')
STAGES = {
    'original': 'Successful original primary Flip return @0x412cc1',
    'native': 'Native primary Flip / actual platform presentation',
    'browser': 'Actual browser platform presentation / canvas readback',
}


def load(directory, target):
    directory = directory.resolve()
    if WORK not in directory.parents:
        raise ValueError('Captures must be under /tmp/wasm-dd2/')
    check_space(directory)
    manifest = json.loads((directory/'startup.json').read_text())
    if manifest.get('stage') != STAGES[target]:
        raise ValueError(f'{target}: not the actual primary presentation capture stage')
    if manifest.get('initial_movie_observed') is not True or manifest.get('intro_skip') is not True:
        raise ValueError(f'{target}: normal movie startup and actual intro skip required')
    if manifest.get('input') != ('real browser Escape' if target == 'browser' else 'real X11 Escape'):
        raise ValueError(f'{target}: actual keyboard input required')
    if target == 'original':
        if manifest.get('exe_modified') is not False or manifest.get('exe_sha256') != EXE_SHA256:
            raise ValueError('Supported unmodified original EXE required')
        if manifest.get('breakpoints') != 'one hardware breakpoint; no inferior memory/register writes':
            raise ValueError('Original observer must not write engine memory/registers')
    elif manifest.get('engine_state_writes') is not False:
        raise ValueError(f'{target}: engine state writes are outside this verification scope')
    if target != 'original' and not re.fullmatch('[0-9a-f]{64}', manifest.get(
            'binary_sha256' if target == 'native' else 'wasm_sha256', '')):
        raise ValueError(f'{target}: missing captured binary hash')
    digest = manifest.get('initial_save_sha256', '')
    if not re.fullmatch('[0-9a-f]{64}', digest):
        raise ValueError(f'{target}: missing initial save input hash')
    rows = manifest['frames']
    if not 128 <= len(rows) <= 512 or [r['index'] for r in rows] != list(range(len(rows))):
        raise ValueError(f'{target}: incomplete/reordered first presentation sequence')
    expected = set()
    frames = []
    for row in rows:
        prefix = row['prefix']
        if not re.fullmatch(r'(?:frame|f)[0-9]{5}', prefix):
            raise ValueError(f'{target}: invalid frame prefix')
        data = []
        for suffix, size, key in [('bin', 307200, 'framebuffer_sha256'), ('pal', 1024, 'palette_sha256')]:
            name = prefix+'.'+suffix
            if name in expected:
                raise ValueError(f'{target}: duplicate presentation file')
            expected.add(name)
            value = (directory/name).read_bytes()
            if len(value) != size:
                raise ValueError(f'{target}: incomplete {name}')
            if key in row and hashlib.sha256(value).hexdigest() != row[key]:
                raise ValueError(f'{target}: captured hash differs for {name}')
            data.append(value)
        if target == 'browser' and row.get('canvas_mismatches') != 0:
            raise ValueError('Actual browser canvas differs from indexed framebuffer/palette')
        if row['cf'] != 0 or row['level'] != 0:
            raise ValueError(f'{target}: capture left frontend scope')
        frames.append((row, *data))
    actual = {p.name for p in directory.iterdir() if p.suffix in ('.bin', '.pal') and p.name != 'palette.bin'}
    if actual != expected:
        raise ValueError(f'{target}: unexpected/missing presentation files')
    settled = [r for r in rows if r['poly_list'] == 0x4696b0 and
               r['restart_cd_audio'] == 0 and r['cd_playing'] == 1]
    if {r['highlight_phase'] for r in settled} != set(range(64)):
        raise ValueError(f'{target}: complete settled highlight cycle missing')
    return manifest, frames


def compare(original, port):
    failures = []
    if original[0]['initial_save_sha256'] != port[0]['initial_save_sha256']:
        failures.append({'reason': 'initial save input differs'})
    if len(original[1]) != len(port[1]):
        failures.append({'reason': 'presentation count differs'})
    for index, (left, right) in enumerate(zip(original[1], port[1])):
        for field in ('index', *FIELDS):
            if left[0][field] != right[0][field]:
                failures.append({'index': index, 'reason': f'{field} differs'})
        for item, name in [(1, 'framebuffer'), (2, 'palette')]:
            if left[item] != right[item]:
                offsets = [i for i, (a, b) in enumerate(zip(left[item], right[item])) if a != b]
                failures.append({'index': index, 'reason': name+' differs',
                                 'differing_bytes': len(offsets), 'first_offset': offsets[0]})
    return failures


def negative_checks(original, target):
    """Reject damaged real captures; all mutations stay in diagnostic memory."""
    cases = []
    for name, index, item in [('loading-pixel', 10, 1), ('transition-pixel', 38, 1),
                              ('settled-menu-pixel', 100, 1), ('palette-byte', 100, 2)]:
        changed = (target[0], list(target[1]))
        row = list(changed[1][index])
        damaged = bytearray(row[item]); damaged[416*640 if item == 1 else 17] ^= 1
        row[item] = bytes(damaged); changed[1][index] = tuple(row)
        if not compare(original, changed):
            raise AssertionError(f'Accepted {name}')
        cases.append(name)
    changed = (target[0], target[1][1:]+target[1][:1])
    if not compare(original, changed):
        raise AssertionError('Accepted one-presentation shift')
    cases.append('one-presentation-shift')
    manifest = copy.deepcopy(target[0]); manifest['initial_save_sha256'] = '0'*64
    if not compare(original, (manifest, target[1])):
        raise AssertionError('Accepted different input save')
    return cases+['different-save-input']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', required=True, type=Path, help='original startup/ directory')
    parser.add_argument('--native', required=True, type=Path, help='native startup/ directory')
    parser.add_argument('--browser', required=True, type=Path, help='browser startup/ directory')
    parser.add_argument('--output', required=True, type=Path, help='report JSON under /tmp/wasm-dd2/')
    parser.add_argument('--negative', action='store_true', help='also reject damaged actual captures')
    parser.add_argument('--clean', action='store_true', help='delete successful raw frames after saving report')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Report must be under /tmp/wasm-dd2/')
    output.parent.mkdir(parents=True, exist_ok=True)
    paths = {'original': args.original, 'native': args.native, 'browser': args.browser}
    data = {name: load(path, name) for name, path in paths.items()}
    result = dict(scope='First actual frontend video presentations, including loading, transitions and '
                       'one complete settled menu highlight cycle; no frame alignment; '
                       'intro output, audio, racing, live timing and physical display remain open',
                  original_exe_sha256=EXE_SHA256, initial_save_sha256=data['original'][0]['initial_save_sha256'],
                  frames=len(data['original'][1]), captures={k: str(v.resolve()) for k, v in paths.items()},
                  targets={})
    for target in ('native', 'browser'):
        failures = compare(data['original'], data[target])
        manifest = data[target][0]
        result['targets'][target] = dict(pass_=not failures, failures=failures,
            binary_sha256=manifest.get('binary_sha256', manifest.get('wasm_sha256')),
            negative_cases=negative_checks(data['original'], data[target]) if args.negative and not failures else [])
    result['pass_'] = all(v['pass_'] for v in result['targets'].values())
    # Keep provenance hashes in the report before any successful raw deletion.
    result['frame_hashes'] = {name: [dict(index=row['index'],
        framebuffer_sha256=hashlib.sha256(fb).hexdigest(), palette_sha256=hashlib.sha256(pal).hexdigest())
        for row, fb, pal in capture[1]] for name, capture in data.items()}
    output.write_text(json.dumps(result, indent=2)+'\n')
    if args.clean and result['pass_']:
        opened = open_files()
        files = [path/(row['prefix']+'.'+suffix)
                 for name, path in paths.items() for row, _, _ in data[name][1]
                 for suffix in ('bin', 'pal')]
        if any((p.stat().st_dev, p.stat().st_ino) in opened for p in files):
            raise RuntimeError('Successful report saved, but raw capture files are still open')
        for name, path in paths.items():
            for row, _, _ in data[name][1]:
                for suffix in ('bin', 'pal'):
                    (path/(row['prefix']+'.'+suffix)).unlink()
    print(json.dumps({k: v for k, v in result.items() if k != 'frame_hashes'}, indent=2))
    return 0 if result['pass_'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
