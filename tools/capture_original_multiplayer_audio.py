#!/usr/bin/env python3
"""Capture original two-human menu/racing audio without debugger pauses.

Real X11 input and bounded repeated, non-atomic read-only observations drive
the unchanged original. --menu-only verifies the A/B route and live race start.
The full route requires two living finishes; neither route establishes matched
original/port clocks, chronological video, engine PCM or full-game parity.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import time

from artifacts import check_space
import live_lap_record_driver as policy
from natural_multiplayer_protocol import ACTIONS, OVER_STATES
from original_realtime_ui import OriginalRealtimeUI
from reference import capture
from reference.audio import summarize_audio
from verify_championship_save import setup
from verify_configuration_persistence import ROOT, original_args, require


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def drive_turn(ui, out, player, deadline):
    ui.wait(lambda: ui.integer(0x936ff4) == 1 and ui.integer(0x7746c0) >= 8 and
            ui.integer(0x7746ac) == 0 and ui.integer(0x93decc) == player, 60)
    require(ui.integer(0x4673f4) == 3 and ui.integer(0x4673f8) == 0 and
            ui.integer(0x467658) == 2, 'actual two-human Wrecking race required')
    driver = policy.ArcRoadKeyboardDriver()
    end = min(deadline - 10, time.monotonic() + 900)
    previous = None
    samples = rejected = 0
    last_report = last_space = 0
    held = set()
    turn = out / f'player-{player}'
    turn.mkdir()

    def release():
        for key in sorted(held):
            ui.edge(key, False)
        held.clear()

    try:
        with (turn / 'driver.jsonl').open('x') as journal:
            while time.monotonic() < end:
                now = time.monotonic()
                if now - last_space >= 1:
                    check_space(out)
                    last_space = now
                if ui.integer(0x7746ac):
                    break
                require(ui.integer(0x936ff4) == 1 and ui.integer(0x93decc) == player,
                        'live race owner/level changed before quit')
                if ui.integer(0x795df4) > 14:
                    release()
                    time.sleep(.005)
                    continue
                if ui.integer(0x784298) >= 0:
                    time.sleep(.005)
                    continue
                tick = ui.integer(0x7746c0)
                if tick == previous:
                    time.sleep(.002)
                    continue
                reads = {}

                def read(address, size):
                    key = (address, size)
                    if key not in reads:
                        reads[key] = ui.read(address, size)
                    return reads[key]

                candidate = copy.deepcopy(driver)
                try:
                    row = candidate.controls(read, tick)
                except (ValueError, OSError):
                    rejected += 1
                    time.sleep(.002)
                    continue
                # These repeated equal reads are a bounded observation filter;
                # they are not an atomic snapshot of the running engine.
                if (ui.integer(0x7746c0) != tick or
                        any(ui.read(address, size) != raw
                            for (address, size), raw in reads.items())):
                    rejected += 1
                    time.sleep(.002)
                    continue
                wanted = set(row['wanted'])
                require(wanted <= {'a', 'z', 'Left', 'Right', 'space'}, 'unsupported driver key')
                changes = []
                for key in sorted(held - wanted):
                    ui.edge(key, False)
                    changes.append(dict(key=key, down=False))
                for key in sorted(wanted - held):
                    ui.edge(key, True)
                    changes.append(dict(key=key, down=True))
                held = wanted
                previous, driver = tick, candidate
                samples += 1
                journal.write(json.dumps(dict(tick=tick, observed_monotonic_ns=time.monotonic_ns(),
                                              repeated_reads=len(reads), changes=changes, **row)) + '\n')
                if now - last_report >= 5:
                    last_report = now
                    journal.flush()
                    progress = dict(player=player, samples=samples, rejected_samples=rejected,
                                    tick=tick, **row)
                    (turn / 'progress.json').write_text(json.dumps(progress, indent=2) + '\n')
                    print('Realtime driver:', player, samples, 'samples;', row['lap'],
                          row['lap_progress'], 'lap/checkpoint;', row['speed'],
                          'speed;', row['manoeuvre'], flush=True)
                require(samples < 60000, 'bounded driver journal exceeded')
        require(ui.integer(0x7746ac) != 0, 'actual living finish did not occur before bound')
        final = dict(driver=policy.metrics(ui.read), finished=ui.integer(0x795df4),
                     retired=ui.integer(0x9376a8), samples=samples, rejected_samples=rejected)
        require(final['driver']['dead'] == 0 and final['driver']['finished_laps'] == 1 and
                final['finished'] > 14 and final['retired'] == 0,
                'actual living completed laps required')
        return final
    finally:
        release()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--menu-only', action='store_true', help='stop after real menu route and live race start')
    parser.add_argument('--trace-keyboard', action='store_true', help='include actual Wine window-procedure messages')
    args = parser.parse_args()
    initial = (ROOT / 'DestructionDerby2/SaveGames').read_bytes()
    out, game = setup(args, initial)
    capture.WORK = out / 'work'
    sources = [Path(__file__), ROOT / 'tools/original_realtime_ui.py', Path(policy.__file__),
               ROOT / 'tools/reference/capture.py', ROOT / 'tools/reference/trace_log.py',
               ROOT / 'tools/verify_championship_save.py', ROOT / 'tools/verify_native_replay.py',
               ROOT / 'tools/verify_configuration_persistence.py',
               ROOT / 'tools/natural_multiplayer_protocol.py', ROOT / 'tools/natural_multiplayer_ui.json']
    archive = out / 'capture-source'
    archive.mkdir()
    for source in sources:
        shutil.copyfile(source, archive / source.name)
    report = dict(scope=__doc__, pass_=False, menu_only=args.menu_only,
                  debugger=False, engine_state_writes=False,
                  original_exe_sha256=capture.EXE_SHA256,
                  initial_save_sha256=sha(game / 'SaveGames'),
                  source_hashes={str(p): sha(p) for p in sources}, input_keys=[], finishes=[])

    def action(pid, directory, env, deadline, rundir):
        ui = OriginalRealtimeUI(pid, rundir, directory, env, deadline)
        try:
            report['actual_process'] = dict(pid=pid, start_token=Path(f'/proc/{pid}/stat')
                                           .read_text().split(') ', 1)[1].split()[19])
            focus = subprocess.run(['xdotool', 'getwindowfocus', 'getwindowname'], env=env,
                                   capture_output=True, text=True, check=True, timeout=5)
            require(focus.stdout.strip() == 'PC-DD2', 'actual original keyboard focus required')
            report['initial_x11_focus'] = focus.stdout.strip()
            for index, key in enumerate(ACTIONS[:22], 1):
                ui.key(key)
                report['input_keys'].append(key)
                expected_menu = (0x46a288 if index == 1 else 0x46a624 if index <= 5 else
                                 0x469f70 if index <= 18 else 0x4696b0 if index <= 21 else 0x4674bc)
                ui.wait(lambda: ui.integer(0x940010) == expected_menu and
                        ui.integer(0x936ff4) == (1 if index == 22 else 0), 20)
            require(ui.read(0x93e318, 12).split(b'\0')[0] == b'A' and
                    ui.read(0x93e324, 12).split(b'\0')[0] == b'B', 'actual A/B names required')
            require(ui.integer(0x4673f4) == 3 and ui.integer(0x467658) == 2 and
                    ui.integer(0x4673f8) == 0, 'actual two-human Wrecking mode required')
            ui.wait(lambda: ui.integer(0x936ff4) == 1 and ui.integer(0x7746c0) >= 8 and
                    ui.integer(0x7746ac) == 0 and ui.integer(0x93decc) == 0, 60)
            report['menu_route'] = dict(pass_=True, names=['A', 'B'], type=3, count=2,
                                       live_start=capture.state(pid), key_effects=copy.deepcopy(ui.input_history))
            (out / 'menu-route-validation.json').write_text(json.dumps(dict(
                scope='Actual original A/B menu input and live race start; no port or A/V parity',
                **report['menu_route']), indent=2) + '\n')
            if not args.menu_only:
                for player in range(2):
                    report['finishes'].append(drive_turn(ui, out, player, deadline))
                    ui.wait(lambda: ui.integer(0x936ff4) == 15 and
                            ui.integer(0x940010) == OVER_STATES[player]['poly_list'], 60)
                    if player == 0:
                        for key in ACTIONS[23:25]:
                            ui.key(key)
                            report['input_keys'].append(key)
            require((rundir / 'SaveGames').read_bytes() == initial and
                    ui.read(0x754460, len(initial)) == initial, 'actual original card changed')
            report['card_unchanged'] = True
        except BaseException:
            report['failure_state'] = capture.state(pid)
            report['failure_menu'] = ui.observed_menu()
            raise
        finally:
            (out / 'input-effects.json').write_text(json.dumps(ui.input_history, indent=2) + '\n')
            ui.stop()

    try:
        options = original_args()
        options.timeout = 150 if args.menu_only else 2120
        options.mode = 'audio'
        options.audio = True
        options.audio_rate = 44100
        options.audio_device = 'clock'
        options.trace_cd = True
        options.compress_trace = True
        options.wine_debug = '-all,+timestamp,+dsound,+ddraw'
        if args.trace_keyboard:
            options.wine_debug += ',+relay,+msg'
            # Restrict API relay to startup/timer calls; window procedure logs
            # still retain real key pairs without tracing all GetTickCount loops.
            options.trace_multimedia_timer = True
        report['capture_options'] = dict(wine_debug=options.wine_debug, compress_trace=True,
                                        audio_rate=44100, audio_device='clock', trace_cd=True,
                                        trace_multimedia_timer=options.trace_multimedia_timer)
        capture.run(game, out, options, on_menu=action)
        (out / 'audio/summary.json').write_text(json.dumps(
            summarize_audio(out / 'audio', require_played=True), indent=2) + '\n')
        require(all(sha(p) == h for p, h in report['source_hashes'].items()),
                'capture sources changed while live')
        require(args.menu_only or len(report['finishes']) == 2, 'both living finishes required')
        report['pass_'] = True
    except BaseException as error:
        report['error'] = str(error)
        raise
    finally:
        (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        check_space(out)
    print('Original no-debugger menu/audio capture: PASS; port A/V comparison pending', flush=True)


if __name__ == '__main__':
    main()
