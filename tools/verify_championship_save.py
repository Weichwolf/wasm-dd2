#!/usr/bin/env python3
"""Generate a real original championship card and compare actual frontend loading.

The initial card is a file input, produced through original X11 menus after a
real race and Retire/Yes. No state, clocks, RNG or results are injected. Loading
must preserve every saved field/region, start the next race and permit pause.
Selected complete card-menu cycles are compared with the running original.
This does not prove chronological racing video/PCM or every save type/season.
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
    original_args, run_original, WINE_WORK, EXE_SHA256, digest, require)
from verify_configuration_card_ui import file_ui, rendered_cycle, cycle_frames, match_frame

LAYOUT_FILE = ROOT/'tools/championship_save_layout.json'
LAYOUT = json.loads(LAYOUT_FILE.read_text())
POINTS = ('manager', 'selected')
SCOPE = __doc__


def snapshot(ui):
    return {**{name: ui.integer(address) for name, address, _ in LAYOUT['fields']},
            **{name: ui.read(address, size).hex() for name, address, size, _ in LAYOUT['regions']},
            'joy_present': ui.read(0x754451, 1)[0]}


def pack(state):
    data = bytearray(0x197e)
    struct.pack_into('<H', data, 0, 0x3030)
    struct.pack_into('<H', data, 18, state['joy_present'])
    for name, _, offset in LAYOUT['fields']:
        struct.pack_into('<H', data, offset, state[name] & 0xffff)
    for name, _, size, offset in LAYOUT['regions']:
        raw = bytes.fromhex(state[name]); require(len(raw) == size, 'incomplete saved '+name)
        data[offset:offset+size] = raw
    return bytes(data)


def fixture(directory):
    producer = json.loads((directory/'report.json').read_text())
    require(producer['operation'] == 'generate' and producer['pass_'] and
            producer['binary_sha256'] == EXE_SHA256 and not producer['engine_state_writes'],
            'completed original-generated fixture required')
    raw = (directory/'original.card').read_bytes()
    require(len(raw) == 0x20000 and digest(raw) == producer['card_sha256'], 'fixture card changed')
    require(struct.unpack_from('<I', raw)[0] == 1 and raw[4:6] == b'A\0' and
            all(struct.unpack_from('<I', raw, i*0x200)[0] == 0 for i in range(1,15)),
            'one actual named championship save required')
    payload = raw[0x2000:0x397e]
    require(payload == pack(producer['saved_state']), 'original packed payload differs from live state')
    state = producer['saved_state']
    require((state['car'],state['type'],state['race'],state['season'],state['stats']) == (1,4,1,0,1)
            and state['mode'] in (0,1) and state['races'] == (5 if state['mode'] == 0 else 4),
            'nontrivial first-race championship fixture required')
    require(bytes.fromhex(state['names'])[:2] == b'A\0', 'actual player name required')
    return raw, payload


def setup(args, initial):
    out = prepare_output(args.output)
    require(WORK in out.parents and not out.exists(), 'fresh output under /tmp/wasm-dd2 required')
    out.mkdir(parents=True)
    game = out/'game'; game.mkdir()
    for asset in (ROOT/'DestructionDerby2').iterdir():
        if asset.name == 'SaveGames': (game/asset.name).write_bytes(initial)
        else: (game/asset.name).symlink_to(asset.resolve(), target_is_directory=asset.is_dir())
    return out, game


def execute(args, out, game, action):
    if args.target == 'original':
        WINE_WORK.mkdir(parents=True, exist_ok=True)
        with (WINE_WORK/'capture.lock').open('w') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            def driver(pid, output, env, deadline, rundir):
                ui = OriginalUI(pid, rundir, output, env, deadline)
                try: action(ui)
                finally: ui.stop()
            options = original_args(); options.timeout = 700
            run_original(game, out, options, on_menu=driver)
    else:
        with (out/'xvfb.log').open('wb') as log:
            display = subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','1280x1024x24'],
                                       stdout=subprocess.PIPE, stderr=log)
            ui = None
            try:
                number = display.stdout.readline().decode().strip(); require(number, 'Xvfb did not start')
                ui = ConfigUI(args.binary.resolve(), game, out, ':'+number, 1, 700)
                ui.boot(); action(ui)
            finally:
                if ui: ui.stop()
                display.terminate(); display.wait(timeout=5)


def generate(args):
    args.target = 'original'
    out, game = setup(args, (ROOT/'DestructionDerby2/SaveGames').read_bytes())
    report = dict(scope=SCOPE, operation='generate', pass_=False, engine_state_writes=False,
                  binary_sha256=EXE_SHA256, layout_sha256=digest(LAYOUT_FILE.read_bytes()))
    def action(ui):
        require(all(struct.unpack_from('<I',(ui.game/'SaveGames').read_bytes(),i*0x200)[0] == 0
                    for i in range(15)), 'empty original provisioned card required')
        keys = ['Right','Return','Right','Return','Left','Return']
        if args.mode == 1: keys.append('Right')
        keys += ['Return','Return','Return','Up','Left','Return','Down','Down','Return']
        for code in keys: ui.key(code)
        report['input_keys'] = keys[:]
        ui.wait(lambda: ui.integer(0x7746c0) >= 8 and ui.integer(0x7746ac) == 0 and
                1 <= ui.integer(0x936ff4) <= 10, 60)
        ui.wait(lambda: ui.integer(0x784298) < 1, 60)
        ui.edge('a', True)
        try: time.sleep(1)
        finally: ui.edge('a', False)
        keys = ['Escape','Down','Down','Down','Return','Up','Return','Down','Return']
        for code in keys: ui.key(code)
        report['input_keys'] += keys
        ui.wait(lambda: ui.integer(0x940010) == 0x46a9dc)
        require('Save Game' in ui.text(0x46aa74), 'actual season File Options/Save Game required')
        report['saved_state'] = snapshot(ui)
        ui.key('Return'); ui.wait(lambda: ui.integer(0x93a318) == 4)
        require(ui.read(0x93a490,0x197e) == pack(report['saved_state']), 'game packed state differs')
        ui.key('Return'); ui.wait(lambda: 'Select' in ui.text(0x46725c) and ui.integer(0x774680) == 0)
        ui.key('Return'); ui.enter_name('A')
        ui.wait(lambda: (ui.game/'SaveGames').read_bytes()[0] == 1)
        raw = (ui.game/'SaveGames').read_bytes()
        require(raw == ui.read(0x754460,len(raw)), 'original card disk/RAM differ')
        (out/'original.card').write_bytes(raw)
        report['card_sha256'], report['payload_sha256'] = digest(raw), digest(raw[0x2000:0x397e])
        report['pass_'] = True
    try:
        execute(args,out,game,action)
    finally:
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n'); check_space(out)
    fixture(out)
    print('Actual original championship fixture: PASS', flush=True)


def validate(data, payload):
    require(data['pass_'] and not data['engine_state_writes'] and
            data['layout_sha256'] == digest(LAYOUT_FILE.read_bytes()), 'completed current capture required')
    initial = data['initial']
    require((initial['mode'],initial['type'],initial['car']) == (0,0,0),
            'saved game incorrectly auto-loaded as configuration')
    state = data['loaded']
    for name, _, offset in LAYOUT['fields']:
        require(state[name] == struct.unpack_from('<h',payload,offset)[0], 'loaded field differs: '+name)
    for name, _, size, offset in LAYOUT['regions']:
        require(bytes.fromhex(state[name]) == payload[offset:offset+size], 'loaded region differs: '+name)
    require(state['joy_present'] == struct.unpack_from('<h',payload,18)[0] and
            state['active_bindings'] == state['bindings'][:28], 'actual racing bindings differ')
    require(state['level'] == state['replay_level'] and 1 <= state['level'] <= 10 and
            state['quit'] == state['replay'] == 0 and len(bytes.fromhex(state['order'])) == 20 and
            sorted(bytes.fromhex(state['order'])) == list(range(20)), 'actual live next race required')
    require(data['pause_ticks'][0] >= 8 and len(set(data['pause_ticks'])) == 1,
            'loaded race did not pause')
    require([row['name'] for row in data['checkpoints']] == list(POINTS), 'incomplete card menu capture')
    require(data['card_unchanged'] and data['engine_matches_file'], 'loading changed the complete card')
    if data['target'] == 'browser':
        require(data['trusted_keyboard_events'] and all(e['trusted'] for e in data['trusted_keyboard_events']),
                'actual trusted browser keyboard input required')


def capture(args):
    initial, payload = fixture(args.fixture)
    out, game = setup(args, initial)
    report = dict(scope=SCOPE, operation='capture', target=args.target, pass_=False,
                  engine_state_writes=False, initial_save_sha256=digest(initial),
                  layout_sha256=digest(LAYOUT_FILE.read_bytes()), checkpoints=[],
                  binary_sha256=EXE_SHA256 if args.target == 'original' else digest(args.binary.read_bytes()))
    source = json.loads((args.reference/'report.json').read_text()) if args.reference else None
    if source:
        validate(source,payload); require(source['binary_sha256'] == EXE_SHA256, 'original reference required')
    def checkpoint(ui, name):
        ui.settled()
        row = dict(name=name, file_ui=file_ui(ui), menu=ui.integer(0x940010),
                   file_mode=ui.integer(0x93a318), file_slot=ui.integer(0x774680))
        wanted = None
        if source:
            saved = next(row for row in source['checkpoints'] if row['name'] == name)
            metadata = json.loads((Path(saved['cycle'])/'cycle.json').read_text())
            wanted = [(f['phase'],f['card_phase']) for f in metadata['frames']]
        row['cycle'] = rendered_cycle(ui, {'checkpoint':name}, args.target == 'original', wanted)
        report['checkpoints'].append(row)
    def action(ui):
        report['initial'] = settings(ui)
        for code in ['Right','Right','Right','Return']: ui.key(code)
        checkpoint(ui,'manager')
        ui.key('Return')
        ui.wait(lambda: 'Select' in ui.text(0x46725c) and ui.integer(0x774680) == 0)
        checkpoint(ui,'selected')
        ui.key('Return')
        # Pause_Mode rejects the first two physics frames in the original.
        ui.wait(lambda: ui.integer(0x7746c0) >= 8 and ui.integer(0x7746ac) == 0 and
                1 <= ui.integer(0x936ff4) <= 10,60)
        ui.key('Escape')
        report['pause_ticks'] = []
        for _ in range(3):
            report['pause_ticks'].append(ui.integer(0x7746c0)); time.sleep(0.2)
        report['loaded'] = {**snapshot(ui), 'level':ui.integer(0x936ff4),
            'replay_level':ui.integer(0x9392bc), 'quit':ui.integer(0x7746ac), 'replay':ui.integer(0x467074),
            'active_bindings':ui.read(0x46302c,14).hex(), 'order':ui.read(0x795c28,20).hex()}
        raw = (ui.game/'SaveGames').read_bytes()
        report['card_unchanged'] = raw == initial
        report['engine_matches_file'] = raw == ui.read(0x754460,len(raw))
        report['pass_'] = True; validate(report,payload)
    try:
        execute(args,out,game,action)
    finally:
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n'); check_space(out)
    print('Actual '+args.target+' championship loading: PASS', flush=True)


def compare(args):
    initial, payload = fixture(args.fixture)
    source = json.loads((args.original/'report.json').read_text()); validate(source,payload)
    require(source['binary_sha256'] == EXE_SHA256, 'actual supported original required')
    results, negatives, paths = {}, [], []
    for target, directory in [('native',args.native),('browser',args.browser),('asan',args.asan)]:
        if directory is None: continue
        actual = json.loads((directory/'report.json').read_text()); validate(actual,payload)
        require(actual['initial_save_sha256'] == digest(initial) == source['initial_save_sha256'],
                'initial file inputs differ')
        require(actual['target'] == ('browser' if target == 'browser' else 'native'), 'wrong capture target')
        require(actual['initial'] == source['initial'] and actual['loaded'] == source['loaded'],
                'actual original/port saved state or race progression differs')
        frames = []
        for left, right in zip(source['checkpoints'],actual['checkpoints']):
            require({k:v for k,v in left.items() if k != 'cycle'} ==
                    {k:v for k,v in right.items() if k != 'cycle'}, 'card UI state differs')
            a = cycle_frames(left['cycle'], False)
            b = cycle_frames(right['cycle'], target == 'browser')
            for phase in range(64):
                match_frame(*a[phase],*b[phase],True)
                frames.append(dict(checkpoint=left['name'],phase=phase,
                    framebuffer_sha256=digest(b[phase][1][0]),palette_sha256=digest(b[phase][1][1])))
            paths.extend([Path(left['cycle']),Path(right['cycle'])])
            for region in range(2):
                altered = [bytearray(v) for v in b[0][1]]; altered[region][0] ^= 1
                try: match_frame(*a[0],b[0][0],altered,True)
                except RuntimeError: negatives.append(dict(target=target,case=right['name']+'-'+str(region)))
                else: raise RuntimeError('altered frame accepted')
        for field in actual['loaded']:
            altered = copy.deepcopy(actual)
            value = altered['loaded'][field]
            altered['loaded'][field] = value+1 if isinstance(value,int) else ('00' if value[:2] != '00' else '01')+value[2:]
            try:
                validate(altered,payload)
                require(altered['loaded'] == source['loaded'], 'changed loaded state')
            except RuntimeError: negatives.append(dict(target=target,case='loaded-'+field))
            else: raise RuntimeError('altered loaded state accepted')
        results[target] = dict(pass_=True,capture=actual,frames=frames)
    report = dict(scope=SCOPE,pass_=True,fixture=json.loads((args.fixture/'report.json').read_text()),
                  original=source,targets=results,negative_cases=negatives)
    path = prepare_output(args.report); require(WORK in path.parents and not path.exists(), 'fresh report required')
    path.write_text(json.dumps(report,indent=2)+'\n')
    if args.clean:
        opened = open_files()
        for folder in set(paths):
            for file in folder.glob('*.bin'):
                stat = file.stat(); require((stat.st_dev,stat.st_ino) not in opened, 'capture still in use')
                file.unlink()
    print('Actual original/native/browser championship loading: PASS;',
          len(results)*128,'complete menu frame/palette pairs;',len(negatives),'negative cases rejected')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='operation',required=True)
    gen = sub.add_parser('generate'); gen.add_argument('--mode',type=int,choices=[0,1],default=1)
    gen.add_argument('--output',type=Path,required=True)
    cap = sub.add_parser('capture')
    cap.add_argument('--target',choices=['original','native'],required=True)
    cap.add_argument('--binary',type=Path,default=Path('/tmp/dd2_native'))
    cap.add_argument('--output',type=Path,required=True); cap.add_argument('--reference',type=Path)
    cmp = sub.add_parser('compare')
    for arg in ['original','native','browser','report']: cmp.add_argument('--'+arg,type=Path,required=True)
    cmp.add_argument('--asan',type=Path); cmp.add_argument('--clean',action='store_true')
    for command in [cap,cmp]: command.add_argument('--fixture',type=Path,required=True)
    args = parser.parse_args()
    {'generate':generate,'capture':capture,'compare':compare}[args.operation](args)


if __name__ == '__main__': main()
