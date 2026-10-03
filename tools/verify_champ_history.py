#!/usr/bin/env python3
"""Validate actual championship API history and locate live-browser RNG divergence.

Native must consume every original clock/RNG record and match the five standings
and 20 complete league framebuffers/palettes. Browser telemetry is read-only on
the actual production WASM, checked against the LCG from boot seed 1. Different
live input/physics timing remains a diagnosis, not complete original A/V parity.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct

from artifacts import WORK, open_files, prepare_output
from verify_champ_season import EXE, KEYS, decode, validate_end, validate_race_start


def work(path):
    path = path.resolve()
    if WORK not in path.parents:
        raise ValueError('Use captures/reports under /tmp/wasm-dd2/')
    return path


def recorded_apis(meta, events, random, ticks):
    if len(random) != meta['rng_calls'] * 12 or len(ticks) != meta['clock_calls'] * 4:
        raise ValueError('Incomplete original API records')
    r = [event for event in events if event['kind'] == 'rand']
    t = [event for event in events if event['kind'] == 'GetTickCount']
    if len(r) != meta['rng_calls'] or len(t) != meta['clock_calls']:
        raise ValueError('Incomplete original API timeline')
    seed = 1
    for index, ((before, after, value), event) in enumerate(zip(struct.iter_unpack('<III', random), r)):
        calculated = (seed * 1103515245 + 12345) & 0xffffffff
        if before != seed or after != calculated or value != (after >> 16) & 32767:
            raise ValueError('Original RNG sequence/arithmetic differs')
        if (event['before'], event['after'], event['result'], event['rng_calls']) != (before, after, value, index + 1):
            raise ValueError('Original RNG timeline differs')
        seed = after
    if [value[0] for value in struct.iter_unpack('<I', ticks)] != [event['value'] for event in t]:
        raise ValueError('Original clock timeline differs')


def history(root, target):
    root = work(root)
    meta = json.loads((root / 'history.json').read_text())
    names = [f'step{i:02d}-{key or "boot"}' for i, key in enumerate([None, *KEYS])]
    if not meta['complete_retirement_season'] or meta['keys'] != KEYS or meta['checkpoints'] != names:
        raise ValueError('Actual complete retirement season required')
    if not meta['acknowledged_keys'] or len(meta['held_pad_polls']) != 95 or min(meta['held_pad_polls']) < 1:
        raise ValueError('Actual press/release acknowledgements missing')
    if target == 'original' and (meta['exe_modified'] is not False or meta['exe_sha256'] != EXE or meta['input'] != 'real X11 keys'):
        raise ValueError('Actual unmodified supported original required')
    if target == 'native' and (meta['input'] != 'dd2_key_event' or len(meta.get('binary_sha256', '')) != 64 or 'api_inputs' not in meta):
        raise ValueError('Actual native computed API replay required')
    events = [json.loads(line) for line in (root / 'events.jsonl').read_text().splitlines()]
    if [row['index'] for row in events] != list(range(1, len(events) + 1)):
        raise ValueError('Incomplete/reordered history observations')
    random = (root / 'random.bin').read_bytes()
    ticks = (root / 'ticks.bin').read_bytes()
    recorded_apis(meta, events, random, ticks)
    actions = [row for row in events if row['kind'] == 'key']
    if [(row['key'], row['down'], row['action']) for row in actions] != [(key, down, i) for i, key in enumerate(KEYS, 1) for down in (True, False)]:
        raise ValueError('Actual input history differs')
    states = [json.loads((root / name / 'checkpoint.json').read_text()) for name in names]
    for race, level in enumerate((1, 2, 5, 7, 10)):
        validate_race_start(states[10 + 17 * race], level, race)
    validate_end(states[-1])
    return root, meta, events, random, ticks, states, names


def league_images(reference, actual, names):
    differences = []
    hashes = []
    for race in range(5):
        for division in range(4):
            name = names[19 + 17 * race + division]
            record = dict(checkpoint=name, original={}, actual={})
            for filename, size in [('framebuf.bin', 307200), ('palette.bin', 1024)]:
                a = (reference / name / filename).read_bytes()
                b = (actual / name / filename).read_bytes()
                if len(a) != size or len(b) != size:
                    raise ValueError('Incomplete league framebuffer/palette')
                if a != b:
                    differences.append(dict(checkpoint=name, region=filename,
                        differing_bytes=sum(x != y for x, y in zip(a, b))))
                record['original'][filename] = hashlib.sha256(a).hexdigest()
                record['actual'][filename] = hashlib.sha256(b).hexdigest()
            hashes.append(record)
    return differences, hashes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('original', 'native', 'browser', 'report'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--before-browser', type=Path, help='actual failed loading/input capture to reject explicitly')
    parser.add_argument('--clean', action='store_true')
    args = parser.parse_args()
    original = history(args.original, 'original')
    native = history(args.native, 'native')
    if native[1]['initial_save_sha256'] != original[1]['initial_save_sha256']:
        raise ValueError('Native save differs from original')
    if native[3:5] != original[3:5]:
        raise ValueError('Native actual clock/RNG records differ')
    result_indices = [17 + 17 * i for i in range(5)]
    if any(original[5][i]['cars'] != native[5][i]['cars'] for i in [*result_indices, 95]):
        raise ValueError('Native full standings differ')
    native_differences, native_hashes = league_images(original[0], native[0], original[6])
    if native_differences:
        raise ValueError('Native original league pixels/palettes differ')
    browser = work(args.browser)
    nav = json.loads((browser / 'navigation.json').read_text())
    observer = json.loads((browser / 'rng-observations.json').read_text())
    if nav['keys'] != KEYS or nav['checkpoints'] != original[6] or nav['input'] != 'browser keyboard events' or nav['initial_save_sha256'] != original[1]['initial_save_sha256']:
        raise ValueError('Actual browser input/save sequence differs')
    if nav['wasm_sha256'] != observer['layout']['wasm_sha256']:
        raise ValueError('Browser RNG telemetry binary differs')
    observations = observer['observations']
    if not observations or (observations[0]['count'], observations[0]['seed']) != (0, 1):
        raise ValueError('Browser boot RNG state was not observed')
    states = [decode(browser / name, 'browser') for name in original[6]]
    for race, level in enumerate((1, 2, 5, 7, 10)):
        validate_race_start(states[10 + 17 * race], level, race)
    validate_end(states[-1])
    seeds = [1]
    differences = []
    for i, state in enumerate(states):
        count = state['rng']['count']
        if not 0 <= count <= 100000:
            raise ValueError('Unbounded browser RNG counter')
        while len(seeds) <= count:
            seeds.append((seeds[-1] * 1103515245 + 12345) & 0xffffffff)
        if state['rng']['seed'] != seeds[count]:
            raise ValueError('Browser observed LCG seed/counter mismatch')
        expected = original[5][i]['observed']['rng_calls']
        if count != expected:
            differences.append(dict(checkpoint=original[6][i], original_calls=expected,
                browser_calls=count, original_ticks=original[5][i]['ticks'], browser_ticks=state['ticks']))
    browser_differences, browser_hashes = league_images(original[0], browser, original[6])
    standings = [dict(race=r + 1, pass_=original[5][i]['cars'] == states[i]['cars'],
        original_calls=original[5][i]['observed']['rng_calls'], browser_calls=states[i]['rng']['count']) for r, i in enumerate(result_indices)]
    negatives = []
    bad = bytearray(original[3]); bad[4] ^= 1
    for name, random, ticks in [('changed-rng-state', bad, original[4]), ('truncated-rng', original[3][:-1], original[4]), ('truncated-clock', original[3], original[4][:-1])]:
        try:
            recorded_apis(original[1], original[2], random, ticks)
        except ValueError:
            negatives.append(name)
        else:
            raise AssertionError('Damaged API input accepted')
    if args.before_browser:
        state = decode(work(args.before_browser) / 'step10-Return', 'browser')
        try:
            validate_race_start(state, 1, 0)
        except ValueError:
            negatives.append('actual-loading-checkpoint-instead-of-gameplay')
        else:
            raise AssertionError('Actual early loading checkpoint accepted')
    report = dict(scope=__doc__.strip(), diagnostic_complete=True,
        original_exe_sha256=EXE, initial_save_sha256=original[1]['initial_save_sha256'],
        native=dict(pass_=True, clock_calls=original[1]['clock_calls'], computed_rng_calls=original[1]['rng_calls'],
            binary_sha256=native[1]['binary_sha256'], league_images=20, image_hashes=native_hashes),
        original_rand_callers=dict(Counter(row['caller'] for row in original[2] if row['kind'] == 'rand')),
        browser=dict(original_gate_pass=not differences and not browser_differences and all(row['pass_'] for row in standings),
            wasm_sha256=nav['wasm_sha256'], rng_divergences=differences, standings=standings,
            image_differences=browser_differences, image_hashes=browser_hashes), negative_cases=negatives,
        api_sha256={name:hashlib.sha256(raw).hexdigest() for name, raw in [('clock', original[4]), ('random', original[3])]})
    out = prepare_output(work(args.report)); out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2) + '\n')
    if args.clean:
        # The native/original comparison is complete; browser divergence stays
        # open. Retain tiny API recordings needed for the next timing diagnosis.
        opened = open_files()
        files = [p for root in (original[0], native[0]) for p in root.glob('step*/*.bin') if not p.is_symlink()]
        if any((p.stat().st_dev, p.stat().st_ino) in opened for p in files):
            raise RuntimeError('Completed league capture still in use')
        for p in files:
            p.unlink()
    print('PASS native: all original clock/RNG inputs, five standings, 20 league images/palettes; negatives:', negatives)
    print('Browser original gate:', report['browser']['original_gate_pass'], '; first RNG divergence:', differences[:1])


if __name__ == '__main__':
    main()
