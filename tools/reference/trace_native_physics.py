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


def compare_cars(reference, frames, source, output):
    layout = reference['physics_layout']
    for expected, actual in zip(reference['frames'], frames):
        if any(expected[key] != actual[key] for key in ('level', 'cf', 'ticks', 'clock_calls')):
            return dict(index=actual['index'], error='car checkpoint phase differs', original=expected, native=actual)
        original = (source / f'{expected["prefix"]}.cars').read_bytes()
        port = (output / f'frame{actual["index"]:05d}.cars').read_bytes()
        if len(original) != sum(row['size'] for row in layout) or len(port) != len(original):
            raise RuntimeError('incomplete car-state checkpoint')
        if original != port:
            offset = next(i for i, (a, b) in enumerate(zip(original, port)) if a != b)
            remaining = offset
            for row in layout:
                if remaining < row['size']:
                    return dict(index=actual['index'], cf=actual['cf'], ticks=actual['ticks'],
                                region=row['name'], address=hex(row['address']+remaining),
                                original=original[offset], native=port[offset])
                remaining -= row['size']
    return None


def compare_callers(originals, callers, definitions):
    for original, port in zip(originals, callers):
        functions = [name for address, name in definitions if address < original['caller']]
        function = functions[-1] if functions else None
        if (function != str(port['function']).rstrip('_') or
                any(original[key] != port[key] for key in ('level', 'cf', 'ticks'))):
            return dict(index=port['index'], original=dict(original, function=function), native=port)
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
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    level = reference['level']
    layout = reference['physics_layout']
    script = f'''set pagination off
set confirm off
starti
python
import gdb, json
from pathlib import Path
out = Path({str(out)!r})
layout = {layout!r}
frames, callers = [], []
def word(address):
    return int.from_bytes(gdb.selected_inferior().read_memory(address,4).tobytes(),'little',signed=True)
class Present(gdb.Breakpoint):
    def stop(self):
        if word(0x936ff4)!={level} or word(0x7746ac) or int(gdb.parse_and_eval('dd2_tick_calls'))<={reference['prefix_clock_calls']}:
            return False
        index=len(frames)
        data=b''.join(gdb.selected_inferior().read_memory(row['address'],row['size']).tobytes() for row in layout)
        out.joinpath(f'frame{{index:05d}}.cars').write_bytes(data)
        frames.append(dict(index=index,flip=int(gdb.parse_and_eval('g_frameno')),level=word(0x936ff4),
                           cf=word(0x462ff0),ticks=word(0x7746c0),
                           clock_calls=int(gdb.parse_and_eval('dd2_tick_calls')),
                           rng_calls=int(gdb.parse_and_eval('dd2_random_calls'))))
        return False
class Random(gdb.Breakpoint):
    def stop(self):
        caller=gdb.newest_frame().older()
        callers.append(dict(index=int(gdb.parse_and_eval('dd2_random_calls')),function=caller.name(),
                            pc=int(caller.pc()),level=word(0x936ff4),cf=word(0x462ff0),ticks=word(0x7746c0)))
        return False
Present('ids_flip',type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
Random('rand',type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
end
continue
python
out.joinpath('frames.json').write_text(json.dumps(frames,indent=2)+'\\n')
out.joinpath('random-callers.jsonl').write_text(''.join(json.dumps(row)+'\\n' for row in callers))
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
        subprocess.run(['gdb', '--nx', '-q', '-batch', '-x', str(out / 'trace.gdb'),
                        str(args.native.resolve())], cwd=ROOT / 'DestructionDerby2', env=env,
                       stdout=log, stderr=subprocess.STDOUT, timeout=args.timeout, check=True)
    text = (out / 'gdb.log').read_text()
    if 'Traceback' in text or not (out / 'frames.json').is_file():
        raise RuntimeError('native observer failed; inspect gdb.log')
    frames = json.loads((out / 'frames.json').read_text())
    callers = [json.loads(line) for line in (out / 'random-callers.jsonl').read_text().splitlines()]
    if not frames or not callers:
        raise RuntimeError('native observer did not reach the selected racing loop')
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
                  first_car_difference=None, first_random_caller_difference=None,
                  complete_original_or_wasm_acceptance=False)
    report['first_car_difference'] = compare_cars(reference, frames, source, out)
    report['first_random_caller_difference'] = compare_callers(originals, callers, definitions)
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
    print(json.dumps(report,indent=2))


if __name__ == '__main__':
    main()
