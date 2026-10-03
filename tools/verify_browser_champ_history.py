#!/usr/bin/env python3
"""Compare production browser championship with original API inputs and retained image hashes.

Original raw images were deleted after the prior literal native/original comparison.
Reuse only its explicitly retained image hashes, with matching API/save provenance.
This controlled retirement trace does not prove chronological whole-game A/V parity.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path

from artifacts import open_files, prepare_output
from verify_champ_history import history, work
from verify_champ_season import EXE, KEYS, decode, validate_end, validate_race_start


def checkpoints(original, states):
    if len(states) != 96:
        raise ValueError('Incomplete actual browser championship')
    for race, level in enumerate((1, 2, 5, 7, 10)):
        validate_race_start(states[10 + 17 * race], level, race)
    validate_end(states[-1])
    seeds = [1]
    for expected, actual in zip(original, states):
        count = expected['observed']['rng_calls']
        while len(seeds) <= count:
            seeds.append((seeds[-1] * 1103515245 + 12345) & 0xffffffff)
        if actual['rng'] != dict(count=count, seed=seeds[count]):
            raise ValueError('Browser original RNG seed/counter differs')
        if actual['api_calls'] != dict(clock=expected['observed']['clock_calls'], random=count):
            raise ValueError('Browser original calculated API-call extent differs')
        for field in ('level', 'cf', 'ticks', 'quit', 'race', 'season', 'poly_list'):
            if actual[field] != expected[field]:
                raise ValueError(f'Browser original checkpoint {field} differs')
    for index in [17 + 17 * i for i in range(5)] + [95]:
        if states[index]['cars'] != original[index]['cars']:
            raise ValueError('Browser complete standings differ')
    for race in range(5):
        for division in range(4):
            index = 19 + 17 * race + division
            if states[index]['rows'] != original[index]['rows']:
                raise ValueError('Browser actual league rows differ')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('original', 'reference-report', 'browser', 'report'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--clean', action='store_true')
    args = parser.parse_args()
    original = history(args.original, 'original')
    accepted = json.loads(work(args.reference_report).read_text())
    api_hashes = {name: hashlib.sha256(raw).hexdigest() for name, raw in [('clock', original[4]), ('random', original[3])]}
    if (accepted['original_exe_sha256'] != EXE or accepted['initial_save_sha256'] != original[1]['initial_save_sha256']
            or accepted['api_sha256'] != api_hashes or accepted['native']['pass_'] is not True):
        raise ValueError('Retained original image proof has different provenance')
    browser = work(args.browser)
    nav = json.loads((browser / 'navigation.json').read_text())
    observer = json.loads((browser / 'rng-observations.json').read_text())
    if (nav['keys'] != KEYS or nav['checkpoints'] != original[6] or nav['input'] != 'browser keyboard events'
            or nav['initial_save_sha256'] != accepted['initial_save_sha256']
            or nav['wasm_sha256'] != observer['layout']['wasm_sha256']):
        raise ValueError('Actual browser input/save/binary provenance differs')
    if nav['api_reference'] != dict(clock_calls=original[1]['clock_calls'], computed_rng_calls=original[1]['rng_calls'], api_sha256=api_hashes):
        raise ValueError('Actual browser original API replay incomplete')
    actions = nav['input_observations']
    if [(row['action'], row['down']) for row in actions] != [(i, down) for i in range(1, 96) for down in (True, False)]:
        raise ValueError('Actual complete press/release sequence missing')
    codes = dict(Return='Enter', Escape='Escape', Up='ArrowUp', Down='ArrowDown', Left='ArrowLeft', Right='ArrowRight')
    if [row['code'] for row in actions] != [codes[key] for key in KEYS for _ in range(2)]:
        raise ValueError('Actual DOM input codes differ')
    states = [decode(browser / name, 'browser') for name in original[6]]
    checkpoints(original[5], states)
    originals = accepted['native']['image_hashes']
    names = [original[6][19 + 17 * race + division] for race in range(5) for division in range(4)]
    if [row['checkpoint'] for row in originals] != names:
        raise ValueError('Retained original league images incomplete/reordered')
    hashes = []
    for row in originals:
        record = dict(checkpoint=row['checkpoint'], original=row['original'], browser={})
        if row['original'] != row['actual']:
            raise ValueError('Retained native/original image comparison did not pass')
        for filename, size in [('framebuf.bin', 307200), ('palette.bin', 1024)]:
            data = (browser / row['checkpoint'] / filename).read_bytes()
            digest = hashlib.sha256(data).hexdigest()
            if len(data) != size or digest != row['original'][filename]:
                raise ValueError('Actual browser league image differs from retained original hash')
            record['browser'][filename] = digest
        hashes.append(record)
    negatives = []
    for name, field, changed in [('changed-rng-seed', 'rng', dict(count=484, seed=0)),
                                  ('changed-api-extent', 'api_calls', dict(clock=0, random=0)),
                                  ('loading-instead-of-gameplay', 'ticks', 0)]:
        damaged = copy.deepcopy(states)
        damaged[10][field] = changed
        try:
            checkpoints(original[5], damaged)
        except ValueError:
            negatives.append(name)
        else:
            raise AssertionError('Damaged actual browser history accepted')
    report = dict(scope=__doc__.strip(), pass_=True, original_exe_sha256=EXE,
        initial_save_sha256=accepted['initial_save_sha256'], wasm_sha256=nav['wasm_sha256'],
        clock_calls=original[1]['clock_calls'], computed_rng_calls=original[1]['rng_calls'],
        actual_dom_actions=95, checkpoints=96, complete_standings=5, league_images=20,
        original_image_evidence=dict(report=str(work(args.reference_report)),
            sha256=hashlib.sha256(args.reference_report.read_bytes()).hexdigest(),
            method='Retained hashes from prior literal original/native comparison; original raw deleted'),
        api_sha256=api_hashes, image_hashes=hashes, negative_cases=negatives)
    output = prepare_output(work(args.report))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    if args.clean:
        opened = open_files()
        files = [p for p in browser.glob('step*/*.bin') if not p.is_symlink()]
        if any((p.stat().st_dev, p.stat().st_ino) in opened for p in files):
            raise RuntimeError('Completed browser capture still open')
        for p in files:
            p.unlink()
    print('PASS browser: actual original API inputs, 96 checkpoints, five standings, 20 retained original image hashes; negatives:', negatives)


if __name__ == '__main__':
    main()
