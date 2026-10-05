"""Original championship/normal-arena history using four read-only hardware slots.

Real X11 input drives five retirements, a natural Total Destruction finish or
one/five Wrecking championship races, league pages and the next actual race or
season transition.
Natural player races record indexed pictures at each racing Draw_All entry;
championship racing images use lossless zlib to bound storage. Debugger stops affect
time; recorded API returns are explicit inputs, not physical-clock acceptance.
Optional full video records PutDispEnv entries, including loading/fades and
Swap_Buffers. Chronological audio is not accepted by this recorder.
"""
import json
import hashlib
from pathlib import Path
import struct
import subprocess
import time
import zlib

import gdb
import natural_champ_driver

from verify_champ_season import (ADDRESSES, EXE, KEYS, NORMAL_ARENA_KEYS,
    NATURAL_CHAMP_ACTIONS, NATURAL_CHAMP_KEYS, NATURAL_SEASON_ACTIONS,
    NATURAL_SEASON_KEYS, NATURAL_SEASON_STARTS, CHAMP_LEVELS, validate_end)
from natural_champ_driver import metrics as driver_metrics, KeyboardDriver
import race_results_protocol
import multiplayer_results_protocol
import multiplayer_loaded_protocol
import natural_multiplayer_protocol


def record_champ_history(output, steps=95, target='original', game_frame_delay_ms=0, normal_arena=False, full_video=False, natural_champ=False, driving_reference=None, steady_driver=False, natural_season=False, result_tables=False, result_reference=None, result_race_mode=None, natural_multiplayer=False, natural_multiplayer_speed=250, slow_recovery=False, wall_recovery=False):
    protocol = multiplayer_loaded_protocol if result_tables == 'multiplayer-loaded' else multiplayer_results_protocol if result_tables == 'multiplayer' else race_results_protocol
    if result_tables == 'multiplayer-loaded' and result_race_mode not in (0,1):
        raise ValueError('Actual saved Wrecking or Stock Car mode required')
    captures = getattr(protocol,'CAPTURES',protocol.TABLES)
    card_steps = protocol.PLAN.get('card_steps',[])
    card_wanted = {}
    if result_reference:
        for step in card_steps:
            name = f'step{step:02d}-'+protocol.KEYS[step-1]
            meta = json.loads((Path(result_reference)/name/'cycle/cycle.json').read_text())
            card_wanted[step] = {(f['phase'],f['card_phase']) for f in meta['frames']}
    if not 60 <= natural_multiplayer_speed <= 250:
        raise ValueError('Natural multiplayer throttle limit must be 60..250')
    if natural_multiplayer and (not natural_champ or natural_season or result_tables):
        raise ValueError('Natural multiplayer requires its dedicated natural championship route')
    if result_tables and (normal_arena or full_video or natural_champ or natural_season):
        raise ValueError('Result-table API history requires its own route')
    if natural_champ and (normal_arena or full_video):
        raise ValueError('Natural championship uses its own bounded racing capture')
    if steady_driver and (not natural_champ or target != 'original'):
        raise ValueError('Steady driver is only used for the original natural championship; ports replay its recorded inputs')
    if natural_season and not natural_champ:
        raise ValueError('Natural season requires natural championship input')
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
    keys = natural_multiplayer_protocol.KEYS if natural_multiplayer else protocol.KEYS if result_tables else NATURAL_SEASON_KEYS if natural_season else NATURAL_CHAMP_KEYS if natural_champ else NORMAL_ARENA_KEYS if normal_arena else KEYS[:steps]
    actions = natural_multiplayer_protocol.ACTIONS if natural_multiplayer else NATURAL_SEASON_ACTIONS if natural_season else NATURAL_CHAMP_ACTIONS if natural_champ else keys
    driving_inputs = []
    driving_cursor = 0
    driving_held = []
    last_control_tick = None
    def new_driver():
        return KeyboardDriver(steady=True, movement_distance=100, progress_ticks=1200,
                              slow_ticks=75 if slow_recovery else 0,
                              escape_steering=96 if wall_recovery else 0) if natural_multiplayer else KeyboardDriver(steady=steady_driver)

    keyboard_driver = new_driver()
    finish_controls_released = False
    race_frames = []
    presentations = []
    result_cycles = {}
    final_race = None
    final_races = []
    command = ['xdotool', 'search', '--name', 'PC-DD2', 'windowfocus']
    original = target == 'original'
    driver_source_sha256 = hashlib.sha256(Path(natural_champ_driver.__file__).read_bytes()).hexdigest() if original and natural_champ else None
    draw_pc = (0x412ca0 if full_video else 0x420c9c) if original else int(gdb.parse_and_eval('&PutDispEnv' if full_video else '&Draw_All'))
    pad_pc = 0x422da4 if original else int(gdb.parse_and_eval('&FUN_00422da4'))
    rng_entry = 0x456cc6 if original else int(gdb.parse_and_eval('&rand'))

    def read(address, size):
        return inferior.read_memory(address, size).tobytes()

    def word(address):
        return int.from_bytes(read(address, 4), 'little')

    def integer(address):
        return int.from_bytes(read(address, 4), 'little', signed=True)

    def state():
        value = dict(level=integer(0x936ff4), cf=integer(0x462ff0),
                    ticks=integer(0x7746c0), countdown=integer(0x784298),
                    frame_skip=integer(0x7746b8), quit=integer(0x7746ac),
                    race=integer(0x93dec8), season=integer(0x93dec0),
                    finished=integer(0x795df4), retired=integer(0x9376a8), damage=integer(0x792a76),
                    phase=integer(0x4699cc),
                    poly_list=word(0x940010), clock_calls=len(clocks),
                    rng_calls=len(rng), pad_polls=len(pads), draws=draws)
        if natural_multiplayer: value['player'] = integer(0x93decc)
        return value

    timeline = (root / 'events.jsonl').open('x')

    def event(kind, **extra):
        nonlocal event_index
        event_index += 1
        if event_index > (1500000 if natural_season or natural_multiplayer else 300000 if natural_champ else 100000):
            raise RuntimeError('Championship history exceeded its bounded observation limit')
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
            vk = dict(Return=0x0d, Escape=0x1b, Up=0x26, Down=0x28, Left=0x25, Right=0x27, a=0x41, z=0x5a)[key]
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
        name = f'step{key_index:02d}-'+('natural-finish' if normal_arena and key_index > len(keys) else actions[key_index - 1] if key_index else 'boot')
        directory = root / name
        directory.mkdir(exist_ok=result_tables and key_index in captures)
        value = {key: integer(address) for key, address in ADDRESSES.items()}
        text = lambda address, size: read(address, size).split(b'\0')[0].decode('ascii')
        value.update(stage=target+(' PutDispEnv entry' if full_video else ' Draw_All entry'), exe_modified=False if original else None, exe_sha256=EXE if original else None,
            observed=state(), cars=[dict(name=text(0x93dee0 + i * 54, 16),
                values=list(struct.unpack('<7h', read(0x93def0 + i * 54, 14)))) for i in range(20)],
            rows=[dict(name=text(0x940290 + i * 26, 26), points=text(0x940240 + i * 16, 16)) for i in range(5)])
        # Check the actual path rather than publishing a complete-looking trace
        # after a key was consumed on the wrong screen.
        if not result_tables and not normal_arena and not natural_champ and key_index >= 10 and (key_index - 10) % 17 == 0 and key_index <= 78:
            race = (key_index - 10) // 17
            if (value['level'], value['race']) != ((1, 2, 5, 7, 10)[race], race):
                raise RuntimeError('Wrong actual championship race start')
        if not result_tables and not normal_arena and not natural_champ and key_index >= 17 and (key_index - 17) % 17 == 0:
            race = (key_index - 17) // 17
            if (value['level'], value['race'], value['poly_list']) != (15, race + 1, 0x46bf38 if race < 4 else 0x46ae44):
                raise RuntimeError('Wrong actual championship result screen')
        if not result_tables and not normal_arena and not natural_champ and key_index == 95:
            validate_end(value)
        if normal_arena and key_index > len(keys) and (value['poly_list'] != 0x46a468 or integer(0x795df4) <= 14 or integer(0x9376a8)):
            raise RuntimeError('Normal arena did not naturally reach Practice_Over')
        if natural_season:
            if key_index in NATURAL_SEASON_STARTS:
                race=NATURAL_SEASON_STARTS.index(key_index)
                if (value['level'],value['race'],value['quit'])!=(CHAMP_LEVELS[race],race,0):
                    raise RuntimeError('Wrong actual natural-season race start')
            if key_index>=11 and (key_index-11)%11==0:
                race=(key_index-11)//11
                if (value['level'],value['race'],value['poly_list'],value['stats'])!=(15,race+1,0x46bf38 if race<4 else 0x46ae44,1):
                    raise RuntimeError('Natural season result did not reach the required statistics screen')
            if 13<=key_index<=61 and 0<=(key_index-13)%11<=4:
                if (value['poly_list'],value['division'])!=(0x46ab30,(key_index-13)%11%4):
                    raise RuntimeError('Natural season input did not reach the requested league page')
            if key_index==len(actions) and len(final_races)!=5:
                raise RuntimeError('Natural season must contain five actual natural race completions')
        if natural_multiplayer:
            value['player'] = integer(0x93decc)
            value['multi_count'] = integer(0x467658)
            natural_multiplayer_protocol.validate_checkpoint(key_index, value)
        if natural_champ and not natural_season and not natural_multiplayer:
            if key_index in (10, 21) and (value['level'],value['race'],value['quit']) != ((1,0,0) if key_index==10 else (2,1,0)):
                raise RuntimeError('Wrong naturally continued championship race start')
            if key_index == 11 and (value['level'],value['race'],value['poly_list'],value['stats']) != (15,1,0x46bf38,1):
                raise RuntimeError('Natural first championship result did not reach season statistics')
            if 13<=key_index<=17 and (value['poly_list'],value['division'])!=(0x46ab30,(key_index-13)%4):
                raise RuntimeError('Natural championship input did not reach the requested league page')
        if natural_champ and integer(0x9376a8):
            raise RuntimeError('Natural championship trace must never Retire')
        if result_tables:
            from verify_championship_save import LAYOUT
            value['saved_state'] = {**{key:integer(address) for key,address,_ in LAYOUT['fields']},
                **{key:read(address,size).hex() for key,address,size,_ in LAYOUT['regions']},
                'joy_present':read(0x754451,1)[0]}
            for steps, expected in [(protocol.STARTS,protocol.PLAN['start_states']),
                                    (protocol.OVERS,protocol.PLAN['over_states'])]:
                if key_index in steps:
                    wanted = expected[steps.index(key_index)]
                    if result_tables == 'multiplayer-loaded' and 'race_mode' in wanted:
                        wanted = dict(wanted,race_mode=result_race_mode)
                    actual = {**value,**value['saved_state']}
                    if any(actual[key] != val for key,val in wanted.items()):
                        raise RuntimeError('Wrong actual result route state: '+str((key_index,wanted,{k:actual[k] for k in wanted})))
            if str(result_tables).startswith('multiplayer') and key_index >= len(protocol.PLAN['load_keys']):
                if value['saved_state']['multi_count'] != 2:
                    raise RuntimeError('Two actual multiplayer names required')
            if key_index in protocol.TABLES:
                plan = protocol.PLAN
                if value['level'] != 15 or value['poly_list'] != plan['menu']:
                    raise RuntimeError('Actual result viewer required')
                value['result_rows'] = [dict(name=text(plan['name_address']+i*26,26),
                    points=text(plan['point_address']+i*plan['point_stride'],plan['point_stride'])) for i in range(20)]
                value['rectangle'] = list(struct.unpack('<hhhh',read(plan['rectangle_address'],8)))
            if key_index in card_steps:
                card_text = lambda pointer: text(word(pointer),80) if 0x400000 < word(pointer) < 0x980400 else ''
                value.update(file_mode=integer(0x93a318),file_slot=integer(0x774680),file_ui=dict(
                    caption=card_text(0x46725c),detail=card_text(0x4672c0),selection=card_text(0x467284),
                    name_cursor=list(struct.unpack('<hh',read(0x4673dc,4))),ring=list(struct.unpack('<hh',read(0x467198,4)))))
            if key_index in captures: value['cycle'] = str(directory/'cycle')
            if key_index == len(keys) and [value[k] for k in ['level','poly_list','race','stats']] != protocol.PLAN['end']:
                raise RuntimeError('Actual completed season title return required')
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
                        send(actions[key_index - 1], False)
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
                present_caller = caller
                if original:
                    if full_video and caller == 0x420cc2:
                        # At PutDispEnv entry ebp still belongs to Draw_All.
                        caller = word((int(gdb.parse_and_eval('$ebp')) & 0xffffffff) + 4)
                    game_caller = caller == 0x423fe2
                else:
                    owner = gdb.newest_frame().older()
                    if full_video and owner and owner.name() == 'Draw_All':
                        owner = owner.older()
                    block = gdb.block_for_pc(int(owner.pc()) if full_video and owner else caller)
                    while block and block.function is None:
                        block = block.superblock
                    game_caller = bool(block and block.function.name == 'Play_Game')
                # Explicitly change elapsed physical time at an observed draw,
                # without writing frame_skip, clock returns or engine state.
                # This diagnoses the original's naturally adaptive rest loop.
                if game_frame_delay_ms and game_caller:
                    time.sleep(game_frame_delay_ms / 1000)
                if (normal_arena or natural_champ) and game_caller:
                    if len(race_frames) >= (30000 if natural_champ else 5900):
                        raise RuntimeError('Racing capture reached the bounded per-run image limit')
                    value = state()
                    filename = f'race{len(race_frames):05d}'
                    raw = read(0x700450, 307200)
                    palette = read(0x700050, 1024)
                    (root / (filename+('.bin.z' if natural_champ else '.bin'))).write_bytes(zlib.compress(raw,6) if natural_champ else raw)
                    (root / (filename+'.pal')).write_bytes(palette)
                    race_frames.append(dict(value, index=len(race_frames), prefix=filename,
                        framebuffer_sha256=hashlib.sha256(raw).hexdigest(), palette_sha256=hashlib.sha256(palette).hexdigest()))
                    if len(race_frames) % 100 == 0:
                        print(f'{target} racing: {len(race_frames)} frames, level={value["level"]}, race={value["race"]}, ticks={value["ticks"]}, planar_speed={value["damage"]}',flush=True)
                if full_video:
                    value = state()
                    filename = f'present{len(presentations):05d}'
                    raw = read(0x700450, 307200)
                    palette = read(0x700050, 1024)
                    (root / (filename + '.bin')).write_bytes(raw)
                    (root / (filename + '.pal')).write_bytes(palette)
                    presentations.append(dict(value, index=len(presentations), prefix=filename,
                        caller=hex(caller), present_caller=hex(present_caller), game=game_caller,
                        framebuffer_sha256=hashlib.sha256(raw).hexdigest(),
                        palette_sha256=hashlib.sha256(palette).hexdigest()))
                event('PutDispEnv' if full_video else 'Draw_All', caller=hex(caller), slab=int.from_bytes(read(0x46996c, 2), 'little', signed=True))
                if stage == 'drive' and integer(0x7746ac):
                    final_race = state()
                    if natural_champ:
                        final_race.update(driver=driver_metrics(read))
                        final_races.append(final_race)
                    if integer(0x795df4) <= 14 or integer(0x9376a8):
                        raise RuntimeError('Arena left gameplay without natural finish')
                    if natural_champ:
                        if original:
                            for key in list(driving_held):
                                send(key,False)
                                driving_inputs.append(dict(frame=len(race_frames)-1,key=key,down=False,finish=True))
                            driving_held.clear()
                    else:
                        send('Up', False);send('Right', False)
                    stage = 'settle';steady = 0
                if natural_champ:
                    frame = len(race_frames)-1
                    if not original and driving_reference:
                        while driving_cursor < len(driving_reference) and driving_reference[driving_cursor]['frame'] == frame and (bool(integer(0x7746ac)) if driving_reference[driving_cursor].get('finish') else game_caller):
                            transition=driving_reference[driving_cursor]
                            send(transition['key'],transition['down']);driving_inputs.append(transition)
                            driving_cursor+=1
                    elif stage == 'drive' and game_caller and integer(0x795df4)>14:
                        # Release the real keys before Play_Game's rest batch
                        # and Setup_Pad(0) remap A's flag to menu Return. A late
                        # A release after that remap cannot clear that flag.
                        for key in list(driving_held):
                            send(key,False);driving_inputs.append(dict(frame=frame,key=key,down=False))
                        driving_held.clear();finish_controls_released=True
                    elif stage == 'drive' and game_caller and not finish_controls_released and integer(0x784298)<0:
                        tick=integer(0x7746c0)
                        if last_control_tick is None or tick-last_control_tick>=(1 if natural_multiplayer else 6):
                            if natural_season and integer(0x936ff4)>=8:
                                observed=dict(driver_metrics(read),wanted=['a','Right'],manoeuvre='arena')
                            else:
                                observed=keyboard_driver.controls(read,tick)
                            wanted=observed['wanted']
                            if natural_multiplayer and observed['speed'] >= natural_multiplayer_speed and 'a' in wanted:
                                wanted.remove('a')
                            for key in list(driving_held):
                                if key not in wanted:
                                    send(key,False);driving_inputs.append(dict(frame=frame,key=key,down=False))
                                    driving_held.remove(key)
                            for key in wanted:
                                if key not in driving_held:
                                    send(key,True);driving_inputs.append(dict(frame=frame,key=key,down=True))
                                    driving_held.append(key)
                            event('driver',**observed)
                            last_control_tick=tick
                if stage == 'held':
                    send(actions[key_index - 1], False)
                    stage = 'release'
                if stage != 'settle':
                    continue
                race_start = key_index in natural_multiplayer_protocol.STARTS if natural_multiplayer else key_index in protocol.STARTS if result_tables else ((key_index in NATURAL_SEASON_STARTS or key_index==len(actions) and integer(0x936ff4) in range(1,13)) if natural_season else key_index in (10,21) if natural_champ else key_index == len(keys) if normal_arena else key_index >= 10 and (key_index - 10) % 17 == 0 and key_index <= 78)
                if race_start:
                    ready = game_caller and integer(0x7746ac) == 0 and (not result_tables or integer(0x7746c0)>=8)
                else:
                    result_action=natural_champ and key_index>0 and actions[key_index-1]=='natural-finish'
                    result_screen=natural_multiplayer_protocol.OVER_STATES[natural_multiplayer_protocol.OVERS.index(key_index)]['poly_list'] if natural_multiplayer and result_action else 0x46ae44 if natural_season and key_index==55 else 0x46bf38
                    ready = read(0x46996c, 2) == b'\0\0' and (not normal_arena or key_index <= len(keys) or word(0x940010) == 0x46a468) and (not result_action or word(0x940010)==result_screen)
                steady = steady + 1 if ready else 0
                if steady < (2 if race_start else 16):
                    continue
                if result_tables and key_index in captures:
                    name = f'step{key_index:02d}-'+actions[key_index-1]
                    directory = root/name/'cycle'; directory.mkdir(parents=True,exist_ok=True)
                    cycle = result_cycles.setdefault(key_index,[])
                    value = state()
                    if key_index in card_steps:
                        value['card_phase'] = integer(0x467390)%255
                        pair = (value['phase'],value['card_phase'])
                        if key_index in card_wanted:
                            if steady>3328: raise RuntimeError('Actual card animation pairs not reached')
                            if pair not in card_wanted[key_index]: continue
                            card_wanted[key_index].remove(pair)
                        elif value['phase'] in {f['phase'] for f in cycle}: continue
                    prefix=f'frame{len(cycle):03d}'
                    for region,address,size in [('framebuf',0x700450,307200),('palette',0x700050,1024)]:
                        (directory/(prefix+'-'+region+'.bin')).write_bytes(read(address,size))
                    value.update(index=len(cycle),prefix=prefix,
                        sound_volume=integer(0x467410),working_sound_volume=integer(0x93fd20),master_sfx_volume=integer(0x462d84))
                    cycle.append(value)
                    if len(cycle)<64: continue
                    (directory/'cycle.json').write_text(json.dumps(dict(stage='Draw_All entry / pending presentation',frames=cycle),indent=2)+'\n')
                snapshot()
                if natural_champ and key_index<len(actions) and actions[key_index]=='natural-finish':
                    key_index+=1;stage='drive'
                    keyboard_driver=new_driver()
                    last_control_tick=None;finish_controls_released=False
                    continue
                if normal_arena and key_index == len(keys):
                    if integer(0x4673f8)!=2 or integer(0x4673f4)!=2 or integer(0x936ff4)<8:
                        raise RuntimeError('Wrong actual Total Destruction arena')
                    key_index += 1
                    send('Up', True);send('Right', True)
                    stage = 'drive'
                    continue
                if normal_arena and key_index > len(keys):
                    break
                if key_index == len(actions):
                    break
                key_index += 1
                key_flag = flag_for(actions[key_index - 1])
                stage = 'down'
                send(actions[key_index - 1], True)
            else:
                raise RuntimeError(f'Unexpected original history stop: {pc:x}')
        if not rng or rng[0][0] != 1 or rng_pc != rng_entry or clock_pc != clock_entry:
            raise RuntimeError('Incomplete original RNG/clock entry-to-return pairs')
        if natural_champ and not original and driving_cursor!=len(driving_reference):
            raise RuntimeError('Recorded original driving input extent differs')
        (root / 'ticks.bin').write_bytes(b''.join(struct.pack('<I', value) for value in clocks))
        (root / 'random.bin').write_bytes(b''.join(struct.pack('<III', *row) for row in rng))
        (root / 'history.json').write_text(json.dumps(dict(scope=(natural_multiplayer_protocol.SCOPE if natural_multiplayer else __doc__.strip()) if original else 'Actual native keyboard-bridge championship trace; calculated RNG and recorded clock inputs; no physical sink/whole-game parity claim',
            exe_modified=False if original else None, exe_sha256=EXE if original else None, keys=keys, checkpoints=checkpoints,
            target=target,input='real X11 keys' if original else 'dd2_key_event', acknowledged_keys=True, held_pad_polls=held_counts,
            clock_calls=len(clocks), rng_calls=len(rng), pad_polls=len(pads), draws=draws,
            observed_play_draw_delay_ms=game_frame_delay_ms,
            complete_retirement_season=not result_tables and not normal_arena and not natural_champ and steps == 95, normal_arena=normal_arena,
            result_tables=result_tables,
            natural_championship=natural_champ, natural_season=natural_season, natural_multiplayer=natural_multiplayer,
            actions=actions, driving_inputs=driving_inputs,
            driving_policy=('steady-persistent' if natural_multiplayer else 'steady' if steady_driver else 'default') if original and natural_champ else None,
            driving_source_sha256=driver_source_sha256,
            driving_speed_limit=natural_multiplayer_speed if natural_multiplayer and original else None,
            driving_slow_recovery=slow_recovery if natural_multiplayer and original else None,
            driving_wall_recovery=wall_recovery if natural_multiplayer and original else None,
            racing_image_format='indexed-zlib' if natural_champ else 'indexed-raw',
            natural_finish=bool(final_race), final_race=final_race, final_races=final_races, race_frames=race_frames,
            full_video=full_video, presentation_boundary='PutDispEnv' if full_video else None, presentations=presentations,
            elapsed_seconds=time.monotonic() - start_time), indent=2) + '\n')
    except Exception as failure:
        # Keep one bounded causal checkpoint before batch GDB closes the
        # inferior. A failed trace must never resemble a completed comparison.
        diagnostic = dict(scope='Incomplete capture failure checkpoint; no parity or completed-race acceptance',
            target=target,error=str(failure),state=state(),racing_frames=len(race_frames),
            pc=hex(int(gdb.parse_and_eval('$pc')) & 0xffffffff),
            stack=gdb.execute('bt',to_string=True),locals=gdb.execute('info locals',to_string=True),
            registers=gdb.execute('info registers',to_string=True),regions={})
        regions=[('render-globals',0x74c520,0x1d0),('rot-points',0x74f1c0,16384),
                 ('rot-flags',0x7531c0,4096),('framebuf',0x700450,307200),('palette',0x700050,1024),
                 ('order-table',word(0x7541c4),16384)]
        if not original:
            for name,variables,size in [('polygon',('iStack_18','piStack_14'),48),
                                        ('primitive',('puStack_14','puStack_18'),40)]:
                for variable in variables:
                    try:
                        regions.append((name,int(gdb.parse_and_eval(variable)) & 0xffffffff,size))
                        break
                    except gdb.error:pass
            frame=gdb.newest_frame()
            while frame:
                if frame.name()=='FUN_0041ff98':
                    object_address=int(frame.read_var('param_1')) & 0xffffffff
                    shape=word(object_address+4)
                    regions.extend([('object',object_address,28),('shape',shape,44)])
                    vertices=int.from_bytes(read(shape+10,2),'little')
                    if 0<vertices<=2048:
                        regions.append(('input-vertices',word(shape+32),vertices*8))
                    break
                frame=frame.older()
        for name,address,size in regions:
            try:
                raw=read(address,size);filename='failure-'+name+'.bin';(root/filename).write_bytes(raw)
                diagnostic['regions'][name]=dict(address=hex(address),size=size,file=filename,sha256=hashlib.sha256(raw).hexdigest())
            except gdb.MemoryError as error:
                diagnostic['regions'][name]=dict(address=hex(address),size=size,error=str(error))
        (root/'failure.json').write_text(json.dumps(diagnostic,indent=2)+'\n')
        raise
    finally:
        timeline.close()
        for point in points:
            if point.is_valid():
                point.delete()
