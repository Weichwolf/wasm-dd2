#!/usr/bin/env python3
"""Compare bounded original menu/racing PCM with chronological production mixing.

Original source creation, duplication, seeks, gains, changing frequencies,
CD sector/ring writes and primary block extents determine the replay. Cursors
and per-source gains are checked independently, without waveform alignment or
cursor/phase injection. This component proof does not establish identical-input
engine scheduling, browser audio sinks, physical timing or full game A/V parity.
"""
import argparse
import copy
import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import sys
import wave

from artifacts import WORK, check_space, open_files, prepare_output, run_bounded

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference'))
from audio import summarize_audio
from capture import EXE_SHA256
from race_audio_timeline import original_timeline


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def bank_sources(game):
    archive=(game/'Dirinfo').read_bytes()
    if digest(archive)!='03c6ca7adc5e616a4784a82f3b1b7a1f85489e97318b7e01f867e65504d5f22b':
        raise ValueError('Supported unmodified original Dirinfo required')
    rows=[archive[i:i+24] for i in range(0,0x2808,24)]
    row=next(row for row in rows if row[:18].split(b'\0')[0]==b'VAGS\\BANK1.SBK')
    sector,size=struct.unpack_from('<HI',row,18)
    bank=archive[sector*2048:sector*2048+size]
    if struct.unpack_from('<I',bank,12)[0]!=45:raise ValueError('Actual 45-source bank required')
    sources=[]
    for i in range(45):
        offset=struct.unpack_from('<I',bank,16+i*28)[0]
        riff=bank[offset:offset+struct.unpack_from('<I',bank,offset+4)[0]+8]
        with wave.open(io.BytesIO(riff)) as wav:
            sources.append(dict(channels=wav.getnchannels(),bits=wav.getsampwidth()*8,
                                rate=wav.getframerate(),raw=wav.readframes(wav.getnframes())))
    return sources


def encode(timeline,game):
    raws=bank_sources(game)
    disc=json.loads((game/'Redbook/disc.json').read_text())
    body=bytearray();count=mono=0;assets={}
    def word(*args):return struct.pack('<'+'I'*len(args),*[n&0xffffffff for n in args])
    for op in timeline['operations']:
        kind=op['operation'];sid=op.get('source')
        if kind in ('begin','end'):
            block=timeline['blocks'][op['block']]
            if block['device']!=timeline['target_device']:continue
        if kind=='create':
            source=timeline['sources'][sid];fmt=source['format'];raw=b''
            if fmt['channels']==1:
                wave_source=raws[mono%45];mono+=1;raw=wave_source['raw']
                if any(wave_source[k]!=fmt[k] for k in ('channels','bits','rate')) or len(raw)!=source['bytes']:
                    raise ValueError('Original sound bank creation order/format differs')
            elif (fmt['channels'],fmt['bits'],fmt['rate'],source['bytes'])!=(2,16,44100,91728):
                raise ValueError('Unsupported actual CD ring format')
            body+=word(1,sid,source['bytes'],source['flags'],fmt['rate'],fmt['channels'],fmt['bits'],fmt['align'],len(raw))+raw
        elif kind=='duplicate':body+=word(2,sid,op['parent'])
        elif kind=='release':body+=word(3,sid)
        elif kind.startswith('Set'):
            body+=word(4,sid,['SetCurrentPosition','SetPan','SetVolume','SetFrequency'].index(kind),op['value'])
        elif kind=='Play':body+=word(5,sid,op['flags'])
        elif kind=='Stop':body+=word(6,sid)
        elif kind=='write':
            remaining=op['sectors'];at=op['lba'];raw=bytearray()
            while remaining:
                track=next(t for t in disc['tracks'][1:] if t['start_sector']<=at<t['end_sector'])
                name=track['file']
                if name not in assets:
                    data=(game/'Redbook'/name).read_bytes()
                    if digest(data)!=track['sha256'] or len(data)!=(track['end_sector']-track['start_sector'])*2352:
                        raise ValueError('Actual provisioned CDDA sectors differ')
                    assets[name]=data
                length=min(remaining,track['end_sector']-at)
                offset=(at-track['start_sector'])*2352
                raw+=assets[name][offset:offset+length*2352];at+=length;remaining-=length
            if len(raw)!=op['length']:raise ValueError('Original CD write and read length differ')
            body+=word(7,sid,op['offset'],len(raw))+raw
        elif kind=='begin':body+=word(8,block['frames'])
        elif kind=='mix':body+=word(9,sid,op['cursor'],op['looping'],*(op['gains'] if op['gains'] is not None else [0xffffffff,0xffffffff]))
        elif kind=='end':body+=word(10)
        else:raise ValueError('Unknown original operation: '+kind)
        count+=1
    if mono<90 or mono%45:raise ValueError('Complete original sound bank generations required')
    return b'DD2MX01\0'+word(count)+body,count,{name:digest(raw) for name,raw in assets.items()}


def compare_pcm(original,actual):
    if len(original)!=len(actual):raise ValueError('Incomplete chronological racing PCM')
    if original!=actual:
        first=next(i for i,(x,y) in enumerate(zip(original,actual)) if x!=y)
        raise ValueError(f'Chronological racing PCM differs at byte {first}, frame {first//8}')


def validate_capture(cap, replay_fixture=None):
    checkpoint=json.loads((cap/'checkpoint.json').read_text())
    if checkpoint.get('exe_modified') is not False or checkpoint.get('exe_sha256')!=EXE_SHA256 or checkpoint.get('mode')!='audio':
        raise ValueError('Actual unmodified original non-debugger audio capture required')
    if replay_fixture is None:
        if checkpoint.get('scenario')=='original-replay':
            raise ValueError('Original replay audio requires its original-produced fixture')
        if checkpoint['end_state']['level']!=9 or checkpoint['end_state']['cf']<60:
            raise ValueError('Actual attract race with changing engine controls required')
        return checkpoint
    from verify_original_replay import fixture
    initial,payload,producer=fixture(replay_fixture)
    proof=json.loads((cap/'report.json').read_text())
    keys=['Right','Right','Right','Return','Return','Return']
    if (checkpoint.get('scenario')!='original-replay' or not checkpoint.get('complete_original_replay') or
        checkpoint.get('engine_state_writes') is not False or checkpoint.get('debugger') is not False or
        checkpoint.get('initial_save_sha256')!=digest(initial) or checkpoint.get('input_keys')!=keys or
        not proof.get('pass_') or proof.get('engine_state_writes') is not False or proof.get('debugger') is not False or
        proof.get('original_exe_sha256')!=EXE_SHA256 or proof.get('initial_save_sha256')!=digest(initial) or
        proof.get('input_keys')!=keys or proof.get('completion')!=dict(script_cursor=producer['recorded']['end'],first_time=1) or
        proof.get('restored')!=proof.get('start_state') or checkpoint['end_state']['level']!=0 or checkpoint['end_state']['movie']):
        raise ValueError('Complete actual original replay and frontend restoration required')
    rows=[json.loads(line) for line in (cap/'replay.jsonl').read_text().splitlines()]
    if (len(rows)!=proof.get('observed_replay_samples') or len({r['tick'] for r in rows})<10 or
        not any(r['pedal']>0 for r in rows)):
        raise ValueError('Actual recorded replay acceleration/physics required')
    for row in rows:
        if (any(row[k]!=v for k,v in producer['recorded'].items()) or
            row['actual_level']!=producer['recorded']['level'] or row['replay']!=1 or row['quit']!=0 or
            row['countdown']>=1 or not 0x9376ae<=row['script_cursor']<=producer['recorded']['end'] or row['script_cursor']%2):
            raise ValueError('Original replay metadata/tape decoding differs')
    return checkpoint


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--replay-fixture',type=Path,help='validate a complete real replay instead of the default attract scenario')
    args=parser.parse_args();cap=args.capture.resolve();out=prepare_output(args.output)
    if WORK not in cap.parents or WORK not in out.parents or out.exists():parser.error('Fresh output and original capture must be under /tmp/wasm-dd2/')
    out.mkdir(parents=True);check_space(out);game=ROOT/'DestructionDerby2'
    checkpoint=validate_capture(cap,args.replay_fixture)
    timeline=original_timeline(cap/'wine.log')
    duplicated={source['id'] for source in timeline['sources'] if 'duplicate_of' in source}
    if not duplicated or not any(op['operation']=='mix' and op['source'] in duplicated and op['cursor']>0 for op in timeline['operations']):
        raise ValueError('Capture must exercise a naturally played racing duplicate past its first sample')
    (out/'timeline.json').write_text(json.dumps(timeline,indent=2)+'\n')
    encoded,commands,assets=encode(timeline,game);(out/'input.bin').write_bytes(encoded)
    audio=summarize_audio(cap/'audio',write=False,require_played=True)
    if len(audio['streams'])!=2 or len(audio['played_streams'])!=2:raise ValueError('Actual intro and shared menu/race streams required')
    accepted,played=audio['streams'][-1],audio['played_streams'][-1]
    if (accepted['format'],accepted['rate'],accepted['channels'])!=('FLOAT_LE',44100,2):raise ValueError('Actual stereo Float32 device required')
    original=(cap/'audio'/accepted['file']).read_bytes();consumed=(cap/'audio'/played['file']).read_bytes()
    if original[:len(consumed)]!=consumed:raise ValueError('Consumed PCM differs from actual accepted FIFO prefix')
    writes=[json.loads(line) for line in (cap/'audio'/accepted['events']).read_text().splitlines()]
    first=next(row for row in writes if row['event']=='write');probe=first['accepted']
    if probe!=first['requested'] or played['segments'][0]['frames']!=probe or played['segments'][0]['offset_frames']:
        raise ValueError('Actual independently consumed rate-probe extent differs')
    transport=[row for row in writes if row['event'] in ('snd_pcm_prepare','snd_pcm_reset','snd_pcm_drop','snd_pcm_rewind')]
    journal=[json.loads(line) for line in (cap/'audio'/played['events']).read_text().splitlines()]
    if not any(row['offset_frames']==probe for row in transport):raise ValueError('Missing recorded rate-probe boundary')
    for row in transport:
        prior=[event for event in journal if event['time_ns']<=row['time_ns']]
        last=prior[-1] if prior else None
        consumed_so_far=(last['offset_frames']+last['frames'] if last['event']=='played' else last.get('played_frames',0)) if last else 0
        if row['offset_frames']!=consumed_so_far or row['result']<0 or (row['event']=='snd_pcm_rewind' and row['result']):
            raise ValueError('Original transport discards queued samples or rewinds its FIFO')
    device=timeline['devices'][timeline['target_device']]
    queued=device['frames']+probe-accepted['accepted_frames']
    if not 0<=queued<=device['capacity']:raise ValueError('Unobserved original primary extent exceeds its queue')
    common=['-O2','-std=gnu89','-w','-DDD2_NO_FOPEN_WRAP','-ffunction-sections','-fdata-sections','-fno-strict-aliasing',
            '-I'+str(ROOT/'build'),str(ROOT/'tools/race_audio_source_test.c'),'-Wl,--gc-sections']
    targets={}
    for target in ('native','native-asan','wasm'):
        wasm=target=='wasm';binary=out/(target+'.js' if wasm else target)
        compiler=['emcc','-mllvm','-fast-isel=false','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760'] if wasm else ['gcc','-m32','-no-pie']
        command=compiler+common+(['-fsanitize=address'] if target=='native-asan' else [])+['-o',str(binary)]
        with (out/(target+'-build.log')).open('w') as log:run_bounded(command,directory=out,timeout=120,check=True,stdout=log,stderr=subprocess.STDOUT)
        pcm=out/(target+'.pcm')
        with (out/(target+'.log')).open('w') as log:
            run_bounded((['node',str(binary)] if wasm else [str(binary)])+[str(out/'input.bin'),str(pcm)],directory=out,timeout=180,check=True,stdout=log,stderr=subprocess.STDOUT)
        rendered=pcm.read_bytes();run=json.loads((out/(target+'.log')).read_text())
        if len(rendered)!=device['frames']*8 or run['frames']!=device['frames']:raise ValueError('Incomplete port primary output')
        actual=(b'\0'*(probe*8)+rendered)[:len(original)]
        compare_pcm(original,actual);compare_pcm(consumed,actual[:len(consumed)])
        targets[target]=dict(pass_=True,**run,accepted_frames=len(original)//8,played_frames=len(consumed)//8,
                             compared_sha256=digest(actual),original_sha256=digest(original),played_sha256=digest(consumed))
        print('PASS',target,'chronological original menu/racing PCM:',len(original),'bytes',flush=True)
        check_space(out)
    negatives={}
    for label,offset in [('prefix',0),('left',len(original)//2//8*8),('right',len(original)//2//8*8+4),('tail',len(original)-1)]:
        damaged=bytearray(original);damaged[offset]^=1
        try:compare_pcm(original,damaged)
        except ValueError:negatives[label]=True
        else:raise AssertionError('Corrupted '+label+' accepted')
    try:compare_pcm(original,original[:-1])
    except ValueError:negatives['truncated']=True
    else:raise AssertionError('Truncated racing PCM accepted')
    old=out/'old';old.mkdir()
    mixer=(ROOT/'build/dd2h_stubs.c').read_text()
    if mixer.count('    b->phase=src->phase;')!=1:raise ValueError('Actual duplicate phase inheritance required')
    (old/'dd2h_stubs.c').write_text(mixer.replace('    b->phase=src->phase;', '    b->phase=0;',1))
    for target in ('native','wasm'):
        wasm=target=='wasm';binary=out/(target+'-phase-zero.js' if wasm else target+'-phase-zero')
        compiler=['emcc','-mllvm','-fast-isel=false','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760'] if wasm else ['gcc','-m32','-no-pie']
        with (out/(target+'-phase-zero-build.log')).open('w') as log:
            run_bounded(compiler+['-I'+str(old)]+common+['-o',str(binary)],directory=out,timeout=120,check=True,stdout=log,stderr=subprocess.STDOUT)
        pcm=out/(target+'-phase-zero.pcm')
        with (out/(target+'-phase-zero.log')).open('w') as log:
            result=run_bounded((['node',str(binary)] if wasm else [str(binary)])+[str(out/'input.bin'),str(pcm)],directory=out,timeout=180,check=False,stdout=log,stderr=subprocess.STDOUT)
        trace=(out/(target+'-phase-zero.log')).read_text()
        if not result.returncode or 'independently advanced source cursor differs' not in trace:
            raise AssertionError('Zero duplicate phase accepted on '+target)
        negatives[target+'-duplicate-phase-zero']=dict(rejected=True,trace=trace.strip())
    report=dict(scope=__doc__.strip(),pass_=True,original_exe_sha256=EXE_SHA256,original_checkpoint=checkpoint,trace_sha256=timeline['trace_sha256'],
                mixer_source_sha256=digest((ROOT/'build/dd2h_stubs.c').read_bytes()),commands=commands,sources=len(timeline['sources']),
                cd_assets=assets,original_device=accepted,original_played=played,probe_frames=probe,primary_queued_frames=queued,
                targets=targets,negative_cases=negatives,empty_fifo_transport_boundaries=transport,
                original_control_calls=len(timeline['control_calls']),
                maximum_simultaneous_sources=max(len(block['sources']) for block in timeline['blocks']))
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    opened=open_files()
    for p in [out/'input.bin',*out.glob('*.pcm')]:
        st=p.stat()
        if (st.st_dev,st.st_ino) in opened:raise RuntimeError('Successful raw comparison output still in use')
        p.unlink()
    return 0


if __name__=='__main__':raise SystemExit(main())
