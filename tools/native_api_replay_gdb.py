"""Replay observed browser DOM inputs at native presentation boundaries.

The ordinary-clock browser recorder supplies exact clock values and checked
LCG triples. Only real keyboard-bridge events are sent; all engine state reads
are observational. State replay does not establish original or A/V parity.
"""
import json
from pathlib import Path

import gdb


def replay(reference, output):
    reference,output=Path(reference),Path(output)
    record=json.loads((reference/'api-record.json').read_text())
    frames=record['presentations']
    anchor=next(i for i,row in enumerate(frames) if row and row['level']==0 and row['movie']==0 and row['poly_list']==0x4696b0)
    events=[row for row in record['inputs'] if row['presentation']>=anchor]
    inferior=gdb.selected_inferior()
    points=[];cursor=0;index=anchor
    addresses={name:int(gdb.parse_and_eval('&'+symbol)) for name,symbol in
               [('clock_count','dd2_tick_calls'),('rng_count','g_rand_calls'),('rng_seed','_dd2_rand_seed')]}
    fields=dict(level=0x936ff4,cf=0x462ff0,ticks=0x7746c0,countdown=0x784298,
                frame_skip=0x7746b8,quit=0x7746ac,poly_list=0x940010,movie=0x462cd4,**addresses)
    def read(address,size):return inferior.read_memory(address,size).tobytes()
    extra_fields=dict(speed=0x792a7a,planar_speed=0x792a76,x=0x78a744,z=0x78a74c,
                      dead=0x792ac6,front_damage=0x792aee,rear_damage=0x792af6)
    words=dict(heading=0x78a792,lap=0x795c48,lap_progress=0x795c4a,finished_laps=0x795c52,race_points=0x795c46)
    for name,address in extra_fields.items():
        if name in frames[anchor]:fields[name]=address
    words={name:address for name,address in words.items() if name in frames[anchor]}
    def state():
        value={name:int.from_bytes(read(address,4),'little',signed=name not in ('poly_list','clock_count','rng_count','rng_seed')) for name,address in fields.items()}
        value.update({name:int.from_bytes(read(address,2),'little')&(4095 if name=='heading' else 65535) for name,address in words.items()})
        return value
    codes=dict(Enter=13,Escape=27,ArrowUp=38,ArrowDown=40,ArrowLeft=37,ArrowRight=39,KeyA=65,KeyZ=90)
    report=dict(scope=__doc__.strip(),source='observed production browser; not original',pass_=False,
                source_anchor=anchor,source_frames=len(frames),matched_frames=0,input_events=len(events),sent_events=0)
    def save_failure(error):
        report.update(error=str(error),expected=frames[index] if index<len(frames) else None,
                      actual=state(),stack=gdb.execute('bt',to_string=True))
        for name,address,size in [('framebuf.bin',0x700450,307200),('palette.bin',0x700050,1024),
                                  ('car-fd.bin',0x792690,44),('car-handling.bin',0x792a04,434),
                                  ('car-actor.bin',0x78a704,636),('car-info.bin',0x795c40,20)]:
            (output/name).write_bytes(read(address,size))
    try:
        point=gdb.Breakpoint('*PutDispEnv',type=gdb.BP_HARDWARE_BREAKPOINT);point.silent=True;points.append(point)
        # The caller has already stopped at the first main-menu presentation.
        while index<len(frames):
            actual=state();expected=frames[index]
            if not expected or any(actual[name]!=expected[name] for name in (*fields,*words)):
                raise ValueError(f'Native presentation {index} differs: '+str([name for name in (*fields,*words) if actual[name]!=expected.get(name)]))
            report['matched_frames']+=1
            while cursor<len(events) and events[cursor]['presentation']==index:
                event=events[cursor]
                if event['state']!=expected:
                    raise ValueError('Source key did not occur at the recorded presentation state')
                if event['code'] not in codes:raise ValueError('Unsupported recorded DOM key')
                gdb.execute(f'call (void)dd2_key_event({codes[event["code"]]},{int(event["down"])})',to_string=True)
                cursor+=1;report['sent_events']=cursor
            if cursor<len(events) and events[cursor]['presentation']<index:
                raise ValueError('Recorded DOM input presentation was skipped')
            if index==len(frames)-1:break
            gdb.execute('continue',to_string=True)
            if int(gdb.parse_and_eval('$pc'))!=int(gdb.parse_and_eval('&PutDispEnv')):
                raise ValueError('Native engine stopped away from a presentation')
            index+=1
        if cursor!=len(events):raise ValueError('Incomplete recorded input extent')
        actual=state()
        if actual['clock_count']!=record['clock_calls'] or actual['rng_count']!=record['rng_calls']:
            raise ValueError('Final calculated API extent differs')
        report.update(pass_=True,final=actual)
    except Exception as error:
        save_failure(error)
    finally:
        (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        for point in points:
            if point.is_valid():point.delete()
    print('PASS' if report['pass_'] else 'FAIL','native recorded browser input/API diagnosis:',report['matched_frames'],'matched states',flush=True)
