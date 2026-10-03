"""Original championship/normal-arena history using four read-only hardware slots.

Real X11 input drives five retirements or a natural Total Destruction finish.
The normal arena records indexed pictures at each racing Draw_All entry;
other paths capture only requested navigation pictures. Debugger stops affect
time; recorded API returns are explicit inputs, not physical-clock acceptance.
Loading/fades and chronological audio are not accepted by this recorder.
"""
import json
import hashlib
from pathlib import Path
import struct
import subprocess
import time

import gdb

from verify_champ_season import ADDRESSES, EXE, KEYS, NORMAL_ARENA_KEYS, validate_end


def record_champ_history(output, steps=95, target='original', game_frame_delay_ms=0, normal_arena=False):
    root = Path(output) / 'history'
    root.mkdir()
    inferior = gdb.selected_inferior()
    kind = gdb.BP_HARDWARE_BREAKPOINT
    points = []
    rng = []
    clocks = []
    pads = []
    checkpoints = []
    draws = 0
    event_index = 0
    key_index = 0
    stage = 'settle'
    steady = 0
    key_flag = None
    held_polls = 0
    held_counts = []
    start_time = time.monotonic()
    keys = NORMAL_ARENA_KEYS if normal_arena else KEYS[:steps]
    race_frames = []
    final_race = None
    command = ['xdotool', 'search', '--name', 'PC-DD2', 'windowfocus']
    original = target == 'original'
    draw_pc = 0x420c9c if original else int(gdb.parse_and_eval('&Draw_All'))
    pad_pc = 0x422da4 if original else int(gdb.parse_and_eval('&FUN_00422da4'))
    rng_entry = 0x456cc6 if original else int(gdb.parse_and_eval('&rand'))

    def read(address, size):
        return inferior.read_memory(address, size).tobytes()

    def word(address):
        return int.from_bytes(read(address, 4), 'little')

    def integer(address):
        return int.from_bytes(read(address, 4), 'little', signed=True)

    def state():
        return dict(level=integer(0x936ff4), cf=integer(0x462ff0),
                    ticks=integer(0x7746c0), countdown=integer(0x784298),
                    frame_skip=integer(0x7746b8), quit=integer(0x7746ac),
                    race=integer(0x93dec8), season=integer(0x93dec0),
                    finished=integer(0x795df4), retired=integer(0x9376a8), damage=integer(0x792a76),
                    poly_list=word(0x940010), clock_calls=len(clocks),
                    rng_calls=len(rng), pad_polls=len(pads), draws=draws)

    timeline = (root / 'events.jsonl').open('x')

    def event(kind, **extra):
        nonlocal event_index
        event_index += 1
        if event_index > 100000:
            raise RuntimeError('Championship history exceeded 100,000 observations')
        row = dict(index=event_index, kind=kind, **state(), **extra)
        timeline.write(json.dumps(row) + '\n')
        timeline.flush()
        return row

    def breakpoint(pc, condition=None):
        point = gdb.Breakpoint(f'*0x{pc:x}', type=kind)
        point.silent = True
        if condition:
            point.condition = condition
        points.append(point)
        return point

    def send(key, down):
        if original:
            subprocess.run([*command, 'keydown' if down else 'keyup', key],
                           check=True, timeout=5, stdout=subprocess.DEVNULL)
        else:
            vk = dict(Return=0x0d, Escape=0x1b, Up=0x26, Down=0x28, Left=0x25, Right=0x27)[key]
            gdb.execute(f'call (void)dd2_key_event({vk}, {int(down)})')
        event('key', key=key, down=down, action=key_index)

    def flag_for(key):
        vkeys = dict(Return=0x0d, Escape=0x1b, Up=0x26, Down=0x28, Left=0x25, Right=0x27)
        flags = (0x46303f, 0x463040, 0x463043, 0x463046, 0x463044, 0x463045,
                 0x463048, 0x46304a, 0x463047, 0x463049, 0x46304b, 0x46304e, 0x46304c, 0x46304d)
        mapping = read(0x46302c, 14)
        index = next((i for i in (0, 1, 2, 3, 4, 5, 8, 6, 9, 7, 10, 11, 12, 13)
                      if mapping[i] == vkeys[key]), None)
        if index is None:
            raise RuntimeError('Original key is not mapped: ' + key)
        return flags[index]

    def snapshot():
        name = f'step{key_index:02d}-'+('natural-finish' if normal_arena and key_index > len(keys) else keys[key_index - 1] if key_index else 'boot')
        directory = root / name
        directory.mkdir()
        value = {key: integer(address) for key, address in ADDRESSES.items()}
        text = lambda address, size: read(address, size).split(b'\0')[0].decode('ascii')
        value.update(stage=target+' Draw_All entry', exe_modified=False if original else None, exe_sha256=EXE if original else None,
            observed=state(), cars=[dict(name=text(0x93dee0 + i * 54, 16),
                values=list(struct.unpack('<7h', read(0x93def0 + i * 54, 14)))) for i in range(20)],
            rows=[dict(name=text(0x940290 + i * 26, 26), points=text(0x940240 + i * 16, 16)) for i in range(5)])
        # Check the actual path rather than publishing a complete-looking trace
        # after a key was consumed on the wrong screen.
        if not normal_arena and key_index >= 10 and (key_index - 10) % 17 == 0 and key_index <= 78:
            race = (key_index - 10) // 17
            if (value['level'], value['race']) != ((1, 2, 5, 7, 10)[race], race):
                raise RuntimeError('Wrong actual championship race start')
        if not normal_arena and key_index >= 17 and (key_index - 17) % 17 == 0:
            race = (key_index - 17) // 17
            if (value['level'], value['race'], value['poly_list']) != (15, race + 1, 0x46bf38 if race < 4 else 0x46ae44):
                raise RuntimeError('Wrong actual championship result screen')
        if not normal_arena and key_index == 95:
            validate_end(value)
        if normal_arena and key_index > len(keys) and (value['poly_list'] != 0x46a468 or integer(0x795df4) <= 14 or integer(0x9376a8)):
            raise RuntimeError('Normal arena did not naturally reach Practice_Over')
        for filename, address, size in [('framebuf.bin', 0x700450, 307200), ('palette.bin', 0x700050, 1024)]:
            (directory / filename).write_bytes(read(address, size))
        (directory / 'checkpoint.json').write_text(json.dumps(value, indent=2) + '\n')
        checkpoints.append(name)
        event('checkpoint', checkpoint=name, action=key_index)
        print(f'{target} history checkpoint {key_index}: level={value["level"]}, clocks={len(clocks)}, rng={len(rng)}', flush=True)

    rng_pc = rng_entry
    rng_point = breakpoint(rng_pc)
    rng_pointer = rng_before = rng_caller = None
    clock_entry = word(0x9500c0) if original else int(gdb.parse_and_eval('&GetTickCount'))
    clock_pc = clock_entry
    clock_condition = '*(unsigned int*)$esp >= 0x400000 && *(unsigned int*)$esp < 0x470000' if original else None
    clock_point = breakpoint(clock_pc, clock_condition)
    clock_caller = None
    breakpoint(draw_pc)
    breakpoint(pad_pc)
    expected_clock_callers = (0x423c2d, 0x423ecd, 0x424007, 0x424024, 0x424040)
    try:
        while True:
            gdb.execute('continue')
            pc = int(gdb.parse_and_eval('$pc')) & 0xffffffff
            eax = int(gdb.parse_and_eval('$eax')) & 0xffffffff
            if pc == rng_pc:
                if pc == rng_entry:
                    rng_pointer = eax if original else int(gdb.parse_and_eval('&_dd2_rand_seed'))
                    rng_before = word(rng_pointer)
                    rng_caller = word(int(gdb.parse_and_eval('$esp')) & 0xffffffff)
                    next_pc = 0x456cde if original else rng_caller
                else:
                    after = word(rng_pointer)
                    rng.append((rng_before, after, eax))
                    event('rand', before=rng_before, after=after, result=eax, caller=hex(rng_caller))
                    next_pc = rng_entry
                rng_point.delete()
                rng_pc = next_pc
                rng_point = breakpoint(rng_pc)
            elif pc == clock_pc:
                if pc == clock_entry:
                    clock_caller = word(int(gdb.parse_and_eval('$esp')) & 0xffffffff)
                    if original and clock_caller not in expected_clock_callers:
                        raise RuntimeError(f'Unidentified original GetTickCount caller: {clock_caller:x}')
                    next_pc = clock_caller
                else:
                    clocks.append(eax)
                    event('GetTickCount', value=eax, caller=hex(clock_caller))
                    next_pc = clock_entry
                clock_point.delete()
                clock_pc = next_pc
                clock_point = breakpoint(clock_pc, clock_condition if next_pc == clock_entry else None)
            elif pc == pad_pc:
                flags = read(0x46303e, 17)
                caller = word(int(gdb.parse_and_eval('$esp')) & 0xffffffff)
                pads.append(event('ReadPad', flags=flags.hex(), caller=hex(caller)))
                if stage == 'down' and read(key_flag, 1) != b'\0':
                    held_polls = 1
                    if original:
                        send(keys[key_index - 1], False)
                        stage = 'release'
                    else:
                        # Clear the native bridge after this real poll executes,
                        # at the next Draw_All boundary, rather than before it.
                        stage = 'held'
                elif stage == 'held' and read(key_flag, 1) != b'\0':
                    held_polls += 1
                elif stage == 'release':
                    if read(key_flag, 1) != b'\0':
                        held_polls += 1
                    else:
                        held_counts.append(held_polls)
                        stage = 'settle'
                        steady = 0
            elif pc == draw_pc:
                draws += 1
                caller = word(int(gdb.parse_and_eval('$esp')) & 0xffffffff)
                if original:
                    game_caller = caller == 0x423fe2
                else:
                    block = gdb.block_for_pc(caller)
                    while block and block.function is None:
                        block = block.superblock
                    game_caller = bool(block and block.function.name == 'Play_Game')
                # Explicitly change elapsed physical time at an observed draw,
                # without writing frame_skip, clock returns or engine state.
                # This diagnoses the original's naturally adaptive rest loop.
                if game_frame_delay_ms and game_caller:
                    time.sleep(game_frame_delay_ms / 1000)
                if normal_arena and game_caller:
                    value = state()
                    filename = f'race{len(race_frames):05d}'
                    raw = read(0x700450, 307200)
                    palette = read(0x700050, 1024)
                    (root / (filename+'.bin')).write_bytes(raw)
                    (root / (filename+'.pal')).write_bytes(palette)
                    race_frames.append(dict(value, index=len(race_frames), prefix=filename,
                        framebuffer_sha256=hashlib.sha256(raw).hexdigest(), palette_sha256=hashlib.sha256(palette).hexdigest()))
                    if len(race_frames) % 100 == 0:
                        print(f'{target} normal arena: {len(race_frames)} frames, ticks={value["ticks"]}, damage={value["damage"]}',flush=True)
                event('Draw_All', caller=hex(caller), slab=int.from_bytes(read(0x46996c, 2), 'little', signed=True))
                if stage == 'drive' and integer(0x7746ac):
                    final_race = state()
                    if integer(0x795df4) <= 14 or integer(0x9376a8):
                        raise RuntimeError('Arena left gameplay without natural finish')
                    send('Up', False);send('Right', False)
                    stage = 'settle';steady = 0
                if stage == 'held':
                    send(keys[key_index - 1], False)
                    stage = 'release'
                if stage != 'settle':
                    continue
                race_start = (key_index == len(keys) if normal_arena else key_index >= 10 and (key_index - 10) % 17 == 0 and key_index <= 78)
                if race_start:
                    ready = game_caller and integer(0x7746ac) == 0
                else:
                    ready = read(0x46996c, 2) == b'\0\0' and (not normal_arena or key_index <= len(keys) or word(0x940010) == 0x46a468)
                steady = steady + 1 if ready else 0
                if steady < (2 if race_start else 16):
                    continue
                snapshot()
                if normal_arena and key_index == len(keys):
                    if integer(0x4673f8)!=2 or integer(0x4673f4)!=2 or integer(0x936ff4)<8:
                        raise RuntimeError('Wrong actual Total Destruction arena')
                    key_index += 1
                    send('Up', True);send('Right', True)
                    stage = 'drive'
                    continue
                if normal_arena and key_index > len(keys):
                    break
                if key_index == len(keys):
                    break
                key_index += 1
                key_flag = flag_for(keys[key_index - 1])
                stage = 'down'
                send(keys[key_index - 1], True)
            else:
                raise RuntimeError(f'Unexpected original history stop: {pc:x}')
        if not rng or rng[0][0] != 1 or rng_pc != rng_entry or clock_pc != clock_entry:
            raise RuntimeError('Incomplete original RNG/clock entry-to-return pairs')
        (root / 'ticks.bin').write_bytes(b''.join(struct.pack('<I', value) for value in clocks))
        (root / 'random.bin').write_bytes(b''.join(struct.pack('<III', *row) for row in rng))
        (root / 'history.json').write_text(json.dumps(dict(scope=__doc__.strip() if original else 'Actual native keyboard-bridge championship trace; calculated RNG and recorded clock inputs; no physical sink/whole-game parity claim',
            exe_modified=False if original else None, exe_sha256=EXE if original else None, keys=keys, checkpoints=checkpoints,
            target=target,input='real X11 keys' if original else 'dd2_key_event', acknowledged_keys=True, held_pad_polls=held_counts,
            clock_calls=len(clocks), rng_calls=len(rng), pad_polls=len(pads), draws=draws,
            observed_play_draw_delay_ms=game_frame_delay_ms,
            complete_retirement_season=not normal_arena and steps == 95, normal_arena=normal_arena,
            natural_finish=bool(final_race), final_race=final_race, race_frames=race_frames,
            elapsed_seconds=time.monotonic() - start_time), indent=2) + '\n')
    finally:
        timeline.close()
        for point in points:
            if point.is_valid():
                point.delete()
