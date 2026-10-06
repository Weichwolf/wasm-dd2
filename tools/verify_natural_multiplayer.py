#!/usr/bin/env python3
"""Compare two natural multiplayer turns against unchanged original execution.

Every racing indexed pixel and palette is compared literally, with observed
clock returns, independently calculated RNG, actual key transitions, natural
finishes, positive human points and next-round checkpoints. An optional gate
requires living lap-completed drivers. Menu animation,
chronological PCM, physical timing and a complete season remain unproven.
"""
import argparse
import copy
import json
from pathlib import Path
import struct
import tempfile

from artifacts import WORK, open_files, prepare_output
from natural_multiplayer_protocol import ACTIONS, KEYS, NAMES, STARTS, OVERS, TABLES, OVER_STATES, validate_checkpoint
from verify_champ_history import recorded_apis
from verify_normal_arena_history import (STATE, compare_frames, compare_checkpoints,
                                        digest, natural, picture, validate_racing_timeline)
from verify_champ_season import EXE

CODES = dict(Return='Enter', Escape='Escape', Up='ArrowUp', Down='ArrowDown',
             Left='ArrowLeft', Right='ArrowRight', a='KeyA', z='KeyZ', space='Space')


def load(path):
    return json.loads(path.read_text())


def driving_events(meta, events):
    """Bind every real transition to its observed frame/finish, including player."""
    drive = [row for row in events if row['kind'] == 'key' and row['action'] in OVERS]
    if len(drive) != len(meta['driving_inputs']):
        raise ValueError('Unrecorded or extra driving key transitions')
    held, previous = set(), -1
    for transition, event in zip(meta['driving_inputs'], drive):
        owner = OVERS.index(event['action'])
        expected_player = OVER_STATES[owner]['player'] if transition.get('finish') else owner
        if event['player'] != expected_player:
            raise ValueError('Driving action assigned to the wrong hotseat player')
        frame = transition['frame']
        if not previous <= frame < len(meta['race_frames']) or transition['key'] not in ('a', 'z', 'Left', 'Right', 'space'):
            raise ValueError('Invalid driving frame or key')
        boundary = meta['final_races'][OVERS.index(event['action'])] if transition.get('finish') else meta['race_frames'][frame]
        if any(event[k] != boundary[k] for k in (*STATE, 'player', 'clock_calls', 'rng_calls', 'draws', 'pad_polls')):
            raise ValueError('Driving input moved to another frame or hotseat player')
        if (event['key'], event['down']) != (transition['key'], transition['down']):
            raise ValueError('Actual driving key differs from metadata')
        pair = (event['player'], event['key'])
        if transition['down']:
            if pair in held:
                raise ValueError('Duplicate driving key press')
            held.add(pair)
        else:
            if pair not in held:
                raise ValueError('Unmatched driving key release')
            held.remove(pair)
        previous = frame
    if held:
        raise ValueError('Driving keys remain held after natural finishes')


def finish_requirements(meta, points, completed_laps=False):
    if len(meta['final_races']) != 2:
        raise ValueError('Two actual hotseat finishes required')
    for player, finish in enumerate(meta['final_races']):
        natural(finish)
        driver = finish['driver']
        if finish['player'] != OVER_STATES[player]['player'] or (driver['dead'] != 1 and driver['finished_laps'] != 1):
            raise ValueError('Actual destroyed or lap-completed hotseat player required')
        if completed_laps and (driver['finished_laps'] != 1 or driver['dead'] != 0):
            raise ValueError('Both hotseat drivers must survive and finish their laps')
    for step, point in enumerate(points):
        validate_checkpoint(step, point)
    before, result, continued = (points[step]['cars'] for step in (STARTS[0], OVERS[1], STARTS[2]))
    for original, final in zip(before, result):
        if original['name'] != final['name'] or final['values'][0] != original['values'][0] + final['values'][6]:
            raise ValueError('Actual cumulative multiplayer points differ')
    humans = result[:2]
    if [p['name'] for p in humans] != ['A', 'B'] or any(p['values'][6] <= 0 for p in humans):
        raise ValueError('Both actual human drivers must earn positive points')
    if [(p['name'], p['values'][0]) for p in continued] != [(p['name'], p['values'][0]) for p in result]:
        raise ValueError('Continuation lost cumulative points or driver identities')
    return [dict(name=p['name'], race_points=p['values'][6], cumulative_points=p['values'][0]) for p in humans]


def history(root, target, completed_laps=False):
    meta = load(root/'history.json')
    if (meta.get('natural_multiplayer') is not True or meta.get('natural_championship') is not True
            or meta.get('natural_season') or meta['keys'] != KEYS or meta['actions'] != ACTIONS
            or meta['checkpoints'] != NAMES or not meta['acknowledged_keys']
            or len(meta['held_pad_polls']) != len(KEYS) or min(meta['held_pad_polls']) < 1
            or meta['racing_image_format'] != 'indexed-zlib'):
        raise ValueError('Complete acknowledged natural multiplayer history required')
    if target == 'original':
        if meta['exe_modified'] is not False or meta['exe_sha256'] != EXE or meta['input'] != 'real X11 keys':
            raise ValueError('Unchanged original driven through real X11 keys required')
    elif meta['input'] != 'dd2_key_event' or len(meta.get('binary_sha256', '')) != 64 or 'api_inputs' not in meta:
        raise ValueError('Native keyboard bridge and strict API replay required')
    events = [json.loads(line) for line in (root/'events.jsonl').read_text().splitlines()]
    if [e['index'] for e in events] != list(range(1, len(events)+1)):
        raise ValueError('Incomplete chronological API/input observations')
    driving_events(meta, events)
    # The action stored on the event determines which hotseat turn owns a key.
    expected = []
    for action, key in enumerate(ACTIONS, 1):
        if key == 'natural-finish':
            expected.extend((e['key'], e['down'], action) for e in events if e['kind'] == 'key' and e['action'] == action)
        else:
            expected.extend((key, down, action) for down in (True, False))
    if [(e['key'], e['down'], e['action']) for e in events if e['kind'] == 'key'] != expected:
        raise ValueError('Actual menu/driving sequence differs')
    random, ticks = [(root/name).read_bytes() for name in ('random.bin', 'ticks.bin')]
    recorded_apis(meta, events, random, ticks)
    validate_racing_timeline(meta, events, target)
    frames = meta['race_frames']
    if [f['index'] for f in frames] != list(range(len(frames))):
        raise ValueError('Incomplete or reordered chronological racing frames')
    for race, player in ((0, 0), (0, 1), (1, 0)):
        turn = [f for f in frames if (f['race'], f['player']) == (race, player)]
        if not turn or (turn[0]['cf'], turn[0]['ticks']) != (0, 2):
            raise ValueError('Every hotseat turn must start at its first gameplay draw')
    points = [load(root/name/'checkpoint.json') for name in NAMES]
    scores = finish_requirements(meta, points, completed_laps)
    return meta, random, ticks, events, points, scores


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('original', 'native', 'browser', 'report'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--asan', type=Path)
    parser.add_argument('--clean', action='store_true')
    parser.add_argument('--require-completed-laps', action='store_true')
    parser.add_argument('--negative-controls', action='store_true')
    args = parser.parse_args()
    for name in ('original', 'native', 'browser', 'report'):
        path = getattr(args, name).resolve()
        if WORK not in path.parents:
            parser.error('Use /tmp/wasm-dd2/ for captures and reports')
        setattr(args, name, path)
    if args.asan:
        args.asan = args.asan.resolve()
        if WORK not in args.asan.parents:
            parser.error('Use /tmp/wasm-dd2/ for the sanitizer capture')
    prepare_output(args.report)
    om, random, ticks, events, points, scores = history(args.original, 'original', args.require_completed_laps)
    nm, nr, nt, ne, np, ns = history(args.native, 'native', args.require_completed_laps)
    if nr != random or nt != ticks or nm['initial_save_sha256'] != om['initial_save_sha256'] or nm['driving_inputs'] != om['driving_inputs']:
        raise ValueError('Native original API/save/input provenance differs')
    if ns != scores or any(a[k] != b[k] for a, b in zip(om['final_races'], nm['final_races']) for k in (*STATE, 'player', 'driver')):
        raise ValueError('Native natural finish or points differ')
    nav, observer = load(args.browser/'navigation.json'), load(args.browser/'rng-observations.json')
    if (nav.get('natural_multiplayer') is not True or nav['keys'] != KEYS or nav['actions'] != ACTIONS
            or nav['checkpoints'] != NAMES or nav['input'] != 'browser keyboard events'
            or nav['initial_save_sha256'] != om['initial_save_sha256']
            or nav['wasm_sha256'] != observer['layout']['wasm_sha256']
            or nav['final_drivers'] != [f['driver'] for f in om['final_races']]):
        raise ValueError('Browser multiplayer input, binary or finish provenance differs')
    if nav['api_reference'] != dict(clock_calls=om['clock_calls'], computed_rng_calls=om['rng_calls'],
                                    api_sha256=dict(clock=digest(ticks), random=digest(random))):
        raise ValueError('Browser original API streams differ')
    actual_inputs = nav['input_observations']
    original_inputs = [e for e in events if e['kind'] == 'key']
    if [(e['code'], e['down'], e['action']) for e in actual_inputs] != [(CODES[e['key']], e['down'], e['action']) for e in original_inputs]:
        raise ValueError('Browser actual input sequence differs')
    if [(e['frame'], e['finish']) for e in actual_inputs if e['action'] in OVERS] != [(e['frame'], bool(e.get('finish'))) for e in om['driving_inputs']]:
        raise ValueError('Browser driving transitions moved to another presentation')
    seeds = [1, *[after for _, after, _ in struct.iter_unpack('<III', random)]]
    observations = observer['observations']
    if not observations or (observations[0]['count'], observations[0]['seed']) != (0, 1):
        raise ValueError('Browser initial RNG seed/counter required')
    previous = 0
    for row in observations:
        count = row['count']
        if not previous <= count < len(seeds) or row['seed'] != seeds[count]:
            raise ValueError('Actual browser calculated LCG differs')
        previous = count
    bp = [load(args.browser/name/'checkpoint.json') for name in NAMES]
    if finish_requirements(om, bp, args.require_completed_laps) != scores:
        raise ValueError('Browser actual cumulative positive human scores differ')
    for expected, actual in zip(points, bp):
        count = expected['observed']['rng_calls']
        if actual['api_calls'] != dict(clock=expected['observed']['clock_calls'], random=count) or actual['rng'] != dict(count=count, seed=seeds[count]):
            raise ValueError('Browser checkpoint API/RNG extent differs')
    report = dict(scope=__doc__.strip(), pass_=False, original_exe_sha256=EXE,
                  native_sha256=nm['binary_sha256'], wasm_sha256=nav['wasm_sha256'],
                  initial_save_sha256=om['initial_save_sha256'],
                  clock_calls=om['clock_calls'], rng_calls=om['rng_calls'],
                  api_sha256=dict(clock=digest(ticks), random=digest(random)),
                  racing_frames=len(om['race_frames']), human_points=scores,
                  required_completed_laps=args.require_completed_laps,
                  final_drivers=[f['driver'] for f in om['final_races']],
                  chronological_audio='unproven', physical_timing='unproven', targets={})
    captures = [('native', args.native, nm, nm['race_frames'], np),
                ('browser', args.browser, nav, load(args.browser/'race-frames.json'), bp)]
    if args.asan:
        am, ar, at, ae, ap, asc = history(args.asan, 'native', args.require_completed_laps)
        if ar != random or at != ticks or asc != scores or am['driving_inputs'] != om['driving_inputs'] or am['initial_save_sha256'] != om['initial_save_sha256']:
            raise ValueError('Sanitized native API/input/score provenance differs')
        if any(a[k] != b[k] for a, b in zip(om['final_races'], am['final_races']) for k in (*STATE, 'player', 'driver')):
            raise ValueError('Sanitized native actual finish differs')
        log = (args.asan.parent/'gdb.log').read_text()
        if any(message in log for message in ('ERROR: AddressSanitizer', 'ERROR: LeakSanitizer', 'runtime error:')):
            raise ValueError('Native sanitizer diagnostic')
        captures.append(('native-asan', args.asan, am, am['race_frames'], ap))
    for target, root, meta, frames, target_points in captures:
        offset = meta['race_frames'][0]['pad_polls']-om['race_frames'][0]['pad_polls'] if target != 'browser' else 0
        if offset not in (-1, 0, 1):
            raise ValueError('Unexpected native debugger poll observation offset')
        differences, hashes = compare_frames(args.original, root, om['race_frames'], frames, seeds, target == 'browser', pad_offset=offset)
        differences.extend(dict(index=i, region='player') for i, (a, b) in enumerate(zip(om['race_frames'], frames)) if a['player'] != b['player'])
        point_diff, images = compare_checkpoints(args.original, root, NAMES, [NAMES[i] for i in (*STARTS, *TABLES)])
        differences.extend(point_diff)
        differences.extend(dict(checkpoint=name, region='player-state')
                           for name, a, b in zip(NAMES, points, target_points)
                           if any(a[k] != b[k] for k in ('player', 'multi_count')))
        report['targets'][target] = dict(pass_=not differences, differences=differences,
                                        binary_sha256=meta['wasm_sha256'] if target == 'browser' else meta['binary_sha256'],
                                        racing_image_hashes=hashes, checkpoint_image_hashes=images)
    if args.negative_controls:
        controls = []
        for name, mutate in [
            ('invalid-human-finish', lambda m, p: m['final_races'][0]['driver'].update(dead=0, finished_laps=0)),
            ('unfinished-human', lambda m, p: m['final_races'][1].update(finished=0)),
            ('wrong-player', lambda m, p: m['final_races'][1].update(player=0)),
            ('zero-human-score', lambda m, p: p[OVERS[1]]['cars'][0]['values'].__setitem__(6, 0)),
            ('lost-cumulative-score', lambda m, p: p[STARTS[2]]['cars'][0]['values'].__setitem__(0, -1)),
            ('wrong-next-round', lambda m, p: p[STARTS[2]].update(race=0)),
        ]:
            damaged, changed = copy.deepcopy(om), copy.deepcopy(points)
            mutate(damaged, changed)
            try:
                finish_requirements(damaged, changed, args.require_completed_laps)
            except ValueError:
                controls.append(name)
            else:
                raise ValueError('Negative control accepted: '+name)
        report['negative_controls'] = controls
        for name, modify in [('changed-rng', random), ('changed-clock', ticks)]:
            damaged = bytearray(modify)
            damaged[-1] ^= 1
            try:
                recorded_apis(om, events, bytes(damaged) if name == 'changed-rng' else random,
                              bytes(damaged) if name == 'changed-clock' else ticks)
            except ValueError:
                controls.append(name)
            else:
                raise ValueError('Negative API control accepted: '+name)
        changed = copy.deepcopy(om['race_frames'])
        changed[0]['index'] = 1
        try:
            compare_frames(args.original, args.native, om['race_frames'], changed, seeds)
        except ValueError:
            controls.append('reordered-racing-frame')
        else:
            raise ValueError('Reordered racing frame accepted')
        # Exercise the literal comparison with internally consistent altered
        # capture hashes. The changed byte must still disagree with original.
        with tempfile.TemporaryDirectory(prefix='multiplayer-pixel-control-', dir=WORK) as tmp:
            root = Path(tmp)
            first = om['race_frames'][0]
            for changed_suffix, changed_key in [('.bin', 'framebuffer_sha256'), ('.pal', 'palette_sha256')]:
                frame = copy.deepcopy(first)
                for suffix, size, key in [('.bin', 307200, 'framebuffer_sha256'), ('.pal', 1024, 'palette_sha256')]:
                    raw = bytearray(picture(args.original, first['prefix'], suffix, size))
                    if suffix == changed_suffix:
                        raw[0] ^= 1
                    (root/(first['prefix']+suffix)).write_bytes(raw)
                    frame[key] = digest(raw)
                offset = 0
                failed, _ = compare_frames(args.original, root, [first], [frame], seeds, pad_offset=offset)
                if not any(row['region'] == changed_suffix for row in failed):
                    raise ValueError('Altered racing raster accepted: '+changed_suffix)
                controls.append('changed-racing-'+changed_key)
        damaged = copy.deepcopy(om)
        damaged['driving_inputs'][0]['frame'] += 1
        try:
            driving_events(damaged, events)
        except ValueError:
            controls.append('moved-driving-transition')
        else:
            raise ValueError('Moved original driving key accepted')
        if not args.require_completed_laps and any(f['driver']['dead'] for f in om['final_races']):
            try:
                finish_requirements(om, points, completed_laps=True)
            except ValueError:
                controls.append('destroyed-trace-fails-regular-finish-gate')
            else:
                raise ValueError('Destroyed driver accepted by regular-finish gate')
    report['pass_'] = all(t['pass_'] for t in report['targets'].values())
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    if not report['pass_']:
        raise SystemExit('Natural multiplayer comparison differs; report: '+str(args.report))
    if args.clean:
        opened = open_files()
        for root in [args.original, args.native, args.browser, *([args.asan] if args.asan else [])]:
            storage = root/'racing-archive'
            archived = [*storage.glob('pack*.zst'), *storage.glob('lock')] if storage.exists() else []
            for path in [*root.glob('race*.bin.z'), *root.glob('race*.pal'), *root.glob('step*/framebuf.bin'), *root.glob('step*/palette.bin'), *archived]:
                st = path.stat()
                if (st.st_dev, st.st_ino) in opened:
                    raise RuntimeError('Comparison capture is still open: '+str(path))
                path.unlink()
            if storage.exists(): storage.rmdir()
    print('Natural multiplayer original/native/browser comparison PASS:', scores)


if __name__ == '__main__':
    main()
