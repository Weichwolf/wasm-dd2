"""Read-only replay history with four hardware breakpoint slots.

Genuine X11 keys load an actual original replay card through File Manager.
All engine clock returns and all computed random triples are observed from the
frontend through natural replay completion. Every racing pending presentation
is captured; loading/menu video and chronological audio remain separate.
Debugger stops affect physical time. Captured clock values are explicit API
inputs for port comparison, rather than undebugged timing acceptance.
"""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time
import zlib

import gdb
from artifacts import check_space

KEYS = ['Right','Right','Right','Return','Return','Return']
REGIONS = [('car_fd',0x792690,20*44),
           ('car_state',0x792a00,20*0x1b2),('wheel_fd',0x794be8,20*0xb0)]


def record_replay_history(output,target,diagnostic_frames=()):
    root=Path(output)/'history';root.mkdir()
    original=target=='original'
    inferior=gdb.selected_inferior()
    points=[];clocks=[];random=[];frames=[];inputs=[];checkpoints=[]
    stage='settle';steady=0;index=0;draws=0;polls=0;held_polls=0;held_counts=[]
    flag=None;started=time.monotonic();rng_pointer=rng_before=rng_caller=None;clock_caller=None
    draw_pc=0x420c9c if original else int(gdb.parse_and_eval('&Draw_All'))
    pad_pc=0x422da4 if original else int(gdb.parse_and_eval('&FUN_00422da4'))
    rng_entry=0x456cc6 if original else int(gdb.parse_and_eval('&rand'))
    clock_entry=int.from_bytes(inferior.read_memory(0x9500c0,4),'little') if original else int(gdb.parse_and_eval('&GetTickCount'))
    clock_condition='*(unsigned int*)$esp >= 0x400000 && *(unsigned int*)$esp < 0x470000' if original else None
    rng_pc=rng_entry;clock_pc=clock_entry

    def read(a,n):return bytes(inferior.read_memory(a,n))
    def integer(a):return int.from_bytes(read(a,4),'little',signed=True)
    def word(a):return int.from_bytes(read(a,4),'little')
    def text(a):
        pointer=word(a)
        return read(pointer,80).split(b'\0')[0].decode('ascii') if pointer else ''
    def state():
        return dict(level=integer(0x936ff4),cf=integer(0x462ff0),ticks=integer(0x7746c0),
            countdown=integer(0x784298),frame_skip=integer(0x7746b8),quit=integer(0x7746ac),
            replay=integer(0x467074),car=integer(0x467400),mode=integer(0x4673f8),type=integer(0x4673f4),
            end=word(0x9392c4),script_cursor=word(0x9392b4),first_time=integer(0x9392b0),
            pedal=integer(0x792a86),poly_list=word(0x940010),phase=integer(0x4699cc),
            render_buffer=integer(0x462fec),
            clock_calls=len(clocks),rng_calls=len(random))
    def point(pc,condition=None):
        p=gdb.Breakpoint(f'*0x{pc:x}',type=gdb.BP_HARDWARE_BREAKPOINT);p.silent=True
        if condition:p.condition=condition
        points.append(p);return p
    def send(key,down):
        subprocess.run(['xdotool','keydown' if down else 'keyup',key],check=True,timeout=5)
        inputs.append(dict(action=index,key=key,down=down,draws=draws,polls=polls))
    def mapped_flag(key):
        vk={'Right':39,'Return':13}[key]
        flags=(0x46303f,0x463040,0x463043,0x463046,0x463044,0x463045,
               0x463048,0x46304a,0x463047,0x463049,0x46304b,0x46304e,0x46304c,0x46304d)
        mapping=read(0x46302c,14)
        slot=next((i for i in (0,1,2,3,4,5,8,6,9,7,10,11,12,13) if mapping[i]==vk),None)
        if slot is None:raise RuntimeError('unmapped replay navigation key')
        return flags[slot]
    rng_point=point(rng_pc);clock_point=point(clock_pc,clock_condition)
    point(pad_pc);point(draw_pc)
    try:
        while True:
            gdb.execute('continue')
            pc=int(gdb.parse_and_eval('$pc'))&0xffffffff
            eax=int(gdb.parse_and_eval('$eax'))&0xffffffff
            if len(random)+len(clocks)>200000 or draws>10000 or time.monotonic()-started>650:
                raise RuntimeError('replay history exceeded its bounded observation limit')
            if pc==rng_pc:
                if pc==rng_entry:
                    rng_pointer=eax if original else int(gdb.parse_and_eval('&_dd2_rand_seed'))
                    rng_before=word(rng_pointer);rng_caller=word(int(gdb.parse_and_eval('$esp'))&0xffffffff)
                    next_pc=0x456cde if original else rng_caller
                else:
                    random.append((rng_before,word(rng_pointer),eax));next_pc=rng_entry
                rng_point.delete();rng_pc=next_pc;rng_point=point(rng_pc)
                continue
            if pc==clock_pc:
                if pc==clock_entry:
                    clock_caller=word(int(gdb.parse_and_eval('$esp'))&0xffffffff)
                    if original and clock_caller not in (0x423c2d,0x423ecd,0x424007,0x424024,0x424040):
                        raise RuntimeError(f'unknown original engine clock caller: {clock_caller:x}')
                    next_pc=clock_caller
                else:
                    clocks.append(eax);next_pc=clock_entry
                clock_point.delete();clock_pc=next_pc
                clock_point=point(clock_pc,clock_condition if clock_pc==clock_entry else None)
                continue
            if pc==pad_pc:
                polls+=1
                if stage=='down' and read(flag,1)!=b'\0':
                    # Keep each real key down until a genuine pad poll sees it.
                    # The current poll still reads the flags already delivered
                    # by the engine's preceding message pump.
                    held_polls=1;send(KEYS[index-1],False);stage='release'
                elif stage=='release':
                    if read(flag,1)==b'\0':stage='settle';steady=0;held_counts.append(held_polls)
                    else:held_polls+=1
                continue
            if pc!=draw_pc:raise RuntimeError(f'unexpected replay history stop: {pc:x}')
            draws+=1
            caller=word(int(gdb.parse_and_eval('$esp'))&0xffffffff)
            if original:game=caller==0x423fe2
            else:
                owner=gdb.newest_frame().older()
                game=bool(owner and owner.name()=='Play_Game')
            current=state()
            if game:
                if index!=len(KEYS) or current['replay']!=1 or current['quit'] or current['level']!=1:
                    raise RuntimeError('unexpected racing presentation before actual replay loading')
                if len(frames)>=1000:raise RuntimeError('replay racing frame budget exceeded')
                raw,palette=read(0x700450,307200),read(0x700050,1024)
                prefix=f'frame{len(frames):04d}'
                (root/(prefix+'.bin.z')).write_bytes(zlib.compress(raw,6))
                (root/(prefix+'.pal')).write_bytes(palette)
                regions={name:hashlib.sha256(read(a,n)).hexdigest() for name,a,n in REGIONS}
                # Draw_Car and Car_Movement use positions/matrices/angles at
                # per-car +0x224..+0x27b. Earlier bytes include two copies of
                # sprite/primitive packets indexed by buffer_num. Their slot
                # parity depends on prior loading flips, not on racing physics.
                regions['car_pose']=hashlib.sha256(b''.join(read(0x78a744+c*0x27c,0x58) for c in range(20))).hexdigest()
                if len(frames) in diagnostic_frames:
                    (root/(prefix+'-dynamics.bin')).write_bytes(read(0x78a520,20*0x27c))
                frames.append(dict(current,index=len(frames),prefix=prefix,
                    framebuffer_sha256=hashlib.sha256(raw).hexdigest(),palette_sha256=hashlib.sha256(palette).hexdigest(),
                    regions=regions,primitive_storage_sha256=hashlib.sha256(read(0x78a520,20*0x27c)).hexdigest()))
                if len(frames)%25==0:
                    check_space(root)
                    print(target,'replay frames',len(frames),'ticks',current['ticks'],
                          'clocks',len(clocks),'random',len(random),flush=True)
            if stage!='settle':continue
            ready=(read(0x46996c,2)==b'\0\0' and current['level']==0)
            if index==4:ready=ready and current['poly_list']==0x4671ec and integer(0x774680)==-1
            if index==5:ready=ready and current['poly_list']==0x4671ec and integer(0x774680)==0 and 'Select' in text(0x46725c)
            if index==6:
                ready=(ready and frames and current['poly_list']==0x4696b0 and current['replay']==0 and
                       current['quit']==1 and current['script_cursor']==current['end'] and current['first_time']==1)
            steady=steady+1 if ready else 0
            if steady<16:continue
            checkpoints.append(dict(current,action=index,label=text(0x46975c),file_mode=integer(0x93a318),slot=integer(0x774680)))
            if index==6:break
            index+=1;flag=mapped_flag(KEYS[index-1]);stage='down'
            send(KEYS[index-1],True)
            check_space(root)
        if not random or random[0][0]!=1 or rng_pc!=rng_entry or clock_pc!=clock_entry:
            raise RuntimeError('complete computed RNG/clock pairs from seed 1 required')
        if not frames or not any(f['countdown']<1 and f['pedal']>0 for f in frames):
            raise RuntimeError('actual complete accelerated racing playback required')
        (root/'ticks.bin').write_bytes(b''.join(struct.pack('<I',v) for v in clocks))
        (root/'random.bin').write_bytes(b''.join(struct.pack('<III',*row) for row in random))
        result=dict(scope=__doc__,pass_=True,target=target,engine_state_writes=False,hardware_only=True,
            keys=KEYS,inputs=inputs,held_pad_polls=held_counts,checkpoints=checkpoints,frames=frames,
            clock_calls=len(clocks),rng_calls=len(random),draws=draws,polls=polls,
            stage='Draw_All entry / pending racing presentation',image_format='indexed-zlib',
            terminal=state(),elapsed_seconds=time.monotonic()-started)
        (root/'history.json').write_text(json.dumps(result,indent=2)+'\n');check_space(root)
        print(target,'complete original-file replay history:',len(frames),'frames',flush=True)
    except Exception as failure:
        diagnostic=dict(scope='incomplete replay history; no parity acceptance',pass_=False,error=str(failure),
            target=target,stage=stage,action=index,frames=len(frames))
        try:
            diagnostic.update(state=state(),pc=hex(int(gdb.parse_and_eval('$pc'))&0xffffffff),
                              stack=gdb.execute('bt',to_string=True))
        except gdb.error as stopped:
            diagnostic['inferior_observation_error']=str(stopped)
        (root/'failure.json').write_text(json.dumps(diagnostic,indent=2)+'\n')
        raise
    finally:
        for p in points:
            if p.is_valid():p.delete()
