#!/usr/bin/env python3
"""Actual configuration saving, process restart and remapped racing input.

Use the unmodified original and native frontend, actual X11 keys, isolated
cards and read-only observations. Compare card bytes and restored settings;
race probes verify control masks and player response, not timed A/V parity.
The browser companion uses real Playwright keys and normal IndexedDB loading.
"""
import argparse
import copy
import fcntl
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import time
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference'))
from artifacts import WORK, check_space, open_files, prepare_output
from reference.capture import EXE_SHA256, WORK as WINE_WORK, run as run_original, key_acknowledged
from verify_native_replay import NativeUI, digest, require

BINDINGS = [('B', 5), ('C', 3), ('D', 8), ('E', 12), ('F', 13)]
CONFIG_BYTES = 0x197e
CONFIG_MAP_OFFSET = 0x196c
FIELDS = dict(mode=0x4673f8, type=0x4673f4, car=0x467400, track=0x4673fc,
              sound=0x467410, pad_option=0x467414)
PROBES = [(['E'], 0x4000, 32768, 0), (['F'], 0x8000, -32768, 0),
          (['B'], 0x80, 0, -256), (['C'], 0x20, 0, 256),
          (['D', 'B'], 0x480, 0, -511), (['D', 'C'], 0x420, 0, 511),
          (['A', 'Z', 'space'], 0, 0, 0)]


def settings(ui):
    return {**{key: ui.integer(address) for key, address in FIELDS.items()},
            'saved': list(ui.read(0x46757a, 18)), 'active': list(ui.read(0x46302c, 14))}


def observe_card(ui, name):
    raw = (ui.game / 'SaveGames').read_bytes()
    require(len(raw) == 0x20000 and raw == ui.read(0x754460, len(raw)),
            'actual disk card differs from engine RAM')
    (ui.output / (name + '.card')).write_bytes(raw)
    slots = []
    for index in range(15):
        header = index * 0x200
        if struct.unpack_from('<I', raw, header)[0] == 1:
            payload = raw[0x2000 + index*0x2000:0x2000 + index*0x2000 + CONFIG_BYTES]
            slots.append(dict(index=index, name=raw[header+4:header+32].split(b'\0')[0].decode('ascii'),
                              magic=struct.unpack_from('<H', payload)[0],
                              binding=list(payload[CONFIG_MAP_OFFSET:CONFIG_MAP_OFFSET+18]),
                              payload_sha256=digest(payload)))
    return dict(bytes=len(raw), sha256=digest(raw), engine_matches_file=True, slots=slots,
                file=str(ui.output / (name+'.card')))


class ConfigUI(NativeUI):
    def key(self, code):
        super().key(code)
        print('menu', hex(self.integer(0x940010)), 'angle', self.read(0x46996c, 2).hex(), flush=True)

    def bind(self, key, offset):
        self.wait(lambda: self.integer(0x940010) == 0x469298)
        self.settled()
        self.edge(key.lower(), True)
        try:
            self.wait(lambda: self.read(0x93fd90+offset, 1) == bytes([ord(key)]))
        finally:
            self.edge(key.lower(), False)
        self.wait(lambda: self.read(self.table['dd2_keystate']+ord(key), 1) == b'\0')
        time.sleep(0.1)


class OriginalUI(ConfigUI):
    def __init__(self, pid, game, output, env, deadline):
        self.pid, self.game, self.output, self.env, self.deadline = pid, game, output, env, deadline
        self.memory = open(f'/proc/{pid}/mem', 'rb', buffering=0)
        self.serial = 0

    def read(self, address, count):
        import os
        data = os.pread(self.memory.fileno(), count, address)
        if len(data) != count:
            raise OSError('incomplete original observation')
        return data

    def wait(self, predicate, timeout=20):
        end = min(self.deadline, time.monotonic()+timeout)
        while time.monotonic() < end:
            check_space(self.output)
            result = predicate()
            if result:
                return result
            time.sleep(0.005)
        raise TimeoutError('Original condition timed out: '+json.dumps(self.state()))

    def settled(self):
        self.wait(lambda: self.read(0x46996c, 2) == b'\0\0')
        time.sleep(0.3)
        require(self.read(0x46996c, 2) == b'\0\0', 'original slab still rotating')

    def key(self, code):
        self.settled()
        self.serial += 1
        out = self.output / f'input-{self.serial:02d}-{code}'
        out.mkdir()
        key_acknowledged(self.pid, out, self.env, code, max(1, self.deadline-time.monotonic()))
        time.sleep(0.3)
        print('Original X11', code, self.text(0x46975c), self.text(0x4672ac), flush=True)

    def bind(self, key, offset):
        self.key(key)
        require(self.read(0x93fd90+offset, 1) == bytes([ord(key)]), 'original binding differs')

    def stop(self):
        self.memory.close()


def save_configuration(ui, report):
    report['initial'] = settings(ui)
    report['initial_card'] = observe_card(ui, 'initial')
    require(not report['initial_card']['slots'], 'this scenario requires the empty provisioned card')
    for code in ['Down', 'Right', 'Right', 'Return', 'Return', 'Return']:
        ui.key(code)
    for key, offset in BINDINGS:
        ui.bind(key, offset)
    ui.key('Return')
    report['changed'] = settings(ui)
    expected = report['initial']['saved'][:]
    for key, offset in BINDINGS:
        expected[offset] = ord(key)
        if key == 'D':
            expected[9] = ord(key)
    require(report['changed']['saved'] == expected and report['changed']['pad_option'] == 0,
            'committed keyboard configuration differs')
    for code in ['Right', 'Right', 'Return']:
        ui.key(code)
    ui.wait(lambda: ui.integer(0x93a318) == 3)
    packed = ui.read(0x93a490, CONFIG_BYTES)
    require(struct.unpack_from('<H', packed)[0] == 0x1010 and
            list(packed[CONFIG_MAP_OFFSET:]) == expected, 'packed configuration binding differs')
    report['packed_sha256'] = digest(packed)
    for code in ['Return', 'Return', 'Return', 'Down', 'Down', 'Right', 'Return']:
        ui.key(code)
    ui.wait(lambda: (ui.game/'SaveGames').read_bytes()[0] == 1)
    report['saved_card'] = observe_card(ui, 'saved')
    require(len(report['saved_card']['slots']) == 1 and
            report['saved_card']['slots'][0]['name'] == 'A' and
            report['saved_card']['slots'][0]['payload_sha256'] == digest(packed),
            'actual save did not write the packed configuration to named slot A')
    ui.key('Escape'); ui.key('Escape')


def race_controls(ui):
    for code in ['Down', 'Down', 'Return']:
        ui.key(code)
    ui.wait(lambda: 1 <= ui.integer(0x936ff4) <= 10 and ui.integer(0x7746c0) > 0, timeout=40)
    ui.wait(lambda: ui.integer(0x784298) < 1, timeout=60)
    require(ui.integer(0x46385c) == 0 and ui.integer(0x467074) == 0 and ui.integer(0x7746ac) == 0,
            'actual player race required')
    player = ui.integer(0x93ded0)
    require(0 <= player < 20, 'invalid player')
    active = list(ui.read(0x46302c, 14))
    require(active == list(ui.read(0x46757a, 14)), 'race did not activate the saved key map')
    def state():
        return dict(ticks=ui.integer(0x7746c0), held=struct.unpack('<H', ui.read(0x754448, 2))[0],
                    throttle=ui.integer(0x792a86+player*0x1b2), steering=ui.integer(0x792a82+player*0x1b2),
                    position=[ui.integer(a+player*0x1b2) for a in (0x792a30, 0x792a38)])
    probes = []
    # All five changed controls, both fast-steer directions and the old
    # accelerator/brake/fast-steer keys which should have lost their binding.
    for keys, mask, field, condition in [
            (['E'], 0x4000, 'throttle', lambda v: v == 32768),
            (['F'], 0x8000, 'throttle', lambda v: v == -32768),
            (['B'], 0x80, 'steering', lambda v: v == -256),
            (['C'], 0x20, 'steering', lambda v: v == 256),
            (['D', 'B'], 0x480, 'steering', lambda v: v == -511),
            (['D', 'C'], 0x420, 'steering', lambda v: v == 511),
            (['A', 'Z', 'space'], 0, 'throttle', lambda v: v == 0)]:
        before = state()
        print('Race probe', keys, before, flush=True)
        for key in keys:
            ui.edge(key.lower(), True)
        try:
            try:
                ui.wait(lambda: state()['held'] == mask and condition(state()[field]), timeout=15)
            except Exception:
                print('Failed race probe', keys, state(), 'map', active, flush=True)
                raise
            start = state()['ticks']
            ui.wait(lambda: state()['ticks'] >= start+4)
            observed = state()
            require(observed['held'] == mask and condition(observed[field]), 'live control response changed')
            if keys == ['E']:
                require(observed['position'] != before['position'], 'accelerator did not move the player car')
        finally:
            for key in reversed(keys):
                ui.edge(key.lower(), False)
        ui.wait(lambda: state()['held'] == 0 and state()['throttle'] == 0 and state()['steering'] == 0)
        probes.append(dict(keys=keys, mask=mask, before=before, held=observed, released=state()))
    return dict(player=player, level=ui.integer(0x936ff4), active=active, probes=probes)


def after_restart(ui, report):
    report['restored'] = settings(ui)
    report['reloaded_card'] = observe_card(ui, 'reloaded')
    require(report['restored'] == report['changed'], 'saved settings did not survive normal startup')
    require(report['reloaded_card']['sha256'] == report['saved_card']['sha256'], 'restart changed persisted card')
    report['race'] = race_controls(ui)


def original_args():
    return SimpleNamespace(mode='menu', wine_debug='-all', audio=False, trace_cd=False,
                           trace_timer_callbacks=False, trace_game_clock=False, trace_multimedia_timer=False,
                           trace_movie_video=False, trace_movie_timing=False,
                           keep_movie=False, timeout=360, startup_frames=128, champ_history=False,
                           race_stream=False, keys=None, frames=None, frame=1, menu_cycle=0, audio_tail=0)


def capture_target(args):
    out = prepare_output(args.output)
    require(WORK in out.parents and not out.exists(), 'fresh output inside /tmp/wasm-dd2 required')
    out.mkdir(parents=True)
    game = out/'game'; game.mkdir()
    for asset in (ROOT/'DestructionDerby2').iterdir():
        if asset.name == 'SaveGames':
            shutil.copyfile(asset, game/asset.name)
        else:
            (game/asset.name).symlink_to(asset.resolve(), target_is_directory=asset.is_dir())
    report = dict(scope=__doc__, target=args.target, pass_=False, engine_state_writes=False,
                  initial_save_sha256=digest((game/'SaveGames').read_bytes()))
    binary = args.binary.resolve()
    report['binary_sha256'] = EXE_SHA256 if args.target == 'original' else digest(binary.read_bytes())
    display = ui = None
    try:
        if args.target == 'original':
            WINE_WORK.mkdir(parents=True, exist_ok=True)
            with (WINE_WORK/'capture.lock').open('w') as lock:
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
                for number, action in [(1, save_configuration), (2, after_restart)]:
                    startup = out/f'startup-{number}'; startup.mkdir()
                    def driver(pid, output, env, deadline, rundir):
                        ui = OriginalUI(pid, rundir, output, env, deadline)
                        try:
                            action(ui, report)
                            shutil.copyfile(rundir/'SaveGames', game/'SaveGames')
                        finally:
                            ui.stop()
                    run_original(game, startup, original_args(), on_menu=driver)
        else:
            with (out/'xvfb.log').open('wb') as log:
                display = subprocess.Popen(['Xvfb', '-displayfd', '1', '-screen', '0', '1280x1024x24'],
                                           stdout=subprocess.PIPE, stderr=log)
                number = display.stdout.readline().decode().strip()
                require(bool(number), 'Xvfb failed to start')
                for index, action in [(1, save_configuration), (2, after_restart)]:
                    startup = out/f'startup-{index}'; startup.mkdir()
                    ui = ConfigUI(binary, game, startup, ':'+number, index, 360)
                    ui.boot(); action(ui, report); ui.stop(); ui = None
        report['pass_'] = True
    finally:
        if ui:
            ui.stop()
        if display:
            display.terminate(); display.wait(timeout=5)
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        check_space(out)
    print(json.dumps(report, indent=2))


def validate_capture(data):
    require(data['pass_'] and data['engine_state_writes'] is False, 'completed read-only capture required')
    expected = data['initial']['saved'][:]
    for key, offset in BINDINGS:
        expected[offset] = ord(key)
    expected[9] = ord('D')
    require(data['changed']['saved'] == expected and data['changed']['pad_option'] == 0,
            'saved binding differs from actual requested keys')
    require(data['restored'] == data['changed'], 'configuration did not survive restart')
    require(data['initial_save_sha256'] == data['initial_card']['sha256'] and
            data['saved_card']['sha256'] == data['reloaded_card']['sha256'], 'card changed at restart')
    for key in ['initial_card', 'saved_card', 'reloaded_card']:
        card = data[key]
        require(card['bytes'] == 0x20000 and card['engine_matches_file'] is True, 'complete engine/disk card required')
        if key == 'initial_card':
            require(not card['slots'], 'initial card was not empty')
        else:
            require(len(card['slots']) == 1 and card['slots'][0]['index'] == 0 and
                    card['slots'][0]['name'] == 'A' and card['slots'][0]['magic'] == 0x1010 and
                    card['slots'][0]['binding'] == expected and
                    card['slots'][0]['payload_sha256'] == data['packed_sha256'], 'actual packed configuration differs')
    race = data['race']
    require(0 <= race['player'] < 20 and race['level'] == 1 and race['active'] == expected[:14],
            'normal player race did not activate the restored map')
    require(len(race['probes']) == len(PROBES), 'incomplete racing input probes')
    for actual, (keys, mask, throttle, steering) in zip(race['probes'], PROBES):
        require(actual['keys'] == keys and actual['mask'] == mask and
                actual['held']['held'] == mask and actual['held']['throttle'] == throttle and
                actual['held']['steering'] == steering, 'actual race control response differs')
        require(actual['held']['ticks'] >= actual['before']['ticks']+4 and
                actual['released']['ticks'] >= actual['held']['ticks'], 'player physics did not advance')
        require(all(actual['released'][field] == 0 for field in ['held', 'throttle', 'steering']),
                'released controls remain active')
        if keys == ['E']:
            require(actual['held']['position'] != actual['before']['position'], 'accelerator did not move player')
    if data['target'] == 'browser':
        require(len(data['trusted_keyboard_events']) > 0 and
                all(event['trusted'] is True for event in data['trusted_keyboard_events']),
                'browser input did not use actual Playwright keys')


def compare_settings(reference, actual):
    validate_capture(actual)
    for key in ['initial_save_sha256', 'initial', 'changed', 'restored', 'packed_sha256']:
        require(reference[key] == actual[key], 'configuration differs: '+key)
    for key in ['initial_card', 'saved_card', 'reloaded_card']:
        require({k:v for k,v in reference[key].items() if k != 'file'} ==
                {k:v for k,v in actual[key].items() if k != 'file'}, 'card metadata differs: '+key)
    require(actual['race']['active'] == reference['race']['active'] and
            actual['race']['level'] == reference['race']['level'] and
            actual['race']['player'] == reference['race']['player'], 'active player race configuration differs')


def compare(args):
    out = prepare_output(args.report)
    require(WORK in out.parents and not out.exists(), 'fresh report in /tmp/wasm-dd2 required')
    reference = json.loads((args.original/'report.json').read_text())
    validate_capture(reference)
    require(reference['pass_'] and reference['binary_sha256'] == EXE_SHA256 and
            reference['engine_state_writes'] is False, 'completed unmodified original required')
    results, negative = {}, []
    for name, directory in [('native', args.native), ('browser', args.browser)]:
        actual = json.loads((directory/'report.json').read_text())
        compare_settings(reference, actual)
        for key in ['initial_card', 'saved_card', 'reloaded_card']:
            wanted = reference[key]; observed = actual[key]
            require({k:v for k,v in wanted.items() if k != 'file'} ==
                    {k:v for k,v in observed.items() if k != 'file'}, name+' card metadata differs: '+key)
            require(Path(wanted['file']).read_bytes() == Path(observed['file']).read_bytes(), name+' complete card bytes differ')
        results[name] = actual
        for label in ['restored-map', 'saved-card-map', 'active-race-map', 'throttle', 'fast-steer', 'release', 'old-key']:
            damaged = copy.deepcopy(actual)
            if label == 'restored-map': damaged['restored']['saved'][5] ^= 1
            if label == 'saved-card-map': damaged['saved_card']['slots'][0]['binding'][5] ^= 1
            if label == 'active-race-map': damaged['race']['active'][5] ^= 1
            if label == 'throttle': damaged['race']['probes'][0]['held']['throttle'] = 0
            if label == 'fast-steer': damaged['race']['probes'][4]['held']['steering'] = -256
            if label == 'release': damaged['race']['probes'][0]['released']['held'] = 0x4000
            if label == 'old-key': damaged['race']['probes'][6]['held']['held'] = 0x4000
            try:
                compare_settings(reference, damaged)
            except RuntimeError as error:
                negative.append(dict(target=name, case=label, rejected=True, reason=str(error)))
            else:
                raise RuntimeError('Verifier accepted corrupted '+label)
    report = dict(scope=__doc__, pass_=True, original=reference, targets=results, negative_cases=negative)
    out.write_text(json.dumps(report, indent=2)+'\n')
    if args.clean:
        opened = open_files()
        for data in [reference, *results.values()]:
            for key in ['initial_card', 'saved_card', 'reloaded_card']:
                file = Path(data[key]['file'])
                require(WORK in file.resolve().parents and not file.is_symlink(), 'unexpected raw card path')
                stat = file.stat()
                require((stat.st_dev, stat.st_ino) not in opened, 'card still open')
                file.unlink()
    print('Actual configuration save/restart/card and remapped race input: PASS')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='operation', required=True)
    capture = sub.add_parser('capture')
    capture.add_argument('--target', choices=['original', 'native'], required=True)
    capture.add_argument('--binary', type=Path, default=Path('/tmp/dd2_native'))
    capture.add_argument('--output', type=Path, required=True)
    comparison = sub.add_parser('compare')
    for name in ['original', 'native', 'browser', 'report']:
        comparison.add_argument('--'+name, type=Path, required=True)
    comparison.add_argument('--clean', action='store_true')
    args = parser.parse_args()
    capture_target(args) if args.operation == 'capture' else compare(args)


if __name__ == '__main__':
    main()
