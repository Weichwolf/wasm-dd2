"""Drive genuine native X11 keys at original-observed sound presentation positions.

Hardware breakpoints only; all memory observations are read-only. Positions are
derived from the original's six frontend sound triggers, not original OS event
timestamps. This supplies explicit diagnostic scheduling; physical timing and
complete original/port output acceptance require separate comparisons.
"""
import json
import hashlib
from pathlib import Path
import subprocess
import time

import gdb
from artifacts import check_space


def drive(output,schedule,error_pc):
    out=Path(output);inferior=gdb.selected_inferior();points=[];inputs=[]
    index=0;stage='ready';flag=None;mask=None;return_pc=None;held=0
    started=time.monotonic()
    def read(a,n):return bytes(inferior.read_memory(a,n))
    def integer(a):return int.from_bytes(read(a,4),'little',signed=True)
    def value(name):return int(gdb.parse_and_eval(name))
    def state():
        return dict(completed_flips=value('g_frameno'),clock_calls=value('dd2_tick_calls'),
            services=value('ds_service_index'),level=integer(0x936ff4),replay=integer(0x467074),
            quit=integer(0x7746ac),ticks=integer(0x7746c0),script_cursor=integer(0x9392b4),
            first_time=integer(0x9392b0),held=int.from_bytes(read(0x754448,2),'little'),
            pressed=int.from_bytes(read(0x75444a,2),'little'))
    def point(name,condition):
        p=gdb.Breakpoint(name,type=gdb.BP_HARDWARE_BREAKPOINT);p.silent=True;p.condition=condition
        points.append(p);return p
    def send(down):
        key=schedule[index]['key']
        subprocess.run(['xdotool','keydown' if down else 'keyup',key],check=True,timeout=5)
        inputs.append(dict(key=key,down=down,action=index,observation=state()))
    def mapping(key):
        vk={'Right':39,'Return':13}[key];m=read(0x46302c,14)
        flags=(0x46303f,0x463040,0x463043,0x463046,0x463044,0x463045,
               0x463048,0x46304a,0x463047,0x463049,0x46304b,0x46304e,0x46304c,0x46304d)
        bits=(1,8,0x10,0x20,0x40,0x80,0x100,0x200,0x400,0x800,0x1000,0x2000,0x4000,0x8000)
        i=next(i for i in (0,1,2,3,4,5,8,6,9,7,10,11,12,13) if m[i]==vk)
        return flags[i],bits[i]
    poll_pc=value('&dd2_native_poll');pad_pc=value('&FUN_00422da4')
    # Explicit instruction addresses preserve function-entry stack layout;
    # source-line breakpoints can stop after the function prologue. The
    # completion helper is inlined at O2. A hardware write watchpoint observes
    # its actual completion flag immediately after the output/report close.
    poll=point(f'*0x{poll_pc:x}',f'g_frameno == {schedule[0]["flip"]-1}')
    pad=point(f'*0x{pad_pc:x}','0')
    complete=gdb.Breakpoint('ds_service_finished',type=gdb.BP_WATCHPOINT,wp_class=gdb.WP_WRITE)
    complete.silent=True;complete.condition='ds_service_finished == 1';points.append(complete)
    if complete.type!=gdb.BP_HARDWARE_WATCHPOINT:
        raise RuntimeError('Completion observer requires an actual hardware watchpoint')
    error=point(f'*0x{error_pc:x}','1')
    last=None;return_point=None
    try:
        while True:
            if time.monotonic()-started>150:raise RuntimeError('Native replay audio input observation exceeded 150 seconds')
            gdb.execute('continue')
            if gdb.selected_thread() is None:
                raise RuntimeError('Native engine terminated before replay input capture completed; see game-1.log')
            pc=value('$pc')&0xffffffff;last=state()
            if pc==error_pc:
                raise RuntimeError('Native audio assertion failed at its own calculated position: '+json.dumps(last))
            if value('ds_service_finished'):
                if index!=len(schedule) or last['script_cursor']<=0x9376b0 or last['first_time']!=1:
                    raise RuntimeError('Replay did not naturally finish after all real keys')
                if last['level']!=0 or last['replay']!=0 or last['quit']!=1:
                    raise RuntimeError('Audio endpoint precedes natural frontend restoration')
                if not (out/'clock-complete.json').exists():raise RuntimeError('Actual audio completion report missing')
                break
            if return_point and pc==return_pc:
                return_point.delete();return_point=None
                if stage=='ack':
                    if not last['held']&mask or not last['pressed']&mask:
                        raise RuntimeError('Actual native pad poll did not consume the real key')
                    inputs.append(dict(action=index,key=schedule[index]['key'],pad_consumed=True,observation=last))
                    if 'release_flip' in schedule[index]:
                        stage='holding';pad.condition=f'*(unsigned char*)0x{flag:x} != 0';pad.enabled=True
                        poll.condition=f'g_frameno == {schedule[index]["release_flip"]-1}';poll.enabled=True
                    else:
                        stage='release';pad.condition=f'*(unsigned char*)0x{flag:x} == 0';pad.enabled=True
                else:raise RuntimeError('Unexpected continuation acknowledgement')
                continue
            if pc==poll_pc:
                if stage=='holding':
                    if last['completed_flips']!=schedule[index]['release_flip']-1:
                        raise RuntimeError('Diagnostic key release position differs')
                    send(False);stage='release';poll.enabled=False
                    pad.condition=f'*(unsigned char*)0x{flag:x} == 0';pad.enabled=True
                    continue
                if stage!='ready' or index>=len(schedule):raise RuntimeError('Unexpected scheduled input poll')
                if last['completed_flips']!=schedule[index]['flip']-1 or last['clock_calls']!=schedule[index]['clock_calls']:
                    raise RuntimeError('Scheduled native input position differs')
                flag,mask=mapping(schedule[index]['key']);send(True);held=0;stage='down'
                poll.enabled=False;pad.condition=f'*(unsigned char*)0x{flag:x} != 0';pad.enabled=True
            elif pc==pad_pc:
                if stage=='down':
                    held+=1
                    if last['completed_flips']!=schedule[index]['flip']:
                        raise RuntimeError('Native consumed key at a different presentation')
                    if 'release_flip' not in schedule[index]:send(False)
                    stage='ack';pad.enabled=False
                    return_pc=int.from_bytes(read(value('$esp'),4),'little')
                    return_point=point(f'*0x{return_pc:x}','1')
                elif stage=='holding':
                    held+=1
                elif stage=='release':
                    inputs.append(dict(action=index,key=schedule[index]['key'],released_flag=True,held_polls=held,observation=last))
                    index+=1;pad.enabled=False
                    if index<len(schedule):
                        stage='ready';poll.condition=f'g_frameno == {schedule[index]["flip"]-1}';poll.enabled=True
                    else:stage='playback'
                else:raise RuntimeError('Unexpected actual pad observation: '+stage)
            else:raise RuntimeError('Unexpected hardware stop')
            if stage=='release':pad.enabled=True
            check_space(out)
        result=dict(scope=__doc__,pass_=True,hardware_only=True,engine_state_writes=False,
            schedule=schedule,inputs=inputs,terminal=last,elapsed_seconds=time.monotonic()-started)
        # Capture these at the same stopped endpoint, before detach allows the
        # frontend to request services outside the bounded original capture.
        from reference.capture import state as scene
        from verify_configuration_persistence import settings
        from types import SimpleNamespace
        digest=lambda raw:hashlib.sha256(raw).hexdigest()
        result['endpoint']=dict(scene=scene(inferior.pid),
            settings=settings(SimpleNamespace(read=read,integer=integer)),
            script_sha256=digest(read(0x9376b0,0x1c00)),order_sha256=digest(read(0x795c28,20)),
            card_sha256=digest(read(0x754460,0x20000)))
        (out/'input-history.json').write_text(json.dumps(result,indent=2)+'\n')
    except Exception as failure:
        (out/'input-failure.json').write_text(json.dumps(dict(pass_=False,error=str(failure),stage=stage,
            action=index,inputs=inputs,last=last),indent=2)+'\n')
        raise
    finally:
        for p in points:
            if p.is_valid():p.delete()
