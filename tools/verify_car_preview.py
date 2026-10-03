#!/usr/bin/env python3
"""Compare complete car-preview rotations at independently observed model poses.

This is a periodic renderer component check after real menu navigation. It
pairs by car identity and the engine's 12-bit rotation, never by image fitting.
Chronological video, input/live timing, audio and racing are outside scope.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

from artifacts import WORK, check_space, prepare_output, open_files

EXE = '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'


def pose(row):
    return row['race_car'], tuple(value & 4095 for value in row['car_angles'])


def load(root, target):
    root = root.resolve()
    if WORK not in root.parents:
        raise ValueError('Capture must be under /tmp/wasm-dd2/')
    check_space(root)
    navigation = json.loads((root/'navigation.json').read_text())
    if not re.fullmatch('[0-9a-f]{64}',navigation.get('initial_save_sha256','')):
        raise ValueError('Missing initial save input hash')
    if navigation['input'] != {'original': 'real X11 keys', 'native': 'dd2_key_event',
                               'browser': 'browser keyboard events'}[target]:
        raise ValueError('Incorrect keyboard provenance')
    if target == 'original' and not navigation.get('acknowledged_keys'):
        raise ValueError('Original keyboard press/release must be observed')
    if navigation['checkpoints'] != [f"step{i:02d}-{key or 'boot'}"
                                     for i, key in enumerate([None, *navigation['keys']])]:
        raise ValueError('Incomplete menu navigation')
    selected = {}
    for checkpoint in navigation['checkpoints']:
        directory = root/checkpoint/'cycle'
        if target == 'original':
            source = json.loads((root/checkpoint/'checkpoint.json').read_text())
            if source.get('exe_modified') is not False or source.get('exe_sha256') != EXE:
                raise ValueError('Supported unmodified original required')
        capture = json.loads((directory/'cycle.json').read_text())
        stage = 'browser platform present' if target == 'browser' else 'Draw_All entry / pending presentation'
        if capture['stage'] != stage:
            raise ValueError('Incorrect presentation capture stage')
        rows = capture['frames']
        if len(rows) != 256 or [r['index'] for r in rows] != list(range(256)):
            raise ValueError('Complete 256-presentation cycles required')
        if not all(r['level'] == 0 and r['cf'] == 0 for r in rows):
            raise ValueError('Capture left frontend')
        if not all(r['poly_list'] == 0x468ce8 for r in rows):
            if any(r['poly_list'] == 0x468ce8 for r in rows):
                raise ValueError('Car screen changed during capture')
            continue
        if len({r['race_car'] for r in rows}) != 1 or rows[0]['race_car'] not in range(3):
            raise ValueError('Car choice changed during cycle')
        if len({tuple(r['car_angles'][:2]) for r in rows}) != 1:
            raise ValueError('Car preview tilt changed')
        if len({pose(r) for r in rows}) != 256 or any(
                ((rows[i]['car_angles'][2]-rows[i-1]['car_angles'][2]) & 4095) != 4080
                for i in range(1, 256)):
            raise ValueError('Missing/repeated/nonconsecutive car rotation poses')
        frames = {}
        for row in rows:
            if target == 'browser' and row.get('canvas_mismatches') != 0:
                raise ValueError('Actual canvas differs from indexed image')
            files = [directory/f"{row['prefix']}-{name}.bin" for name in ('framebuf', 'palette')]
            values = [file.read_bytes() for file in files]
            if list(map(len, values)) != [307200, 1024]:
                raise ValueError('Incomplete framebuffer/palette')
            frames[pose(row)] = (row, *values, files)
        selected[checkpoint] = frames
    if not selected:
        raise ValueError('No complete car preview captured')
    return navigation, selected


def compare(left, right):
    failures = []
    if set(left) != set(right):
        raise ValueError('Preview checkpoints differ')
    for checkpoint, original in left.items():
        actual = right[checkpoint]
        if set(original) != set(actual):
            raise ValueError('Car identities/model poses differ')
        for key, frame in original.items():
            other = actual[key]
            for i, region in [(1, 'framebuffer'), (2, 'palette')]:
                if frame[i] != other[i]:
                    failures.append(dict(checkpoint=checkpoint,car=key[0],angles=key[1],region=region,
                        differing_bytes=sum(a != b for a, b in zip(frame[i], other[i]))))
    return failures


def negative_cases(original, target):
    cases = []
    for name, item in [('missing-body-pixel', 1), ('palette-byte', 2)]:
        changed = {k: dict(v) for k, v in target.items()}
        checkpoint = next(iter(changed)); key = next(iter(changed[checkpoint]))
        frame = list(changed[checkpoint][key]); value = bytearray(frame[item])
        value[200*640+200 if item == 1 else 17] ^= 1; frame[item] = bytes(value)
        changed[checkpoint][key] = tuple(frame)
        if not compare(original, changed):
            raise AssertionError('Damaged preview accepted: '+name)
        cases.append(name)
    changed = {k: dict(v) for k, v in target.items()}
    checkpoint = next(iter(changed)); keys = list(changed[checkpoint])
    a, b = keys[0], keys[1]
    changed[checkpoint][a], changed[checkpoint][b] = changed[checkpoint][b], changed[checkpoint][a]
    if not compare(original, changed):
        raise AssertionError('Swapped neighboring pose images accepted')
    return cases+['swapped-neighboring-pose-images']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', required=True, type=Path)
    parser.add_argument('--native', required=True, type=Path)
    parser.add_argument('--browser', type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--negative', action='store_true')
    parser.add_argument('--clean', action='store_true', help='remove successful raw comparison frames after report')
    args = parser.parse_args()
    report = prepare_output(args.report)
    if WORK not in report.parents:
        parser.error('Report must be under /tmp/wasm-dd2/')
    paths = dict(original=args.original, native=args.native)
    if args.browser:
        paths['browser'] = args.browser
    data = {target: load(path, target) for target, path in paths.items()}
    reference = data['original']
    result = dict(scope=__doc__.strip(),original_exe_sha256=EXE,targets={},
                  initial_save_sha256=reference[0]['initial_save_sha256'],
                  captures={target: str(path.resolve()) for target, path in paths.items()})
    for target in paths.keys()-{'original'}:
        navigation, frames = data[target]
        if navigation['keys'] != reference[0]['keys'] or navigation['initial_save_sha256'] != result['initial_save_sha256']:
            raise ValueError('Different menu navigation or initial save input')
        failures = compare(reference[1], frames)
        result['targets'][target] = dict(pass_=not failures,failures=failures,
            binary_sha256=navigation.get('binary_sha256',navigation.get('wasm_sha256')),
            frames=sum(map(len, frames.values())),cars=sorted({key[0] for cycle in frames.values() for key in cycle}),
            negative_cases=negative_cases(reference[1], frames) if args.negative and not failures else [])
    result['pass_'] = all(r['pass_'] for r in result['targets'].values())
    result['hashes'] = {target: {checkpoint: [dict(car=key[0],angles=key[1],
        framebuffer_sha256=hashlib.sha256(value[1]).hexdigest(),palette_sha256=hashlib.sha256(value[2]).hexdigest())
        for key, value in sorted(cycle.items())] for checkpoint, cycle in capture[1].items()}
        for target, capture in data.items()}
    report.parent.mkdir(parents=True,exist_ok=True)
    report.write_text(json.dumps(result,indent=2)+'\n')
    if args.clean and result['pass_']:
        files = [file for capture in data.values() for cycle in capture[1].values()
                 for frame in cycle.values() for file in frame[3]]
        opened = open_files()
        if any((p.stat().st_dev,p.stat().st_ino) in opened for p in files):
            raise RuntimeError('Successful report saved; capture files are still open')
        for file in files:
            file.unlink()
    display = {k: v for k,v in result.items() if k != 'hashes'}
    display['targets'] = {name: {**value, 'failure_count': len(value['failures']),
        'failures': value['failures'][:5]} for name,value in result['targets'].items()}
    print(json.dumps(display,indent=2))
    return 0 if result['pass_'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
