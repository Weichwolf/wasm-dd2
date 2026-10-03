#!/usr/bin/env python3
"""Compare a naturally completed Total Destruction arena with the running original.

Exact indexed racing Draw_All-entry/platform pictures, result images, game states and
calculated RNG at recorded original clock inputs. Browser canvas conversion is
checked by the recorder. Loading/fades, physical clocks and PCM are excluded;
this is one input trace, not whole-game or all-mode acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from artifacts import WORK, open_files, prepare_output
from verify_champ_history import recorded_apis
from verify_champ_season import EXE, NORMAL_ARENA_KEYS

STATE = ('level', 'cf', 'ticks', 'countdown', 'frame_skip', 'quit', 'race',
         'season', 'finished', 'retired', 'damage', 'poly_list')
POINT = ('level', 'cf', 'ticks', 'quit', 'poly_list', 'race_type', 'race_mode',
         'race', 'season', 'num_races', 'division', 'stats', 'actual_season',
         'retire_confirm', 'cars', 'rows')
NAMES = ['step00-boot', *[f'step{i:02d}-{key}' for i, key in enumerate(NORMAL_ARENA_KEYS, 1)],
         'step10-natural-finish']
INPUTS = [(key, down, i) for i, key in enumerate(NORMAL_ARENA_KEYS, 1)
          for down in (True, False)] + [(key, down, 10) for down in (True, False)
                                      for key in ('Up', 'Right')]
CODES = {'Return': 'Enter', 'Right': 'ArrowRight', 'Down': 'ArrowDown', 'Up': 'ArrowUp'}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def load(path):
    return json.loads(path.read_text())


def natural(state):
    if state['quit'] != 1 or state['retired'] != 0 or state['finished'] <= 14:
        raise ValueError('Natural completion without Retire required')


def history(root, target):
    meta = load(root / 'history.json')
    if (meta.get('normal_arena') is not True or meta.get('natural_finish') is not True
            or meta['keys'] != NORMAL_ARENA_KEYS or meta['checkpoints'] != NAMES
            or meta.get('acknowledged_keys') is not True
            or len(meta['held_pad_polls']) != 9 or min(meta['held_pad_polls']) < 1):
        raise ValueError('Complete normal arena input acknowledgements required')
    if target == 'original':
        if meta['exe_modified'] is not False or meta['exe_sha256'] != EXE or meta['input'] != 'real X11 keys':
            raise ValueError('Actual unmodified original X11 capture required')
    elif meta['input'] != 'dd2_key_event' or len(meta.get('binary_sha256', '')) != 64 or 'api_inputs' not in meta:
        raise ValueError('Actual native strict calculated API replay required')
    events = [json.loads(line) for line in (root / 'events.jsonl').read_text().splitlines()]
    if [row['index'] for row in events] != list(range(1, len(events) + 1)):
        raise ValueError('Incomplete/reordered API history')
    actions = [(row['key'], row['down'], row['action']) for row in events if row['kind'] == 'key']
    if actions != INPUTS:
        raise ValueError('Normal menu plus held Up/Right input trace required')
    random, ticks = [(root / name).read_bytes() for name in ('random.bin', 'ticks.bin')]
    recorded_apis(meta, events, random, ticks)
    natural(meta['final_race'])
    frames = meta['race_frames']
    if len(frames) < 2 or [frame['index'] for frame in frames] != list(range(len(frames))):
        raise ValueError('Complete ordered racing frame stream required')
    if frames[0]['cf'] != 0 or frames[0]['ticks'] != 2 or any(frame['retired'] for frame in frames):
        raise ValueError('Stream must start at actual racing draw and never Retire')
    return meta, random, ticks


def exact_bytes(a, b, size):
    if len(a) != size or len(b) != size:
        raise ValueError('Incomplete framebuffer/palette')
    return a == b


def compare_frames(original, target, reference, frames, seeds, browser=False):
    if len(frames) != len(reference):
        raise ValueError('Racing draw extent differs')
    differences, hashes = [], []
    for index, (a, b) in enumerate(zip(reference, frames)):
        if b['index'] != index or b['prefix'] != a['prefix']:
            raise ValueError('Reordered racing frames')
        state = [key for key in STATE if a[key] != b[key]]
        if browser:
            if b['canvas_mismatches'] != 0:
                state.append('canvas_mismatches')
            if b['api_calls'] != {'clock': a['clock_calls'], 'random': a['rng_calls']}:
                state.append('api_calls')
            if b['rng'] != {'count': a['rng_calls'], 'seed': seeds[a['rng_calls']]}:
                state.append('calculated_rng')
        else:
            state += [key for key in ('clock_calls', 'rng_calls', 'pad_polls', 'draws') if a[key] != b[key]]
        if state:
            differences.append(dict(index=index, region='state', fields=state))
        record = dict(index=index, original={}, actual={})
        for suffix, size, key in (('.bin', 307200, 'framebuffer_sha256'), ('.pal', 1024, 'palette_sha256')):
            left = (original / (a['prefix'] + suffix)).read_bytes()
            right = (target / (b['prefix'] + suffix)).read_bytes()
            if digest(left) != a[key] or (not browser and digest(right) != b[key]):
                raise ValueError('Recorded racing image hash differs')
            if not exact_bytes(left, right, size):
                differences.append(dict(index=index, region=suffix, differing_bytes=sum(x != y for x, y in zip(left, right))))
            record['original'][suffix] = digest(left)
            record['actual'][suffix] = digest(right)
        hashes.append(record)
    return differences, hashes


def compare_checkpoints(original, actual):
    differences, images = [], []
    for name in NAMES:
        a, b = [load(root / name / 'checkpoint.json') for root in (original, actual)]
        fields = [key for key in POINT if a[key] != b[key]]
        if fields:
            differences.append(dict(checkpoint=name, region='state', fields=fields))
        if name in NAMES[-2:]:
            for filename, size in (('framebuf.bin', 307200), ('palette.bin', 1024)):
                left, right = [(root / name / filename).read_bytes() for root in (original, actual)]
                if not exact_bytes(left, right, size):
                    differences.append(dict(checkpoint=name, region=filename,
                                            differing_bytes=sum(x != y for x, y in zip(left, right))))
                images.append(dict(checkpoint=name, region=filename, original=digest(left), actual=digest(right)))
    return differences, images


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('original', 'native', 'browser', 'report'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--before-report', type=Path, help='retained failing pre-fix comparison with identical reference images/API inputs')
    parser.add_argument('--clean', action='store_true')
    args = parser.parse_args()
    for name in ('original', 'native', 'browser', 'report'):
        path = getattr(args, name).resolve()
        if WORK not in path.parents:
            parser.error('All captures and reports must be under /tmp/wasm-dd2/')
        setattr(args, name, path)
    prepare_output(args.report)
    om, random, ticks = history(args.original, 'original')
    nm, nr, nt = history(args.native, 'native')
    if nr != random or nt != ticks or nm['initial_save_sha256'] != om['initial_save_sha256']:
        raise ValueError('Native actual API inputs/save differ')
    if any(nm['final_race'][key] != om['final_race'][key] for key in STATE):
        raise ValueError('Native final race state differs')
    nav = load(args.browser / 'navigation.json')
    observer = load(args.browser / 'rng-observations.json')
    api = {'clock_calls': om['clock_calls'], 'computed_rng_calls': om['rng_calls'],
           'api_sha256': {'clock': digest(ticks), 'random': digest(random)}}
    actions = [(row['code'], row['down'], row['action']) for row in nav['input_observations']]
    if (nav.get('normal_arena') is not True or nav['keys'] != NORMAL_ARENA_KEYS
            or nav['checkpoints'] != NAMES or nav['input'] != 'browser keyboard events'
            or nav['initial_save_sha256'] != om['initial_save_sha256'] or nav['api_reference'] != api
            or nav['wasm_sha256'] != observer['layout']['wasm_sha256']
            or actions != [(CODES[key], down, action) for key, down, action in INPUTS]):
        raise ValueError('Actual production browser API/input/save provenance differs')
    seeds = [1, *[row[1] for row in struct.iter_unpack('<III', random)]]
    observations = observer['observations']
    if not observations or (observations[0]['count'], observations[0]['seed']) != (0, 1):
        raise ValueError('Browser natural boot seed must be observed')
    previous = 0
    for row in observations:
        count = row['count']
        if count < previous or count >= len(seeds) or row['seed'] != seeds[count]:
            raise ValueError('Browser calculated LCG observation differs')
        previous = count
    end = load(args.browser / NAMES[-1] / 'checkpoint.json')
    natural(end)
    if end['api_calls'] != {'clock': om['clock_calls'], 'random': om['rng_calls']} or end['rng'] != {'count': om['rng_calls'], 'seed': seeds[-1]}:
        raise ValueError('Browser final API consumption differs')
    if end['level'] != 0 or end['poly_list'] != 0x46a468:
        raise ValueError('Practice Over result screen required')
    report = dict(scope=__doc__.strip(), original_exe_sha256=EXE,
                  initial_save_sha256=om['initial_save_sha256'], native_sha256=nm['binary_sha256'],
                  wasm_sha256=nav['wasm_sha256'], api_inputs=api, racing_frames=len(om['race_frames']),
                  natural_finish=True, retired=False, targets={})
    for target, root, frames in [('native', args.native, nm['race_frames']),
                                  ('browser', args.browser, load(args.browser / 'race-frames.json'))]:
        differences, hashes = compare_frames(args.original, root, om['race_frames'], frames, seeds, target == 'browser')
        point_differences, point_images = compare_checkpoints(args.original, root)
        differences.extend(point_differences)
        report['targets'][target] = dict(pass_=not differences, differences=differences,
                                         racing_image_hashes=hashes, checkpoint_image_hashes=point_images)
    report['pass_'] = all(target['pass_'] for target in report['targets'].values())
    if args.before_report:
        before_path = args.before_report.resolve()
        if WORK not in before_path.parents:
            raise ValueError('Before report must be under /tmp/wasm-dd2/')
        before = load(before_path)
        for key in ('original_exe_sha256', 'initial_save_sha256', 'api_inputs', 'racing_frames'):
            if before[key] != report[key]:
                raise ValueError('Failed baseline has different original provenance')
        if before['pass_'] is not False:
            raise ValueError('Baseline must have failed')
        rejected = {}
        for target, binary in (('native', 'native_sha256'), ('browser', 'wasm_sha256')):
            old = before['targets'][target]
            new = report['targets'][target]
            if (old['pass_'] is not False or before[binary] == report[binary]
                    or [row['original'] for row in old['racing_image_hashes']]
                    != [row['original'] for row in new['racing_image_hashes']]):
                raise ValueError('Old target not rejected against the same actual original pictures')
            rejected[target] = dict(binary_sha256=before[binary], differences=old['differences'])
        report['before_fix'] = dict(report=str(before_path), sha256=digest(before_path.read_bytes()), targets=rejected)
    negatives = {}
    sample = (args.original / (om['race_frames'][1]['prefix'] + '.bin')).read_bytes()
    changed = bytes([sample[0] ^ 1]) + sample[1:]
    negatives['one_pixel'] = not exact_bytes(sample, changed, 307200)
    palette = (args.original / (om['race_frames'][1]['prefix'] + '.pal')).read_bytes()
    negatives['one_palette_byte'] = not exact_bytes(palette, bytes([palette[0] ^ 1]) + palette[1:], 1024)
    for key, state in [('retirement', dict(end, retired=1)), ('unfinished', dict(end, finished=14))]:
        try:
            natural(state)
        except ValueError:
            negatives[key] = True
        else:
            negatives[key] = False
    events = [json.loads(line) for line in (args.original / 'events.jsonl').read_text().splitlines()]
    changed_random = bytearray(random)
    changed_random[8] ^= 1
    for key, corrupted in [('changed_rng_return', changed_random), ('truncated_rng', random[:-1])]:
        try:
            recorded_apis(om, events, corrupted, ticks)
        except ValueError:
            negatives[key] = True
        else:
            negatives[key] = False
    report['negative_cases'] = negatives
    if not all(negatives.values()):
        raise ValueError('Comparator accepted invalid evidence')
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS' if report['pass_'] else 'FAIL', 'normal arena:', report['racing_frames'], 'frames;',
          {target: len(value['differences']) for target, value in report['targets'].items()})
    if args.clean and report['pass_']:
        opened = open_files()
        files = []
        for root in (args.original, args.native, args.browser):
            for pattern in ('race*.bin', 'race*.pal', 'step*/framebuf.bin', 'step*/palette.bin'):
                for path in root.glob(pattern):
                    stat = path.stat()
                    if path.is_symlink() or (stat.st_dev, stat.st_ino) in opened:
                        raise RuntimeError('Do not remove captures used by a running process')
                    files.append(path)
        for path in files:
            path.unlink()
    return 0 if report['pass_'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
