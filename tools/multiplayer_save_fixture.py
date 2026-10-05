#!/usr/bin/env python3
"""Produce and validate a real multiplayer card after two lap-completed turns.

Real X11 keys drive the unmodified original. The road follower reads positions
and uses normal keyboard input; no engine state, clocks, RNG or scores are set.
The resulting card is a file input for loading comparisons. This producer does
not compare chronological racing frames/audio or physical timing with ports.
"""
import argparse
import json
from pathlib import Path
import struct
import shutil
import subprocess
from artifacts import WORK, check_space, run_bounded
from verify_championship_save import setup, execute, snapshot, pack
from verify_configuration_persistence import ROOT, EXE_SHA256, digest, require
import multiplayer_results_protocol


def setup_keys(car, mode=0):
    keys = multiplayer_results_protocol.PLAN['load_keys'][:]
    if car == 0:
        return keys[:]
    # The name grid returns to title selection zero. Open Select Car, cycle
    # right to the requested vehicle, then reach Go from title selection one.
    count = 19
    return keys[:count] + ['Right', 'Return'] + ['Right'] * car + ['Return', 'Down', 'Down', 'Return']


def fixture(directory):
    producer = json.loads((directory / 'report.json').read_text())
    require(producer.get('pass_') is True, 'completed original multiplayer producer required')
    additional = directory / 'additional-x11-inputs.jsonl'
    if additional.exists():
        inputs = [json.loads(line) for line in additional.read_text().splitlines()]
        require(producer.get('additional_x11_inputs') == inputs and
                producer.get('additional_x11_inputs_sha256') == digest(additional.read_bytes()),
                'additional real X11 dispatches must be declared in the fixture report')
    for name, sha in producer['driving_sources'].items():
        require(Path(name).name == name and
                digest((directory/'driver-source'/name).read_bytes()) == sha,
                'immutable actual original observer sources required')
    actual_inputs = []
    for player in range(2):
        history = json.loads((directory/f'turn{player}/history/history.json').read_text())
        require(history['pass_'] and history['target'] == 'original' and
                history['exe_modified'] is False and history['exe_sha256'] == EXE_SHA256 and
                not history['engine_state_writes'] and history['standalone_turn'] == player and
                history['input'] == 'real X11 keys' and history['hardware_slots'] == 1 and
                history['recorded_api_streams'] is False and
                history['driving_policy'] == producer['driving_policy'] and
                history['driving_source_sha256'] == producer['driving_sources']['natural_champ_driver.py'],
                'actual complete read-only original player history required')
        final = history['final_race']
        finish = dict(final['driver'], tick=final['ticks'], finished=final['finished'], retired=final['retired'])
        require(finish == producer['finishes'][player], 'actual terminal race observation differs')
        actual_inputs += [dict(event, player=player) for event in history['driving_inputs']]
    require(actual_inputs == producer['driving'], 'actual original driving input history differs')
    raw = (directory / 'original.card').read_bytes()
    return validate_fixture(producer, raw)


def validate_fixture(producer, raw):
    """Accept only actual original bytes and two regular positive-score turns."""
    require(producer['operation'] == 'generate' and producer['pass_'] and (producer['binary_sha256'] == EXE_SHA256) and (not producer['engine_state_writes']), 'completed unmodified original multiplayer producer required')
    require(len(raw) == 0x20000 and digest(raw) == producer['card_sha256'] and (struct.unpack_from('<I', raw)[0] == 1) and (raw[4:6] == b'A\x00') and all((struct.unpack_from('<I', raw, i * 0x200)[0] == 0 for i in range(1, 15))), 'actual single named original card required')
    state = producer['saved_state']
    payload = raw[0x2000:0x397e]
    require(payload == pack(state) and digest(payload) == producer['payload_sha256'], 'actual original packed multiplayer state differs')
    car = producer.get('selected_car', 0)
    mode = producer.get('selected_mode',0)
    require(mode == 0,'the supplied loading route requires actual Wrecking Racing')
    require(car in (0, 1, 2), 'actual original vehicle selection required')
    require([state[k] for k in ['mode', 'type', 'car', 'race', 'season', 'races', 'multi_count', 'players', 'player', 'stats']] == [mode, 3, car, 1, 0, 5, 2, 1, 2, 0], 'actual completed first multiplayer round required')
    names = bytes.fromhex(state['names'])
    require(names[:2] == b'A\x00' and names[12:14] == b'B\x00', 'actual two named players required')
    finishes = producer['finishes']
    require(len(finishes) == 2 and all((f['finished_laps'] == 1 and f['dead'] == 0 and (f['retired'] == 0) for f in finishes)), 'both original players must complete the required laps without retirement')
    points = [struct.unpack_from('<h', bytes.fromhex(state['league']), i * 54 + 16)[0] for i in range(2)]
    require(points == producer['human_points'] and min(points) > 0, 'positive original human points required')
    expected = setup_keys(car,mode)
    require(producer['input_keys'][:len(expected)] == expected and 'Escape' not in producer['input_keys'] and producer['driving'], 'actual natural multiplayer route required')
    return (raw, payload)


def generate(args):
    args.target = 'original'
    initial=(ROOT/'DestructionDerby2/SaveGames').read_bytes()
    require(len(initial)==0x20000 and all(struct.unpack_from('<I',initial,i*0x200)[0]==0 for i in range(15)), 'empty provisioned card required')
    out, game = setup(args,initial)
    driver_source=out/'driver-source';driver_source.mkdir()
    for name in ['natural_champ_driver.py','multiplayer_turn_gdb.py']:
        shutil.copy2(ROOT/'tools'/name,driver_source/name)
    report = dict(driver=args.driver,driving_policy=args.policy,selected_car=args.car,selected_mode=args.mode,
                  max_draws=args.max_draws,driving_sources={p.name:digest(p.read_bytes()) for p in driver_source.iterdir()},
                  scope=__doc__, operation='generate', target='original', binary_sha256=EXE_SHA256, pass_=False, engine_state_writes=False, input_keys=[], driving=[], finishes=[])

    def action(ui):

        def keys(codes):
            for key in codes:
                ui.key(key)
                report['input_keys'].append(key)
        for key in setup_keys(args.car,args.mode):
            keys([key])
        initial_state = snapshot(ui)
        require(initial_state['multi_count'] == 2 and initial_state['car'] == args.car and initial_state['mode']==args.mode, 'two genuine names and the selected vehicle/mode required')
        for player in range(2):
            ui.wait(lambda: ui.integer(0x936ff4) == 1 and ui.integer(0x7746c0) >= 8 and (ui.integer(0x7746ac) == 0), 60)
            require(ui.integer(0x93decc) == player, 'actual hotseat player differs')
            turn=out/f'turn{player}';turn.mkdir()
            script=turn/'capture.gdb';script.write_text('\n'.join(['set pagination off','set confirm off','set auto-solib-add off',f'attach {ui.pid}','python','import sys',f'sys.path.insert(0,{str(ROOT/"tools")!r})',f'sys.path.insert(0,{str(driver_source)!r})','from multiplayer_turn_gdb import record_multiplayer_turn',f'record_multiplayer_turn({str(turn)!r},{player},{args.mode},{args.policy!r},{args.max_draws})','end','detach','quit'])+'\n')
            with (turn/'capture.log').open('wb') as log:
                run_bounded(['gdb','--nx','-q','-batch','-x',str(script)],directory=out,env=ui.env,stdout=log,stderr=subprocess.STDOUT,timeout=args.drive_timeout,check=True)
            history=json.loads((turn/'history/history.json').read_text());final=history['final_race']
            finish=dict(final['driver'],tick=final['ticks'],finished=final['finished'],retired=final['retired'])
            report.setdefault('attempted_finishes',[]).append(finish)
            require(finish['finished_laps']==1 and finish['dead']==0 and finish['retired']==0,'actual lap-completed natural finish required')
            report['finishes'].append(finish);report['driving']+=[dict(event,player=player) for event in history['driving_inputs']]
            print('FINISH',player,finish,flush=True)
            if player==0:keys(['Down','Return'])
            continue
        report['league_state'] = snapshot(ui)
        league = bytes.fromhex(report['league_state']['league'])
        report['human_points'] = [struct.unpack_from('<h', league, i * 54 + 16)[0] for i in range(2)]
        require(min(report['human_points']) > 0, 'both original humans must earn positive points')
        keys(['Down', 'Return'])
        ui.wait(lambda: ui.integer(0x940010) == 0x46a9dc)
        require('Save Game' in ui.text(0x46aa74), 'actual multiplayer save menu required')
        report['saved_state'] = snapshot(ui)
        payload = pack(report['saved_state'])
        keys(['Return'])
        ui.wait(lambda: ui.integer(0x93a318) == 4)
        require(ui.read(0x93a490, 6526) == payload, 'actual save packing differs')
        keys(['Return'])
        ui.wait(lambda: 'Select' in ui.text(0x46725c) and ui.integer(0x774680) == 0)
        keys(['Return'])
        ui.enter_name('A')
        ui.wait(lambda: (ui.game / 'SaveGames').read_bytes()[0] == 1)
        raw = (ui.game / 'SaveGames').read_bytes()
        require(raw == ui.read(0x754460, len(raw)), 'original card disk/RAM differ')
        require(raw[0x2000:0x397e] == payload, 'saved multiplayer payload differs')
        completed = dict(report, card_sha256=digest(raw), payload_sha256=digest(payload), pass_=True)
        validate_fixture(completed, raw)
        (out / 'original.card').write_bytes(raw)
        report.update(completed)
    try:
        execute(args, out, game, action)
    finally:
        (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        check_space(out)
    fixture(out)
    print('Original natural multiplayer fixture:', report['human_points'], flush=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--timeout', type=int, default=4000)
    parser.add_argument('--drive-timeout',type=int,default=1800)
    parser.add_argument('--max-draws',type=int,default=60000,help='bounded observations per original turn (1000..300000)')
    parser.add_argument('--driver',choices=['hardware'],default='hardware')
    parser.add_argument('--policy',choices=['steady','steady-persistent'],default='steady-persistent')
    parser.add_argument('--car',type=int,choices=[0,1,2],default=0)
    parser.add_argument('--mode',type=int,choices=[0],default=0,help='Wrecking Racing; Stock Car needs a separate four-round acceptance route')
    args = parser.parse_args()
    if not 1000<=args.max_draws<=300000:parser.error('--max-draws must be 1000..300000')
    require(WORK in args.output.resolve().parents, 'fixture belongs in /tmp/wasm-dd2')
    generate(args)
if __name__ == '__main__':
    main()
