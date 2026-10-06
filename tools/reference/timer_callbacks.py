"""Validate observed Wine timer callbacks and read-only original fire counters.

Records preserve actual registrations, callback function/user identities and
QPC/GetTickCount observations. Inline markers locate callback begin/end among
original sound/mixer trace events. No return, engine state or PCM is supplied
to the original. Logging changes timing; port comparison remains separate.
"""
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct

from game_clock import verified_sites,EXE
if __package__:
    from .trace_log import read_trace
else:
    from trace_log import read_trace

ROW=struct.Struct('<12I2Q2I')
MARKER=re.compile(r'DD2_TIMER record=(\d+) type=(\d+) id=(\d+)')


def read(journal,*,allow_terminal=False):
    raw=Path(journal).read_bytes()
    if not raw or len(raw)%ROW.size or len(raw)>ROW.size*1000000:
        raise ValueError('Incomplete or unbounded callback journal')
    records=[];pending={};timers={};callbacks={};completed=[]
    previous=-1;frequency=None;tick_offsets=[]
    context_keys=('period','resolution','flags','callback','user')
    for number,values in enumerate(ROW.iter_unpack(raw),1):
        magic,serial,kind,id,period,resolution,flags,callback,user,thread,caller,result,qpc,freq,ticks,reserved=values
        if magic!=0x32544d44 or serial!=number or reserved or kind not in range(1,7) or not freq or not thread:
            raise ValueError('Malformed callback record header')
        if kind in (3,4) and (not id or caller or result):
            raise ValueError('Malformed callback identity')
        ns=qpc*1000000000//freq
        if ns<previous or frequency is not None and freq!=frequency:
            raise ValueError('Callback clock changed or moved backwards')
        # GetTickCount is a wrapping, coarser observation taken separately
        # from QPC. A slightly earlier cached tick must not become a 2**32
        # millisecond difference through unsigned subtraction.
        tick_offset=(ticks-qpc*1000//freq+0x80000000)%0x100000000-0x80000000
        if abs(tick_offset)>2000:
            raise ValueError('Callback QPC/tick domains differ')
        tick_offsets.append(tick_offset)
        previous=ns;frequency=freq
        record=dict(serial=serial,type=kind,id=id,period=period,resolution=resolution,flags=flags,
                    callback=callback,user=user,thread=thread,caller=caller,result=result,
                    qpc=qpc,frequency=freq,ticks=ticks,time_ns=ns)
        records.append(record)
        if kind in (1,5):
            if thread in pending:raise ValueError('Nested timer API entry')
            pending[thread]=record
            if kind==1 and (id or result):raise ValueError('Registration entry already has a result')
        elif kind in (2,6):
            entry=pending.pop(thread,None)
            if entry is None or entry['type']!=kind-1 or entry['caller']!=caller:
                raise ValueError('Timer return has no matching entry')
            if kind==2:
                if any(entry[key]!=record[key] for key in context_keys) or result!=id:
                    raise ValueError('Registration return changed callback arguments')
                if id:
                    if id in timers and not timers[id].get('cancelled'):
                        raise ValueError('Reused active timer ID')
                    timers[id]=dict(entry,return_record=record)
            else:
                if entry['id']!=id:raise ValueError('Cancellation ID changed')
                if not result and id in timers:timers[id]['cancelled']=record
        elif kind==3:
            if thread in callbacks:raise ValueError('Nested callback on one timer thread')
            timer=timers.get(id)
            if timer is None:
                # A very short real timer can fire before timeSetEvent returns.
                candidates=[p for p in pending.values() if p['type']==1 and all(p[k]==record[k] for k in context_keys)]
                if len(candidates)!=1:raise ValueError('Callback has no actual registration')
                timer=candidates[0]
            if any(timer[key]!=record[key] for key in context_keys):
                raise ValueError('Callback function/user/period differs from registration')
            if timer.get('cancelled') and flags&0x100:
                raise ValueError('Callback after synchronous cancellation')
            callbacks[thread]=record
        else:
            begin=callbacks.pop(thread,None)
            if begin is None or begin['id']!=id or any(begin[k]!=record[k] for k in context_keys):
                raise ValueError('Callback end has no matching begin')
            completed.append(dict(id=id,thread=thread,begin_serial=begin['serial'],end_serial=serial,
                                  begin_ns=begin['time_ns'],end_ns=ns))
    if pending:raise ValueError('Unreturned observed timer API call')
    if callbacks and (not allow_terminal or len(callbacks)!=1):
        raise ValueError('Unreturned observed callback')
    if not completed:raise ValueError('No complete original callbacks observed')
    return dict(scope=__doc__.strip(),records=records,completed_callbacks=completed,
                incomplete_callbacks=list(callbacks.values()),journal_sha256=hashlib.sha256(raw).hexdigest(),
                record_bytes=ROW.size,registrations=[r for r in records if r['type']==2],
                tick_offset_ms=dict(min=min(tick_offsets),max=max(tick_offsets)),
                counts=dict(Counter(r['type'] for r in records)))


def original_report(capture,executable,*,allow_terminal=False):
    verified_sites(executable)
    report=read(capture/'timer-callbacks.bin',allow_terminal=allow_terminal)
    registrations=report['registrations']
    if len(registrations)!=2 or [r['caller'] for r in registrations]!=[0x4133d8,0x412a9d]:
        raise ValueError('Original startup registration sites differ')
    if [r['id'] for r in registrations]!=[16,33] or any(
            (r['period'],r['resolution'],r['flags'],r['callback'],r['user'])!=(400,10,1,0x41345c,0) for r in registrations):
        raise ValueError('Original timer registration identity differs')
    expected={r['serial']:r for r in report['records']};seen={};trace_sha=hashlib.sha256()
    with read_trace(capture/'wine.log') as lines:
        for number,raw in enumerate(lines,1):
            trace_sha.update(raw)
            if b'DD2_TIMER record=' not in raw:continue
            match=MARKER.search(raw.decode(errors='replace'))
            if not match:raise ValueError('Malformed inline callback marker')
            serial,kind,id=map(int,match.groups())
            if serial not in expected or serial in seen or (kind,id)!=(expected[serial]['type'],expected[serial]['id']):
                raise ValueError('Duplicate or mismatched inline callback marker')
            if serial!=len(seen)+1:raise ValueError('Inline callback marker order differs')
            seen[serial]=number
    if set(seen)!=set(expected):raise ValueError('Missing inline callback marker')
    for serial,record in expected.items():record['trace_line']=seen[serial]
    starts=[r['time_ns'] for r in report['records'] if r['type']==3]
    ends=[r['time_ns'] for r in report['records'] if r['type']==4]
    counter_checks=[]
    with (capture/'audio/engine.jsonl').open() as lines:
        for line in lines:
            observation=json.loads(line)
            if 'timer' not in observation:raise ValueError('Original fire counter observation missing')
            # Linux Wine QPC uses MONOTONIC_RAW. Keep the existing MONOTONIC
            # audio observations separate; never fit their offset to counters.
            if observation.get('qpc_clock')!='CLOCK_MONOTONIC_RAW':
                raise ValueError('Original counter observation has no QPC clock domain')
            before,after=observation['read_begin_raw_ns'],observation['read_end_raw_ns']
            if before>after:raise ValueError('Reversed original counter read interval')
            lower=sum(ns<=before for ns in ends);upper=sum(ns<=after for ns in starts)
            actual=observation['timer']['fires']
            if not lower<=actual<=upper:
                raise ValueError('Original callback counter does not match observed executions')
            counter_checks.append(dict(read_begin_raw_ns=before,read_end_raw_ns=after,lower=lower,upper=upper,actual=actual))
    if not counter_checks or counter_checks[-1]['actual']<10:
        raise ValueError('Insufficient original callback counter observations')
    report.update(pass_=True,original_exe_sha256=EXE,trace_sha256=trace_sha.hexdigest(),
                  counter_checks=counter_checks,port_comparison='pending; observed original callbacks only')
    return report
