"""Read-only original/native time trial observer; real X11 keys, no state injection."""
import hashlib
import json
from pathlib import Path
import subprocess
import time
import gdb
import live_lap_record_driver
from live_lap_record_driver import ArcRoadKeyboardDriver, metrics


def drive(output, max_draws=30000, entry=0x420c9c, target="original"):
    root = Path(output)
    inferior = gdb.selected_inferior()
    read = lambda a, n: inferior.read_memory(a & 0xffffffff, n).tobytes()
    integer = lambda a: int.from_bytes(read(a, 4), 'little', signed=True)
    index = integer(0x4673fc)
    level = integer(0x936ff4)
    saved = list(__import__('struct').unpack('<HHH', read(0x4680ca + index * 80, 6)))
    packed = lambda t: ((t[0] << 24) | (t[1] << 16) | t[2]) & 0xffffffff
    require = lambda condition, message: condition or (_ for _ in ()).throw(RuntimeError(message))
    require(integer(0x4673f4) == 1 and integer(0x46765c) == 1, 'Actual single-car time trial required')
    require(0 <= index < 7 and 1 <= level <= 7, 'Actual road track required')
    driver = ArcRoadKeyboardDriver()
    inputs, held = [], []
    count, last_tick = 0, None
    started = time.monotonic()
    point = gdb.Breakpoint(f'*{entry}', type=gdb.BP_HARDWARE_BREAKPOINT)
    point.silent = True

    def send(key, down):
        subprocess.run(['xdotool', 'search', '--name', 'PC-DD2' if target=='original' else '^Destruction Derby 2$', 'windowfocus',
                        'keydown' if down else 'keyup', key], check=True,
                       timeout=5, stdout=subprocess.DEVNULL)
        inputs.append(dict(draw=count, tick=integer(0x7746c0), key=key, down=down))

    def release():
        for key in held[:]:
            send(key, False)
            held.remove(key)

    report = dict(scope=__doc__, pass_=False, engine_state_writes=False,
                  input='real X11 keys', target=target, hardware_slots=1, record_before=saved,
                  track=index, level=level,
                  driver_sha256=hashlib.sha256(Path(live_lap_record_driver.__file__).read_bytes()).hexdigest())
    try:
        with (root / 'driving.jsonl').open('x') as observations:
            while count < max_draws:
                gdb.execute('continue', to_string=True)
                require(int(gdb.parse_and_eval('$pc')) == entry, 'Unexpected drawing stop')
                count += 1
                require(integer(0x4673f4) == 1 and integer(0x936ff4) == level and
                        integer(0x7746ac) == 0 and not integer(0x46385c) and
                        not integer(0x467074), 'Actual live time trial left requested route')
                caller = int.from_bytes(read(int(gdb.parse_and_eval('$esp')) & 0xffffffff, 4), 'little')
                if target=='original':
                    gameplay=caller==0x423fe2
                else:
                    parent=gdb.newest_frame().older()
                    gameplay=parent is not None and parent.name()=='Play_Game'
                if not gameplay:
                    continue
                tick = integer(0x7746c0)
                runtime = list(__import__('struct').unpack('<iii', read(0x466e3c + (level - 1) * 12, 12)))
                current = dict(ticks=tick, cf=integer(0x462ff0), runtime=runtime,
                               driver=metrics(read), draws=count,
                               elapsed_seconds=time.monotonic()-started)
                if count % 250 == 0:
                    (root / 'progress.json').write_text(json.dumps(current, indent=2)+'\n')
                    print('Actual time trial:', current, flush=True)
                if current['driver']['lap'] >= 2 and packed(runtime) < packed(saved):
                    release()
                    report.update(pass_=True, final=current)
                    break
                require(current['driver']['lap'] < 4, 'Three actual laps did not beat the saved best')
                require(not current['driver']['dead'], 'Actual trial vehicle destroyed before a best lap')
                if integer(0x784298) < 0 and (last_tick is None or tick > last_tick):
                    observed = driver.controls(read, tick)
                    for key in held[:]:
                        if key not in observed['wanted']:
                            send(key, False)
                            held.remove(key)
                    for key in observed['wanted']:
                        if key not in held:
                            send(key, True)
                            held.append(key)
                    observations.write(json.dumps(dict(tick=tick, runtime=runtime, **observed))+'\n')
                    observations.flush()
                    last_tick = tick
            require(report['pass_'], 'No actual better completed lap within observation limit')
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        release()
        point.delete()
        report.update(draws=count, driving_inputs=inputs, elapsed_seconds=time.monotonic()-started)
        (root / 'driving-report.json').write_text(json.dumps(report, indent=2)+'\n')
