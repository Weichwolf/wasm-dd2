"""Produce one original multiplayer turn with a read-only Draw_All observer.

The original receives real X11 keys. Only consistent driving observations,
key transitions and the final race/menu state are recorded. No clock/RNG API
streams, racing images or PCM are captured or accepted for port comparison.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import time

import gdb
import natural_champ_driver
from natural_champ_driver import KeyboardDriver, metrics
from verify_configuration_persistence import EXE_SHA256


def record_multiplayer_turn(output, player, mode=0, policy='steady-persistent', max_draws=60000):
    if player not in (0, 1):
        raise ValueError('This original producer requires hotseat player zero or one')
    if mode not in (0,1):raise ValueError('Wrecking Racing or Stock Car required')
    if not 1000<=max_draws<=300000:raise ValueError('Bounded original observation count must be 1000..300000')
    root = Path(output) / 'history'
    root.mkdir()
    inferior = gdb.selected_inferior()
    if policy not in ('steady','steady-persistent'):raise ValueError('Unknown actual-keyboard driving policy')
    driver = KeyboardDriver(steady=True,movement_distance=100,progress_ticks=1200) if policy=='steady-persistent' else KeyboardDriver(steady=True)
    driver_sha256 = hashlib.sha256(Path(natural_champ_driver.__file__).read_bytes()).hexdigest()
    held, inputs = [], []
    count, controls, last_tick, steady = 0, 0, None, 0
    final = None
    released = False
    last_progress, progress_tick, diagnostic_frame = None, None, False
    started = time.monotonic()
    command = ['xdotool', 'search', '--name', 'PC-DD2', 'windowfocus']

    def read(address, size):
        return inferior.read_memory(address, size).tobytes()

    def integer(address):
        return int.from_bytes(read(address, 4), 'little', signed=True)

    def state():
        return dict(level=integer(0x936ff4), ticks=integer(0x7746c0),
                    cf=integer(0x462ff0), quit=integer(0x7746ac),
                    race=integer(0x93dec8), player=integer(0x93decc),
                    finished=integer(0x795df4), retired=integer(0x9376a8),
                    poly_list=integer(0x940010), frame_skip=integer(0x7746b8))

    def send(key, down):
        subprocess.run([*command, 'keydown' if down else 'keyup', key],
                       check=True, timeout=5, stdout=subprocess.DEVNULL)
        inputs.append(dict(draw=count, tick=integer(0x7746c0), key=key, down=down))

    def release():
        for key in held[:]:
            send(key, False)
            held.remove(key)

    # A hardware breakpoint leaves every byte of the original code intact.
    point = gdb.Breakpoint('*0x420c9c', type=gdb.BP_HARDWARE_BREAKPOINT)
    point.silent = True
    try:
        with (root / 'driving.jsonl').open('x') as observations:
            while True:
                gdb.execute('continue', to_string=True)
                pc = int(gdb.parse_and_eval('$pc')) & 0xffffffff
                if pc != 0x420c9c:
                    raise RuntimeError(f'Unexpected original drawing stop: {pc:x}')
                count += 1
                if count > max_draws:
                    raise RuntimeError('Original turn exceeded its bounded observation count')
                actual = state()
                if [integer(address) for address in (0x4673f4, 0x4673f8, 0x467658)] != [3, mode, 2]:
                    raise RuntimeError('Actual two-player championship in the selected race mode required')
                caller = int.from_bytes(read(int(gdb.parse_and_eval('$esp')) & 0xffffffff, 4), 'little')
                gameplay = caller == 0x423fe2
                if gameplay and final is None and (actual['race'] != 0 or actual['player'] != player):
                    raise RuntimeError('Original producer left the requested first hotseat turn')
                if gameplay and (integer(0x46385c) or integer(0x467074)):
                    raise RuntimeError('A live player race is required, without demo or replay')
                if final is None and actual['quit']:
                    if actual['finished'] <= 14:
                        raise RuntimeError('Original left gameplay without a natural finish')
                    final = dict(actual, driver=metrics(read))
                    release()
                if gameplay and final is None:
                    if actual['finished'] > 14:
                        # Release A before Setup_Pad(0) remaps it to menu Return.
                        release()
                        released = True
                    elif not released and integer(0x784298) < 0:
                        tick = actual['ticks']
                        if last_tick is None or tick > last_tick:
                            observed = driver.controls(read, tick)
                            progress = (observed['lap'], observed['lap_progress'])
                            if progress != last_progress:
                                last_progress, progress_tick = progress, tick
                            if not diagnostic_frame and tick - progress_tick >= 1200:
                                # One pending raster is enough to diagnose the
                                # obstruction; it does not accept racing parity.
                                (root / 'stalled-framebuf.bin').write_bytes(read(0x700450, 307200))
                                (root / 'stalled-palette.bin').write_bytes(read(0x700050, 1024))
                                (root / 'stalled-frame.json').write_text(json.dumps(dict(
                                    scope='Single pending Draw_All raster for an original driving obstruction; no aligned A/V acceptance',
                                    state=actual, driver=observed), indent=2) + '\n')
                                diagnostic_frame = True
                            wanted = observed['wanted']
                            for key in held[:]:
                                if key not in wanted:
                                    send(key, False)
                                    held.remove(key)
                            for key in wanted:
                                if key not in held:
                                    send(key, True)
                                    held.append(key)
                            observations.write(json.dumps(dict(actual, **observed)) + '\n')
                            observations.flush()
                            last_tick = tick
                            controls += 1
                ready = final is not None and read(0x46996c, 2) == b'\0\0' and actual['poly_list'] == (0x46b6e4 if player == 0 else 0x46bf38)
                steady = steady + 1 if ready else 0
                if count % 250 == 0:
                    progress=dict(player=player,draws=count,state=actual,driver=metrics(read),
                                  elapsed_seconds=time.monotonic()-started,max_draws=max_draws)
                    (root/'progress.json').write_text(json.dumps(progress,indent=2)+'\n')
                    print('Original multiplayer turn:', player, actual['ticks'], progress['driver'], flush=True)
                if steady >= 16:
                    expected = (0, 0) if player == 0 else (1, 2)
                    if (actual['race'], actual['player']) != expected:
                        raise RuntimeError('Actual post-race hotseat progression differs')
                    break
        regular = final['driver']['finished_laps'] == 1 and final['driver']['dead'] == 0 and final['retired'] == 0
        report = dict(scope=__doc__, pass_=regular, target='original',
                      exe_modified=False, exe_sha256=EXE_SHA256,
                      engine_state_writes=False, standalone_turn=player,
                      input='real X11 keys', hardware_slots=1,
                      recorded_api_streams=False, racing_image_format='none',
                      driving_policy=policy, driving_inputs=inputs,
                      driving_source_sha256=driver_sha256,
                      draws=count, max_draws=max_draws, control_observations=controls, minimum_control_tick_interval=1, final_race=final,
                      final_menu=actual, elapsed_seconds=time.monotonic() - started)
        (root / 'history.json').write_text(json.dumps(report, indent=2) + '\n')
    except Exception as error:
        (root / 'diagnosis.json').write_text(json.dumps(dict(
            scope='Incomplete original-only turn; no fixture or port parity acceptance',
            error=str(error), state=state(), final_race=final,
            draws=count, driving_inputs=inputs), indent=2) + '\n')
        raise
    finally:
        release()
        point.delete()
