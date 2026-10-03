"""Recover original Wine mixer sample positions from events, never PCM patterns.

Wine 10 DSOUND_PerformMix obtains one primary block and DSOUND_MixToPrimary
mixes sources into that block in creation order. The cumulative block sizes
give source starts independently of audio values. The scoped ALSA journal
separately records the consumed rate-probe prefix and the primary device epoch.
Source: https://raw.githubusercontent.com/wine-mirror/wine/wine-10.0/dlls/dsound/mixer.c
This is a trace replay timeline, not port/live input scheduling equivalence.
"""
import json
import re


def _events(path):
    return [json.loads(line) for line in path.read_text().splitlines()]


def original_menu_timeline(log, api, played, accepted, stream, accepted_stream):
    objects = {api['effect']['object']:dict(name='effect',frequency=5512,align=1,size=6314),
               api['cd']['object']:dict(name='cd',frequency=44100,align=4,size=91728)}
    devices,threads,blocks,source_calls,positions = {},{},{},{},{}
    target_device = None
    for line_number,line in enumerate(log.read_text(errors='replace').splitlines(),1):
        parsed = re.match(r'[^:]+:([0-9a-f]+):(trace|warn|err):dsound:(.*)',line)
        if not parsed:continue
        thread,level,body = parsed.groups()
        match = re.match(r'DSOUND_PrimaryOpen buflen: \d+, frames (\d+)',body)
        if match:
            device = threads.get(thread)
            if not device:raise ValueError('Primary capacity without device identity')
            devices[device]['capacity'] = int(match[1])
        match = re.match(r'DSOUND_(?:PrimaryOpen|PerformMix|mixthread) \(([0-9A-F]+)\)',body)
        if match:
            device = match[1]
            threads[thread] = device
            devices.setdefault(device,dict(frames=0,blocks=[],capacity=None))
        match = re.match(r'DSOUND_MixToPrimary \(frames (\d+)\)',body)
        if match:
            if thread not in threads:raise ValueError('Mixer block without device identity')
            device = devices[threads[thread]]
            frames = int(match[1])
            if not device['capacity'] or not 0<frames<=device['capacity']:
                raise ValueError('Invalid primary mixer block size')
            block = dict(offset_frames=device['frames'],frames=frames,line=line_number,
                         sources=[],cursors={})
            device['blocks'].append(block)
            blocks[thread] = block
            device['frames'] += frames
        match = re.match(r'DSOUND_MixOne \(([0-9A-F]+), frames=(\d+)\)',body)
        if match and match[1] in objects:
            obj,frames = match[1],int(match[2])
            device = threads[thread]
            if target_device is not None and target_device!=device:
                raise ValueError('Menu sources use different primary devices')
            target_device = device
            block = blocks[thread]
            if frames!=block['frames'] or objects[obj]['name'] in block['sources']:
                raise ValueError('Source extent differs from its primary block')
            if block['sources'] and objects[obj]['name']=='effect':
                raise ValueError('Original effect/CD creation order changed')
            block['sources'].append(objects[obj]['name'])
            source_calls[thread] = (obj,block)
        match = re.match(r'DSOUND_MixInBuffer sec_mixpos=(\d+)/(\d+)',body)
        if match:positions[thread] = (int(match[1]),int(match[2]))
        match = re.match(r'DSOUND_MixInBuffer \(([0-9A-F]+), frames=(\d+)\)',body)
        if match and match[1] in objects:
            obj,frames = match[1],int(match[2])
            if thread not in source_calls or source_calls[thread][0]!=obj or thread not in positions:
                raise ValueError('Source cursor without matching original mix call')
            block = source_calls[thread][1]
            if frames!=block['frames']:raise ValueError('Original source block was trimmed')
            block['cursors'][objects[obj]['name']] = positions.pop(thread)
            source_calls.pop(thread)
        if level=='err' and thread in threads and threads[thread]==target_device:
            raise ValueError('Original mixer/device error: '+body)
    if not target_device:raise ValueError('No original menu source blocks')
    device = devices[target_device]
    starts,source_frames,calls = {},dict(effect=0,cd=0),dict(effect=0,cd=0)
    for block in device['blocks']:
        for obj,info in objects.items():
            name = info['name']
            if name not in block['sources']:continue
            if name not in block['cursors']:raise ValueError('Missing original source cursor')
            cursor,size = block['cursors'][name]
            expected = source_frames[name]*info['frequency']//stream['rate']*info['align']
            if cursor!=expected%info['size'] or size!=info['size']:
                raise ValueError('Original source cursor/ring extent is discontinuous')
            if name in starts and block['offset_frames']!=starts[name]+source_frames[name]:
                raise ValueError('Original menu source stopped/restarted unexpectedly')
            starts.setdefault(name,block['offset_frames'])
            source_frames[name] += block['frames']
            calls[name] += 1
    if set(starts)!={'effect','cd'} or not starts['effect']<starts['cd']:
        raise ValueError('Missing/out-of-order original effect/CD starts')
    if source_frames['effect']*5512//stream['rate']<6314 or starts['cd']+source_frames['cd']!=device['frames']:
        raise ValueError('Incomplete original one-shot/CD mixing extent')

    # The initial 10-ms ALSA rate probe is consumed before the real primary
    # device resets to source pointer zero. Its length comes only from the
    # actual journal, not a fixed shift or a waveform match.
    writes = [event for event in _events(accepted) if event['event']=='write']
    journal = _events(played)
    probe = writes[0]['accepted'] if writes else 0
    segments = stream['segments']
    if not probe or probe!=writes[0]['requested'] or len(segments)!=2 or segments[0]['frames']!=probe:
        raise ValueError('Missing independently consumed device-probe prefix')
    if segments[1]['offset_frames']!=probe:
        raise ValueError('Unexpected device epoch boundary')
    xruns = [event for event in journal if event['event']=='xrun']
    if len(xruns)!=1 or xruns[0]['played_frames']!=probe:
        raise ValueError('Unexpected device underrun after the probe')
    # Some Wine runs use prepare in place of reset. All post-probe reset/drop/
    # rewind extents must still be at this independently consumed boundary.
    transport = [event for event in _events(accepted) if event['event'] in
                 ('snd_pcm_prepare','snd_pcm_reset','snd_pcm_drop','snd_pcm_rewind')]
    if not any(event['offset_frames']==probe for event in transport) or any(
            event['offset_frames'] not in (0,probe) or event['result']<0 for event in transport):
        raise ValueError('Discarded/rewound primary samples invalidate simple trace replay')
    primary_played = [event for event in journal if event['event']=='played' and event['offset_frames']>=probe]
    pointer = 0
    for event in primary_played:
        if event['source_begin']!=pointer or event['offset_frames']!=probe+pointer:
            raise ValueError('Consumed primary pointer is not the recorded FIFO prefix')
        pointer += event['frames']
        if event['source_end']!=pointer:raise ValueError('Unexpected primary pointer wrap')
    total = device['frames']+probe
    if not stream['played_frames']<=accepted_stream['accepted_frames']<=total:
        raise ValueError('Original consumed/accepted extent exceeds traced mixer output')
    queued = total-accepted_stream['accepted_frames']
    if queued>device['capacity']:
        raise ValueError('Unobserved original primary output exceeds queue capacity')
    def sample_ns(frame):
        relative = frame-probe
        if not 0<=relative<segments[1]['frames']:raise ValueError('Source start outside consumed timeline')
        return segments[1]['begin_ns']+(relative*1_000_000_000+stream['rate']-1)//stream['rate']
    start_frames = {name:frame+probe for name,frame in starts.items()}
    return dict(device=target_device,probe_frames=probe,primary_capacity_frames=device['capacity'],
                primary_mixed_frames=device['frames'],primary_queued_frames=queued,
                alsa_queued_frames=accepted_stream['accepted_frames']-stream['played_frames'],
                starts=start_frames,start_sample_ns={name:sample_ns(frame) for name,frame in start_frames.items()},
                source_mixed_frames=source_frames,source_mix_calls=calls,blocks=device['blocks'],
                derivation='Cumulative original DSOUND primary block counts + independently consumed ALSA probe; no PCM pattern search')
