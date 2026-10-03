"""Read-only original clock/RNG observer from frontend through a selected demo.

Every preceding race's engine GetTickCount inputs and every Watcom rand call
are recorded. Only the selected racing loop's video is saved. Hardware stops
alter elapsed time; this is explicit API-input proof, not physical-clock proof.
"""
import json
from pathlib import Path
import struct

import gdb


def record_history(output, level, image_frames=(), image_counters=(), physics_trace=False):
    directory = Path(output) / 'race'
    directory.mkdir()
    inferior = gdb.selected_inferior()
    ticks, random, frames, images, preceding = [], [], [], [], []
    callers = []
    physics_layout = [dict(name='car primitives/dynamics', address=0x78a520, size=20*0x27c),
                      dict(name='car render FD', address=0x792690, size=20*44),
                      dict(name='car state', address=0x792a00, size=20*0x1b2),
                      dict(name='car wheel FD', address=0x794be8, size=20*0xb0)]
    kind = gdb.BP_HARDWARE_BREAKPOINT

    def read(address, size):
        return inferior.read_memory(address, size).tobytes()

    def integer(address):
        return int.from_bytes(read(address, 4), 'little', signed=True)

    def state():
        return dict(level=integer(0x936ff4), cf=integer(0x462ff0),
                    ticks=integer(0x7746c0), countdown=integer(0x784298),
                    frame_skip=integer(0x7746b8), quit=integer(0x7746ac),
                    demo_flash=integer(0x4652a0))

    def breakpoint(pc, condition=None):
        point = gdb.Breakpoint(f'*0x{pc:x}', type=kind)
        point.silent = True
        if condition:
            point.condition = condition
        return point

    # Global pre/post seed observation includes frontend demo selection and
    # Init_Game; never gate on a level that the frontend has not assigned yet.
    rng_pc = 0x456cc6
    rng = breakpoint(rng_pc)
    initial = breakpoint(0x423c2d, '*(int*)0x46385c == 1')
    clock = draw = end = None
    expected = None
    current_entry = first_state = final_state = None
    rng_pointer = rng_before = None
    target = False
    prefix_clock_calls = None
    while True:
        gdb.execute('continue')
        pc = int(gdb.parse_and_eval('$pc'))
        value = int(gdb.parse_and_eval('$eax')) & 0xffffffff
        if pc == rng_pc:
            if pc == 0x456cc6:
                rng_pointer = value
                rng_before = int.from_bytes(read(rng_pointer, 4), 'little')
                if physics_trace:
                    esp = int(gdb.parse_and_eval('$esp')) & 0xffffffff
                    callers.append(dict(index=len(random), caller=int.from_bytes(read(esp,4),'little'),
                                        level=integer(0x936ff4), cf=integer(0x462ff0), ticks=integer(0x7746c0)))
                next_rng = 0x456cde
            else:
                random.append((rng_before, int.from_bytes(read(rng_pointer, 4), 'little'), value))
                next_rng = 0x456cc6
            rng.delete()
            rng_pc = next_rng
            rng = breakpoint(rng_pc)
            continue
        if pc == 0x423c2d and initial:
            current = state()
            if current['level'] not in range(1, 11) or current['ticks'] or current['quit']:
                raise RuntimeError('invalid original demo entry')
            target = current['level'] == level
            current_entry = dict(current, initial_clock=value, clock_offset=len(ticks), rng_calls=len(random))
            if target:
                first_state = current
                prefix_clock_calls = len(ticks)
            ticks.append(value)
            initial.delete()
            initial = None
            expected = 0x424007
            clock = breakpoint(expected)
            end = breakpoint(0x42404a)
            if target:
                draw = breakpoint(0x420c9c, '*(unsigned int*)$esp == 0x423fe2')
            print(f'Original history race: level={current["level"]}, clock offset={current_entry["clock_offset"]}', flush=True)
            continue
        if pc == 0x42404a and end:
            current = state()
            if current['quit'] != 1 or int(gdb.parse_and_eval('$edi')) >= 0 or integer(0x9376ac):
                raise RuntimeError('original demo did not exhaust its physics budget without input')
            clock.delete()
            end.delete()
            clock = end = None
            if target:
                draw.delete()
                final_state = current
                break
            current_entry.update(final_state=current, clock_end=len(ticks), rng_end=len(random))
            preceding.append(current_entry)
            with (directory / 'preceding-demos.jsonl').open('a') as log:
                log.write(json.dumps(current_entry) + '\n')
            initial = breakpoint(0x423c2d, '*(int*)0x46385c == 1')
            continue
        if pc == 0x420c9c and draw:
            current = state()
            if current['level'] != level or current['quit'] or integer(0x46385c) != 1:
                raise RuntimeError('wrong/inactive original target demo render')
            index = len(frames)
            prefix = f'frame{index:05d}'
            (directory / f'{prefix}.bin').write_bytes(read(0x700450, 307200))
            (directory / f'{prefix}.pal').write_bytes(read(0x700050, 1024))
            if physics_trace:
                (directory / f'{prefix}.cars').write_bytes(b''.join(read(row['address'],row['size']) for row in physics_layout))
            if index in image_frames or current['cf'] in image_counters:
                (directory / f'{prefix}.image').write_bytes(read(0x400000, 0x580400))
                images.append(index)
            frames.append(dict(current, index=index, prefix=prefix, clock_calls=len(ticks), rng_calls=len(random)))
            if len(frames) % 100 == 0:
                print(f'Original history target: {len(frames)} frames, {len(ticks)} clocks, {len(random)} RNG calls', flush=True)
            continue
        if not clock or pc != expected:
            raise RuntimeError(f'unexpected original stop: 0x{pc:x}')
        ticks.append(value)
        if pc == 0x424007:
            next_clock = 0x424024
        elif pc == 0x424024:
            delta = (value - (integer(0x7746a0) & 0xffffffff)) & 0xffffffff
            next_clock = 0x424024 if delta < integer(0x7746b8) * 20 else 0x424040
        else:
            next_clock = 0x424007
        if next_clock != expected:
            clock.delete()
            expected = next_clock
            clock = breakpoint(expected)
    rng.delete()
    if not frames or not random or rng_pc != 0x456cc6 or random[0][0] != 1:
        raise RuntimeError('complete original history from boot seed 1 required')
    (directory / 'ticks.bin').write_bytes(b''.join(struct.pack('<I', value) for value in ticks))
    (directory / 'random.bin').write_bytes(b''.join(struct.pack('<III', *row) for row in random))
    if physics_trace:
        if len(callers)!=len(random):raise RuntimeError('incomplete original random caller trace')
        (directory / 'random-callers.jsonl').write_text(''.join(json.dumps(row)+'\n' for row in callers))
    result = dict(stage='Draw_All entry / pending presentation', scope=__doc__, level=level,
                  complete_racing_loop=True, complete_history_api_inputs=True,
                  breakpoints='hardware only; no inferior memory/register writes',
                  clock_source='actual original engine GetTickCount return EAX, all preceding loops',
                  rng_source='global Watcom pre/post seed and EAX, frontend and all preceding loops',
                  first_state=first_state, final_state=final_state, frames=frames,
                  target_entry=current_entry,
                  clock_calls=len(ticks), prefix_clock_calls=prefix_clock_calls,
                  rng_calls=len(random), rng_initial_seed=random[0][0], rng_final_seed=random[-1][1],
                  diagnostic_image_frames=images, preceding_demos=preceding)
    if physics_trace:result['physics_layout']=physics_layout
    (directory / 'race.json').write_text(json.dumps(result, indent=2) + '\n')
    print(f'Original complete history inputs: {len(preceding)} preceding demos, {len(frames)} target frames', flush=True)
