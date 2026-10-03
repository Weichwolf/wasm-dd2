#!/usr/bin/env python3
"""Capture actual native startup controls without writes to game state.

An X11 Escape key skips the real intro. The normal application entry initializes
the frontend and starts audio itself. Records are diagnostic: elapsed device
timing and video/audio equivalence to the original remain separate checks.
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


def capture(binary, output, audio_clock=None):
    output = prepare_output(output)
    if WORK not in output.parents:
        raise ValueError('Menu diagnostics must be under /tmp/wasm-dd2/')
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
    if audio_clock:
        env['DD2_AUDIO_FRAME_CLOCK']=str(audio_clock)
        env['DD2_AUDIO_CLOCK_REPORT']=str(output/'clock-complete.json')
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
            def wait(predicate, timeout=20):
                deadline = time.monotonic()+timeout
                while time.monotonic()<deadline:
                    try:
                        current = observed()
                        if predicate(current):
                            return current
                    except (OSError,struct.error):
                        pass
                    check_space(output)
                    time.sleep(0.01)
                raise RuntimeError('Native startup condition timed out')
            intro = wait(lambda s:s['movie']==1)
            window = subprocess.check_output(['xdotool','search','--name',
                '^Destruction Derby 2$'],env=env,text=True).splitlines()[-1]
            subprocess.run(['xdotool','windowfocus','--sync',window,'keydown','Escape'],
                           env=env,check=True)
            wait(lambda s:s['movie']==0)
            subprocess.run(['xdotool','keyup','Escape'],env=env,check=True)
            start = wait(menu_ready)
            if audio_clock:
                wait(lambda s:(output/'clock-complete.json').is_file(),60)
            else:
                deadline = time.monotonic()+0.3
                while time.monotonic()<deadline:
                    check_space(output)
                    time.sleep(0.01)
            end = observed()
            if not menu_ready(end):
                raise RuntimeError('Capture left the actual main menu startup scope')
            result = dict(scope='Actual native application startup with real X11 intro skip; '
                                'control timing diagnostics, no original parity claim',
                          binary=str(binary),binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                          intro=intro,start_state=start,end_state=end,
                          engine_state_writes=False)
            if audio_clock:
                result['device_clock']=json.loads((output/'clock-complete.json').read_text())
                result['device_clock_sha256']=hashlib.sha256(audio_clock.read_bytes()).hexdigest()
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
    parser.add_argument('--audio-clock',type=Path,help='observed device progress at original presentations')
    args = parser.parse_args()
    print(json.dumps(capture(args.binary.resolve(), args.output,
                            args.audio_clock.resolve() if args.audio_clock else None),indent=2))
