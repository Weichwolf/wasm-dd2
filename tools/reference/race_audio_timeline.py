"""Read original DirectSound calls and CD sector writes in their traced order.

This supplies a mixer component replay, not original/port engine scheduling.
Source cursors and mixer gain observations remain assertions, never corrections.
"""
import copy
from decimal import Decimal
import hashlib
import re


def original_timeline(path):
    sources, live, pending, devices, threads = [], {}, {}, {}, {}
    operations, blocks, active, positions, packets, cd_locks = [], [], {}, {}, {}, {}
    controls, control_pending, control_calls = {}, {}, []

    def commit_control(thread,base):
        call=control_pending.pop(thread)
        call.update(committed_line=base['line'],committed_time_ns=base['time_ns'])
        controls[call['source']][call['operation']]=call['value']
        operations.append(call)

    def finish(thread):
        block = active.pop(thread, None)
        if block:
            if thread in packets or thread in positions:
                raise ValueError('Incomplete original source mix')
            operations.append(dict(operation='end', block=block['id']))

    for line_number, line in enumerate(path.read_text(errors='replace').splitlines(), 1):
        match = re.fullmatch(r'\[VCD READ\] lba=(\d+) sectors=(\d+) begin_ns=(\d+) end_ns=(\d+)', line)
        if match:
            if len(cd_locks) != 1:
                raise ValueError('CD sector read without one matching locked buffer')
            source, offset, length = next(iter(cd_locks.values()))
            lba, sectors, begin, end = map(int, match.groups())
            if sectors * 2352 != length or not 0 < begin <= end:
                raise ValueError('CD read and actual locked extent differ')
            operations.append(dict(operation='write', source=source['id'], offset=offset,
                                   length=length, lba=lba, sectors=sectors,
                                   begin_ns=begin, end_ns=end, line=line_number))
            continue
        parsed = re.match(r'([^:]+):([0-9a-f]+):(trace|warn|err):dsound:(.*)', line)
        if not parsed:
            continue
        timestamp, thread, level, body = parsed.groups()
        ns = int(Decimal(timestamp) * 1000000000)
        state = pending.setdefault(thread, {})
        base = dict(line=line_number, time_ns=ns)
        match = re.match(r'DirectSoundDevice_CreateSoundBuffer \(([0-9A-F]+),[0-9A-F]+,[0-9A-F]+,[0-9A-F]+\)', body)
        if match:
            state.clear(); state['device'] = match[1]
        for name, pattern in [('bytes', r'\(bufferbytes=(\d+)\)'), ('flags', r'\(flags=0x([0-9a-f]+):')]:
            match = re.match(r'DirectSoundDevice_CreateSoundBuffer ' + pattern, body)
            if match:
                state[name] = int(match[1], 16 if name == 'flags' else 10)
        match = re.match(r'DirectSoundDevice_CreateSoundBuffer \(formattag=0x([0-9a-f]+),chans=(\d+),samplerate=(\d+),bytespersec=(\d+),blockalign=(\d+),bitspersamp=(\d+)', body)
        if match:
            state['format'] = dict(zip(('tag','channels','rate','bytes_per_second','align','bits'),
                                      [int(match[1],16), *map(int,match.groups()[1:])]))
        match = re.match(r'secondarybuffer_create Created buffer at ([0-9A-F]+)', body)
        if match:
            obj = match[1]
            if obj in live or not all(k in state for k in ('bytes','flags','format','device')):
                raise ValueError('Missing buffer creation or reused live object')
            fmt = state['format']
            if fmt['tag'] != 1 or fmt['align'] != fmt['channels'] * fmt['bits']//8 or not state['bytes']:
                raise ValueError('Unsupported original source format')
            source = dict(id=len(sources), object=obj, bytes=state['bytes'], flags=state['flags'],
                          format=fmt, device=state['device'], created_line=line_number)
            sources.append(source); live[obj]=source
            controls[source['id']]=dict(SetCurrentPosition=0,SetPan=0,SetVolume=0,SetFrequency=fmt['rate'])
            operations.append(dict(operation='create',source=source['id'],**base))
            state.clear()
        match = re.match(r'DirectSoundDevice_DuplicateSoundBuffer \(([0-9A-F]+),([0-9A-F]+),[0-9A-F]+\)', body)
        if match:
            state['duplicate_of'] = live[match[2]]
        match = re.match(r'DirectSoundDevice_AddBuffer \(([0-9A-F]+), ([0-9A-F]+)\)', body)
        if match and 'duplicate_of' in state:
            obj = match[2]; parent = state.pop('duplicate_of')
            if obj in live:raise ValueError('Duplicate reuses a live object')
            source=copy.deepcopy(parent)
            source.update(id=len(sources),object=obj,duplicate_of=parent['id'],created_line=line_number)
            sources.append(source); live[obj]=source
            controls[source['id']]=copy.deepcopy(controls[parent['id']])
            operations.append(dict(operation='duplicate',source=source['id'],parent=parent['id'],**base))
        match = re.match(r'secondarybuffer_destroy \(([0-9A-F]+)\) released', body)
        if match:
            source=live.pop(match[1])
            operations.append(dict(operation='release',source=source['id'],**base))
        match = re.match(r'IDirectSoundBufferImpl_(SetCurrentPosition|SetPan|SetVolume|SetFrequency) \(([0-9A-F]+),(-?\d+)\)', body)
        if match:
            call=dict(operation=match[1],source=live[match[2]]['id'],value=int(match[3]),**base)
            control_calls.append(call)
            if match[1]=='SetCurrentPosition' or controls[call['source']][match[1]]==call['value']:
                operations.append(call)
            else:
                # Entry TRACE precedes the source lock. A concurrent mix may
                # still use the previous gain/frequency until Recalc runs.
                if thread in control_pending:raise ValueError('Missing original control completion')
                control_pending[thread]=call
        match = re.match(r'DSOUND_RecalcVolPan Vol=(-?\d+) Pan=(-?\d+)',body)
        if match and thread in control_pending:
            call=control_pending[thread]
            if call['operation'] not in ('SetVolume','SetPan'):raise ValueError('Original gain completion differs')
            expected=dict(controls[call['source']]);expected[call['operation']]=call['value']
            if tuple(map(int,match.groups()))!=(expected['SetVolume'],expected['SetPan']):
                raise ValueError('Original committed gain fields differ')
            commit_control(thread,base)
        match = re.match(r'DSOUND_RecalcFormat \(([0-9A-F]+)\)',body)
        if match and thread in control_pending:
            call=control_pending[thread]
            if call['operation']!='SetFrequency' or live[match[1]]['id']!=call['source']:
                raise ValueError('Original frequency completion differs')
            commit_control(thread,base)
        match = re.match(r'IDirectSoundBufferImpl_Play \(([0-9A-F]+),00000000,00000000,([0-9A-F]+)\)', body)
        if match:
            operations.append(dict(operation='Play',source=live[match[1]]['id'],flags=int(match[2],16),**base))
        match = re.match(r'IDirectSoundBufferImpl_Stop \(([0-9A-F]+)\)', body)
        if match:
            operations.append(dict(operation='Stop',source=live[match[1]]['id'],**base))
        match = re.match(r'IDirectSoundBufferImpl_Lock \(([0-9A-F]+),(\d+),(\d+),[0-9A-F]+,[0-9A-F]+,[0-9A-F]+,[0-9A-F]+,0x([0-9a-f]+)\)',body)
        if match and match[1] in live:
            source=live[match[1]]
            if source['format']['channels']==2:
                offset,length,flags=int(match[2]),int(match[3]),int(match[4],16)
                if flags==2 and length==0:offset,length=0,source['bytes']
                if offset+length>source['bytes']:raise ValueError('Unexpected wrapped CD write')
                cd_locks[thread]=(source,offset,length)
        match = re.match(r'IDirectSoundBufferImpl_Unlock \(([0-9A-F]+),',body)
        if match and thread in cd_locks:
            source,_,_=cd_locks.pop(thread)
            if source['object']!=match[1]:raise ValueError('CD unlock object differs')
        match = re.match(r'DSOUND_(?:PrimaryOpen|PerformMix|mixthread) \(([0-9A-F]+)\)', body)
        if match:
            if body.startswith('DSOUND_PerformMix'):finish(thread)
            threads[thread]=match[1]
            devices.setdefault(match[1],dict(frames=0,capacity=None))
        match = re.match(r'DSOUND_PrimaryOpen buflen: \d+, frames (\d+)',body)
        if match:devices[threads[thread]]['capacity']=int(match[1])
        match = re.match(r'DSOUND_MixToPrimary \(frames (\d+)\)',body)
        if match:
            finish(thread)
            device=threads[thread]; frames=int(match[1]);info=devices[device]
            if not info['capacity'] or not 0<frames<=info['capacity']:raise ValueError('Invalid original primary extent')
            block=dict(id=len(blocks),device=device,offset=info['frames'],frames=frames,sources=[],**base)
            blocks.append(block); active[thread]=block;info['frames']+=frames
            operations.append(dict(operation='begin',block=block['id'],**base))
        match = re.match(r'DSOUND_MixOne \(([0-9A-F]+), frames=(\d+)\)',body)
        if match:
            source=live[match[1]];block=active[thread]
            if source['device']!=block['device'] or int(match[2])!=block['frames'] or thread in packets:
                raise ValueError('Original mix source/block identity differs')
            packets[thread]=dict(operation='mix',source=source['id'],block=block['id'],**base)
        match = re.match(r'DSOUND_MixOne looping=(\d+), leadin=(\d+)',body)
        if match:packets[thread]['looping']=int(match[1])
        match = re.match(r'DSOUND_MixInBuffer sec_mixpos=(\d+)/(\d+)',body)
        if match:positions[thread]=tuple(map(int,match.groups()))
        match = re.match(r'DSOUND_MixInBuffer \(([0-9A-F]+), frames=(\d+)\)',body)
        if match:
            packet=packets[thread];cursor,size=positions.pop(thread);source=live[match[1]]
            if packet['source']!=source['id'] or size!=source['bytes'] or not 0<=cursor<size or cursor%source['format']['align']:
                raise ValueError('Original source cursor/extent differs')
            if int(match[2])!=active[thread]['frames']:raise ValueError('Original mix trimmed its primary extent')
            packet.update(cursor=cursor,frames=int(match[2]),size=size)
        match = re.match(r'DSOUND_MixerVol left = ([0-9a-f]+), right = ([0-9a-f]+)',body)
        if match:packets[thread]['gains']=list(int(value,16) for value in match.groups())
        match = re.match(r'DSOUND_MixOne total mixed data=(\d+)',body)
        if match:
            packet=packets.pop(thread)
            if packet.get('frames')!=int(match[1]) or 'looping' not in packet:raise ValueError('Incomplete original source mix packet')
            # No MixerVol is called for muted sources; preserve explicit absence.
            packet.setdefault('gains',None)
            operations.append(packet);active[thread]['sources'].append(packet['source'])
        if level=='err':raise ValueError('Original DirectSound error: '+body)
    for thread in list(active):finish(thread)
    if control_pending:raise ValueError('Incomplete original control completion')
    target={source['device'] for source in sources}
    if len(target)!=1:raise ValueError('Expected one original effects/CD primary device')
    target=target.pop()
    return dict(scope=__doc__.strip(),trace_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                sources=sources,operations=operations,control_calls=control_calls,
                blocks=blocks,devices=devices,target_device=target)
