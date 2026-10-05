#!/usr/bin/env python3
"""Record championship result menus with observed clock/RNG inputs and acknowledged keys.

Original uses real X11 input and read-only hardware breakpoints. Native uses
its keyboard bridge and the recorded original clock stream; every computed RNG
triple is independently checked. Full menu cycles are retained for loaded
single-player or multiplayer cards and a fresh two-player multiplayer season.
This recorder alone does not accept original video/audio parity.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess

from artifacts import check_space, run_bounded
import race_results_protocol
import multiplayer_results_protocol
import multiplayer_loaded_protocol
from multiplayer_save_fixture import fixture as multiplayer_fixture
from verify_championship_save import fixture, setup, execute
from verify_configuration_persistence import ROOT, digest, EXE_SHA256, require


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--target',choices=['original','native'],required=True)
    parser.add_argument('--fixture',type=Path)
    parser.add_argument('--multiplayer',action='store_true')
    parser.add_argument('--loaded-multiplayer',action='store_true')
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--reference',type=Path)
    parser.add_argument('--binary',type=Path,default=Path('/tmp/dd2_native'))
    args = parser.parse_args()
    require(not(args.multiplayer and args.loaded_multiplayer),'choose fresh or loaded multiplayer')
    protocol = multiplayer_loaded_protocol if args.loaded_multiplayer else multiplayer_results_protocol if args.multiplayer else race_results_protocol
    KEYS,TABLES = protocol.KEYS,getattr(protocol,'CAPTURES',protocol.TABLES)
    mode = protocol.PLAN['result_mode']
    result_race_mode=None
    if args.multiplayer:
        require(args.fixture is None,'fresh multiplayer uses the provisioned empty card')
        initial = (ROOT/'DestructionDerby2/SaveGames').read_bytes()
        import struct
        require(len(initial)==0x20000 and all(struct.unpack_from('<I',initial,i*0x200)[0]==0 for i in range(15)),
                'actual fresh provisioned card required')
    else:
        require(args.fixture is not None,'original-produced championship fixture required')
        initial, _ = (multiplayer_fixture if args.loaded_multiplayer else fixture)(args.fixture)
        if args.loaded_multiplayer:
            result_race_mode=json.loads((args.fixture/'report.json').read_text())['saved_state']['mode']
    out, game = setup(args,initial)
    report = dict(scope=protocol.PLAN['scope'],pass_=False,engine_state_writes=False,target=args.target,
                  initial_card_sha256=digest(initial),fixture=str(args.fixture.resolve()) if args.fixture else None)
    common = ['python','import sys',f'sys.path.insert(0,{str(ROOT/"tools")!r})',
              'from champ_history_gdb import record_champ_history',
              f'record_champ_history({str(out)!r},target={args.target!r},result_tables={mode!r},result_reference={str(args.reference.resolve()) if args.reference else None!r},result_race_mode={result_race_mode!r})','end']
    script = out/'history.gdb'
    try:
        if args.target == 'original':
            def action(ui):
                script.write_text('\n'.join(['set pagination off','set confirm off','set auto-solib-add off',
                    f'attach {ui.pid}',*common,'detach','quit'])+'\n')
                with (out/'history.log').open('wb') as log:
                    run_bounded(['gdb','--nx','-q','-batch','-x',str(script)],directory=out,env=ui.env,
                                stdout=log,stderr=subprocess.STDOUT,timeout=650,check=True)
                require((ui.game/'SaveGames').read_bytes()==initial,'history changed the actual card')
            execute(args,out,game,action)
            report['binary_sha256'] = EXE_SHA256
        else:
            require(args.reference is not None,'actual original history reference required')
            source = json.loads((args.reference/'history.json').read_text())
            require(source['result_tables'] == mode and source['exe_modified'] is False and
                    source['exe_sha256'] == EXE_SHA256 and source['keys'] == KEYS and
                    source['initial_save_sha256'] == digest(initial),'supported actual source required')
            for name,size in [('ticks.bin',source['clock_calls']*4),('random.bin',source['rng_calls']*12)]:
                require((args.reference/name).stat().st_size==size,'incomplete original API stream')
            env = {k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
            env.update(DD2_FE='1',DD2_SOUND='1',DD2_NOSEGV='1',DD2_TICK_REPLAY=str((args.reference/'ticks.bin').resolve()),
                       DD2_RANDOM_REFERENCE=str((args.reference/'random.bin').resolve()),DD2_RANDOM_LEVEL='all',
                       DD2_RANDOM_REQUIRE_INITIAL='1')
            script.write_text('\n'.join(['set pagination off','set confirm off','set auto-solib-add off',
                'set disable-randomization off','starti','hbreak *Draw_All',
                'condition 1 *(int*)0x936ff4 == 0 && *(int*)0x940010 == 0x4696b0 && *(int*)0x467420 == 0 && *(short*)0x46996c == 0',
                'continue','delete 1',*common,
                'printf "CLOCK_USED=%u\\n", dd2_tick_replay_calls()',
                'printf "RANDOM_USED=%u\\n", dd2_random_replay_calls()','kill','quit'])+'\n')
            with (out/'history.log').open('wb') as log:
                run_bounded(['gdb','--nx','-q','-batch','-x',str(script),str(args.binary.resolve())],directory=out,
                            cwd=game,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=650,check=True)
            log = (out/'history.log').read_text(errors='replace')
            require(f'CLOCK_USED={source["clock_calls"]}\n' in log and
                    f'RANDOM_USED={source["rng_calls"]}\n' in log,'actual API extent differs')
            require('AddressSanitizer' not in log and 'runtime error:' not in log,'sanitizer diagnostic')
            require((game/'SaveGames').read_bytes()==initial,'native history changed card')
            report['binary_sha256'] = digest(args.binary.read_bytes())
        path = out/'history/history.json'; history = json.loads(path.read_text())
        require(history['result_tables'] == mode and history['keys']==KEYS and len(history['checkpoints'])==len(KEYS)+1,
                'complete result history required')
        history.update(initial_save_sha256=digest(initial),binary_sha256=report['binary_sha256'])
        for step in TABLES:
            point = json.loads((out/'history'/history['checkpoints'][step]/'checkpoint.json').read_text())
            cycle = json.loads((Path(point['cycle'])/'cycle.json').read_text())
            require(len(cycle['frames'])==64 and {f['phase'] for f in cycle['frames']}==set(range(64)),
                    'all actual table phases required')
        path.write_text(json.dumps(history,indent=2)+'\n')
        report['pass_']=True
    finally:
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(out)
    print('Result menu clock/RNG history:',args.target,'PASS',flush=True)


if __name__ == '__main__': main()
