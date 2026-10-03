"""GDB-only read-only racing-loop video and game-clock observer.

Use three hardware breakpoints, never software breakpoints or register/state
writes. One moving breakpoint records the real EAX returned by each engine
GetTickCount call, including every busy-wait iteration. This debugger changes
wall-clock timing; ports must replay that observed API input exactly. It is
not physical display/audio or normal undebugged-clock acceptance.
"""
import json
from pathlib import Path
import struct
import gdb


def record_race(output,level):
    directory=Path(output)/'race';directory.mkdir()
    ticks=[];frames=[];complete=False
    inferior=gdb.selected_inferior()
    def read(address,size):return bytes(inferior.read_memory(address,size))
    def integer(address):return int.from_bytes(read(address,4),'little',signed=True)
    def state():return {'level':integer(0x936ff4),'cf':integer(0x462ff0),
        'ticks':integer(0x7746c0),'countdown':integer(0x784298),
        'frame_skip':integer(0x7746b8),'quit':integer(0x7746ac)}
    kind=gdb.BP_HARDWARE_BREAKPOINT
    initial=gdb.Breakpoint('*0x423c2d',type=kind);initial.silent=True
    initial.condition=f'*(int*)0x936ff4 == {level} && *(int*)0x46385c == 1'
    gdb.execute('continue')
    if int(gdb.parse_and_eval('$pc'))!=0x423c2d:raise RuntimeError('first game GetTickCount return not reached')
    initial.delete()
    first_state=state()
    ticks.append(int(gdb.parse_and_eval('$eax'))&0xffffffff)
    # The return addresses are from the actual unmodified PE instructions:
    # 423c26(initial), 424000(after drawing), 42401d(wait), 424039(last time).
    expected=0x424007
    clock=gdb.Breakpoint(f'*0x{expected:x}',type=kind);clock.silent=True
    draw=gdb.Breakpoint('*0x420c9c',type=kind);draw.silent=True
    draw.condition='*(unsigned int*)$esp == 0x423fe2'
    end=gdb.Breakpoint('*0x42404a',type=kind);end.silent=True
    while True:
        gdb.execute('continue')
        pc=int(gdb.parse_and_eval('$pc'))
        if pc==0x42404a:
            final_state=state()
            if final_state['quit']!=1 or int(gdb.parse_and_eval('$edi'))>=0 or integer(0x9376ac)!=0:
                raise RuntimeError('demo did not end by exhausting its original physics budget without user input')
            complete=True;break
        if pc==0x420c9c:
            current=state()
            if current['level']!=level or current['quit'] or integer(0x46385c)!=1:
                raise RuntimeError('wrong/inactive demo render')
            prefix=f'frame{len(frames):05d}'
            (directory/f'{prefix}.bin').write_bytes(read(0x700450,307200))
            (directory/f'{prefix}.pal').write_bytes(read(0x700050,1024))
            current.update(index=len(frames),prefix=prefix,clock_calls=len(ticks))
            frames.append(current)
            if len(frames)%100==0:print(f'Original race stream: {len(frames)} frames, {len(ticks)} clock returns, cf={current["cf"]}',flush=True)
            continue
        if pc!=expected:raise RuntimeError(f'unexpected stop: 0x{pc:x}, expected clock 0x{expected:x}')
        value=int(gdb.parse_and_eval('$eax'))&0xffffffff;ticks.append(value)
        if pc==0x424007:next_clock=0x424024
        elif pc==0x424024:
            delta=(value-(integer(0x7746a0)&0xffffffff))&0xffffffff
            next_clock=0x424024 if delta < integer(0x7746b8)*20 else 0x424040
        else:next_clock=0x424007
        if next_clock!=expected:
            clock.delete();expected=next_clock
            clock=gdb.Breakpoint(f'*0x{expected:x}',type=kind);clock.silent=True
    for breakpoint in (clock,draw,end):breakpoint.delete()
    if not complete or not frames:raise RuntimeError('incomplete original race loop')
    (directory/'ticks.bin').write_bytes(b''.join(struct.pack('<I',value) for value in ticks))
    result={'stage':'Draw_All entry / pending presentation','scope':__doc__,'level':level,
        'complete_racing_loop':True,'breakpoints':'hardware only; no inferior memory/register writes',
        'clock_source':'actual GetTickCount return EAX at original engine call sites',
        'first_state':first_state,'final_state':final_state,'clock_calls':len(ticks),'frames':frames}
    (directory/'race.json').write_text(json.dumps(result,indent=2)+'\n')
    print(f'Original complete racing loop: {len(frames)} frames, {len(ticks)} actual clock returns',flush=True)
