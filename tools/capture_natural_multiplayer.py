#!/usr/bin/env python3
"""Record two original hotseat races with actual keys and clock/RNG streams.

Four read-only hardware breakpoint slots observe the unchanged executable.
Compressed racing frames are bounded by the shared 2 GiB capture guard.
This original-only recorder does not itself accept port or audio parity.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import time
import shutil

from artifacts import check_space, run_bounded
from verify_championship_save import setup, execute
from verify_configuration_persistence import ROOT, EXE_SHA256, digest, require
from natural_multiplayer_protocol import ACTIONS, KEYS, NAMES, SCOPE


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--timeout', type=int, default=7200)
    parser.add_argument('--speed-limit', type=int, default=250, choices=range(60, 251), metavar='60..250')
    args = parser.parse_args()
    args.target = 'original'
    initial = (ROOT/'DestructionDerby2/SaveGames').read_bytes()
    require(len(initial) == 0x20000 and all(struct.unpack_from('<I', initial, i*0x200)[0] == 0 for i in range(15)),
            'Actual provisioned empty card required')
    out, game = setup(args, initial)
    sources = out/'driver-source'
    sources.mkdir()
    names = ['champ_history_gdb.py', 'natural_champ_driver.py',
             'natural_multiplayer_protocol.py', 'natural_multiplayer_ui.json',
             'multiplayer_results_protocol.py', 'multiplayer_results_ui.json']
    for name in names:
        shutil.copyfile(ROOT/'tools'/name, sources/name)
    report = dict(scope=SCOPE, operation='original-history', pass_=False,
                  port_comparison='pending', chronological_audio='unproven',
                  engine_state_writes=False, binary_sha256=EXE_SHA256,
                  driving_speed_limit=args.speed_limit,
                  observer_sources={name:digest((sources/name).read_bytes()) for name in names})
    try:
        def action(ui):
            script = out/'history.gdb'
            script.write_text('set pagination off\nset confirm off\nset auto-solib-add off\n'
                f'attach {ui.pid}\npython\nimport sys\nsys.path.insert(0,{str(ROOT/"tools")!r})\n'
                f'sys.path.insert(0,{str(sources)!r})\n'
                'from champ_history_gdb import record_champ_history\n'
                f'record_champ_history({str(out)!r},natural_champ=True,natural_multiplayer=True,natural_multiplayer_speed={args.speed_limit})\n'
                'end\ndetach\nquit\n')
            with (out/'history.log').open('wb') as log:
                run_bounded(['gdb', '--nx', '-q', '-batch', '-x', str(script)],
                    directory=out, env=ui.env, stdout=log, stderr=subprocess.STDOUT,
                    timeout=max(1, ui.deadline-time.monotonic()-10), check=True)
            require((ui.game/'SaveGames').read_bytes() == initial, 'Recording changed actual save card')
        execute(args, out, game, action)
        path = out/'history/history.json'
        meta = json.loads(path.read_text())
        require(meta['keys'] == KEYS and meta['actions'] == ACTIONS and meta['checkpoints'] == NAMES,
                'Complete multiplayer route required')
        require(len(meta['final_races']) == 2, 'Both actual race finishes required')
        for final in meta['final_races']:
            require(final['retired'] == 0 and final['finished'] > 14,
                    'Both races must finish naturally')
        meta.update(initial_save_sha256=digest(initial), binary_sha256=EXE_SHA256)
        path.write_text(json.dumps(meta, indent=2)+'\n')
        report.update(pass_=True, racing_frames=len(meta['race_frames']),
                      clock_calls=meta['clock_calls'], rng_calls=meta['rng_calls'],
                      finishes=meta['final_races'])
    finally:
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        check_space(out)
    print('Original natural multiplayer history complete:', out, flush=True)


if __name__ == '__main__':
    main()
