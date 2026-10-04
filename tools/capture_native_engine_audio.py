#!/usr/bin/env python3
"""Capture actual native startup/attract audio with observed service timing.

An X11 Escape key skips the real intro. The normal application entry initializes
the frontend and starts audio itself. The live engine supplies all sound
controls; observed original fields are assertions. Exact PCM verification,
video and interactive scheduling are separate checks.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import time

from artifacts import WORK, prepare_output, check_space
from verify_native_window import symbols

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools/reference'))
from capture import state, menu_ready


def capture(binary, output, audio_clock, game_clock):
    output = prepare_output(output)
    if WORK not in output.parents:
        raise ValueError('Engine diagnostics must be under /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    check_space(output)
    game = output/'game'
    game.mkdir()
    for asset in (ROOT/'DestructionDerby2').iterdir():
        if asset.name == 'SaveGames':
            shutil.copyfile(asset, game/asset.name)
        else:
            (game/asset.name).symlink_to(asset, target_is_directory=asset.is_dir())
    table = symbols(binary)
    env = {k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    save_sha256=hashlib.sha256((game/'SaveGames').read_bytes()).hexdigest()
    env['DD2_AUDIO_SERVICES']=str(audio_clock)
    env['DD2_TICK_REPLAY']=str(game_clock)
    env['DD2_RACE_STREAM']=str(output/'race-stream.jsonl')
    env['DD2_AUDIO_SERVICE_REPORT']=str(output/'clock-complete.json')
    display = process = None
    with (output/'run.log').open('wb') as log:
        try:
            display = subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','1280x1024x24'],
                                       stdout=subprocess.PIPE, stderr=log)
            number = display.stdout.readline().decode().strip()
            if not number:
                raise RuntimeError('Xvfb did not start')
            env['DISPLAY'] = ':'+number
            process = subprocess.Popen([str(binary)], cwd=game, stdout=log, stderr=log,
                env={**env,'DD2_WINDOW':'1','SDL_AUDIODRIVER':'dummy',
                     'DD2_SNDLOG':str(output/'sound.log'),
                     'DD2_MIXPCM':str(output/'mixed.pcm')})
            def observed():
                if process.poll() is not None:
                    raise RuntimeError(f'Native startup exited: {process.returncode}')
                if Path(f'/proc/{process.pid}/exe').resolve() != binary:
                    raise OSError('Child exec has not installed the native executable')
                current = state(process.pid)
                with open(f'/proc/{process.pid}/mem','rb',buffering=0) as memory:
                    current['completed_flips'] = struct.unpack('<i', os.pread(
                        memory.fileno(),4,table['g_frameno']))[0]
                return current
            def wait(predicate, timeout=20, interval=0.001):
                deadline = time.monotonic()+timeout
                while time.monotonic()<deadline:
                    try:
                        current = observed()
                        if predicate(current):
                            return current
                    except (OSError,struct.error):
                        pass
                    check_space(output)
                    time.sleep(interval)
                raise RuntimeError('Native startup condition timed out; last observed state: '+json.dumps(current))
            intro = wait(lambda s:s['movie']==1)
            window = subprocess.check_output(['xdotool','search','--name',
                '^Destruction Derby 2$'],env=env,text=True).splitlines()[-1]
            subprocess.run(['xdotool','windowfocus','--sync',window,'keydown','Escape'],
                           env=env,check=True)
            wait(lambda s:s['movie']==0)
            subprocess.run(['xdotool','keyup','Escape'],env=env,check=True)
            start = wait(menu_ready)
            wait(lambda s:(output/'clock-complete.json').is_file(),180,0.01)
            end = observed()
            if end['level']!=9 or end['cf']<60:
                raise RuntimeError('Capture did not reach the actual attract race')
            result = dict(scope='Actual native application startup through attract race with real X11 intro skip; '
                                'control timing diagnostics, no original parity claim',
                          binary=str(binary),binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                          game_clock=str(game_clock),game_clock_sha256=hashlib.sha256(game_clock.read_bytes()).hexdigest(),
                          intro=intro,start_state=start,end_state=end,
                          engine_state_writes=False)
            result['initial_save_sha256']=save_sha256
            result['audio_services']=json.loads((output/'clock-complete.json').read_text())
            result['audio_services_sha256']=hashlib.sha256(audio_clock.read_bytes()).hexdigest()
            (output/'checkpoint.json').write_text(json.dumps(result,indent=2)+'\n')
            return result
        finally:
            if process is not None and process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill();process.wait()
            if display is not None and display.poll() is None:
                display.terminate();display.wait(timeout=5)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,default=Path('/tmp/dd2_native'))
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--services',type=Path,required=True,help='observed original audio services; control values are assertions')
    parser.add_argument('--clock',type=Path,required=True,help='actual original engine clock returns')
    args=parser.parse_args()
    print(json.dumps(capture(args.binary.resolve(),args.output,args.services.resolve(),args.clock.resolve()),indent=2))
