#!/usr/bin/env python3
"""Compare actual configuration/card UI, volume states and selected rendered cycles.

The shared scenario uses only genuine keyboard input and normal process startup.
Observers read memory/files; original GDB breakpoints are hardware only. Each
selected cycle compares every indexed pixel and palette byte over all 64 phases.
Chronological video/PCM and other settings/card edge cases remain separate.
"""
import argparse
import copy
import fcntl
import json
from pathlib import Path
import shutil
import struct
import subprocess
import time

from artifacts import WORK, check_space, open_files, prepare_output
from verify_configuration_persistence import (ROOT, ConfigUI, OriginalUI, settings,
                                             observe_card, original_args, run_original, WINE_WORK,
                                             EXE_SHA256, digest, require)

PLAN_FILE = ROOT/'tools/configuration_card_ui.json'
PLAN = json.loads(PLAN_FILE.read_text())
POINTS = [action for session in PLAN['sessions'] for action in session if 'checkpoint' in action]


def screen_text(ui, address):
    # Card name entry points to the original routine's stack-local text buffer.
    # Read the string, without comparing target-specific pointer values.
    pointer = struct.unpack('<I', ui.read(address, 4))[0]
    return ui.read(pointer, 80).split(b'\0', 1)[0].decode('ascii') if pointer else ''


def file_ui(ui):
    return dict(caption=screen_text(ui, 0x46725c), detail=screen_text(ui, 0x4672c0),
                selection=screen_text(ui, 0x467284),
                name_cursor=list(struct.unpack('<hh', ui.read(0x4673dc, 4))),
                ring=list(struct.unpack('<hh', ui.read(0x467198, 4))))


def rendered_cycle(ui, action, original):
    name = action['checkpoint']
    directory = ui.output/name
    directory.mkdir()
    entry = 0x420c9c if original else ui.table['Draw_All']
    pid = ui.pid if original else ui.process.pid
    wanted = None
    if action.get('card_highlight') and getattr(ui, 'reference', None):
        source = json.loads((ui.reference/'report.json').read_text())
        validate(source)
        row = next(row for row in source['checkpoints'] if row['name'] == name)
        metadata = json.loads((Path(row['cycle'])/'cycle.json').read_text())
        wanted = [(frame['phase'], frame['card_phase']) for frame in metadata['frames']]
    commands = ['set pagination off', 'set auto-solib-add off', f'attach {pid}', 'python',
                'import sys', f'sys.path.insert(0, {str(ROOT / "tools")!r})',
                'from menu_cycle_gdb import record_cycle',
                f'record_cycle({str(directory)!r}, 64, {entry}, hardware=True, wanted_pairs={wanted!r})',
                'end', 'detach', 'quit']
    script = directory/'cycle.gdb'; script.write_text('\n'.join(commands)+'\n')
    with (directory/'cycle.log').open('wb') as log:
        subprocess.run(['gdb','--nx','-q','-batch','-x',str(script)], env=ui.env,
                       stdout=log, stderr=subprocess.STDOUT, check=True, timeout=420 if wanted else 40)
    return str(directory/'cycle')


def snapshot(ui, action, report, original):
    ui.settled()
    name = action['checkpoint']
    row = dict(name=name, settings=settings(ui), working=ui.integer(0x93fd20),
               master=ui.integer(0x462d84), menu=ui.integer(0x940010),
               file_mode=ui.integer(0x93a318), file_slot=ui.integer(0x774680),
               prompt=ui.text(0x4672ac), card=observe_card(ui, name))
    if action.get('file_ui'):
        row['prompt'] = screen_text(ui, 0x4672ac)
        row['file_ui'] = file_ui(ui)
    raw = Path(row['card']['file']).read_bytes()
    for slot in row['card']['slots']:
        slot['sound'] = struct.unpack_from('<h', raw, 0x2000+slot['index']*0x2000+16)[0]
    check_point(row, action)
    if action.get('cycle'):
        row['cycle'] = rendered_cycle(ui, action, original)
    report['checkpoints'].append(row)
    print('Configuration checkpoint', name, 'sound', row['settings']['sound'],
          'working', row['working'], 'master', row['master'], flush=True)


def check_point(row, action):
    require(row['name'] == action['checkpoint'], 'wrong configuration checkpoint')
    require(row['settings']['sound'] == action['sound'], 'stored volume differs at '+row['name'])
    for field in ['working', 'master']:
        if field in action:
            require(row[field] == action[field], field+' volume differs at '+row['name'])
    require([(slot['index'], slot['name'], slot['sound']) for slot in row['card']['slots']] ==
            [tuple(slot) for slot in action['cards']], 'configuration directory differs at '+row['name'])
    require(row['card']['engine_matches_file'] and row['card']['bytes'] == 0x20000,
            'complete disk/engine card required')
    if action.get('prompt'):
        require(action['prompt'] in row['prompt'], 'actual confirmation prompt differs')
    if 'text' in action:
        require(row['prompt'] == action['text'], 'displayed name differs at '+row['name'])
    for field, wanted in action.get('expected_ui', {}).items():
        require(row['file_ui'][field] == wanted, 'file UI '+field+' differs at '+row['name'])
    if 'file_slot' in action:
        require(row['file_slot'] == action['file_slot'], 'logical/physical slot differs at '+row['name'])


def drive(ui, actions, report, original):
    def select(slot, confirm=True):
        ui.key('Return')
        ui.wait(lambda: 'Select' in ui.text(0x46725c) and ui.integer(0x774680) >= 0)
        for code in ['Down']*(slot//3)+['Right']*(slot%3): ui.key(code)
        require(ui.integer(0x774680) == slot, 'logical card selection differs')
        if confirm: ui.key('Return')

    for action in actions:
        for code in action.get('keys', []):
            ui.key(code)
        if 'menu' in action:
            ui.wait(lambda: ui.integer(0x940010) == action['menu'])
            ui.settled()
        if 'limit' in action:
            require(ui.integer(0x940010) == 0x46898c, 'actual volume dialog required')
            ui.edge(action['limit'], True)
            try:
                ui.wait(lambda: ui.integer(0x93fd20) == action['value'])
                # Keep holding past the limit, checking saturation rather than
                # guessing a repeat count from elapsed time or physical speed.
                time.sleep(0.3)
                require(ui.integer(0x93fd20) == action['value'], 'volume did not clamp at endpoint')
            finally:
                ui.edge(action['limit'], False)
            ui.wait(lambda: ui.controls() == 0)
        if 'select' in action:
            select(action['select'])
        if 'choose' in action:
            select(action['choose'], False)
        if 'save' in action:
            slot, letter = action['save']
            select(slot); ui.enter_name(letter)
            ui.wait(lambda: (ui.game/'SaveGames').read_bytes()[slot*0x200] == 1)
        if 'name' in action:
            ui.enter_name(action['name'], replace=action.get('replace', False))
        if 'checkpoint' in action:
            snapshot(ui, action, report, original)


def capture(args):
    out = prepare_output(args.output)
    require(WORK in out.parents and not out.exists(), 'fresh output under /tmp/wasm-dd2 required')
    out.mkdir(parents=True)
    game = out/'game'; game.mkdir()
    for asset in (ROOT/'DestructionDerby2').iterdir():
        if asset.name == 'SaveGames': shutil.copyfile(asset, game/asset.name)
        else: (game/asset.name).symlink_to(asset.resolve(), target_is_directory=asset.is_dir())
    report = dict(scope=PLAN['scope'], target=args.target, pass_=False, engine_state_writes=False,
                  plan_sha256=digest(PLAN_FILE.read_bytes()), checkpoints=[],
                  initial_save_sha256=digest((game/'SaveGames').read_bytes()),
                  binary_sha256=EXE_SHA256 if args.target == 'original' else digest(args.binary.read_bytes()))
    ui = display = None
    try:
        if args.target == 'original':
            with (WINE_WORK/'capture.lock').open('w') as lock:
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
                for index, actions in enumerate(PLAN['sessions'], 1):
                    startup = out/f'startup-{index}'; startup.mkdir()
                    def driver(pid, output, env, deadline, rundir):
                        ui = OriginalUI(pid, rundir, output, env, deadline)
                        try:
                            drive(ui, actions, report, True)
                            shutil.copyfile(rundir/'SaveGames', game/'SaveGames')
                        finally: ui.stop()
                    options = original_args(); options.timeout = PLAN.get('session_timeout', options.timeout)
                    run_original(game, startup, options, on_menu=driver)
        else:
            with (out/'xvfb.log').open('wb') as log:
                display = subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','1280x1024x24'],
                                           stdout=subprocess.PIPE, stderr=log)
                number = display.stdout.readline().decode().strip()
                require(bool(number), 'Xvfb failed to start')
                for index, actions in enumerate(PLAN['sessions'], 1):
                    startup = out/f'startup-{index}'; startup.mkdir()
                    ui = ConfigUI(args.binary.resolve(), game, startup, ':'+number, index, 1800)
                    ui.reference = args.reference
                    ui.boot(); drive(ui, actions, report, False); ui.stop(); ui = None
            for log in out.glob('startup-*/game-*.log'):
                text = log.read_text(errors='replace')
                require('AddressSanitizer' not in text and 'runtime error:' not in text,
                        'native sanitizer diagnostic in '+str(log))
        report['pass_'] = True
    finally:
        if ui: ui.stop()
        if display: display.terminate(); display.wait(timeout=5)
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        check_space(out)
    print('Actual configuration/card UI capture: PASS')


def validate(data):
    require(data['pass_'] and data['engine_state_writes'] is False and
            data['plan_sha256'] == digest(PLAN_FILE.read_bytes()), 'completed capture of the current scenario required')
    require(len(data['checkpoints']) == len(POINTS), 'incomplete configuration scenario')
    for row, expected in zip(data['checkpoints'], POINTS):
        check_point(row, expected)
    if data['target'] == 'browser':
        events = data.get('trusted_keyboard_events', [])
        require(events and all(event['trusted'] for event in events), 'real browser keyboard events required')


def match_state(wanted, actual):
    left = {key: value for key, value in wanted.items() if key not in ['card','cycle']}
    right = {key: value for key, value in actual.items() if key not in ['card','cycle']}
    require(left == right, 'observed configuration/menu state differs at '+wanted['name'])
    require({key:value for key,value in wanted['card'].items() if key != 'file'} ==
            {key:value for key,value in actual['card'].items() if key != 'file'}, 'card metadata differs')


def cycle_frames(directory, browser):
    directory = Path(directory)
    meta = json.loads((directory/'cycle.json').read_text())
    require(meta['stage'] == ('browser platform present' if browser else 'Draw_All entry / pending presentation'),
            'incorrect frame capture phase')
    sequence = meta['frames']
    require(len(sequence) == 64 and {row['phase'] for row in sequence} == set(range(64)),
            'all 64 highlight phases required')
    result = {}
    for row in sequence:
        require(row['level'] == 0 and row['cf'] == 0, 'settled frontend frames required')
        if browser: require(row['canvas_mismatches'] == 0, 'browser canvas differs from engine indexed pixels')
        pair = [(directory/(row['prefix']+'-'+region+'.bin')).read_bytes() for region in ['framebuf','palette']]
        require(len(pair[0]) == 307200 and len(pair[1]) == 1024, 'incomplete menu framebuffer/palette')
        result[row['phase']] = (row, pair)
    return result


def match_frame(reference_state, reference_pair, observed_state, observed_pair, card_highlight):
    fields = ['poly_list','sound_volume','working_sound_volume','master_sfx_volume']
    if card_highlight: fields.append('card_phase')
    for field in fields:
        require(reference_state[field] == observed_state[field], 'cycle state differs: '+field)
    require(reference_pair == observed_pair, 'menu framebuffer/palette differs')


def compare(args):
    out = prepare_output(args.report)
    require(WORK in out.parents and not out.exists(), 'fresh report under /tmp/wasm-dd2 required')
    original = json.loads((args.original/'report.json').read_text()); validate(original)
    require(original['binary_sha256'] == EXE_SHA256, 'supported unmodified original required')
    results, negatives = {}, []
    directories = [('native', args.native), ('browser', args.browser)]
    if args.asan: directories.append(('asan', args.asan))
    for target, directory in directories:
        actual = json.loads((directory/'report.json').read_text()); validate(actual)
        require(actual['target'] == ('browser' if target == 'browser' else 'native'), 'capture target differs')
        require(actual['initial_save_sha256'] == original['initial_save_sha256'], 'initial card input differs')
        frames = []
        for wanted, row, action in zip(original['checkpoints'], actual['checkpoints'], POINTS):
            match_state(wanted, row)
            left = Path(wanted['card']['file']).read_bytes(); right = Path(row['card']['file']).read_bytes()
            require(len(left) == 0x20000 and digest(left) == wanted['card']['sha256'] and
                    digest(right) == row['card']['sha256'] and left == right, 'complete card bytes differ')
            if 'cycle' in wanted:
                reference = cycle_frames(wanted['cycle'], False); candidate = cycle_frames(row['cycle'], target == 'browser')
                for phase, (reference_state, reference_pair) in reference.items():
                    observed_state, observed_pair = candidate[phase]
                    try:
                        match_frame(reference_state, reference_pair, observed_state, observed_pair, action.get('card_highlight'))
                    except RuntimeError as error:
                        raise RuntimeError(f'{target} at {row["name"]} phase {phase}: {error}') from error
                    frames.append(dict(checkpoint=row['name'], phase=phase,
                                       card_phase=reference_state['card_phase'] if action.get('card_highlight') else None,
                                       framebuffer_sha256=digest(reference_pair[0]), palette_sha256=digest(reference_pair[1])))
                # The same exact comparison must reject pixel/palette corruption,
                # and mismatched independent card phases at both confirmations.
                first_phase = next(iter(reference))
                state, pair = reference[first_phase]
                observed_state, observed_pair = candidate[first_phase]
                for index, label in [(0,'pixel'), (1,'palette')]:
                    corrupted = observed_pair[:]
                    changed = bytearray(corrupted[index]); changed[0] ^= 1; corrupted[index] = bytes(changed)
                    try: match_frame(state, pair, observed_state, corrupted, action.get('card_highlight'))
                    except RuntimeError: negatives.append(dict(target=target, case=row['name']+'-'+label, rejected=True))
                    else: raise RuntimeError('Verifier accepted changed '+label)
                if action.get('card_highlight'):
                    changed = dict(observed_state, card_phase=(observed_state['card_phase']+20)%255)
                    try: match_frame(state, pair, changed, observed_pair, True)
                    except RuntimeError: negatives.append(dict(target=target, case=row['name']+'-card-phase', rejected=True))
                    else: raise RuntimeError('Verifier accepted changed card phase')
        transitions = []
        for index in range(1, len(original['checkpoints'])):
            before, after = original['checkpoints'][index-1:index+1]
            previous, current = actual['checkpoints'][index-1:index+1]
            changed = before['card']['sha256'] != after['card']['sha256']
            require(changed == (previous['card']['sha256'] != current['card']['sha256']), 'card write/refusal transition differs')
            transitions.append(dict(before=before['name'], after=after['name'], changed=changed,
                                    original_sha256=after['card']['sha256'], pass_=True))
        results[target] = dict(capture=actual, frames=frames, card_transitions=transitions, pass_=True)
        cases = PLAN.get('negative_states')
        if cases is None: cases = [dict(case=label, checkpoint=POINTS[index]['checkpoint'], field=field)
                    for label, index, field in [('cancelled-volume',2,'settings'), ('master-volume',1,'master'),
                                    ('loaded-first',13,'settings'), ('directory',8,'card'), ('menu',0,'menu')]]
        for case in cases:
            label, field = case['case'], case['field']
            index = next(i for i, row in enumerate(actual['checkpoints']) if row['name'] == case['checkpoint'])
            damaged = copy.deepcopy(actual['checkpoints'][index])
            if field == 'settings': damaged['settings']['sound'] ^= 1
            elif field == 'card': damaged['card']['sha256'] = '0'*64
            elif field == 'prompt': damaged[field] += 'X'
            elif field == 'file_ui': damaged[field]['name_cursor'][0] ^= 1
            else: damaged[field] ^= 1
            try: match_state(original['checkpoints'][index], damaged)
            except RuntimeError: negatives.append(dict(target=target, case=label, rejected=True))
            else: raise RuntimeError('Verifier accepted changed '+label)
    report = dict(scope=PLAN['scope'], pass_=True, original=original, targets=results, negative_cases=negatives)
    out.write_text(json.dumps(report, indent=2)+'\n')
    if args.clean:
        opened = open_files()
        for data in [original, *[record['capture'] for record in results.values()]]:
            for row in data['checkpoints']:
                files = [Path(row['card']['file'])]
                if 'cycle' in row: files += list(Path(row['cycle']).glob('*.bin'))
                for file in files:
                    require(WORK in file.resolve().parents and not file.is_symlink(), 'unexpected raw verification path')
                    stat = file.stat(); require((stat.st_dev,stat.st_ino) not in opened, 'raw output is still open')
                    file.unlink()
    print('Original volume/card UI and selected menu images: PASS;',
          len(POINTS), 'checkpoints;',
          ', '.join(f'{name}: {len(result["frames"])} framebuffer/palette pairs' for name, result in results.items()),
          ';', len(negatives), 'negative cases rejected')


def main():
    global PLAN_FILE, PLAN, POINTS
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='operation', required=True)
    cap = sub.add_parser('capture')
    cap.add_argument('--target', choices=['original','native'], required=True)
    cap.add_argument('--binary', type=Path, default=Path('/tmp/dd2_native'))
    cap.add_argument('--output', type=Path, required=True)
    cap.add_argument('--reference', type=Path, help='completed original capture; selects observed card/highlight phase pairs only')
    cap.add_argument('--scenario', type=Path, default=PLAN_FILE)
    cmp = sub.add_parser('compare')
    for arg in ['original','native','browser','report']: cmp.add_argument('--'+arg, type=Path, required=True)
    cmp.add_argument('--asan', type=Path, help='optional actual native AddressSanitizer capture')
    cmp.add_argument('--clean', action='store_true')
    cmp.add_argument('--scenario', type=Path, default=PLAN_FILE)
    args = parser.parse_args()
    PLAN_FILE = args.scenario.resolve(); PLAN = json.loads(PLAN_FILE.read_text())
    POINTS = [action for session in PLAN['sessions'] for action in session if 'checkpoint' in action]
    require(len({row['checkpoint'] for row in POINTS}) == len(POINTS), 'distinct checkpoints required')
    capture(args) if args.operation == 'capture' else compare(args)


if __name__ == '__main__': main()
