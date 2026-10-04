"""Validate every original DirectDraw upload/palette in a shared audio trace.

No debugger or alignment by frame counter. Binary records and inline markers
must agree at every original Flip/clock position. The actual observer checks
uploaded indexed pixels against the engine buffer; attached device palettes
are independently read and compared here. A missing device palette is explicit.
This exports original evidence, not port or physical-display parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from artifacts import WORK,check_space
from reference.game_clock import EXE,RETURN,SITES

HEADER=struct.Struct('<32I')
BYTES=HEADER.size+307200+2048
MARKER=re.compile(rb'DD2_VIDEO record=(\d+)')


def observe(capture):
    clock=json.loads((capture/'game-clock/report.json').read_text())
    build=json.loads((capture/'video-observer-build.json').read_text())
    original=json.loads((capture/'report.json').read_text())
    if (not original['pass_'] or original['original_exe_sha256']!=EXE or original['debugger'] or
        original['engine_state_writes'] or not clock['pass_'] or clock['original_exe_sha256']!=EXE or
        build['record_bytes']!=BYTES or build['observer_sha256']!=hashlib.sha256((capture/'video-observer.dll').read_bytes()).hexdigest()):
        raise ValueError('Successful unchanged original and verified video observer required')
    frames=[];sha=hashlib.sha256();previous_qpc=0
    with (capture/'video.bin').open('rb') as file:
        while raw:=file.read(BYTES):
            sha.update(raw)
            if len(raw)!=BYTES:raise ValueError('Incomplete original video record')
            v=HEADER.unpack_from(raw);number=len(frames)+1;qpc=v[6]+(v[7]<<32)
            if (v[:4]!=(0x32564444,1,number,number) or v[4]!=0x412cc1 or v[5]!=int(clock['engine_thread'],16) or
                qpc<previous_qpc or v[17:24]!=(640,480,640,8,307200,1024,1024) or v[25] or any(v[26:]) or v[10]):
                raise ValueError('Malformed, unordered or unsupported original video header')
            previous_qpc=qpc;pixels=raw[HEADER.size:HEADER.size+307200]
            palette=raw[HEADER.size+307200:HEADER.size+308224];device=raw[-1024:]
            if not v[24] and palette!=device:raise ValueError('Actual DirectDraw palette differs from engine palette')
            if v[24] and (v[24]!=0x8876023c or any(device)):
                raise ValueError('Unexpected actual palette API failure or invented palette')
            frames.append(dict(index=number-1,flip=number,caller=v[4],thread=v[5],qpc=qpc,
                level=v[8],cf=v[9],movie=v[10],poly_list=v[11],restart_cd_audio=v[12],ticks=v[13],
                replay=v[14],quit=v[15],script_cursor=v[16],
                framebuffer_sha256=hashlib.sha256(pixels).hexdigest(),palette_sha256=hashlib.sha256(palette).hexdigest(),
                device_palette_observed=not v[24],device_palette_result=v[24],
                device_palette_sha256=hashlib.sha256(device).hexdigest() if not v[24] else None))
    if not frames or len(frames)>4096:raise ValueError('Empty/unbounded original video')
    seen=flips=calls=0;trace=hashlib.sha256()
    with (capture/'wine.log').open('rb') as lines:
        for number,raw in enumerate(lines,1):
            trace.update(raw)
            if b'ddraw_surface1_Flip iface' in raw:flips+=1
            if b'.GetTickCount()' in raw:
                returned=RETURN.fullmatch(raw.decode(errors='replace').rstrip())
                if returned and int(returned[5],16) in SITES:calls+=1
            if b'DD2_VIDEO record=' not in raw:continue
            m=MARKER.search(raw)
            if not m or int(m[1])!=seen+1 or seen>=len(frames) or flips!=frames[seen]['flip']:
                raise ValueError('Missing/duplicate/reordered original video marker')
            frames[seen].update(trace_line=number,clock_calls=calls);seen+=1
    if trace.hexdigest()!=clock['trace_sha256'] or seen!=len(frames) or flips!=len(frames) or calls!=clock['calls']:
        raise ValueError('Original video/clock trace provenance or extent differs')
    racing=[f for f in frames if f['level']]
    terminal=original['completion']['script_cursor']
    if (not racing or any(f['level']!=1 or (not f['replay'] and
        (not f['quit'] or f['script_cursor']!=terminal)) for f in racing) or frames[-1]['script_cursor']!=terminal):
        raise ValueError('Complete original replay racing video/terminal required')
    return dict(scope=__doc__.strip(),pass_=True,capture_directory=str(capture.resolve()),original_exe_sha256=EXE,trace_sha256=trace.hexdigest(),
        video_sha256=sha.hexdigest(),game_clock_sha256=clock['ticks_sha256'],observer_sha256=build['observer_sha256'],record_bytes=BYTES,
        frames=frames,frame_count=len(frames),racing_frames=len(racing),
        attached_device_palettes=sum(f['device_palette_observed'] for f in frames),
        missing_device_palette_frames=[f['index'] for f in frames if not f['device_palette_observed']],
        debugger=False,engine_state_writes=False,port_comparison='pending')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=args.output.resolve()
    if WORK not in out.parents or out.exists():parser.error('Fresh /tmp/wasm-dd2/ report required')
    r=observe(args.capture);out.parent.mkdir(parents=True,exist_ok=True);check_space(args.capture)
    out.write_text(json.dumps(r,indent=2)+'\n')
    print('Verified original presentations:',r['frame_count'],'racing:',r['racing_frames'])
