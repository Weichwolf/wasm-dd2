"""Bounded original-only driving observations through one hardware breakpoint.

No API replay or framebuffer/audio comparison is performed. The executable,
engine state, clock returns and RNG are never replaced. GDB changes timing.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import time


def drive(output, player, max_ticks, max_draws, seconds):
    import gdb
    import live_lap_record_driver as policy
    out = Path(output)
    inferior = gdb.selected_inferior()
    read = lambda address, size: inferior.read_memory(address & 0xffffffff, size).tobytes()
    integer = lambda address: int.from_bytes(read(address, 4), 'little', signed=True)
    driver = policy.ArcRoadKeyboardDriver()
    held, inputs = set(), []
    draws, last_tick = 0, None
    started, first_tick = time.monotonic(), integer(0x7746c0)
    point = gdb.Breakpoint('*0x420c9c', type=gdb.BP_HARDWARE_BREAKPOINT, internal=True)
    point.silent = True
    report = dict(scope=__doc__, pass_=False, engine_state_writes=False,
                  hardware_slots=1, player=player, input='real X11 keys',
                  driver_sha256=hashlib.sha256(Path(policy.__file__).read_bytes()).hexdigest())

    def require(condition, message):
        if not condition:
            raise RuntimeError(message)

    def edge(key, down):
        subprocess.run(['xdotool', 'search', '--name', 'PC-DD2', 'windowfocus',
                        'keydown' if down else 'keyup', key], check=True, timeout=5,
                       stdout=subprocess.DEVNULL)
        inputs.append(dict(draw=draws, tick=integer(0x7746c0), key=key, down=down))

    def release():
        for key in sorted(held):
            edge(key, False)
        held.clear()

    try:
        with (out / 'driver.jsonl').open('x') as observations:
            while draws < max_draws:
                gdb.execute('continue', to_string=True)
                require(int(gdb.parse_and_eval('$pc')) == 0x420c9c, 'unexpected drawing stop')
                draws += 1
                tick = integer(0x7746c0)
                state = dict(level=integer(0x936ff4), ticks=tick, quit=integer(0x7746ac),
                             finished=integer(0x795df4), retired=integer(0x9376a8),
                             player=integer(0x93decc), driver=policy.metrics(read))
                report['last_state'] = state
                if state['quit']:
                    release()
                    report['natural_finish_observed'] = True
                    report['pass_'] = (state['finished'] > 14 and state['retired'] == 0 and
                                       state['driver']['dead'] == 0 and state['driver']['finished_laps'] == 1)
                    break
                require(state['level'] == 1 and state['player'] == player and not state['retired'],
                        'race left intended hotseat turn')
                require(time.monotonic() - started < seconds, 'elapsed driving limit reached')
                if tick - first_tick >= max_ticks:
                    report['bounded_tick_limit'] = True
                    break
                caller = int.from_bytes(read(int(gdb.parse_and_eval('$esp')) & 0xffffffff, 4), 'little')
                if caller != 0x423fe2:
                    continue
                # Release before Setup_Pad(0) remaps the accelerator to Return.
                if state['finished'] > 14:
                    release()
                    continue
                if integer(0x784298) < 0 and tick != last_tick:
                    row = driver.controls(read, tick)
                    wanted = set(row['wanted'])
                    require(wanted <= {'a', 'z', 'Left', 'Right', 'space'}, 'unsupported driving key')
                    for key in sorted(held - wanted):
                        edge(key, False)
                    for key in sorted(wanted - held):
                        edge(key, True)
                    held, last_tick = wanted, tick
                    observations.write(json.dumps(dict(tick=tick, draw=draws, **row)) + '\n')
                    observations.flush()
                    if draws % 100 == 0:
                        progress = dict(draws=draws, elapsed_seconds=time.monotonic() - started,
                                        state=state, controls=row)
                        (out / 'progress.json').write_text(json.dumps(progress, indent=2) + '\n')
                        print('Driving probe:', player, draws, 'draws;', row['lap'], row['lap_progress'],
                              'lap/progress;', row['speed'], 'speed;', row['manoeuvre'], flush=True)
    except BaseException as error:
        report['error'] = str(error)
        raise
    finally:
        try:
            release()
        finally:
            point.delete()
            report.update(draws=draws, elapsed_seconds=time.monotonic() - started, inputs=inputs)
            (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
