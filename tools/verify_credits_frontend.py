#!/usr/bin/env python3
"""Exercise the actual native CREDITZ! name-grid route through Outro and exit.

Read-only name/cursor observations and real X11 keys follow the original route.
This is frontend/exit acceptance, not a new literal video, PCM or timing proof.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import time

from artifacts import check_space
from driver_name_input import name_actions
from verify_championship_save import setup
from verify_configuration_persistence import ROOT, EXE_SHA256, digest, require
from verify_native_replay import NativeUI


def reference(directory):
    original = json.loads((directory/'report.json').read_text())
    require(original['pass_'] and original['exe_modified'] is False and
            original['exe_sha256'] == EXE_SHA256 and original['original_process_exited'] and
            original['outro_started'] and not original['engine_state_writes'] and original['card_unchanged'],
            'Completed actual original credits route required')
    require(original['input_keys'] == ['Return']*3+[a['key'] for a in name_actions('CREDITZ!')] and
            original['name_helper_sha256'] == digest((ROOT/'tools/driver_name_input.py').read_bytes()),
            'Original credit-grid input provenance differs')
    return original


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', type=Path, required=True)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    original = reference(args.original)
    initial = (ROOT/'DestructionDerby2/SaveGames').read_bytes()
    require(digest(initial) == original['initial_save_sha256'], 'Actual initial card differs')
    out, game = setup(args, initial)
    report = dict(scope=__doc__.strip(), target='native', pass_=False,
                  engine_state_writes=False, input_keys=[], grid_observations=[],
                  original_report=str((args.original/'report.json').resolve()),
                  binary_sha256=digest(args.binary.read_bytes()))
    ui = display = None
    try:
        with (out/'xvfb.log').open('wb') as log:
            display = subprocess.Popen(['Xvfb', '-displayfd', '1', '-screen', '0', '640x480x24'],
                                       stdout=subprocess.PIPE, stderr=log)
            number = display.stdout.readline().decode().strip()
            require(number, 'Native credits display failed')
            ui = NativeUI(args.binary.resolve(), game, out, ':'+number, 1, 300)
            ui.boot()
            for key in ['Return']*3:
                ui.key(key); report['input_keys'].append(key)
            require(ui.integer(0x940010) == 0x469f70, 'Actual native driver-name grid required')
            for action in name_actions('CREDITZ!')[:-1]:
                ui.key(action['key']); report['input_keys'].append(action['key'])
                name = ui.read(ui.integer(0x469fd4)+8, 10).split(b'\0')[0].decode('ascii')
                cursor = list(struct.unpack('<hh', ui.read(0x469f34, 4)))
                require(name == action['entered'] and cursor == action['cursor'], 'Actual native credit grid differs')
                report['grid_observations'].append(dict(name=name, cursor=cursor))
            report['before_accept'] = dict(name=name, movie=ui.integer(0x462cd4), menu=ui.integer(0x940010))
            require(report['before_accept'] == original['before_accept'], 'Native credit acceptance state differs')
            # The final Return leaves ReadPad; it has no later menu poll to
            # acknowledge. Ordinary key delivery matches the original driver.
            subprocess.run(['xdotool', 'key', '--delay', '50', 'Return'], env=ui.env, check=True, timeout=5)
            report['input_keys'].append('Return')
            ui.wait(lambda: ui.integer(0x462cd4) == 1, 15)
            started = time.monotonic()
            while ui.process.poll() is None:
                require(time.monotonic()-started < 120, 'Native credit Outro did not exit')
                check_space(out); time.sleep(.1)
            report['outro_elapsed_seconds'] = time.monotonic()-started
            require(report['outro_elapsed_seconds'] > 70, 'Native credits movie ended early')
            report['exit_code'] = ui.process.returncode
            require(ui.process.returncode == 0, 'Original Play_Xtro exit(0) required')
            require(report['input_keys'] == original['input_keys'], 'Native credits input sequence differs')
            require((game/'SaveGames').read_bytes() == initial, 'Native credit route changed the card')
            report.update(pass_=True, process_exited=True, exit_code=ui.process.returncode, card_unchanged=True)
    except BaseException as error:
        report['error'] = repr(error)
        raise
    finally:
        if ui: ui.stop()
        if display:
            display.terminate(); display.wait(timeout=5)
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('Native actual CREDITZ! grid, Outro and exit PASS', flush=True)


if __name__ == '__main__':
    main()
