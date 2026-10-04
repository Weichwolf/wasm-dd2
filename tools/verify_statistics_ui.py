#!/usr/bin/env python3
"""Observe persisted, nonempty season statistics through actual frontend input.

The producer loads an original-generated championship, retires the remaining
races and saves configuration through the original menus. No engine state is
written. This is a bounded statistics/persistence check, not racing A/V parity.
"""
import argparse
import copy
import json
from pathlib import Path
import struct

from artifacts import WORK, check_space, open_files, prepare_output
from verify_championship_save import fixture as championship_fixture, setup, execute, snapshot, pack
from verify_configuration_persistence import EXE_SHA256, digest, require
from verify_configuration_card_ui import rendered_cycle, cycle_frames, match_frame

PLAN_FILE = Path(__file__).with_name('statistics_ui.json')
PLAN = json.loads(PLAN_FILE.read_text())
POINTS = PLAN['actions']


def raw_text(ui, address, size=80):
    return ui.read(address, size).split(b'\0', 1)[0].decode('ascii')


def generate(args):
    initial, _ = championship_fixture(args.championship)
    args.target = 'original'
    out, game = setup(args, initial)
    report = dict(scope=__doc__, operation='generate', pass_=False,
                  engine_state_writes=False, binary_sha256=EXE_SHA256,
                  initial_card_sha256=digest(initial),
                  championship_fixture=str(args.championship.resolve()),
                  input_keys=[], races=[])

    def action(ui):
        def keys(codes):
            for code in codes:
                ui.key(code)
                report['input_keys'].append(code)

        keys(['Right', 'Right', 'Right', 'Return', 'Return', 'Return'])
        saved = json.loads((args.championship/'report.json').read_text())['saved_state']
        for race in range(saved['race'], saved['races']):
            ui.wait(lambda: ui.integer(0x7746c0) >= 8 and ui.integer(0x7746ac) == 0
                    and 1 <= ui.integer(0x936ff4) <= 10, 60)
            require(ui.integer(0x93dec8) == race, 'loaded race sequence differs')
            keys(['Escape', 'Down', 'Down', 'Down', 'Return', 'Up', 'Return'])
            menu = 0x46ae44 if race+1 == saved['races'] else 0x46bf38
            ui.wait(lambda: ui.integer(0x940010) == menu and ui.integer(0x936ff4) == 15)
            state = snapshot(ui)
            require(state['race'] == race+1 and state['stats'] == 1, 'race statistics not recorded')
            report['races'].append(state)
            # Move from View Results through View League to Next Race/Season.
            keys(['Right', 'Down', 'Down'])
            caption = ui.text(0x46af30 if menu == 0x46ae44 else 0x46c01c)
            require(('Return To Title Screen' if race+1 == saved['races'] else 'Next Race') in caption,
                    'actual continuation/elimination action required: '+caption)
            keys(['Return'])
        ui.wait(lambda: ui.integer(0x936ff4) == 0 and ui.integer(0x940010) == 0x4696b0)
        require(ui.integer(0x46741c) == 1, 'completed statistics lost on frontend return')
        report['completed_state'] = snapshot(ui)
        # Loading through File Manager preserves that main-menu selection.
        print('Returned main selection:', ui.text(0x46975c), flush=True)
        require('File Manager' in ui.text(0x46975c), 'actual loaded-game frontend return required')
        keys(['Down', 'Return'])
        ui.wait(lambda: ui.integer(0x940010) == 0x4690c4)
        keys(['Right', 'Right', 'Return'])
        ui.wait(lambda: ui.integer(0x93a318) == 3 and ui.integer(0x940010) == 0x4671ec)
        report['saved_state'] = snapshot(ui)
        expected = bytearray(pack(report['saved_state']))
        struct.pack_into('<H', expected, 0, 0x1010)
        require(ui.read(0x93a490, len(expected)) == expected,
                'original configuration did not pack all live statistics')
        keys(['Return', 'Right', 'Return'])
        ui.enter_name('B')
        report['input_keys'] += ['Right', 'Return', 'Down', 'Down', 'Right', 'Return']
        ui.wait(lambda: (ui.game/'SaveGames').read_bytes()[0x200:0x206] == b'\x01\0\0\0B\0')
        raw = (ui.game/'SaveGames').read_bytes()
        require(raw == ui.read(0x754460, len(raw)), 'original card disk/RAM differ')
        require(raw[0x4000:0x597e] == expected, 'saved statistics configuration payload differs')
        require(raw[:0x200] == initial[:0x200] and raw[0x2000:0x4000] == initial[0x2000:0x4000],
                'original championship slot changed while saving configuration')
        (out/'original.card').write_bytes(raw)
        report['card_sha256'] = digest(raw)
        report['payload_sha256'] = digest(expected)
        report['pass_'] = True

    try:
        execute(args, out, game, action)
    finally:
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        check_space(out)
    print('Actual original completed-statistics fixture: PASS', flush=True)


def fixture(directory):
    producer = json.loads((directory/'report.json').read_text())
    require(producer['operation'] == 'generate' and producer['pass_'] and
            producer['binary_sha256'] == EXE_SHA256 and not producer['engine_state_writes'],
            'completed original statistics producer required')
    initial, _ = championship_fixture(Path(producer['championship_fixture']))
    require(digest(initial) == producer['initial_card_sha256'], 'initial championship fixture changed')
    raw = (directory/'original.card').read_bytes()
    require(len(raw) == 0x20000 and digest(raw) == producer['card_sha256'], 'statistics card changed')
    require(raw[:0x200] == initial[:0x200] and raw[0x2000:0x4000] == initial[0x2000:0x4000],
            'original championship was overwritten')
    require(raw[0x200:0x206] == b'\x01\0\0\0B\0' and
            all(struct.unpack_from('<I', raw, i*0x200)[0] == 0 for i in range(2,15)),
            'actual named configuration in slot one required')
    state = producer['saved_state']
    expected = bytearray(pack(state)); struct.pack_into('<H', expected, 0, 0x1010)
    require(raw[0x4000:0x597e] == expected and digest(expected) == producer['payload_sha256'],
            'complete original configuration does not match live saved state')
    require((state['mode'], state['type'], state['race'], state['races'], state['stats_index'],
             state['actual_season'], state['stats']) == (1,4,4,4,0,0,1),
            'one actual completed Stock Car season required')
    require([race['race'] for race in producer['races']] == [2,3,4],
            'remaining real championship races not completed')
    stats = bytes.fromhex(state['statistics'])
    names = [stats[620+i*16:636+i*16].split(b'\0')[0] for i in range(20)]
    require(len(set(names)) == 20 and all(names), 'nonempty completed championship standings required')
    require(state['statistics'] == producer['completed_state']['statistics'],
            'configuration save changed completed season statistics')
    return raw, state


def displayed(ui, action):
    row = dict(name=action['checkpoint'], menu=ui.integer(0x940010))
    if 'label' in action:
        row['label'] = ui.text(action['label_address'])
    if 'driver' in action:
        row.update(driver=ui.integer(0x467eec), sprite=raw_text(ui, 0x93e7a0, 32),
                   driver_name=raw_text(ui, 0x93e680, 32), car=raw_text(ui, 0x93e6f0, 16),
                   car_type=ui.read(0x466a0c+action['driver'],1)[0],
                   seasons=[{name:raw_text(ui,base+i*16,16) for name,base in
                             [('season',0x93e700),('wins',0x93e6a0),('kills',0x93e750),('dnf',0x93e630)]}
                            for i in range(5)])
    if 'track' in action:
        row.update(track=ui.integer(0x4685e0), caption=ui.text(0x4683e8),
                   seasons=[{name:raw_text(ui,base+i*stride,stride) for name,base,stride in
                             [('season',0x93fb5c,12),('winner',0x93fa80,32),
                              ('kills',0x93fb98,12),('dnf',0x93fb20,12)]} for i in range(5)])
    if 'championship' in action:
        row.update(championship=ui.integer(0x467bf4), caption=raw_text(ui,0x93e390,32),
                   standings=[raw_text(ui,0x93e3b0+i*32,32) for i in range(20)])
    return row


def check_point(row, action, saved):
    require(row['name'] == action['checkpoint'] and row['menu'] == action['menu'],
            'wrong statistics menu at '+action['checkpoint'])
    if 'label' in action:
        require(action['label'] in row['label'], 'wrong selected statistics category')
    stats = bytes.fromhex(saved['statistics'])
    jl, jc = '%R%JL%T/', '%R%JC%T/'
    if 'driver' in action:
        driver = action['driver']; require(row['driver'] == driver, 'wrong selected driver')
        data = stats[220+driver*20:240+driver*20]
        require(row['sprite'] == 'DRIVER'+str(driver) and row['driver_name'] == jl+data[:16].split(b'\0')[0].decode('ascii')
                and row['car'] == jc+f"Car {row['car_type']:02d}", 'wrong driver identity')
        expected = dict(season=jl+'1',wins=jl+str(struct.unpack('<b',data[16:17])[0]),
                        kills=jl+str(struct.unpack('<b',data[17:18])[0]),
                        dnf=jl+str(struct.unpack('<h',data[18:20])[0]))
        require(row['seasons'] == [expected]+[dict.fromkeys(expected,'') for _ in range(4)],
                'displayed driver statistics differ from the original saved payload')
    if 'track' in action:
        track = action['track']; require(row['track'] == track, 'wrong selected track')
        data = stats[track*20:track*20+20]
        expected = dict(season=jc+'1',winner=jl+data[:16].split(b'\0')[0].decode('ascii'),
                        kills=jl+str(struct.unpack('<h',data[16:18])[0]),
                        dnf=jl+str(struct.unpack('<h',data[18:20])[0]))
        require(row['seasons'] == [expected]+[dict.fromkeys(expected,'') for _ in range(4)],
                'displayed track statistics differ from the original saved payload')
        require(row['caption'], 'missing track identity')
    if 'championship' in action:
        require(row['championship'] == action['championship'] == 0 and row['caption'] == jl+'Season 1',
                'wrong championship season')
        require(row['standings'] == [jl+stats[620+i*16:636+i*16].split(b'\0')[0].decode('ascii')
                                     for i in range(20)], 'displayed championship standings differ from saved payload')


def validate(report, saved, initial):
    require(report['pass_'] and not report['engine_state_writes'] and
            report['plan_sha256'] == digest(PLAN_FILE.read_bytes()) and
            report['initial_card_sha256'] == digest(initial), 'completed current statistics capture required')
    require(report['loaded_state'] == saved, 'normal startup did not restore the complete configuration')
    require(report['statistics_unchanged'] and report['card_unchanged'] and report['engine_matches_file'],
            'statistics navigation modified the saved data or card')
    require(len(report['checkpoints']) == len(POINTS), 'incomplete statistics navigation')
    require(report['input_keys'] == [key for action in POINTS for key in action['keys']],
            'statistics navigation input sequence differs')
    for row, action in zip(report['checkpoints'],POINTS):
        check_point(row,action,saved)
        require(('cycle' in row) == action['cycle'], 'missing or unexpected rendered cycle')
    if report['target'] == 'browser':
        events = report.get('trusted_keyboard_events', [])
        require(events and all(event['trusted'] for event in events), 'trusted browser keyboard input required')


def capture(args):
    initial, saved = fixture(args.fixture)
    args.boot_label = 'Stock Car'
    out, game = setup(args, initial)
    report = dict(scope=PLAN['scope'], operation='capture', target=args.target, pass_=False,
                  engine_state_writes=False, initial_card_sha256=digest(initial),
                  plan_sha256=digest(PLAN_FILE.read_bytes()), input_keys=[], checkpoints=[],
                  binary_sha256=EXE_SHA256 if args.target == 'original' else digest(args.binary.read_bytes()))
    def action(ui):
        report['loaded_state'] = snapshot(ui)
        require(report['loaded_state'] == saved, 'normal startup did not restore all saved statistics')
        for action in POINTS:
            for code in action['keys']:
                ui.key(code); report['input_keys'].append(code)
            ui.wait(lambda: ui.integer(0x940010) == action['menu']); ui.settled()
            row = displayed(ui, action); check_point(row, action, saved)
            if action['cycle']:
                row['cycle'] = rendered_cycle(ui, action, args.target == 'original')
            report['checkpoints'].append(row)
            print('Statistics checkpoint', row['name'], flush=True)
        report['statistics_unchanged'] = snapshot(ui) == saved
        raw = (ui.game/'SaveGames').read_bytes()
        report['card_unchanged'] = raw == initial
        report['engine_matches_file'] = raw == ui.read(0x754460,len(raw))
        validate(dict(report,pass_=True),saved,initial); report['pass_'] = True
    try:
        execute(args,out,game,action)
    finally:
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n'); check_space(out)
    print('Actual '+args.target+' persisted statistics: PASS', flush=True)


def baseline(args):
    """Capture a real old executable and require all three background regressions."""
    initial, saved = fixture(args.fixture)
    source = json.loads((args.original/'report.json').read_text()); validate(source,saved,initial)
    require(source['target'] == 'original' and source['binary_sha256'] == EXE_SHA256,
            'actual original statistics reference required')
    args.target, args.boot_label = 'native', 'Stock Car'
    out, game = setup(args,initial)
    report = dict(scope='Actual native pre-correction statistics table captures, rejected by the original frame comparison. A regression baseline, not original parity.',
                  operation='baseline',target='native',pass_=False,engine_state_writes=False,
                  initial_card_sha256=digest(initial),binary_sha256=digest(args.binary.read_bytes()),points=[])
    def action(ui):
        require(snapshot(ui) == saved, 'baseline did not load original statistics')
        for name, keys in [('driver-00',['Down','Right','Return','Return','Return']),
                           ('track-00',['Escape','Right','Return']),
                           ('championship',['Escape','Right','Return'])]:
            for code in keys: ui.key(code)
            plan = next(action for action in POINTS if action['checkpoint'] == name)
            ui.wait(lambda: ui.integer(0x940010) == plan['menu']); ui.settled()
            check_point(displayed(ui,plan),plan,saved)
            cycle = Path(rendered_cycle(ui,plan,False))
            actual_state, actual_pair = cycle_frames(cycle,False)[0]
            original = next(point for point in source['checkpoints'] if point['name'] == name)
            wanted_state, wanted_pair = cycle_frames(original['cycle'],False)[0]
            try: match_frame(wanted_state,wanted_pair,actual_state,actual_pair,False)
            except RuntimeError: pass
            else: raise RuntimeError('baseline does not reproduce the missing background at '+name)
            require(actual_pair[1] == wanted_pair[1], 'baseline palette changed')
            paths = []
            for suffix,data in zip(['bin','pal'],actual_pair):
                file = out/(name+'.'+suffix); file.write_bytes(data); paths.append(str(file))
            report['points'].append(dict(name=name,state=actual_state,files=paths,
                                         sha256=[digest(v) for v in actual_pair],
                                         original_sha256=[digest(v) for v in wanted_pair],
                                         different_pixels=sum(a!=b for a,b in zip(actual_pair[0],wanted_pair[0]))))
        require(snapshot(ui) == saved and (ui.game/'SaveGames').read_bytes() == initial,
                'baseline navigation changed original saved data')
        report['pass_'] = True
    try:
        execute(args,out,game,action)
    finally:
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n'); check_space(out)
    opened = open_files()
    for file in out.glob('*/cycle/*.bin'):
        stat = file.stat(); require((stat.st_dev,stat.st_ino) not in opened, 'baseline capture still open'); file.unlink()
    print('Actual old executable: all three missing backgrounds rejected',flush=True)


def compare(args):
    initial, saved = fixture(args.fixture)
    original = json.loads((args.original/'report.json').read_text()); validate(original,saved,initial)
    require(original['target'] == 'original' and original['binary_sha256'] == EXE_SHA256,
            'supported unmodified original reference required')
    out = prepare_output(args.report)
    require(WORK in out.parents and not out.exists(), 'fresh report under /tmp/wasm-dd2 required')
    targets, negatives, paths = {}, [], []
    for target, directory in [('native',args.native),('native-asan',args.asan),('browser',args.browser)]:
        actual = json.loads((directory/'report.json').read_text()); validate(actual,saved,initial)
        require(actual['target'] == ('browser' if target == 'browser' else 'native'), 'capture target differs')
        frames = []
        for left, right, action in zip(original['checkpoints'],actual['checkpoints'],POINTS):
            require({k:v for k,v in left.items() if k != 'cycle'} == {k:v for k,v in right.items() if k != 'cycle'},
                    'original statistics state differs at '+left['name'])
            if action['cycle']:
                source = cycle_frames(left['cycle'],False)
                candidate = cycle_frames(right['cycle'],target == 'browser')
                for phase in range(64):
                    try: match_frame(*source[phase],*candidate[phase],False)
                    except RuntimeError as error:
                        raise RuntimeError(f'{target} at {left["name"]} phase {phase}: {error}') from error
                    frames.append(dict(checkpoint=left['name'],phase=phase,
                                       framebuffer_sha256=digest(candidate[phase][1][0]),
                                       palette_sha256=digest(candidate[phase][1][1])))
                paths += [Path(left['cycle']),Path(right['cycle'])]
                for region in range(2):
                    changed = [bytearray(v) for v in candidate[0][1]]; changed[region][0] ^= 1
                    try: match_frame(*source[0],candidate[0][0],changed,False)
                    except RuntimeError: negatives.append(dict(target=target,case=left['name']+'-'+str(region)))
                    else: raise RuntimeError('altered statistics frame accepted')
        for name in ['driver-00','track-00','championship']:
            changed = copy.deepcopy(actual)
            row = next(row for row in changed['checkpoints'] if row['name'] == name)
            if 'standings' in row: row['standings'][0] += 'X'
            else: row['seasons'][0]['dnf'] += 'X'
            try: validate(changed,saved,initial)
            except RuntimeError: negatives.append(dict(target=target,case=name+'-statistics'))
            else: raise RuntimeError('altered statistics accepted')
        targets[target] = dict(pass_=True,capture=actual,frames=frames)
    before = json.loads(args.before.read_text())
    require(before['pass_'] and before['operation'] == 'baseline' and not before['engine_state_writes']
            and before['initial_card_sha256'] == digest(initial) and
            before['binary_sha256'] != targets['native']['capture']['binary_sha256'],
            'distinct actual pre-correction executable required')
    require([row['name'] for row in before['points']] == ['driver-00','track-00','championship'],
            'all three pre-correction table backgrounds required')
    for row in before['points']:
        source = next(point for point in original['checkpoints'] if point['name'] == row['name'])
        state, pair = cycle_frames(source['cycle'],False)[0]
        observed = [Path(file).read_bytes() for file in row['files']]
        require([digest(data) for data in observed] == row['sha256'] and
                [digest(data) for data in pair] == row['original_sha256'], 'regression baseline changed')
        try: match_frame(state,pair,row['state'],observed,False)
        except RuntimeError: negatives.append(dict(target='native-before',case=row['name']+'-missing-background'))
        else: raise RuntimeError('actual missing table background was accepted')
    report = dict(scope=PLAN['scope'],pass_=True,fixture=str(args.fixture.resolve()),
                  producer=json.loads((args.fixture/'report.json').read_text()),
                  original=original,targets=targets,negative_cases=negatives,regression_baseline=before)
    out.write_text(json.dumps(report,indent=2)+'\n')
    if args.clean:
        opened = open_files()
        files = {file for directory in paths for file in directory.glob('*.bin')}
        files.update(Path(file) for row in before['points'] for file in row['files'])
        for file in files:
            require(WORK in file.resolve().parents and not file.is_symlink(), 'unexpected raw output path')
            stat = file.stat(); require((stat.st_dev,stat.st_ino) not in opened, 'raw comparison output is open')
            file.unlink()
    print('Original statistics UI: PASS;',len(POINTS),'checkpoints;',
          ', '.join(f'{name}: {len(result["frames"])} exact framebuffer/palette pairs' for name,result in targets.items()),
          ';',len(negatives),'negative cases',flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='operation', required=True)
    producer = commands.add_parser('generate')
    producer.add_argument('--championship', type=Path, required=True)
    producer.add_argument('--output', type=Path, required=True)
    cap = commands.add_parser('capture')
    cap.add_argument('--fixture', type=Path, required=True)
    cap.add_argument('--target', choices=['original','native'], required=True)
    cap.add_argument('--binary', type=Path, default=Path('/tmp/dd2_native'))
    cap.add_argument('--output', type=Path, required=True)
    old = commands.add_parser('baseline')
    for name in ['fixture','original','binary','output']: old.add_argument('--'+name,type=Path,required=True)
    cmp = commands.add_parser('compare')
    for name in ['fixture','original','native','asan','browser','report']:
        cmp.add_argument('--'+name,type=Path,required=True)
    cmp.add_argument('--before',type=Path,required=True,help='observed pre-correction statistics frames')
    cmp.add_argument('--clean',action='store_true')
    args = parser.parse_args()
    {'generate':generate,'capture':capture,'compare':compare,'baseline':baseline}[args.operation](args)


if __name__ == '__main__':
    main()
