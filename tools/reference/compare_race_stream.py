#!/usr/bin/env python3
"""Compare every original racing-loop frame/palette under its exact API clock.

Reference: real unmodified dd2h.exe, read-only hardware breakpoints, no input
while demo runs. Debugger-altered GetTickCount results are replayed literally.
This covers the complete captured Play_Game racing render loop, including its
countdown, every duplicate cf/presentation and both endpoints. It excludes
Init_Game/loading/fades, original audio, movie and physical output clocks.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
ROOT=Path(__file__).resolve().parents[2]
EXE_SHA='0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
FIELDS=('level','cf','ticks','countdown','frame_skip','quit','clock_calls')

def compare(reference,actual_root,rows):
    failures=[];frames=reference['frames']
    if len(rows)!=len(frames):failures.append({'error':'presentation count differs','original':len(frames),'port':len(rows)})
    for index,(expected,actual) in enumerate(zip(frames,rows)):
        if any(actual.get(key)!=expected[key] for key in FIELDS):
            failures.append({'index':index,'error':'engine state/clock call phase differs','original':{key:expected[key] for key in FIELDS},'port':actual})
        for suffix,size in [('bin',307200),('pal',1024)]:
            source=reference['_root']/f'{expected["prefix"]}.{suffix}'
            target=actual_root/f'f{actual["flip"]:05d}.{suffix}'
            data=target.read_bytes() if target.is_file() else b''
            original=source.read_bytes()
            if len(original)!=size:raise RuntimeError('incomplete original frame/palette')
            if data!=original:
                failures.append({'index':index,'error':f'{suffix} bytes differ','port_bytes':len(data),
                    'first_difference':next((i for i,(a,b) in enumerate(zip(original,data)) if a!=b),min(len(original),len(data)))})
    return failures

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path,default=Path('/tmp/dd2_native'))
    p.add_argument('--wasm',type=Path,default=Path('/tmp/lvltest/dd2run.js'));p.add_argument('--node',default='node');p.add_argument('--timeout',type=float,default=120)
    a=p.parse_args();root=a.capture.resolve()/'race';out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    r=json.loads((root/'race.json').read_text());ticks=root/'ticks.bin'
    if r.get('exe_modified') is not False or r.get('exe_sha256')!=EXE_SHA or hashlib.sha256((ROOT/'DestructionDerby2/dd2h.exe').read_bytes()).hexdigest()!=EXE_SHA:
        raise RuntimeError('supported unmodified original required')
    frames=r['frames'];count=r['clock_calls']
    if (not r.get('complete_racing_loop') or r['stage']!='Draw_All entry / pending presentation' or r['level']!=9 or
        not frames or [row['index'] for row in frames]!=list(range(len(frames))) or
        [row['prefix'] for row in frames]!=[f'frame{i:05d}' for i in range(len(frames))] or
        r['final_state']['quit']!=1 or r['first_state']['ticks']!=0 or ticks.stat().st_size!=count*4 or
        frames[0]['clock_calls']!=1 or any(row['quit'] or row['level']!=9 for row in frames) or
        any(row['clock_calls']>=count for row in frames) or
        any(frames[i]['clock_calls']<=frames[i-1]['clock_calls'] for i in range(1,len(frames)))):
        raise RuntimeError('incomplete/inconsistent original loop/clock manifest')
    r['_root']=root
    report={'scope':__doc__,'original_exe_sha256':EXE_SHA,'original_clock_sha256':hashlib.sha256(ticks.read_bytes()).hexdigest(),
        'original_frames':len(frames),'original_clock_calls':count,'targets':[]}
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    for name,command,artifact in [('native',[str(a.native.resolve())],a.native),('wasm',[a.node,str(a.wasm.resolve()),'9'],a.wasm.with_suffix('.wasm'))]:
        directory=out/name;directory.mkdir();log=directory/'run.log'
        with log.open('w') as stream:
            run=subprocess.run(command,cwd=ROOT/'DestructionDerby2',env={**env,'DD2_LEVEL':'9','DD2_SOUND':'1',
                'DD2_TICK_REPLAY':str(ticks),'DD2_RACE_STREAM':str(directory/'race.jsonl'),
                'DD2_FRAMEDIR':str(directory),'DD2_PALDUMP':'1'},stdout=stream,stderr=subprocess.STDOUT,timeout=a.timeout)
        text=log.read_text()
        if run.returncode or f'[clock-replay] consumed={count} complete' not in text or re.search(r'SIGSEGV|SIGBUS|RuntimeError|FATAL|abort',text):
            raise RuntimeError(f'{name}: engine or exact clock-consumption failure; see {log}')
        rows=[json.loads(line) for line in (directory/'race.jsonl').read_text().splitlines()]
        if any(row['flip']<=rows[i-1]['flip'] for i,row in enumerate(rows) if i):raise RuntimeError('port presentation indices not increasing')
        failures=compare(r,directory,rows)
        result={'target':name,'binary_sha256':hashlib.sha256(artifact.read_bytes()).hexdigest(),'pass':not failures,'frames':len(rows),
            'framebuffer_bytes':len(rows)*307200,'palette_bytes':len(rows)*1024,'failures':failures}
        report['targets'].append(result);(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        if failures:raise RuntimeError(f'{name}: complete original stream differs ({len(failures)} records); first {failures[:2]}')
        # This must reject a real corrupted first capture byte, including the
        # often-excluded countdown endpoint. Restore accepted artifact exactly.
        path=directory/f'f{rows[0]["flip"]:05d}.bin';accepted=path.read_bytes();changed=bytearray(accepted);changed[0]^=1
        try:
            path.write_bytes(changed);bad=compare(r,directory,rows)
            if bad!=[{'index':0,'error':'bin bytes differ','port_bytes':307200,'first_difference':0}]:raise RuntimeError('corrupted first original-comparison frame was not rejected exactly')
        finally:path.write_bytes(accepted)
        result['negative_first_pixel_rejected']=True
        print(f'PASS {name} vs actual original: {len(rows)} entire racing-loop frames/palettes, all {count} clock returns exact; first-pixel corruption rejected',flush=True)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':main()
