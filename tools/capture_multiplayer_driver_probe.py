#!/usr/bin/env python3
"""Preflight actual living original hotseat finishes before a full API capture.

Use real menu/driving keys and one read-only hardware drawing breakpoint.
No engine fields, code, clock returns or RNG are replaced. GDB changes timing.
This original-only probe does not establish port, framebuffer or audio parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference')]
from artifacts import check_space, run_bounded
from reference import capture
from verify_championship_save import setup
from verify_configuration_persistence import ROOT, OriginalUI, original_args, require
from natural_multiplayer_protocol import ACTIONS, OVER_STATES


def sha(path):
    with Path(path).open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--driver-source', type=Path, default=ROOT / 'tools/live_lap_record_driver.py',
                        help='explicit host keyboard policy; frozen and hash-bound before starting')
    parser.add_argument('--turns', type=int, choices=(1, 2), default=2)
    parser.add_argument('--seconds-per-turn', type=int, default=900, choices=range(30, 1801), metavar='30..1800')
    parser.add_argument('--max-ticks', type=int, default=90000, choices=range(1000, 200001), metavar='1000..200000')
    parser.add_argument('--max-draws', type=int, default=30000, choices=range(100, 30001), metavar='100..30000')
    args = parser.parse_args()
    initial = (ROOT / 'DestructionDerby2/SaveGames').read_bytes()
    require(len(initial) == 0x20000 and all(initial[i * 0x200:i * 0x200 + 4] == b'\0' * 4 for i in range(15)),
            'provisioned empty original save card required')
    out, game = setup(args, initial)
    policy = out / 'driver-source'
    policy.mkdir()
    shutil.copyfile(args.driver_source.resolve(), policy / 'live_lap_record_driver.py')
    shutil.copyfile(ROOT / 'tools/reference/original_driving_probe.py', policy / 'original_driving_probe.py')
    sources = {str(path): sha(path) for path in (Path(__file__),
        ROOT / 'tools/reference/capture.py', ROOT / 'tools/verify_configuration_persistence.py',
        ROOT / 'tools/verify_championship_save.py', ROOT / 'tools/natural_multiplayer_protocol.py',
        ROOT / 'tools/natural_multiplayer_ui.json', *policy.glob('*.py'))}
    capture.WORK = out / 'work'
    capture.WORK.mkdir()
    report = dict(scope=__doc__, pass_=False, engine_state_writes=False,
                  original_port_parity='unproven', exe_sha256=capture.EXE_SHA256,
                  driver_source=str(args.driver_source.resolve()), sources=sources,
                  turns_required=args.turns, finishes=[], input_keys=[])

    def action(pid, directory, env, deadline, rundir):
        ui = OriginalUI(pid, rundir, directory, env, deadline)
        try:
            report['actual_process'] = dict(pid=pid,
                start_token=Path(f'/proc/{pid}/stat').read_text().split(') ', 1)[1].split()[19])
            for key in ACTIONS[:22]:
                ui.key(key)
                report['input_keys'].append(key)
            for player in range(args.turns):
                ui.wait(lambda: ui.integer(0x936ff4) == 1 and ui.integer(0x7746c0) >= 8 and
                        ui.integer(0x7746ac) == 0 and ui.integer(0x93decc) == player, 60)
                require(ui.integer(0x4673f4) == 3 and ui.integer(0x4673f8) == 0 and
                        ui.integer(0x467658) == 2, 'actual two-player Wrecking race required')
                turn = out / f'player-{player}'
                turn.mkdir()
                script = turn / 'probe.gdb'
                script.write_text('set pagination off\nset confirm off\nset auto-solib-add off\n'
                    'handle SIGSEGV nostop noprint pass\nhandle SIGUSR1 nostop noprint pass\n'
                    'handle SIG32 nostop noprint pass\nhandle SIG33 nostop noprint pass\n'
                    f'attach {pid}\npython\nimport sys\nsys.path.insert(0,{str(policy)!r})\n'
                    'from original_driving_probe import drive\n'
                    f'drive({str(turn)!r},{player},{args.max_ticks},{args.max_draws},{args.seconds_per_turn})\n'
                    'end\ndetach\nquit\n')
                with (turn / 'probe.log').open('w') as log:
                    run_bounded(['gdb', '--nx', '-q', '-batch', '-x', str(script)],
                        directory=out, env=env, stdout=log, stderr=subprocess.STDOUT,
                        timeout=min(args.seconds_per_turn + 30, deadline - time.monotonic() - 10), check=True)
                finish = json.loads((turn / 'report.json').read_text())
                require(finish['driver_sha256'] == sources[str(policy / 'live_lap_record_driver.py')],
                        'actual driving policy source changed')
                report['finishes'].append(finish)
                require(finish['pass_'], 'actual living lap-completed finish required')
                ui.wait(lambda: ui.integer(0x936ff4) == 15 and
                        ui.integer(0x940010) == OVER_STATES[player]['poly_list'], 60)
                if player + 1 < args.turns:
                    for key in ACTIONS[23:25]:
                        ui.key(key)
                        report['input_keys'].append(key)
            report['card_unchanged'] = (rundir / 'SaveGames').read_bytes() == initial
            require(report['card_unchanged'], 'probe changed original save card')
        finally:
            ui.stop()

    try:
        options = original_args()
        options.timeout = args.turns * (args.seconds_per_turn + 100) + 120
        capture.run(game, out, options, on_menu=action)
        require(all(sha(path) == digest for path, digest in sources.items()), 'probe sources changed during run')
        require(len(report['finishes']) == args.turns, 'requested hotseat finishes missing')
        report['pass_'] = True
    except BaseException as error:
        report['error'] = str(error)
        raise
    finally:
        (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        check_space(out)
    print('Living original driving preflight:', args.turns, 'turns passed; port A/V comparison remains open')


if __name__ == '__main__':
    main()
