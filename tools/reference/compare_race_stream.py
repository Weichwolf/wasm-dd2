#!/usr/bin/env python3
"""Compare every original racing-loop frame/palette under its exact API clock.

Reference: real unmodified dd2h.exe, read-only hardware breakpoints, no input
while demo runs. Debugger-altered GetTickCount results are replayed literally.
Later demos also match the observed initial RNG seed and DEMO MODE blink counter;
every subsequent random state/result and blink state is calculated by the port.
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
import struct
import subprocess
ROOT=Path(__file__).resolve().parents[2]
EXE_SHA='0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
FIELDS=('level','cf','ticks','countdown','frame_skip','quit','clock_calls')

def compare(reference,actual_root,rows):
    failures=[];frames=reference['frames']
    fields=(*FIELDS,'rng_calls') if 'rng_calls' in reference else FIELDS
    if 'demo_flash' in reference['first_state']:fields=(*fields,'demo_flash')
    if len(rows)!=len(frames):failures.append({'error':'presentation count differs','original':len(frames),'port':len(rows)})
    for index,(expected,actual) in enumerate(zip(frames,rows)):
        if any(actual.get(key)!=expected[key] for key in fields):
            failures.append({'index':index,'error':'engine state/clock call phase differs','original':{key:expected[key] for key in fields},'port':actual})
        for suffix,size in [('bin',307200),('pal',1024)]:
            source=reference['_root']/f'{expected["prefix"]}.{suffix}'
            target=actual_root/f'f{actual["flip"]:05d}.{suffix}'
            data=target.read_bytes() if target.is_file() else b''
            original=source.read_bytes()
            if len(original)!=size:raise RuntimeError('incomplete original frame/palette')
            if data!=original:
                failures.append({'index':index,'error':f'{suffix} bytes differ','port_bytes':len(data),
                    'first_difference':next((i for i,(a,b) in enumerate(zip(original,data)) if a!=b),min(len(original),len(data)))})
        if index in reference.get('diagnostic_image_frames',[]):
            # Narrow diagnostic for patch 842; engine images contain host API
            # pointers and cannot be compared as an entire address space.
            original=(reference['_root']/f'{expected["prefix"]}.image').read_bytes()
            port=(actual_root/f'imagef{actual["flip"]:05d}.bin').read_bytes()
            if len(original)!=0x580400 or len(port)!=0x580400:raise RuntimeError('incomplete diagnostic engine image')
            offset=0x789358-0x400000
            if original[offset:offset+2]!=port[offset:offset+2]:
                failures.append({'index':index,'error':'first scenery object x differs',
                    'original':struct.unpack_from('<h',original,offset)[0],'port':struct.unpack_from('<h',port,offset)[0]})
    return failures

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--native',type=Path,default=Path('/tmp/dd2_native'))
    p.add_argument('--wasm',type=Path,default=Path('/tmp/lvltest/dd2run.js'));p.add_argument('--node',default='node');p.add_argument('--timeout',type=float,default=120)
    p.add_argument('--attract-history',action='store_true',help='run the real frontend and preceding demos; require naturally calculated initial RNG/blink states')
    a=p.parse_args();root=a.capture.resolve()/'race';out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    r=json.loads((root/'race.json').read_text());ticks=root/'ticks.bin'
    if r.get('exe_modified') is not False or r.get('exe_sha256')!=EXE_SHA or hashlib.sha256((ROOT/'DestructionDerby2/dd2h.exe').read_bytes()).hexdigest()!=EXE_SHA:
        raise RuntimeError('supported unmodified original required')
    frames=r['frames'];count=r['clock_calls']
    if (not r.get('complete_racing_loop') or r['stage']!='Draw_All entry / pending presentation' or r['level'] not in range(1,11) or
        not frames or [row['index'] for row in frames]!=list(range(len(frames))) or
        [row['prefix'] for row in frames]!=[f'frame{i:05d}' for i in range(len(frames))] or
        r['final_state']['quit']!=1 or r['first_state']['ticks']!=0 or ticks.stat().st_size!=count*4 or
        frames[0]['clock_calls']!=1 or any(row['quit'] or row['level']!=r['level'] for row in frames) or
        any(row['clock_calls']>=count for row in frames) or
        any(frames[i]['clock_calls']<=frames[i-1]['clock_calls'] for i in range(1,len(frames)))):
        raise RuntimeError('incomplete/inconsistent original loop/clock manifest')
    random_env={}
    if 'demo_flash' in r['first_state']:
        if r['first_state']['demo_flash'] not in range(81):raise RuntimeError('invalid original initial demo flash state')
        random_env['DD2_RACE_FLASH_INITIAL']=str(r['first_state']['demo_flash'])
    if 'rng_calls' in r:
        random=root/'random.bin'
        if random.stat().st_size!=r['rng_calls']*12 or not r['rng_calls'] or any(row.get('rng_calls',-1)<1 or row['rng_calls']>r['rng_calls'] for row in frames):
            raise RuntimeError('incomplete original random state/return trace')
        records=list(struct.iter_unpack('<III',random.read_bytes()))
        if records[0][0]!=r['rng_initial_seed'] or records[-1][1]!=r['rng_final_seed'] or any(
                (before*0x41c64e6d+0x3039)&0xffffffff!=after or (after>>16)&0x7fff!=value or
                (i and before!=records[i-1][1]) for i,(before,after,value) in enumerate(records)):
            raise RuntimeError('inconsistent original random state/return records')
        random_env.update(DD2_RANDOM_REFERENCE=str(random),DD2_RANDOM_LEVEL=str(r['level']))
    if a.attract_history:
        if 'rng_calls' not in r or 'demo_flash' not in r['first_state']:raise RuntimeError('attract history requires recorded random and blink states')
        random_env.update(DD2_TICK_LEVEL=str(r['level']),DD2_RACE_STOP_AFTER_CAPTURE='1',
            DD2_RACE_FLASH_REQUIRE='1',DD2_RANDOM_REQUIRE_INITIAL='1',DD2_FE='1')
    r['_root']=root
    image_counters={frames[index]['cf'] for index in r.get('diagnostic_image_frames',[]) if index<len(frames)}
    if image_counters:random_env.update(DD2_IMGDUMP=','.join(map(str,sorted(image_counters))),DD2_IMGDUMP_FLIP='1')
    report={'scope':__doc__,'original_exe_sha256':EXE_SHA,'original_clock_sha256':hashlib.sha256(ticks.read_bytes()).hexdigest(),
        'original_frames':len(frames),'original_clock_calls':count,'original_rng_calls':r.get('rng_calls'),
        'original_initial_state':r['first_state'],'original_random_sha256':hashlib.sha256(random.read_bytes()).hexdigest() if 'rng_calls' in r else None,
        'level':r['level'],'attract_history':a.attract_history,'initial_rng_and_blink':
            'calculated by preceding real demos; checked, never initialized from reference' if a.attract_history else 'initialized once from observed original',
        'targets':[]}
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    for name,command,artifact in [('native',[str(a.native.resolve())],a.native),('wasm',[a.node,str(a.wasm.resolve()),'fe' if a.attract_history else str(r['level'])],a.wasm.with_suffix('.wasm'))]:
        directory=out/name;directory.mkdir();log=directory/'run.log'
        with log.open('w') as stream:
            run=subprocess.run(command,cwd=ROOT/'DestructionDerby2',env={**env,**random_env,**({} if a.attract_history else {'DD2_LEVEL':str(r['level'])}),'DD2_SOUND':'1',
                'DD2_TICK_REPLAY':str(ticks),'DD2_RACE_STREAM':str(directory/'race.jsonl'),
                'DD2_FRAMEDIR':str(directory),'DD2_PALDUMP':'1'},stdout=stream,stderr=subprocess.STDOUT,timeout=a.timeout)
        text=log.read_text()
        if run.returncode or f'[clock-replay] consumed={count} complete' not in text or re.search(r'SIGSEGV|SIGBUS|RuntimeError|FATAL|abort',text):
            raise RuntimeError(f'{name}: engine or exact clock-consumption failure; see {log}')
        if 'rng_calls' in r and f'[random-reference] consumed={r["rng_calls"]} complete' not in text:raise RuntimeError(f'{name}: incomplete original random trace consumption')
        if a.attract_history and '[race-stream] target racing loop finished' not in text:raise RuntimeError(f'{name}: target attract loop not completed')
        rows=[json.loads(line) for line in (directory/'race.jsonl').read_text().splitlines()]
        if any(row['flip']<=rows[i-1]['flip'] for i,row in enumerate(rows) if i):raise RuntimeError('port presentation indices not increasing')
        failures=compare(r,directory,rows)
        result={'target':name,'binary_sha256':hashlib.sha256(artifact.read_bytes()).hexdigest(),'pass':not failures,'frames':len(rows),
            'framebuffer_bytes':len(rows)*307200,'palette_bytes':len(rows)*1024,'failures':failures,
            'diagnostic_first_scene_x_checks':len([i for i in r.get('diagnostic_image_frames',[]) if i<len(rows)])}
        if a.attract_history:result['preceding_demo_levels']=[int(value) for value in re.findall(r'\[clock-replay\] warmup level=(\d+)',text)]
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
        for field in ['rng_calls','demo_flash']:
            if field not in frames[0]:continue
            altered=[dict(row) for row in rows];altered[0][field]+=1
            bad=compare(r,directory,altered)
            if len(bad)!=1 or bad[0].get('index')!=0 or bad[0]['error']!='engine state/clock call phase differs':
                raise RuntimeError(f'changed first {field} phase was not rejected')
            result[f'negative_{field}_rejected']=True
        print(f'PASS {name} vs actual original: {len(rows)} entire racing-loop frames/palettes, all {count} clock returns exact; first-pixel corruption rejected',flush=True)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':main()
