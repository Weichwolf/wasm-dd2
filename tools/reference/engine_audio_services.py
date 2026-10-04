#!/usr/bin/env python3
"""Export observed audio service order for a real engine comparison.

Only actual source mix steps and timer executions drive services. API arguments,
statuses, cursors and gains are assertions of the port's own calculations.
No recorded control is applied to a port buffer. This is comparison input,
not a full-game parity result. The input retains original trace provenance.
"""
import argparse
import hashlib
import json
import mmap
from pathlib import Path
import re
import struct
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from artifacts import WORK,check_space,prepare_output

ROW=struct.Struct('<12I')
KINDS={'create':1,'duplicate':2,'release':3,'SetCurrentPosition':4,'SetPan':5,
       'SetVolume':6,'SetFrequency':7,'Play':8,'Stop':9,'GetStatus':10}


def export(capture,mixer,output):
    output=prepare_output(output)
    if WORK not in output.parents or output.exists():raise ValueError('Fresh output under /tmp/wasm-dd2/ required')
    output.mkdir(parents=True)
    timeline=json.loads((mixer/'timeline.json').read_text())
    proof=json.loads((mixer/'report.json').read_text())
    callbacks=json.loads((capture/'timer-callbacks.json').read_text())
    clock=json.loads((capture/'game-clock/report.json').read_text())
    if not all(d.get('pass_') for d in (proof,callbacks,clock)):
        raise ValueError('Verified original mixer, callbacks and clock required')
    sha=timeline['trace_sha256']
    if not all(d['trace_sha256']==sha for d in (proof,callbacks,clock)):
        raise ValueError('Service inputs describe different original traces')
    for target in ('native','native-asan','wasm'):
        if not proof['targets'][target]['pass_']:raise ValueError('Incomplete mixer component proof')
    markers={r['trace_line']:r for r in callbacks['records']}
    registrations={r['caller']:r for r in callbacks['registrations']}
    starts={op['line']:op for op in timeline['operations'] if 'line' in op and op['operation']!='write'}
    # A delayed control remains an entry gate followed by its actual commit.
    commits={op['committed_line']:op for op in timeline['control_calls'] if 'committed_line' in op}
    blocks={b['line']:b for b in timeline['blocks'] if b['device']==timeline['target_device']}
    events=[];active_callbacks={};live={};mixing={};status={};block=None
    flip=0;last_block_line=None;last_block_thread=None
    event_flip={}
    def add(line,kind,sid=0,args=(),context=0,priority=1):
        events.append(dict(line=line,kind=kind,source=sid,args=list(args),context=context,priority=priority))
    def end_block():
        nonlocal block,last_block_line
        if block is not None:
            add(last_block_line,22,args=(block['id'],),priority=3)
            block=None;last_block_line=None
    def source(sid):return timeline['sources'][sid]
    def music(sid):return source(sid)['format']['channels']==2
    with (capture/'wine.log').open('rb') as lines:
        trace_digest=hashlib.sha256()
        for number,raw in enumerate(lines,1):
            trace_digest.update(raw)
            if number in markers:
                r=markers[number];thread=r['thread']
                if r['type']==1:
                    reg=registrations[r['caller']]
                    add(number,11,reg['id'],(r['period'],r['resolution'],r['flags'],r['user'],r['callback']))
                elif r['type']==3:
                    if thread in active_callbacks:raise ValueError('Nested original callback')
                    active_callbacks[thread]=r['id'];add(number,30,r['id'])
                elif r['type']==4:
                    if active_callbacks.pop(thread,None)!=r['id']:raise ValueError('Original callback end differs')
                    add(number,31,r['id'],context=r['id'])
                elif r['type']==5:
                    returned=next(x for x in callbacks['records'] if x['type']==6 and x['serial']>r['serial'] and x['thread']==thread)
                    add(number,12,r['id'],(returned['result'],))
            op=starts.get(number)
            thread_match=re.match(rb'[^:]+:([0-9a-f]+):',raw)
            thread=int(thread_match[1],16) if thread_match else None
            context=active_callbacks.get(thread,0)
            if op:
                kind=op['operation'];sid=op.get('source')
                if kind in ('create','duplicate'):
                    obj=source(sid)['object'];live[obj]=sid
                    if kind=='create':
                        s=source(sid);fmt=s['format']
                        add(number,1,sid,(s['bytes'],s['flags'],fmt['rate'],fmt['channels'],fmt['bits'],fmt['align']),context)
                    else:add(number,2,sid,(op['parent'],),context)
                elif kind=='release':
                    # Wine logs Release's zero result after destruction, when
                    # the object is already absent from the live table.
                    add(number,3,sid,(0,),context)
                    live.pop(source(sid)['object'])
                elif kind in KINDS and (not music(sid) or kind in ('Play','Stop')):
                    add(number,KINDS[kind],sid,(op.get('value',op.get('flags',0)),),context)
                elif kind=='mix':mixing[thread]=op
            if number in commits:
                op=commits[number]
                if not music(op['source']):add(number,32,op['source'],(KINDS[op['operation']],op['value']),context,2)
            if b':dsound:IDirectSoundBufferImpl_GetStatus (' in raw:
                m=re.search(rb'GetStatus \(([0-9A-F]+),',raw);obj=m[1].decode()
                if obj in live and not music(live[obj]) and (thread==int(clock['engine_thread'],16) or context):
                    status[thread]=dict(source=live[obj],line=number,context=context)
            elif b':dsound:IDirectSoundBufferImpl_GetStatus status=' in raw and thread in status:
                call=status.pop(thread);value=int(raw.split(b'status=',1)[1].strip())
                add(call['line'],10,call['source'],(value,),call['context'])
            if b':dsound:IDirectSoundBufferImpl_Release (' in raw:
                m=re.search(rb'Release \(([0-9A-F]+)\) ref (\d+)',raw)
                if m and m[1].decode() in live:
                    sid=live[m[1].decode()]
                    if int(m[2]) and not music(sid) and (thread==int(clock['engine_thread'],16) or context):
                        add(number,3,sid,(int(m[2]),),context)
            if b':dsound:DSOUND_PerformMix (' in raw and thread==last_block_thread:end_block()
            if number in blocks:
                end_block();block=blocks[number];last_block_line=number;last_block_thread=thread
                add(number,20,args=(block['frames'],block['id']))
            if block is not None and thread==last_block_thread and b':dsound:DSOUND_MixToPrimary MixToPrimary for ' in raw:
                last_block_line=number
            if b':dsound:DSOUND_MixOne total mixed data=' in raw and thread in mixing:
                packet=mixing.pop(thread)
                if timeline['blocks'][packet['block']]['device']==timeline['target_device']:
                    gains=packet['gains'] or [0xffffffff,0xffffffff]
                    add(number,21,packet['source'],(packet['cursor'],packet['looping'],*gains,packet['frames'],packet['size']))
                    last_block_line=number
            # Retain original presentation counts before each literal event.
            if number in starts or number in commits or number in markers or b':dsound:' in raw:
                event_flip[number]=flip
            if b'ddraw_surface1_Flip iface' in raw:flip+=1
        end_block()
    if trace_digest.hexdigest()!=sha or mixing or status or active_callbacks:
        raise ValueError('Incomplete or changed original service trace')
    events.sort(key=lambda e:(e['line'],e['priority']))
    with (capture/'game-clock/observations.bin').open('rb') as f:
        with mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as mem:
            if len(mem)!=16+clock['calls']*32 or mem[:16]!=struct.pack('<8sII',b'DD2GC01\0',1,32):
                raise ValueError('Incomplete original clock observations')
            call_index=0
            for e in events:
                while call_index<clock['calls'] and struct.unpack_from('<I',mem,16+call_index*32+20)[0]<e['line']:call_index+=1
                e['clock_calls']=call_index;e['flip']=event_flip[e['line']]
                if e['flip'] is None:raise ValueError('Missing original event presentation position')
    # Preserve both callers in the literal order. The production service
    # scheduler keeps the callback continuation while main APIs execute.
    # Reject malformed contexts and overlapping timer callbacks; neither a
    # recorded status nor an API value substitutes for either caller's work.
    inside=None;interleaved=[]
    for e in events:
        if e['kind']==30:
            if inside or e['context'] or not e['source']:
                raise ValueError('Overlapping or malformed original callback begin')
            inside=e['source']
        elif e['kind']==31:
            if not inside or e['source']!=inside or e['context']!=inside:
                raise ValueError('Original callback end/context differs')
            inside=None
        else:
            if e['context'] and e['context']!=inside:
                raise ValueError('Original API outside its callback context')
            if inside and (1<=e['kind']<=12 or e['kind']==32) and not e['context']:
                interleaved.append(dict(line=e['line'],kind=e['kind'],source=e['source'],callback=inside))
    if inside:raise ValueError('Unreturned original callback')
    device=proof['original_device'];accepted=device['accepted_frames']
    header=struct.pack('<8s8I',b'DD2AS01\0',44100,len(events),2,proof['probe_frames'],accepted&0xffffffff,accepted>>32,clock['calls'],flip)
    body=b''.join(ROW.pack(e['kind'],e['source'],*(list(x&0xffffffff for x in e['args'])+[0]*6)[:6],e['flip'],e['clock_calls'],e['context'],e['line']) for e in events)
    raw=header+body;(output/'services.bin').write_bytes(raw)
    report=dict(scope=__doc__.strip(),input_sha256=hashlib.sha256(raw).hexdigest(),trace_sha256=sha,
                original_exe_sha256=proof['original_exe_sha256'],mixer_component_report=str(mixer/'report.json'),
                clock_sha256=clock['ticks_sha256'],events=len(events),sources=len(timeline['sources']),
                frames=accepted,probe_frames=proof['probe_frames'],callbacks=sum(e['kind']==30 for e in events),
                interleaved_main_apis=interleaved,
                assertions='API arguments/status/cursors/gains are checked; never applied from the input',
                port_comparison='pending')
    report['completion_position']=dict(flip=events[-1]['flip'],clock_calls=events[-1]['clock_calls'])
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    (output/'events.json').write_text(json.dumps(events,indent=2)+'\n');check_space(output)
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True);parser.add_argument('--mixer',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    print(json.dumps(export(args.capture,args.mixer,args.output),indent=2))
