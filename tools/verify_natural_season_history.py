#!/usr/bin/env python3
"""Compare five naturally completed Wrecking championship races with the original.

Check every captured racing indexed pixel/palette, actual clock/calculated RNG
input, acknowledged keyboard transition, all twenty drivers' cumulative points,
four league pages after every race and the actual end-of-season transition.
Destroyed cars remain explicit natural results. This does not accept every
regular finish or win, other menu/loading/fade video, PCM or physical timing.
"""
import argparse
import copy
import json
from pathlib import Path
import struct

from artifacts import WORK, open_files, prepare_output
from verify_champ_history import recorded_apis
from verify_champ_season import (EXE, NATURAL_SEASON_ACTIONS,
    NATURAL_SEASON_KEYS, NATURAL_SEASON_STARTS, CHAMP_LEVELS, validate_race_start)
from verify_natural_champ_history import (CODES, completion_requirements,
    navigation_inputs, validate_driving_inputs)
from verify_normal_arena_history import (compare_frames,
    compare_checkpoints, digest, natural, validate_racing_timeline)

NAMES = ['step00-boot', *[f'step{i:02d}-{key}' for i,key in enumerate(NATURAL_SEASON_ACTIONS,1)]]
IMAGE_NAMES = [NAMES[i] for start in NATURAL_SEASON_STARTS
               for i in (start,start+1,start+3,start+4,start+5,start+6)] + [NAMES[-1]]


def load(path):
    return json.loads(path.read_text())


def standings(points, finals):
    if len(points)!=66 or len(finals)!=5:
        raise ValueError('All five natural race completions and 66 checkpoints required')
    results=[]
    for race,start in enumerate(NATURAL_SEASON_STARTS):
        before,result=points[start],points[start+1]
        validate_race_start(before,CHAMP_LEVELS[race],race)
        natural(finals[race])
        if (finals[race]['level'],finals[race]['race'])!=(15,race+1):
            raise ValueError('Natural completion belongs to another race/result')
        if (result['level'],result['race'],result['poly_list'],result['stats'],result['retire_confirm'])!=(15,race+1,0x46bf38 if race<4 else 0x46ae44,1,0):
            raise ValueError('Natural season result screen/race/statistics differs')
        if len(before['cars'])!=20 or len(result['cars'])!=20 or len({car['name'] for car in result['cars']})!=20:
            raise ValueError('Twenty distinct driver names required')
        for a,b in zip(before['cars'],result['cars']):
            if a['name']!=b['name'] or b['values'][0]!=a['values'][0]+b['values'][6]:
                raise ValueError('Driver identity or cumulative race points differ')
        if race and before['cars']!=points[NATURAL_SEASON_STARTS[race-1]+1]['cars']:
            raise ValueError('Cumulative standings changed between actual races')
        driver=finals[race]['driver']
        if race<4 and driver['dead']!=1 and driver['finished_laps']!=1:
            raise ValueError('Road race must complete laps or destroy the actual player')
        player=completion_requirements(driver,before,result)
        results.append(dict(level=CHAMP_LEVELS[race],race=race,driver=driver,player=player))
        for division in range(4):
            page=points[start+3+division]
            if (page['poly_list'],page['division'],page['race'],page['retire_confirm'])!=(0x46ab30,division,race+1,0):
                raise ValueError('All four actual league pages required after each race')
            if page['cars']!=result['cars']:
                raise ValueError('Viewing league pages changed cumulative standings')
            league=[car for car in page['cars'] if car['values'][1]==division]
            if len(page['rows'])!=5 or sorted(car['values'][2] for car in league)!=list(range(5)):
                raise ValueError('Each league must contain all five ranked drivers and rendered rows')
            for rank,row in enumerate(page['rows']):
                car=next((car for car in page['cars'] if car['values'][1:3]==[division,rank]),None)
                if car is None or row!={'name':'%R%JL%T/'+car['name'],'points':'%R%JL%T/'+str(car['values'][0])}:
                    raise ValueError('League row does not contain its classified driver and score')
    ended=points[-1]
    if ended['retire_confirm']:
        raise ValueError('Natural season must never Retire')
    if ended['level']==0:
        if ended['poly_list']!=0x4696b0:
            raise ValueError('Eliminated player must actually reach the main menu')
        ending='frontend'
    else:
        if not 1<=ended['level']<=10 or ended['race']!=0 or ended['quit'] or ended['ticks']<=0 or ended['race_type']!=4:
            raise ValueError('Continuing player must actually start the next championship season')
        ending='next-season-gameplay'
    return results,ending


def history(root,target):
    meta=load(root/'history.json')
    if (meta.get('natural_season') is not True or meta.get('natural_championship') is not True
            or meta.get('natural_finish') is not True or meta.get('keys')!=NATURAL_SEASON_KEYS
            or meta.get('actions')!=NATURAL_SEASON_ACTIONS or meta.get('checkpoints')!=NAMES
            or meta.get('acknowledged_keys') is not True or len(meta['held_pad_polls'])!=60
            or min(meta['held_pad_polls'])<1 or meta.get('racing_image_format')!='indexed-zlib'):
        raise ValueError('Complete acknowledged natural-season capture required')
    if target=='original':
        if meta.get('exe_modified') is not False or meta.get('exe_sha256')!=EXE or meta['input']!='real X11 keys':
            raise ValueError('Actual unmodified original X11 input required')
    elif meta['input']!='dd2_key_event' or len(meta.get('binary_sha256',''))!=64 or 'api_inputs' not in meta:
        raise ValueError('Native keyboard bridge and strict original API replay required')
    events=[json.loads(line) for line in (root/'events.jsonl').read_text().splitlines()]
    if [row['index'] for row in events]!=list(range(1,len(events)+1)):
        raise ValueError('Incomplete/reordered actual observation stream')
    expected=navigation_inputs(meta)
    if [(row['key'],row['down'],row['action']) for row in events if row['kind']=='key']!=expected:
        raise ValueError('Actual complete menu/driving key sequence differs')
    validate_driving_inputs(meta,events)
    random,ticks=[(root/name).read_bytes() for name in ('random.bin','ticks.bin')]
    recorded_apis(meta,events,random,ticks)
    frames=meta['race_frames']
    if not frames or [row['index'] for row in frames]!=list(range(len(frames))):
        raise ValueError('Complete chronological racing picture extent required')
    if (frames[0]['level'],frames[0]['cf'],frames[0]['ticks'])!=(1,0,2) or any(row['retired'] for row in frames):
        raise ValueError('First real player picture and no Retire required')
    validate_racing_timeline(meta,events,target)
    for race,level in enumerate(CHAMP_LEVELS):
        if not any(row['race']==race and row['level']==level for row in frames):
            raise ValueError('Missing actual championship racing video')
    held=set();previous=-1
    for row in meta['driving_inputs']:
        frame,key=row['frame'],row['key']
        if frame<previous or not 0<=frame<len(frames) or key not in ('a','z','Left','Right'):
            raise ValueError('Invalid driving presentation/key')
        if row['down']:
            if key in held:raise ValueError('Duplicate driving press')
            held.add(key)
        else:
            if key not in held:raise ValueError('Unmatched driving release')
            held.remove(key)
        previous=frame
    if held:raise ValueError('All driving controls must be released')
    points=[load(root/name/'checkpoint.json') for name in NAMES]
    results,ending=standings(points,meta['final_races'])
    return meta,random,ticks,results,ending


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('original','native','browser','report'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--clean',action='store_true')
    args=parser.parse_args()
    for name in ('original','native','browser','report'):
        p=getattr(args,name).resolve()
        if WORK not in p.parents:parser.error('Use /tmp/wasm-dd2/ for captures/reports')
        setattr(args,name,p)
    prepare_output(args.report)
    om,random,ticks,results,ending=history(args.original,'original')
    nm,nr,nt,nresults,nending=history(args.native,'native')
    if (nr,nt,nm['initial_save_sha256'],nm['driving_inputs'],nm['final_races'],nresults,nending)!=(random,ticks,om['initial_save_sha256'],om['driving_inputs'],om['final_races'],results,ending):
        raise ValueError('Native actual API/save/input/natural results differ')
    nav=load(args.browser/'navigation.json');observer=load(args.browser/'rng-observations.json')
    api=dict(clock_calls=om['clock_calls'],computed_rng_calls=om['rng_calls'],
             api_sha256=dict(clock=digest(ticks),random=digest(random)))
    if (nav.get('natural_season') is not True or nav.get('keys')!=NATURAL_SEASON_KEYS
            or nav.get('actions')!=NATURAL_SEASON_ACTIONS or nav.get('checkpoints')!=NAMES
            or nav.get('input')!='browser keyboard events' or nav['initial_save_sha256']!=om['initial_save_sha256']
            or nav['api_reference']!=api or nav['wasm_sha256']!=observer['layout']['wasm_sha256']
            or nav.get('final_drivers')!=[row['driver'] for row in om['final_races']]):
        raise ValueError('Actual browser complete season input/save/API/results differ')
    expected=navigation_inputs(om)
    if [(row['code'],row['down'],row['action']) for row in nav['input_observations']]!=[(CODES[key],down,action) for key,down,action in expected]:
        raise ValueError('Actual DOM menu/driving events differ')
    driving=[row for row in nav['input_observations'] if 'frame' in row]
    if [(row['frame'],row['finish']) for row in driving]!=[(row['frame'],bool(row.get('finish'))) for row in om['driving_inputs']]:
        raise ValueError('Actual DOM driving transitions moved to another presentation')
    seeds=[1,*[row[1] for row in struct.iter_unpack('<III',random)]]
    observations=observer['observations']
    if not observations or (observations[0]['count'],observations[0]['seed'])!=(0,1):
        raise ValueError('Observed original boot seed/counter required')
    previous=0
    for row in observations:
        count=row['count']
        if count<previous or count>=len(seeds) or row['seed']!=seeds[count]:
            raise ValueError('Actual browser calculated LCG differs')
        previous=count
    end=load(args.browser/NAMES[-1]/'checkpoint.json')
    if end['api_calls']!={'clock':om['clock_calls'],'random':om['rng_calls']} or end['rng']!={'count':om['rng_calls'],'seed':seeds[-1]}:
        raise ValueError('Actual browser final API extent differs')
    report=dict(scope=__doc__.strip(),original_exe_sha256=EXE,initial_save_sha256=om['initial_save_sha256'],
        native_sha256=nm['binary_sha256'],wasm_sha256=nav['wasm_sha256'],api_inputs=api,
        racing_frames=len(om['race_frames']),races=results,ending=ending,targets={})
    for target,root,frames in [('native',args.native,nm['race_frames']),('browser',args.browser,load(args.browser/'race-frames.json'))]:
        pad_offset=nm['race_frames'][0]['pad_polls']-om['race_frames'][0]['pad_polls'] if target=='native' else 0
        if pad_offset not in (-1,0,1):raise ValueError('Unexpected debugger pad observation origin')
        differences,images=compare_frames(args.original,root,om['race_frames'],frames,seeds,target=='browser',pad_offset=pad_offset)
        points=[load(root/name/'checkpoint.json') for name in NAMES]
        actual,actual_ending=standings(points,om['final_races'])
        if (actual,actual_ending)!=(results,ending):raise ValueError('Port cumulative natural season results differ')
        state_diff,point_images=compare_checkpoints(args.original,root,NAMES,IMAGE_NAMES)
        differences.extend(state_diff)
        report['targets'][target]=dict(pass_=not differences,differences=differences,
            racing_image_hashes=images,checkpoint_image_hashes=point_images,debugger_pad_origin_offset=pad_offset)
    negatives={}
    original_points=[load(args.original/name/'checkpoint.json') for name in NAMES]
    for label in ('missing-race','retire','wrong-finish-race','wrong-score','wrong-league-row','missing-league-row','wrong-ending'):
        points=copy.deepcopy(original_points);finals=copy.deepcopy(om['final_races'])
        if label=='missing-race':finals.pop()
        elif label=='retire':finals[2]['retired']=1
        elif label=='wrong-finish-race':finals[2]['race']=1
        elif label=='wrong-score':points[33]['cars'][7]['values'][0]+=1
        elif label=='wrong-league-row':points[46]['rows'][2]['points']='wrong'
        elif label=='missing-league-row':points[46]['rows'].pop()
        else:points[-1]['level']=15
        try:standings(points,finals)
        except ValueError:negatives[label]=True
        else:negatives[label]=False
    if not all(negatives.values()):raise ValueError('Invalid natural-season evidence accepted')
    report['negative_cases']=negatives
    report['pass_']=all(row['pass_'] for row in report['targets'].values())
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS' if report['pass_'] else 'FAIL','natural season:',report['racing_frames'],'pictures, five races, twenty league pages,',ending)
    if args.clean and report['pass_']:
        opened=open_files();paths=[]
        for root in (args.original,args.native,args.browser):
            for pattern in ('race*.bin.z','race*.pal','step*/framebuf.bin','step*/palette.bin'):
                for p in root.glob(pattern):
                    st=p.stat()
                    if p.is_symlink() or (st.st_dev,st.st_ino) in opened:raise RuntimeError('Capture still needed by a running process')
                    paths.append(p)
        for p in paths:p.unlink()
    return 0 if report['pass_'] else 1


if __name__=='__main__':
    raise SystemExit(main())
