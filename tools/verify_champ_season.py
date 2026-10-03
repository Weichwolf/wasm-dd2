#!/usr/bin/env python3
"""Verify the actual five-race Retire/Yes championship against the original.

Checks race progression, cumulative standings, all 100 league rows, 20 complete
league framebuffers/palettes and final elimination. Actual input/render timing
differs; chronological race/transition video, audio, ordinary finishes, promotion
and other seasons are outside this check. No engine-state injection or alignment.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

from artifacts import WORK, check_space, open_files, prepare_output, run_bounded

ROOT=Path(__file__).resolve().parents[1]
EXE='0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
KEYS=['Return']*4+['Up','Left','Return','Down','Down','Return']+[
    'Escape','Down','Down','Down','Return','Up','Return','Right','Return',
    'Right','Right','Right','Right','Escape','Down','Down','Return']*5
NORMAL_ARENA_KEYS=['Return','Right','Right','Return','Right','Return','Down','Down','Return']
INPUTS={'original':'real X11 keys','native':'dd2_key_event','browser':'browser keyboard events'}
ADDRESSES=dict(level=0x936ff4,cf=0x462ff0,ticks=0x7746c0,quit=0x7746ac,poly_list=0x940010,
    race_type=0x4673f4,race_mode=0x4673f8,race=0x93dec8,season=0x93dec0,num_races=0x467654,
    division=0x46ad00,stats=0x46741c,actual_season=0x4682f4,phase=0x4699cc,retire_confirm=0x9376a8)


def work(path):
    path=path.resolve()
    if WORK not in path.parents:raise ValueError('Capture/report must be under /tmp/wasm-dd2/')
    return path


def decode(directory,target):
    meta=json.loads((directory/'checkpoint.json').read_text())
    if target=='browser':
        if meta.get('stage')!='browser platform present' or meta.get('canvas_mismatches')!=0:
            raise ValueError('Need actual browser canvas checkpoint')
        return meta
    if target=='original':
        if meta.get('exe_modified') is not False or meta.get('exe_sha256')!=EXE or meta.get('phase')!='Draw_All entry @0x420c9c':
            raise ValueError('Supported unmodified original checkpoint required')
    elif meta.get('phase')!='native Draw_All entry':raise ValueError('Incorrect native capture stage')
    raw=(directory/'image.bin').read_bytes()
    if len(raw)!=0x580400:raise ValueError('Incomplete targeted navigation checkpoint image')
    get=lambda a:struct.unpack_from('<i',raw,a-0x400000)[0]
    text=lambda a,n:raw[a-0x400000:a-0x400000+n].split(b'\0')[0].decode('ascii')
    state={key:get(a) for key,a in ADDRESSES.items()}
    state['cars']=[dict(name=text(0x93dee0+i*54,16),values=list(struct.unpack_from('<7h',raw,0x93def0+i*54-0x400000))) for i in range(20)]
    state['rows']=[dict(name=text(0x940290+i*26,26),points=text(0x940240+i*16,16)) for i in range(5)]
    return state


def validate_end(ended):
    if (ended['level'],ended['poly_list'],ended['race'],ended['season'])!=(0,0x4696b0,5,0) or ended['cars'][0]['values'][1:3]!=[3,4]:
        raise ValueError('Bottom-division elimination did not return to the frontend')


def validate_race_start(started,level,race):
    if (started['level'],started['race'],started['race_type'],started['num_races'])!=(level,race,4,5) or started['quit']!=0 or started['ticks']<=0:
        raise ValueError('Actual championship gameplay has not started')


def load(root,target):
    root=work(root);check_space(root);nav=json.loads((root/'navigation.json').read_text())
    names=[f"step{i:02d}-{key or 'boot'}" for i,key in enumerate([None,*KEYS])]
    if nav['keys']!=KEYS or nav['checkpoints']!=names or nav['input']!=INPUTS[target]:
        raise ValueError('Incomplete/incorrect championship input sequence')
    if target=='original' and not nav.get('acknowledged_keys'):raise ValueError('Original must acknowledge press/release')
    if len(nav.get('initial_save_sha256',''))!=64:raise ValueError('Initial SaveGames provenance missing')
    states=[decode(root/name,target) for name in names]
    previous=[0]*20;races=[];pages={}
    for race,level in enumerate((1,2,5,7,10)):
        start=10+race*17;started,confirmation,result=states[start],states[start+5],states[start+7]
        validate_race_start(started,level,race)
        if confirmation['retire_confirm']!=1:raise ValueError('Actual Retire confirmation missing')
        screen=0x46bf38 if race<4 else 0x46ae44
        if (result['level'],result['race'],result['poly_list'],result['stats'])!=(15,race+1,screen,1):
            raise ValueError('Race/end-of-season progression incorrect')
        if len({car['name'] for car in result['cars']})!=20:
            raise ValueError('Incomplete/distorted championship names')
        for i,car in enumerate(result['cars']):
            if car['values'][0]!=previous[i]+car['values'][6]:raise ValueError('Cumulative points incorrect')
        previous=[car['values'][0] for car in result['cars']]
        races.append(result)
        for division in range(4):
            index=start+9+division;state=states[index]
            if (state['poly_list'],state['division'],state['race'])!=(0x46ab30,division,race+1):
                raise ValueError('Actual league page missing')
            if state['cars']!=result['cars']:raise ValueError('Standings changed while viewing')
            league=sorted((car for car in state['cars'] if car['values'][1]==division),key=lambda c:c['values'][2])
            if [car['values'][2] for car in league]!=list(range(5)):raise ValueError('Incomplete league ranks')
            expected=[dict(name='%R%JL%T/'+car['name'],points='%R%JL%T/'+str(car['values'][0])) for car in league]
            if state['rows']!=expected:raise ValueError('League renderer uses wrong names/points')
            pages[names[index]]=state
    validate_end(states[-1])
    return root,nav,races,pages,states[-1]


def compare(reference,actual):
    failures=[];hashes=[]
    for race,(a,b) in enumerate(zip(reference[2],actual[2]),1):
        if a['cars']!=b['cars']:failures.append(dict(race=race,region='complete-name/standing/last-race-fields'))
    for name,state in reference[3].items():
        if state['rows']!=actual[3][name]['rows']:failures.append(dict(checkpoint=name,region='league-text'))
        record=dict(checkpoint=name,original={},actual={})
        for filename,size in [('framebuf.bin',307200),('palette.bin',1024)]:
            a=(reference[0]/name/filename).read_bytes();b=(actual[0]/name/filename).read_bytes()
            if len(a)!=size or len(b)!=size:raise ValueError('Incomplete framebuffer/palette')
            if a!=b:failures.append(dict(checkpoint=name,region=filename,differing_bytes=sum(x!=y for x,y in zip(a,b))))
            record['original'][filename]=hashlib.sha256(a).hexdigest();record['actual'][filename]=hashlib.sha256(b).hexdigest()
        hashes.append(record)
    if reference[4]['cars']!=actual[4]['cars']:failures.append(dict(region='standings-after-elimination'))
    return failures,hashes


def negative(reference,actual):
    name=next(iter(actual[3]));cases=[]
    for filename,offset in [('framebuf.bin',200*640+200),('palette.bin',17)]:
        path=actual[0]/name/filename;original=path.read_bytes();bad=bytearray(original);bad[offset]^=1
        try:
            path.write_bytes(bad)
            if not compare(reference,actual)[0]:raise AssertionError('Damaged league output accepted')
        finally:path.write_bytes(original)
        cases.append('damaged-'+filename)
    ended={**actual[4],'level':1}
    try:validate_end(ended)
    except ValueError:cases.append('wrong-new-season-instead-of-elimination')
    else:raise AssertionError('Wrong season transition accepted')
    return cases


def capture(args):
    output=prepare_output(work(args.output))
    if output.exists() and any(output.iterdir()):raise ValueError('Use a fresh output directory')
    if args.target=='browser':
        output.mkdir(parents=True,exist_ok=True)
        options=(['--reference='+str(work(args.reference))] if args.reference else [])+(['--stop-after-pause'] if args.stop_after_pause else [])+(['--rng-layout='+str(work(args.rng_layout))] if args.rng_layout else [])
        run_bounded(['node',str(ROOT/'tools/browser/capture_champ_season.js'),str(ROOT/'web/dd2'),str(output),*options],
                    directory=output,timeout=420,check=True,cwd=ROOT)
    else:
        if args.reference or args.stop_after_pause or args.rng_layout:raise ValueError('Input/RNG diagnosis requires browser capture')
        script='reference/capture.py' if args.target=='original' else 'capture_native_menu.py'
        command=[sys.executable,str(ROOT/'tools'/script),'--output',str(output),'--timeout','420','--keys',*KEYS]
        if args.target=='original':command+=['--mode','menu','--acknowledged-key']
        subprocess.run(command,check=True,cwd=ROOT)


def main():
    parser=argparse.ArgumentParser(description=__doc__);commands=parser.add_subparsers(dest='command',required=True)
    cap=commands.add_parser('capture');cap.add_argument('--target',choices=INPUTS,required=True);cap.add_argument('--output',type=Path,required=True)
    cap.add_argument('--reference',type=Path,help='browser-only diagnostic: schedule pauses at independently captured original counters')
    cap.add_argument('--stop-after-pause',action='store_true',help='browser-only focused input diagnosis; not full-season acceptance')
    cap.add_argument('--rng-layout',type=Path,help='browser-only read-only seed/counter layout from this actual WASM binary')
    gate=commands.add_parser('compare')
    for target in INPUTS:gate.add_argument('--'+target,type=Path,required=True)
    gate.add_argument('--report',type=Path,required=True);gate.add_argument('--negative',action='store_true');gate.add_argument('--clean',action='store_true')
    gate.add_argument('--before-native',type=Path,help='reject actual pre-fix full-season capture')
    args=parser.parse_args()
    if args.command=='capture':capture(args);return 0
    report=prepare_output(work(args.report));data={target:load(getattr(args,target),target) for target in INPUTS};reference=data['original']
    result=dict(scope=__doc__.strip(),original_exe_sha256=EXE,initial_save_sha256=reference[1]['initial_save_sha256'],targets={},
        captures={target:str(value[0]) for target,value in data.items()},original_standings=[state['cars'] for state in reference[2]],original_ended=reference[4])
    for target in ('native','browser'):
        actual=data[target]
        if actual[1]['initial_save_sha256']!=result['initial_save_sha256']:raise ValueError('Initial SaveGames differ')
        failures,hashes=compare(reference,actual)
        result['targets'][target]=dict(pass_=not failures,failures=failures,frames=20,league_rows=100,hashes=hashes,
            binary_sha256=actual[1].get('binary_sha256',actual[1].get('wasm_sha256')),
            negative_cases=negative(reference,actual) if args.negative and not failures else [])
    if args.before_native:
        try:load(args.before_native,'native')
        except ValueError as error:
            if 'elimination' not in str(error):raise
            result['before_native_rejected']=str(error)
        else:raise AssertionError('Actual old season behavior was accepted')
    result['pass_']=all(value['pass_'] for value in result['targets'].values())
    report.parent.mkdir(parents=True,exist_ok=True);report.write_text(json.dumps(result,indent=2)+'\n')
    if args.clean and result['pass_']:
        opened=open_files();files=[p for root,*_ in data.values() for p in root.rglob('*') if p.is_file() and not p.is_symlink() and p.suffix in ('.bin','.ppm','.png')]
        if any((p.stat().st_dev,p.stat().st_ino) in opened for p in files):raise RuntimeError('Raw files still in use')
        for p in files:p.unlink()
    display={k:v for k,v in result.items() if k not in ('original_standings','original_ended')}
    display['targets']={target:{**{k:v for k,v in value.items() if k not in ('hashes','failures')},
        'failure_count':len(value['failures']),'failures':value['failures'][:5]} for target,value in result['targets'].items()}
    print(json.dumps(display,indent=2))
    return 0 if result['pass_'] else 1


if __name__=='__main__':raise SystemExit(main())
