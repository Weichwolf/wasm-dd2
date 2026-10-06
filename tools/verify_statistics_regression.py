#!/usr/bin/env python3
"""Compare a fresh statistics capture with a retained accepted original manifest.

The manifest comes from verify_statistics_ui.compare, which compares every raw
original/native/ASan/browser image before cleanup. Require its explicit SHA and
consistent frame catalogs on all three targets. This repeat checks settled menu
images and saved-state behavior; it does not record a new original run or prove
chronological video/audio, multiple seasons or whole-game parity.
"""
import argparse
import copy
import json
from pathlib import Path

from artifacts import WORK, check_space, open_files, prepare_output
from verify_configuration_persistence import EXE_SHA256, digest, require
from verify_configuration_card_ui import cycle_frames
from verify_statistics_ui import PLAN_FILE, POINTS, fixture, validate


def compare_capture(actual, directory, accepted, saved, initial):
    validate(actual, saved, initial)
    target = actual['target']
    require(target in ('native', 'browser'), 'native or browser capture required')
    original = accepted['original']
    validate(original, saved, initial)
    require(original['target'] == 'original' and original['binary_sha256'] == EXE_SHA256,
            'supported actual original reference required')
    wanted = [(action['checkpoint'], phase) for action in POINTS if action['cycle'] for phase in range(64)]
    catalogs = []
    for name in ('native', 'native-asan', 'browser'):
        previous = accepted['targets'][name]
        require(previous['pass_'], 'previous target comparison did not pass')
        validate(previous['capture'], saved, initial)
        rows = previous['frames']
        require([(row['checkpoint'], row['phase']) for row in rows] == wanted,
                'accepted manifest lacks complete ordered frame coverage')
        catalogs.append(rows)
    require(catalogs[0] == catalogs[1] == catalogs[2], 'accepted frame catalogs disagree')
    references = {(row['checkpoint'], row['phase']): row for row in catalogs[0]}
    frames = []
    for left, right, action in zip(original['checkpoints'], actual['checkpoints'], POINTS):
        require({k: v for k, v in left.items() if k != 'cycle'} ==
                {k: v for k, v in right.items() if k != 'cycle'},
                'original statistics state differs at ' + left['name'])
        if not action['cycle']:
            continue
        cycle = directory / action['checkpoint'] / 'cycle'
        require(Path(right['cycle']).resolve() == cycle.resolve(), 'capture cycle path escapes its checkpoint')
        for phase, (state, pair) in sorted(cycle_frames(cycle, target == 'browser').items()):
            require(state['poly_list'] == action['menu'], 'statistics image menu differs')
            row = dict(checkpoint=action['checkpoint'], phase=phase,
                       framebuffer_sha256=digest(pair[0]), palette_sha256=digest(pair[1]))
            require(row == references[(action['checkpoint'], phase)],
                    'accepted original image differs at ' + action['checkpoint'] + ' phase ' + str(phase))
            frames.append(row)
    return frames


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fixture', 'capture', 'accepted', 'report'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--accepted-sha256', required=True)
    parser.add_argument('--clean', action='store_true')
    args = parser.parse_args()
    directory = args.capture.resolve()
    require(WORK in directory.parents, 'capture must be under /tmp/wasm-dd2/')
    check_space(directory)
    output = prepare_output(args.report)
    require(WORK in output.parents and not output.exists(), 'fresh report under /tmp/wasm-dd2/ required')
    manifest = args.accepted.read_bytes()
    require(digest(manifest) == args.accepted_sha256, 'accepted manifest SHA differs')
    accepted = json.loads(manifest)
    require(accepted['pass_'] and accepted['negative_cases'], 'previous verified comparison required')
    initial, saved = fixture(args.fixture)
    actual = json.loads((directory / 'report.json').read_text())
    frames = compare_capture(actual, directory, accepted, saved, initial)
    negative = []
    mutations = {
        'card-changed': lambda data: data.update(card_unchanged=False),
        'missing-checkpoint': lambda data: data['checkpoints'].pop(),
        'missing-key': lambda data: data['input_keys'].pop(),
        'wrong-standings': lambda data: next(row for row in data['checkpoints'] if 'standings' in row)['standings'].__setitem__(0, 'wrong'),
    }
    if actual['target'] == 'browser':
        mutations['untrusted-key'] = lambda data: data['trusted_keyboard_events'][0].update(trusted=False)
    for name, mutate in mutations.items():
        damaged = copy.deepcopy(actual)
        mutate(damaged)
        try:
            compare_capture(damaged, directory, accepted, saved, initial)
        except RuntimeError:
            negative.append(name)
        else:
            raise RuntimeError('damaged capture accepted: ' + name)
    for field in ('framebuffer_sha256', 'palette_sha256'):
        damaged = copy.deepcopy(accepted)
        for previous in damaged['targets'].values():
            digest_ = previous['frames'][0][field]
            previous['frames'][0][field] = ('0' if digest_[0] != '0' else '1') + digest_[1:]
        try:
            compare_capture(actual, directory, damaged, saved, initial)
        except RuntimeError:
            negative.append('wrong-reference-' + field)
        else:
            raise RuntimeError('damaged reference image accepted: ' + field)
    report = dict(scope=__doc__, pass_=True, target=actual['target'], original_port_full_parity='unproven',
                  accepted_manifest_sha256=args.accepted_sha256, capture_report_sha256=digest((directory / 'report.json').read_bytes()),
                  verifier_sha256=digest(Path(__file__).read_bytes()), plan_sha256=digest(PLAN_FILE.read_bytes()),
                  binary_sha256=actual['binary_sha256'], initial_card_sha256=digest(initial),
                  checkpoint_count=len(actual['checkpoints']), frames=frames, negative_controls=negative)
    output.write_text(json.dumps(report, indent=2) + '\n')
    if args.clean:
        opened = open_files()
        files = list(directory.glob('*/cycle/*.bin'))
        removed = []
        for file in files:
            require(not file.is_symlink(), 'unexpected raw capture symlink')
            stat = file.stat()
            require((stat.st_dev, stat.st_ino) not in opened, 'raw capture still open')
            removed.append(dict(file=str(file.relative_to(directory)), bytes=stat.st_size, sha256=digest(file.read_bytes())))
        (directory / 'cleanup.json').write_text(json.dumps(dict(removed=removed), indent=2) + '\n')
        for file in files:
            file.unlink()
    print('Statistics regression: PASS;', len(actual['checkpoints']), 'states;', len(frames),
          'accepted original frame/palette pairs;', len(negative), 'rejected controls')


if __name__ == '__main__':
    main()
