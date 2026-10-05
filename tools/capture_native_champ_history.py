#!/usr/bin/env python3
"""Native real keyboard-bridge championship history with strict original API inputs.

Replays actual GetTickCount returns and independently verifies each calculated
random before/after/return. No later engine-state, seed or framebuffer injection.
This does not exercise physical window-system input or audio/video sinks.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from artifacts import WORK, prepare_output, run_bounded
from verify_champ_season import (EXE, KEYS, NORMAL_ARENA_KEYS, NATURAL_CHAMP_KEYS,
    NATURAL_CHAMP_ACTIONS, NATURAL_SEASON_KEYS, NATURAL_SEASON_ACTIONS)

from natural_multiplayer_protocol import KEYS as MULTIPLAYER_KEYS, ACTIONS as MULTIPLAYER_ACTIONS

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--binary', type=Path, default=Path('/tmp/dd2_native'))
    parser.add_argument('--api-return-log', action='store_true',
                        help='observe actual provider returns in a compact log; debugger stops only for drawing/input')
    args = parser.parse_args()
    reference = args.reference.resolve()
    meta = json.loads((reference / 'history.json').read_text())
    normal_arena=meta.get('normal_arena') is True
    natural_champ=meta.get('natural_championship') is True
    natural_season=meta.get('natural_season') is True
    natural_multiplayer=meta.get('natural_multiplayer') is True
    full_video = meta.get('full_video') is True
    valid_keys=(meta['keys']==NATURAL_SEASON_KEYS and meta.get('natural_finish') is True and meta.get('actions')==NATURAL_SEASON_ACTIONS and len(meta.get('final_races',[]))==5) if natural_season else (meta['keys']==NATURAL_CHAMP_KEYS and meta.get('natural_finish') is True and meta.get('actions')==NATURAL_CHAMP_ACTIONS) if natural_champ else (meta['keys']==NORMAL_ARENA_KEYS and meta.get('natural_finish') is True) if normal_arena else meta['keys'] in (KEYS,KEYS[:17])
    if natural_multiplayer:
        valid_keys = natural_champ and not natural_season and meta['keys'] == MULTIPLAYER_KEYS and meta['actions'] == MULTIPLAYER_ACTIONS and len(meta.get('final_races', [])) == 2
    if WORK not in reference.parents or meta['exe_modified'] is not False or meta['exe_sha256'] != EXE or not valid_keys:
        raise ValueError('Supported actual original history required')
    for name, size in [('ticks.bin', meta['clock_calls'] * 4), ('random.bin', meta['rng_calls'] * 12)]:
        if (reference / name).stat().st_size != size:
            raise ValueError('Incomplete reference API input')
    output = prepare_output(args.output)
    if WORK not in output.parents or output.exists():
        parser.error('Use a fresh output directory under /tmp/wasm-dd2/')
    output.mkdir(parents=True)
    env = {key: value for key, value in os.environ.items() if not key.startswith('DD2_')}
    env.update(DD2_FE='1', DD2_SOUND='1', DD2_NOSEGV='1',
        DD2_TICK_REPLAY=str(reference / 'ticks.bin'), DD2_RANDOM_REFERENCE=str(reference / 'random.bin'),
        DD2_RANDOM_LEVEL='all', DD2_RANDOM_REQUIRE_INITIAL='1')
    api_return_log = output/'native-api-returns.bin' if args.api_return_log else None
    if api_return_log:
        env['DD2_API_OBSERVE'] = str(api_return_log)
    script = output / 'history.gdb'
    phase_gate = ''
    if full_video:
        first_phase = meta['presentations'][0]['phase']
        if not normal_arena or not 0 <= first_phase < 64 or meta.get('presentation_boundary') != 'PutDispEnv':
            raise ValueError('Full video requires a valid observed main-menu blink phase')
        # Stop on the previous normal presentation. The recorder resumes through
        # the next real pad poll/draw, matching the original first phase.
        phase_gate = f' && *(int*)0x4699cc == {(first_phase - 1) % 64}'
    else:
        # The source snapshot follows sixteen settled Draw_All entries.
        # Observe the same preceding blink phase before starting, rather than
        # comparing a newly booted menu with an arbitrary later original one.
        first = json.loads((reference/meta['checkpoints'][0]/'checkpoint.json').read_text())
        if 'phase' in first:
            if not 0 <= first['phase'] < 64:
                raise ValueError('Original initial menu phase outside its observed cycle')
            phase_gate = f' && *(int*)0x4699cc == {(first["phase"] - 16) % 64}'
    script.write_text('set pagination off\nset confirm off\nset auto-solib-add off\nset disable-randomization off\nstarti\n'+
        ('hbreak *PutDispEnv\n' if full_video else 'hbreak *Draw_All\n')+'condition 1 *(int*)0x936ff4 == 0 && *(int*)0x940010 == 0x4696b0 && *(int*)0x467420 == 0 && *(short*)0x46996c == 0'+phase_gate+'\ncontinue\ndelete 1\npython\n'+
        f'import sys, json\nsys.path.insert(0,{str(ROOT / "tools")!r})\nfrom champ_history_gdb import record_champ_history\nrecord_champ_history({str(output)!r},{len(meta["keys"])},"native",normal_arena={normal_arena!r},full_video={full_video!r},natural_champ={natural_champ!r},natural_season={natural_season!r},natural_multiplayer={natural_multiplayer!r},driving_reference=json.load(open({str(reference / "history.json")!r})).get("driving_inputs"),native_api_return_log={str(api_return_log) if api_return_log else None!r})\nend\n'+
        'printf "NATIVE_CLOCK_CONSUMED=%u\\n", dd2_tick_replay_calls()\nprintf "NATIVE_RANDOM_CONSUMED=%u\\n", dd2_random_replay_calls()\nkill\nquit\n')
    with tempfile.TemporaryDirectory(prefix='native-champ-history-assets-', dir=WORK) as tmp:
        game = Path(tmp)
        for asset in (ROOT / 'DestructionDerby2').iterdir():
            if asset.name == 'SaveGames':
                shutil.copyfile(asset, game / asset.name)
            else:
                (game / asset.name).symlink_to(asset, target_is_directory=asset.is_dir())
        save_sha = hashlib.sha256((game / 'SaveGames').read_bytes()).hexdigest()
        if save_sha != meta['initial_save_sha256']:
            raise ValueError('Initial save differs from original')
        with (output / 'gdb.log').open('w') as log:
            run_bounded(['gdb', '--nx', '-q', '-batch', '-x', str(script), str(args.binary.resolve())],
                        directory=output, cwd=game, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=7200 if natural_season or natural_multiplayer else 1500 if natural_champ else 420, check=True)
    log = (output / 'gdb.log').read_text()
    if any(message in log for message in ('ERROR: AddressSanitizer', 'ERROR: LeakSanitizer', 'runtime error:')):
        raise RuntimeError('Native history contains a sanitizer diagnostic')
    if f'NATIVE_CLOCK_CONSUMED={meta["clock_calls"]}\n' not in log or f'NATIVE_RANDOM_CONSUMED={meta["rng_calls"]}\n' not in log:
        raise RuntimeError('Actual native API-input extent differs')
    native = output / 'history/history.json'
    result = json.loads(native.read_text())
    result.update(initial_save_sha256=save_sha, binary_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),
                  api_inputs='actual original clock returns; every computed RNG triple checked, initial seed required')
    native.write_text(json.dumps(result, indent=2) + '\n')
    print('Native championship API/input history complete:', output)


if __name__ == '__main__':
    main()
