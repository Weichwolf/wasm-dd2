#!/usr/bin/env python3
"""Read-only native car/RNG diagnosis under complete original history inputs.

GDB observes the native port; it never copies or changes engine states. The
original capture must include --race-physics. This diagnosis is native-only;
complete video/audio and WASM acceptance require their separate comparisons.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

from compare_race_stream import EXE_SHA, ROOT
from artifacts import prepare_output, run_bounded, discard_frames


def compare_cars(reference, frames, source, output):
    layout = reference['physics_layout']
    for expected, actual in zip(reference['frames'], frames):
        if any(expected[key] != actual[key] for key in ('level', 'cf', 'ticks', 'clock_calls',
                   *(['car','phase','rng_calls'] if 'phase' in expected else []))):
            return dict(index=actual['index'], error='car checkpoint phase differs', original=expected, native=actual)
        original = (source / f'{expected["prefix"]}.cars').read_bytes()
        prefix=actual.get('prefix',f'frame{actual["index"]:05d}')
        port = (output / f'{prefix}.cars').read_bytes()
        if len(original) != sum(row['size'] for row in layout) or len(port) != len(original):
            raise RuntimeError('incomplete car-state checkpoint')
        if original != port:
            offset = next(i for i, (a, b) in enumerate(zip(original, port)) if a != b)
            remaining = offset
            for row in layout:
                if remaining < row['size']:
                    return dict(index=actual['index'], level=actual['level'], cf=actual['cf'], ticks=actual['ticks'],
                                clock_calls=actual['clock_calls'],
                                region=row['name'], address=hex(row['address']+remaining),
                                car=remaining//(row['size']//20),field_offset=hex(remaining%(row['size']//20)),
                                original=original[offset], native=port[offset],
                                **(dict(phase=actual['phase'],movement_car=actual['car']) if 'phase' in actual else {}))
                remaining -= row['size']
    if len(reference['frames']) != len(frames):
        return dict(index=min(len(reference['frames']),len(frames)), error='car checkpoint count differs',
                    original=len(reference['frames']), native=len(frames))
    return None


def compare_callers(originals, callers, definitions):
    for original, port in zip(originals, callers):
        functions = [name for address, name in definitions if address < original['caller']]
        function = functions[-1] if functions else None
        if (function != str(port['function']).rstrip('_') or
                any(original[key] != port[key] for key in ('level', 'cf', 'ticks'))):
            return dict(index=port['index'], original=dict(original, function=function), native=port)
    if len(originals) != len(callers):
        return dict(index=min(len(originals),len(callers)), error='random caller count differs',
                    original=len(originals), native=len(callers))
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--native', type=Path, default=Path('/tmp/dd2_native'))
    parser.add_argument('--timeout', type=float, default=360)
    args = parser.parse_args()
    source = args.capture.resolve() / 'race'
    reference = json.loads((source / 'race.json').read_text())
    if (not reference.get('complete_history_api_inputs') or not reference.get('complete_racing_loop') or
            not reference.get('physics_layout') or reference.get('exe_modified') is not False or
            reference.get('exe_sha256') != EXE_SHA or
            hashlib.sha256((ROOT / 'DestructionDerby2/dd2h.exe').read_bytes()).hexdigest() != EXE_SHA):
        raise RuntimeError('supported unmodified original --race-physics capture required')
    out = prepare_output(args.output)
    out.mkdir(parents=True, exist_ok=False)
    level = reference['level']
    layout = reference['physics_layout']
    motion = ''
    if reference.get('physics_steps'):
        first_clock,last_clock = reference['physics_steps'][0]['clock_calls'],reference['physics_steps'][-1]['clock_calls']
        movement_levels=sorted({row['level'] for row in reference['physics_steps']})
        cf_start,cf_end=reference['physics_step_window']
        motion = f'''
steps=[]
motion_pending=None
def motion_snapshot(phase,car):
    row=dict(index=len(steps),prefix=f'step{{len(steps):05d}}',phase=phase,car=car,
             level=word(0x936ff4),cf=word(0x462ff0),ticks=word(0x7746c0),
             clock_calls=int(gdb.parse_and_eval('dd2_tick_calls')),rng_calls=int(gdb.parse_and_eval('dd2_random_calls')))
    out.joinpath(row['prefix']+'.cars').write_bytes(b''.join(gdb.selected_inferior().read_memory(region['address'],region['size']).tobytes() for region in layout))
    steps.append(row)
class MotionReturn(gdb.Breakpoint):
    def __init__(self,pc,car):
        super().__init__('*'+hex(pc),type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
        self.car=car
    def stop(self):
        global motion_pending
        motion_snapshot('after Car_Movement',self.car)
        motion_pending=('entry',)
        return True
class Motion(gdb.Breakpoint):
    def stop(self):
        global motion_pending
        if word(0x936ff4) in {movement_levels!r} and {cf_start}<=word(0x462ff0)<={cf_end} and {first_clock}<=int(gdb.parse_and_eval('dd2_tick_calls'))<={last_clock} and word(0x46385c)==1:
            car=int(gdb.parse_and_eval('param_1'))
            motion_snapshot('before Car_Movement',car)
            motion_pending=(car,int(gdb.newest_frame().older().pc()))
            return True
        return False
motion_entry=Motion('*'+hex(int(gdb.parse_and_eval('&Car_Movement'))),type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
def observe_motion():
    global motion_pending
    motion_return=None
    while True:
        gdb.execute('continue')
        if not gdb.selected_inferior().pid:
            break
        if motion_pending is None:
            raise RuntimeError('unexpected native observer stop')
        # Change breakpoint locations outside stop callbacks. Entry and return
        # share one hardware slot, including when rand has several locations.
        if motion_pending==('entry',):
            motion_return.delete()
            motion_return=None
            motion_entry.enabled=True
        else:
            car,pc=motion_pending
            motion_entry.enabled=False
            motion_return=MotionReturn(pc,car)
        motion_pending=None
'''
    script = f'''set pagination off
set confirm off
starti
python
import gdb, json
from pathlib import Path
out = Path({str(out)!r})
layout = {layout!r}
frames, all_frames, callers = [], [], []
def word(address):
    return int.from_bytes(gdb.selected_inferior().read_memory(address,4).tobytes(),'little',signed=True)
class Present(gdb.Breakpoint):
    def stop(self):
        if not 1<=word(0x936ff4)<=10 or word(0x7746ac) or word(0x46385c)!=1 or not int(gdb.parse_and_eval('dd2_tick_calls')):
            return False
        data=b''.join(gdb.selected_inferior().read_memory(row['address'],row['size']).tobytes() for row in layout)
        row=dict(index=len(all_frames),prefix=f'allframe{{len(all_frames):05d}}',
                           flip=int(gdb.parse_and_eval('g_frameno')),level=word(0x936ff4),
                           cf=word(0x462ff0),ticks=word(0x7746c0),
                           clock_calls=int(gdb.parse_and_eval('dd2_tick_calls')),
                           rng_calls=int(gdb.parse_and_eval('dd2_random_calls')))
        out.joinpath(row['prefix']+'.cars').write_bytes(data)
        all_frames.append(row)
        if row['level']=={level}:
            index=len(frames)
            out.joinpath(f'frame{{index:05d}}.cars').write_bytes(data)
            frames.append(dict(row,index=index,prefix=f'frame{{index:05d}}'))
        return False
class Random(gdb.Breakpoint):
    def stop(self):
        caller=gdb.newest_frame().older()
        callers.append(dict(index=int(gdb.parse_and_eval('dd2_random_calls')),function=caller.name(),
                            pc=int(caller.pc()),level=word(0x936ff4),cf=word(0x462ff0),ticks=word(0x7746c0)))
        return False
Present('ids_flip',type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
Random('rand',type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
{motion}
end
{'python\nobserve_motion()\nend' if motion else 'continue'}
python
out.joinpath('frames.json').write_text(json.dumps(frames,indent=2)+'\\n')
out.joinpath('all-frames.json').write_text(json.dumps(all_frames,indent=2)+'\\n')
out.joinpath('random-callers.jsonl').write_text(''.join(json.dumps(row)+'\\n' for row in callers))
{'out.joinpath("steps.json").write_text(json.dumps(steps,indent=2))' if motion else ''}
end
quit
'''
    (out / 'trace.gdb').write_text(script)
    initial = reference['preceding_demos'][0] if reference['preceding_demos'] else reference['first_state']
    env = {key: value for key, value in os.environ.items() if not key.startswith('DD2_')}
    env.update(DD2_FE='1', DD2_SOUND='1', DD2_TICK_REPLAY=str(source / 'ticks.bin'),
               DD2_RANDOM_REFERENCE=str(source / 'random.bin'), DD2_RANDOM_LEVEL='all',
               DD2_RANDOM_REQUIRE_INITIAL='1', DD2_RACE_FLASH_REQUIRE='1',
               DD2_RACE_FLASH_INITIAL=str(initial['demo_flash']), DD2_RACE_FULL_HISTORY='1',
               DD2_RACE_CAPTURE_LEVEL=str(level), DD2_RACE_STOP_AFTER_CAPTURE='1',
               DD2_RACE_STREAM=str(out / 'race.jsonl'))
    with (out / 'gdb.log').open('w') as log:
        run_bounded(['gdb', '--nx', '-q', '-batch', '-x', str(out / 'trace.gdb'),
                        str(args.native.resolve())], cwd=ROOT / 'DestructionDerby2', env=env,
                       directory=out,stdout=log, stderr=subprocess.STDOUT, timeout=args.timeout, check=True)
    text = (out / 'gdb.log').read_text()
    if (any(error in text for error in ('Traceback', 'Python Exception', 'Cannot insert hardware breakpoint',
                                       'Could not insert hardware breakpoints')) or
            not (out / 'frames.json').is_file()):
        raise RuntimeError('native observer failed; inspect gdb.log')
    frames = json.loads((out / 'frames.json').read_text())
    all_frames = json.loads((out / 'all-frames.json').read_text())
    callers = [json.loads(line) for line in (out / 'random-callers.jsonl').read_text().splitlines()]
    if not callers:
        raise RuntimeError('native observer did not reach the frontend random calls')
    originals = [json.loads(line) for line in (source / 'random-callers.jsonl').read_text().splitlines()]
    if [row['index'] for row in callers] != list(range(len(callers))):
        raise RuntimeError('native random observation lost calls')
    definitions = sorted((int(address,16),name.rstrip('"').rstrip('_')) for name,address in
                         re.findall(r'/\* ===== (.*?) @ ([0-9a-fA-F]{8}) ===== \*/', (ROOT / 'build/dd2.c').read_text()))
    report = dict(scope=__doc__, native_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest(),
                  original_exe_sha256=EXE_SHA, level=level, physics_layout=layout,
                  original_frames=len(reference['frames']), native_frames=len(frames),
                  original_rng_calls=len(originals), native_rng_calls=len(callers),
                  engine_loop_complete='[race-stream] target racing loop finished' in text,
                  target_racing_frames_observed=bool(frames),
                  first_car_difference=None, first_random_caller_difference=None,
                  complete_original_or_wasm_acceptance=False)
    report['first_car_difference'] = compare_cars(reference, frames, source, out)
    report['first_random_caller_difference'] = compare_callers(originals, callers, definitions)
    if motion:
        measured_steps=json.loads((out/'steps.json').read_text())
        report['original_car_movement_checkpoints']=len(reference['physics_steps'])
        report['native_car_movement_checkpoints']=len(measured_steps)
        report['first_car_movement_difference']=compare_cars(dict(reference,frames=reference['physics_steps']),measured_steps,source,out)
    if all('physics_frames' in row for row in reference['preceding_demos']):
        prefix_frames=[frame for row in reference['preceding_demos'] for frame in row['physics_frames']]
        full_reference=dict(reference,frames=[*prefix_frames,*reference['frames']])
        report['original_all_car_checkpoints']=len(full_reference['frames'])
        report['native_all_car_checkpoints']=len(all_frames)
        report['first_all_history_car_difference']=compare_cars(full_reference,all_frames,source,out)
    if report['first_car_difference'] is None and len(frames) == len(reference['frames']):
        path = out / 'frame00000.cars'
        accepted = path.read_bytes()
        changed = bytearray(accepted)
        changed[0] ^= 1
        try:
            path.write_bytes(changed)
            bad = compare_cars(reference, frames, source, out)
            if bad is None or bad['index'] != 0 or bad['address'] != hex(layout[0]['address']):
                raise RuntimeError('changed first native car byte was not rejected at its exact original address')
        finally:
            path.write_bytes(accepted)
        report['negative_first_car_byte_rejected'] = True
    if report['first_random_caller_difference'] is None and len(callers) == len(originals):
        changed = [dict(row) for row in callers]
        changed[0]['function'] = 'corrupted_native_caller'
        bad = compare_callers(originals, changed, definitions)
        if bad is None or bad['index'] != 0:
            raise RuntimeError('changed first native random caller was not rejected')
        report['negative_first_random_caller_rejected'] = True
    (out / 'report.json').write_text(json.dumps(report,indent=2)+'\n')
    if (report['engine_loop_complete'] and report['first_car_difference'] is None and
            report['first_random_caller_difference'] is None and
            report.get('first_all_history_car_difference') is None and
            report.get('first_car_movement_difference') is None and
            len(frames) == len(reference['frames']) and len(callers) == len(originals)):
        discard_frames(out)
    print(json.dumps(report,indent=2))


if __name__ == '__main__':
    main()
