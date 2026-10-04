#!/usr/bin/env python3
"""Real native window replay save/overwrite/restart/load and cancelled/confirmed deletion.

Normal original startup, real X11 keys and read-only process/file observations.
An isolated card protects provisioned/user saves. This verifies the exercised
native functionality; complete original player video/audio parity is separate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time

from artifacts import WORK, prepare_output, check_space
from verify_native_window import symbols

ROOT = Path(__file__).resolve().parents[1]
PACKED_BYTES = 18 + 0x1c00 + 20


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


class NativeUI:
    def __init__(self, binary, game, output, display, number, limit=240, env_override=None):
        self.binary, self.game, self.output = binary, game, output
        self.table = symbols(binary)
        self.env = {k: v for k, v in os.environ.items() if not k.startswith('DD2_')}
        self.env.update(DISPLAY=display, DD2_WINDOW='1', DD2_FE='1', DD2_SOUND='1',
                        DD2_REALTIME='1', SDL_AUDIODRIVER='dummy')
        if env_override:
            for key, value in env_override.items():
                if value is None:
                    self.env.pop(key, None)
                else:
                    self.env[key] = value
        self.log = (output / f'game-{number}.log').open('wb')
        self.process = subprocess.Popen([str(binary)], cwd=game, env=self.env,
                                        stdout=self.log, stderr=self.log)
        self.memory = None
        self.started = time.monotonic()
        self.limit = limit

    def read(self, address, count):
        if self.memory is None:
            if Path(f'/proc/{self.process.pid}/exe').resolve() != self.binary:
                raise OSError('native exec not installed yet')
            self.memory = open(f'/proc/{self.process.pid}/mem', 'rb', buffering=0)
        data = os.pread(self.memory.fileno(), count, address)
        if len(data) != count:
            raise OSError('incomplete native observation')
        return data

    def integer(self, address):
        return struct.unpack('<i', self.read(address, 4))[0]

    def text(self, address):
        pointer = struct.unpack('<I', self.read(address, 4))[0]
        if not 0x400000 < pointer < 0x980400:
            return ''
        return self.read(pointer, 80).split(b'\0', 1)[0].decode('ascii', errors='replace')

    def state(self):
        fields = dict(car=0x467400, mode=0x4673f8, type=0x4673f4, season=0x93dec0,
                      level=0x936ff4, replayLevel=0x9392bc, pad=0x467078, end=0x9392c4,
                      cars=0x46765c, replay=0x467074, quit=0x7746ac, ticks=0x7746c0,
                      fileMode=0x93a318, fileSlot=0x774680)
        return {**{key: self.integer(address) for key, address in fields.items()},
                'label': self.text(0x46975c), 'practice': self.text(0x46a508),
                'file': self.text(0x467284), 'prompt': self.text(0x4672ac)}

    def wait(self, predicate, timeout=20):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.process.poll() is not None:
                raise RuntimeError(f'native exited: {self.process.returncode}')
            if time.monotonic() - self.started > self.limit:
                raise RuntimeError('native replay verification exceeded its time limit')
            check_space(self.output)
            try:
                result = predicate()
                if result:
                    return result
            except (OSError, struct.error):
                pass
            time.sleep(0.005)
        raise RuntimeError('native condition timed out: ' + json.dumps(self.state()))

    def edge(self, code, down):
        subprocess.run(['xdotool', 'keydown' if down else 'keyup', code],
                       env=self.env, check=True, timeout=5)

    def boot(self):
        self.wait(lambda: self.integer(0x462cd4) == 1)
        subprocess.run(['xdotool', 'search', '--name', '^Destruction Derby 2$',
                        'windowfocus', 'keydown', 'Escape'], env=self.env, check=True, timeout=5)
        self.wait(lambda: self.integer(0x462cd4) == 0)
        self.edge('Escape', False)
        self.wait(lambda: self.integer(0x936ff4) == 0 and 'Wrecking' in self.text(0x46975c))
        window = subprocess.check_output(['xdotool', 'search', '--name', '^Destruction Derby 2$'],
                                         env=self.env, text=True, timeout=5).splitlines()[-1]
        subprocess.run(['xdotool', 'windowfocus', '--sync', window], env=self.env, check=True, timeout=5)
        self.settled()

    def settled(self):
        previous, stable = -1, 0
        def ready():
            nonlocal previous, stable
            frame = self.integer(self.table['g_frameno'])
            if frame != previous:
                stable = stable + 1 if self.read(0x46996c, 2) == b'\0\0' else 0
                previous = frame
            return stable >= 16
        self.wait(ready)

    def key(self, code):
        live = 1 <= self.integer(0x936ff4) <= 12 and self.integer(0x7746ac) == 0 and self.integer(0x7746c0) > 0
        if not live:
            self.settled()
        vk = {'Right': 0x27, 'Left': 0x25, 'Down': 0x28, 'Up': 0x26, 'Return': 0x0d, 'Escape': 0x1b}[code]
        mapping = self.read(0x46302c, 14)
        bits = (1, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000)
        mask = next(bits[i] for i in (0, 1, 2, 3, 4, 5, 8, 6, 9, 7, 10, 11, 12, 13) if mapping[i] == vk)
        level = self.integer(0x936ff4)
        self.wait(lambda: self.controls() & mask == 0)
        self.edge(code, True)
        # Retire/Yes restores the previous pad before the next presentation.
        # The genuine level transition also acknowledges the consumed key.
        self.wait(lambda: self.controls() & mask or self.integer(0x936ff4) != level)
        self.edge(code, False)
        self.wait(lambda: struct.unpack('<H', self.read(0x754448, 2))[0] & mask == 0)
        time.sleep(0.25)
        print('X11', code, self.text(0x46975c), self.text(0x4672ac), flush=True)

    def controls(self):
        held, pressed = struct.unpack('<HH', self.read(0x754448, 4))
        return held | pressed

    def select_block(self, slot):
        self.key('Return')
        for code in ['Down'] * (slot // 3) + ['Right'] * (slot % 3):
            self.key(code)
        require(self.integer(0x774680) == slot, 'native block selection differs')
        self.key('Return')

    def enter_name(self, letter, replace=False):
        if replace:
            for code in ['Down', 'Down', 'Return', 'Up', 'Up']:
                self.key(code)
        row, column = divmod(ord(letter) - ord('A'), 13)
        for code in ['Right'] * column + ['Down'] * row + ['Return'] + ['Down'] * (2-row) + ['Right', 'Return']:
            self.key(code)

    def card(self):
        data = (self.game / 'SaveGames').read_bytes()
        require(len(data) == 0x20000, 'native card size')
        require(data == self.read(0x754460, len(data)), 'native disk card differs from engine RAM')
        slots = []
        for index in range(15):
            base = index * 0x200
            if struct.unpack_from('<I', data, base)[0] == 1:
                start = 0x2000 + index * 0x2000
                payload = data[start:start + PACKED_BYTES]
                slots.append(dict(index=index, name=data[base+4:base+32].split(b'\0', 1)[0].decode('ascii'),
                                  header=list(payload[:18]), payload_sha256=digest(payload)))
        return dict(bytes=len(data), sha256=digest(data), engine_matches_file=True, slots=slots)

    def stop(self):
        if self.memory:
            self.memory.close()
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill(); self.process.wait(timeout=5)
        self.log.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=Path('/tmp/dd2_native'))
    parser.add_argument('--output', type=Path)
    parser.add_argument('--full-card', action='store_true', help='fill all 15 slots and test full-card last-slot overwrite and navigation')
    args = parser.parse_args()
    WORK.mkdir(parents=True, exist_ok=True)
    out = prepare_output(args.output or tempfile.mkdtemp(prefix='native-replay-', dir=WORK))
    require(WORK in out.parents, 'native verification output must be under /tmp/wasm-dd2')
    out.mkdir(parents=True, exist_ok=args.output is None)
    binary = args.binary.resolve()
    report = dict(scope=__doc__, pass_=False, binary_sha256=digest(binary.read_bytes()))
    game = out / 'game'; game.mkdir()
    for asset in (ROOT / 'DestructionDerby2').iterdir():
        if asset.name != 'SaveGames':
            (game / asset.name).symlink_to(asset.resolve(), target_is_directory=asset.is_dir())
    display = ui = None
    with (out / 'xvfb.log').open('wb') as log:
        try:
            display = subprocess.Popen(['Xvfb', '-displayfd', '1', '-screen', '0', '1280x1024x24'],
                                       stdout=subprocess.PIPE, stderr=log)
            number = display.stdout.readline().decode().strip()
            require(bool(number), 'Xvfb failed to start')
            limit = 600 if args.full_card else 240
            ui = NativeUI(binary, game, out, ':' + number, 1, limit); ui.boot()
            report['initial'] = ui.card()
            for code in ['Right', 'Return', 'Right', 'Return']:
                ui.key(code)
            report['selected'] = ui.state(); require(report['selected']['car'] > 0, 'nonzero car required')
            for code in ['Left', 'Down', 'Down']:
                ui.key(code)
            require('Go!' in ui.text(0x46975c), 'native Go selection failed')
            ui.key('Return'); ui.wait(lambda: 1 <= ui.integer(0x936ff4) <= 10 and ui.integer(0x7746c0) > 0)
            ui.wait(lambda: ui.integer(0x784298) < 1, timeout=60)
            ui.edge('a', True); time.sleep(3); ui.edge('a', False)
            for code in ['Escape', 'Down', 'Down', 'Down', 'Return', 'Up', 'Return']:
                ui.key(code)
            ui.wait(lambda: 'View Replay' in ui.text(0x46a508) and ui.integer(0x7746ac) == 1, timeout=30)
            report['recorded'] = ui.state(); script = ui.read(0x9376b0, 0x1c00); order = ui.read(0x795c28, 20)
            report['script_sha256'], report['order_sha256'] = digest(script), digest(order)
            ui.key('Right'); require('Save Replay' in ui.text(0x46a508), 'native Save Replay missing')
            for code in ['Return', 'Return', 'Return', 'Return', 'Down', 'Down', 'Right', 'Return']:
                ui.key(code)
            ui.wait(lambda: (game / 'SaveGames').read_bytes()[0] == 1)
            report['saved'] = ui.card(); saved = next(s for s in report['saved']['slots'] if s['name'] == 'A')
            payload = (game / 'SaveGames').read_bytes()[0x2000 + saved['index'] * 0x2000:0x2000 + saved['index'] * 0x2000 + PACKED_BYTES]
            magic, car, end, mode, race_type, season, level, pad = struct.unpack('<HhIhhhhh', payload[:18])
            recorded = report['recorded']
            require((magic, car, end, mode, race_type, season, level, pad) ==
                    (0x2020, recorded['car'], recorded['end'], recorded['mode'], recorded['type'], recorded['season'], recorded['replayLevel'], recorded['pad']), 'native saved replay metadata differs')
            require(payload[18:18+0x1c00] == script and payload[18+0x1c00:] == order, 'native saved script/order differ')
            report['first_saved'] = report['saved']
            for code in ['Return', 'Return']:
                ui.key(code)
            require('Overwrite File' in ui.text(0x4672ac), 'native overwrite confirmation missing')
            ui.key('Escape'); report['cancelled_overwrite'] = ui.card()
            require(report['cancelled_overwrite'] == report['first_saved'], 'native cancelled overwrite changed card')
            for code in ['Return', 'Return', 'Left', 'Return']:
                ui.key(code)
            # Existing name A: third-row backspace, select B, then tick.
            for code in ['Down', 'Down', 'Return', 'Up', 'Up', 'Right', 'Return', 'Down', 'Down', 'Right', 'Return']:
                ui.key(code)
            ui.wait(lambda: (game / 'SaveGames').read_bytes()[4:6] == b'B\0')
            report['saved'] = ui.card()
            require(len(report['saved']['slots']) == 1 and report['saved']['slots'][0]['name'] == 'B', 'native overwrite did not rename the same card entry')
            new_payload = (game / 'SaveGames').read_bytes()[0x2000:0x2000 + PACKED_BYTES]
            require(new_payload == payload, 'native overwrite changed the replay payload')
            if args.full_card:
                for slot, name in enumerate('ACDEFGHIJKLMNO', 1):
                    ui.select_block(slot); ui.enter_name(name)
                    ui.wait(lambda slot=slot: (game / 'SaveGames').read_bytes()[slot*0x200] == 1)
                    card = ui.card()
                    require(len(card['slots']) == slot+1 and card['slots'][slot]['name'] == name,
                            'native filling card saved wrong slot/name')
                    stored = (game / 'SaveGames').read_bytes()[0x2000+slot*0x2000:0x2000+slot*0x2000+PACKED_BYTES]
                    require(stored == payload, 'native full-card replay payload differs')
                    print('Native populated slot', slot, name, flush=True)
                report['full_card'] = ui.card()
                ui.key('Return')
                for code in ['Down'] * 4 + ['Right'] * 2 + ['Down', 'Right', 'Down', 'Right']:
                    ui.key(code)
                require(ui.integer(0x774680) == 14, 'native full-card navigation exceeded last slot')
                ui.key('Return'); require('Overwrite File' in ui.text(0x4672ac), 'native full-card overwrite missing')
                ui.key('Escape'); report['full_card_cancelled'] = ui.card()
                require(report['full_card_cancelled'] == report['full_card'], 'native cancelled full-card overwrite changed data')
                ui.select_block(14); ui.key('Left'); ui.key('Return'); ui.enter_name('P', replace=True)
                ui.wait(lambda: (game / 'SaveGames').read_bytes()[14*0x200+4:14*0x200+6] == b'P\0')
                report['full_card_overwritten'] = ui.card()
                require(len(report['full_card_overwritten']['slots']) == 15 and
                        report['full_card_overwritten']['slots'][14]['name'] == 'P', 'native full-card last-slot overwrite failed')
                require(report['full_card_overwritten']['slots'][:14] == report['full_card']['slots'][:14], 'native full-card overwrite changed other entries')
                require((game / 'SaveGames').read_bytes()[0x2000+14*0x2000:0x2000+14*0x2000+PACKED_BYTES] == payload,
                        'native full-card overwrite changed replay payload')
                report['saved'] = report['full_card_overwritten']
            ui.stop(); ui = NativeUI(binary, game, out, ':' + number, 2, limit); ui.boot()
            report['reloaded'] = ui.card(); require(report['reloaded'] == report['saved'], 'native card did not survive process restart')
            before = ui.state(); report['before_loading'] = before
            for code in ['Right', 'Right', 'Right', 'Return', 'Return', 'Return']:
                ui.key(code)
            ui.wait(lambda: ui.integer(0x467074) == 1 and ui.integer(0x7746ac) == 0)
            require(ui.read(0x9376b0, 0x1c00) == script and ui.read(0x795c28, 20) == order, 'native loaded script/order differ')
            frames = {}; deadline = time.monotonic() + 25
            while ui.integer(0x467074) == 1 and time.monotonic() < deadline:
                if ui.integer(0x7746c0) > 0:
                    frame = ui.integer(ui.table['g_frameno'])
                    state = {**ui.state(), 'pedal': ui.integer(0x792a86)}
                    # Read-only polling can straddle Play_Game's return; do not
                    # treat frontend loading as a replay presentation.
                    if state['replay'] == 1 and state['quit'] == 0 and 1 <= state['level'] <= 12:
                        frames[frame] = state
                time.sleep(0.005)
            require(ui.integer(0x467074) == 0 and ui.integer(0x7746ac) == 1, 'native loaded playback did not finish naturally')
            report['playback'] = list(frames.values())
            require(len(frames) >= 10 and any(f['pedal'] > 0 for f in frames.values()), 'native playback failed to apply recorded acceleration')
            require(all((f['car'], f['mode'], f['type'], f['level'], f['end']) ==
                        (recorded['car'], recorded['mode'], recorded['type'], recorded['replayLevel'], recorded['end']) for f in frames.values()), 'native loaded playback metadata differs')
            ui.wait(lambda: all(ui.state()[field] == before[field] for field in ['car', 'mode', 'type', 'cars']))
            report['returned'] = ui.state()
            require('File Manager' in ui.text(0x46975c), 'native replay did not return to File Manager')
            for code in ['Return', 'Right', 'Return', 'Return']:
                ui.key(code)
            require('Delete File' in ui.text(0x4672ac), 'native delete confirmation missing')
            ui.key('Escape'); report['cancelled_deletion'] = ui.card()
            require(report['cancelled_deletion'] == report['saved'], 'native cancelled deletion changed card')
            for code in ['Return', 'Return', 'Left', 'Return']:
                ui.key(code)
            ui.wait(lambda: (game / 'SaveGames').read_bytes()[0] == 0)
            report['deleted'] = ui.card()
            require(len(report['deleted']['slots']) == len(report['saved']['slots'])-1 and
                    all(slot['index'] != 0 for slot in report['deleted']['slots']), 'native deletion failed')
            ui.stop(); ui = NativeUI(binary, game, out, ':' + number, 3, limit); ui.boot()
            report['deleted_reloaded'] = ui.card()
            require(report['deleted_reloaded'] == report['deleted'], 'native deletion did not survive restart')
            report['pass_'] = True
            print('PASS real native replay save/overwrite/restart/load/natural playback and cancelled/confirmed deletion', flush=True)
        except Exception as error:
            report['error'] = str(error)
            if ui:
                try:
                    report['last_state'] = ui.state()
                except (OSError, struct.error):
                    pass
            raise
        finally:
            if ui:
                ui.stop()
            if display:
                display.terminate(); display.wait(timeout=5)
            (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
            if report['pass_']:
                shutil.rmtree(game)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
