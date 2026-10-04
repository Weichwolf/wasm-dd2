#!/usr/bin/env python3
"""Compare a natural first Wrecking championship race and its four league pages.

Every racing indexed pixel/palette, recorded clock/calculated RNG input and game
state is checked against the unmodified original. Results, cumulative points,
league rows and the next actual race start are included. Other menu animation,
loading/fades, physical clocks, PCM, wins and a full season are not accepted.
"""
import argparse
import copy
import json
from pathlib import Path
import struct
import tempfile
import zlib

from artifacts import WORK, open_files, prepare_output
from verify_champ_history import recorded_apis
from verify_champ_season import EXE, NATURAL_CHAMP_KEYS, NATURAL_CHAMP_ACTIONS
from verify_normal_arena_history import (STATE, compare_frames, compare_checkpoints,
                                        digest, exact_bytes, picture, natural,
                                        validate_racing_timeline)

NAMES=['step00-boot',*[f'step{i:02d}-{key}' for i,key in enumerate(NATURAL_CHAMP_ACTIONS,1)]]
IMAGE_NAMES=[NAMES[i] for i in (10,11,13,14,15,16,21)]
CODES=dict(Return='Enter',Escape='Escape',Up='ArrowUp',Down='ArrowDown',
           Left='ArrowLeft',Right='ArrowRight',a='KeyA',z='KeyZ')


def load(path):
    return json.loads(path.read_text())


def navigation_inputs(meta):
    result=[]
    for index,key in enumerate(meta.get('actions',NATURAL_CHAMP_ACTIONS),1):
        if key=='natural-finish':
            driving=meta['driving_inputs']
            if meta.get('natural_season'):
                race=(index-11)//11
                driving=[row for row in driving if meta['race_frames'][row['frame']]['race']==race]
            result.extend((row['key'],row['down'],index) for row in driving)
        else:
            result.extend((key,down,index) for down in (True,False))
    return result


def validate_driving_inputs(meta, events):
    drive_actions={i for i,key in enumerate(meta.get('actions',NATURAL_CHAMP_ACTIONS),1) if key=='natural-finish'}
    observed=[row for row in events if row['kind']=='key' and row['action'] in drive_actions]
    if len(observed)!=len(meta['driving_inputs']):
        raise ValueError('Driving metadata does not cover every observed real-key transition')
    for transition,event in zip(meta['driving_inputs'],observed):
        frame=transition['frame']
        if not 0<=frame<len(meta['race_frames']):raise ValueError('Invalid driving frame')
        finish=meta['final_races'][(event['action']-11)//11] if meta.get('natural_season') else meta['final_race']
        boundary=finish if transition.get('finish') else meta['race_frames'][frame]
        if (transition['key'],transition['down'])!=(event['key'],event['down']):
            raise ValueError('Driving metadata differs from actual observed key')
        if any(event[key]!=boundary[key] for key in (*STATE,'clock_calls','rng_calls','draws','pad_polls')):
            raise ValueError('Driving input was moved to another observed presentation')


def history(root, target):
    meta=load(root/'history.json')
    if (meta.get('natural_championship') is not True or meta.get('natural_finish') is not True
            or meta.get('keys')!=NATURAL_CHAMP_KEYS or meta.get('actions')!=NATURAL_CHAMP_ACTIONS
            or meta.get('checkpoints')!=NAMES or meta.get('acknowledged_keys') is not True
            or len(meta['held_pad_polls'])!=20 or min(meta['held_pad_polls'])<1
            or meta.get('racing_image_format')!='indexed-zlib'):
        raise ValueError('Complete acknowledged natural championship capture required')
    if target=='original':
        if meta.get('exe_modified') is not False or meta.get('exe_sha256')!=EXE or meta['input']!='real X11 keys':
            raise ValueError('Actual unmodified original X11 input required')
    elif meta['input']!='dd2_key_event' or len(meta.get('binary_sha256',''))!=64 or 'api_inputs' not in meta:
        raise ValueError('Native keyboard bridge and strict API replay required')
    events=[json.loads(line) for line in (root/'events.jsonl').read_text().splitlines()]
    if [row['index'] for row in events]!=list(range(1,len(events)+1)):
        raise ValueError('Incomplete or reordered original observation stream')
    if [(row['key'],row['down'],row['action']) for row in events if row['kind']=='key']!=navigation_inputs(meta):
        raise ValueError('Actual menu/driving key sequence differs')
    validate_driving_inputs(meta,events)
    random,ticks=[(root/name).read_bytes() for name in ('random.bin','ticks.bin')]
    recorded_apis(meta,events,random,ticks)
    natural(meta['final_race'])
    driver=meta['final_race']['driver']
    if driver['dead']!=1 and driver['finished_laps']!=1:
        raise ValueError('Destroyed player or actual completed laps required')
    frames=meta['race_frames']
    if not frames or [row['index'] for row in frames]!=list(range(len(frames))):
        raise ValueError('Complete chronological racing picture extent required')
    if frames[0]['level']!=1 or frames[0]['cf']!=0 or frames[0]['ticks']!=2:
        raise ValueError('Capture must start at the first actual player race picture')
    if any(row['retired'] for row in frames) or [row['level'] for row in frames[-2:]]!=[2,2]:
        raise ValueError('Natural completion followed by actual second race required')
    validate_racing_timeline(meta,events,target)
    held=set();previous=-1
    for row in meta['driving_inputs']:
        frame=row['frame'];key=row['key']
        if frame<previous or not 0<=frame<len(frames)-2 or key not in ('a','z','Left','Right'):
            raise ValueError('Invalid driving presentation/key')
        if row['down']:
            if key in held:raise ValueError('Duplicate original key press')
            held.add(key)
        else:
            if key not in held:raise ValueError('Unmatched original key release')
            held.remove(key)
        previous=frame
    if held:raise ValueError('All driving keys must be released before results')
    return meta,random,ticks


def completion_requirements(driver, before, result, completed_laps=False, player_points=False):
    if completed_laps and (driver['finished_laps']!=1 or driver['dead']!=0):
        raise ValueError('Regular finish with completed laps and a surviving player required')
    a,b=before['cars'][0],result['cars'][0]
    if a['name']!=b['name'] or b['values'][0]!=a['values'][0]+b['values'][6]:
        raise ValueError('Player identity or cumulative race score differs')
    if player_points and b['values'][6]<=0:
        raise ValueError('Positive points earned by the actual player required')
    return dict(name=b['name'],points_before=a['values'][0],race_points=b['values'][6],
                points_after=b['values'][0])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('original','native','browser','report'):parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--clean',action='store_true')
    parser.add_argument('--require-completed-laps',action='store_true',help='reject destruction or an unfinished player')
    parser.add_argument('--require-player-points',action='store_true',help='reject a player who earned zero race points')
    args=parser.parse_args()
    for name in ('original','native','browser','report'):
        path=getattr(args,name).resolve()
        if WORK not in path.parents:parser.error('Use /tmp/wasm-dd2/ for captures and reports')
        setattr(args,name,path)
    prepare_output(args.report)
    om,random,ticks=history(args.original,'original')
    nm,nr,nt=history(args.native,'native')
    if nr!=random or nt!=ticks or nm['initial_save_sha256']!=om['initial_save_sha256'] or nm['driving_inputs']!=om['driving_inputs']:
        raise ValueError('Native input/save provenance differs')
    if any(nm['final_race'][key]!=om['final_race'][key] for key in (*STATE,'driver')):
        raise ValueError('Native actual natural finish differs')
    nav=load(args.browser/'navigation.json');observer=load(args.browser/'rng-observations.json')
    if (nav.get('final_driver') is not None or args.require_completed_laps) and nav.get('final_driver')!=om['final_race']['driver']:
        raise ValueError('Browser observed actual player finish differs')
    original_before,original_result=[load(args.original/NAMES[i]/'checkpoint.json') for i in (10,11)]
    player_result=completion_requirements(om['final_race']['driver'],original_before,original_result,
                                          args.require_completed_laps,args.require_player_points)
    api=dict(clock_calls=om['clock_calls'],computed_rng_calls=om['rng_calls'],api_sha256=dict(clock=digest(ticks),random=digest(random)))
    if (nav.get('natural_championship') is not True or nav['keys']!=NATURAL_CHAMP_KEYS
            or nav['actions']!=NATURAL_CHAMP_ACTIONS or nav['checkpoints']!=NAMES
            or nav['input']!='browser keyboard events' or nav['initial_save_sha256']!=om['initial_save_sha256']
            or nav['api_reference']!=api or nav['wasm_sha256']!=observer['layout']['wasm_sha256']
            or [(row['code'],row['down'],row['action']) for row in nav['input_observations']]
               !=[(CODES[key],down,action) for key,down,action in navigation_inputs(om)]):
        raise ValueError('Browser input/API/save provenance differs')
    actual_drive=[row for row in nav['input_observations'] if row['action']==11]
    if [(row['frame'],row['finish']) for row in actual_drive]!=[(row['frame'],bool(row.get('finish'))) for row in om['driving_inputs']]:
        raise ValueError('Browser driving transitions moved to another presentation')
    seeds=[1,*[row[1] for row in struct.iter_unpack('<III',random)]]
    previous=0
    for row in observer['observations']:
        count=row['count']
        if count<previous or count>=len(seeds) or row['seed']!=seeds[count]:raise ValueError('Browser observed LCG differs')
        previous=count
    end=load(args.browser/NAMES[-1]/'checkpoint.json')
    if end['api_calls']!={'clock':om['clock_calls'],'random':om['rng_calls']} or end['rng']!={'count':om['rng_calls'],'seed':seeds[-1]}:
        raise ValueError('Browser final API extent differs')
    report=dict(scope=__doc__.strip(),original_exe_sha256=EXE,initial_save_sha256=om['initial_save_sha256'],
                native_sha256=nm['binary_sha256'],wasm_sha256=nav['wasm_sha256'],api_inputs=api,
                racing_frames=len(om['race_frames']),natural_finish=True,retired=False,
                final_driver=om['final_race']['driver'],browser_final_driver=nav.get('final_driver'),
                player_result=player_result,
                required_completion=dict(completed_laps=args.require_completed_laps,positive_player_points=args.require_player_points),
                field_notes={'damage':'Historical recorder field at 0x792a76 is planar speed magnitude, not body damage; new driver metrics use planar_speed.'},
                ending='destroyed' if om['final_race']['driver']['dead']==1 else 'completed_laps',targets={})
    for target,root,frames in [('native',args.native,nm['race_frames']),('browser',args.browser,load(args.browser/'race-frames.json'))]:
        pad_offset=nm['race_frames'][0]['pad_polls']-om['race_frames'][0]['pad_polls'] if target=='native' else 0
        if pad_offset not in (-1,0,1):raise ValueError('Unexpected debugger poll observation origin')
        differences,hashes=compare_frames(args.original,root,om['race_frames'],frames,seeds,target=='browser',pad_offset=pad_offset)
        point_diff,images=compare_checkpoints(args.original,root,NAMES,IMAGE_NAMES);differences.extend(point_diff)
        points=[load(root/name/'checkpoint.json') for name in NAMES]
        before,result,started=points[10],points[11],points[21]
        if completion_requirements(om['final_race']['driver'],before,result,args.require_completed_laps,args.require_player_points)!=player_result:
            raise ValueError('Port player score differs from original')
        if (result['level'],result['race'],result['poly_list'],result['stats'],result['retire_confirm'])!=(15,1,0x46bf38,1,0):
            raise ValueError('First natural championship result was not reached')
        for a,b in zip(before['cars'],result['cars']):
            if b['values'][0]!=a['values'][0]+b['values'][6]:raise ValueError('Cumulative points do not include the actual race')
        for division in range(4):
            point=points[13+division]
            if point['poly_list']!=0x46ab30 or point['division']!=division:raise ValueError('Missing actual league page')
            for rank,row in enumerate(point['rows']):
                car=next((car for car in point['cars'] if car['values'][1:3]==[division,rank]),None)
                if not car or row!={'name':'%R%JL%T/'+car['name'],'points':'%R%JL%T/'+str(car['values'][0])}:
                    raise ValueError('League row is not the classified driver and cumulative score')
        if (started['level'],started['race'],started['quit'])!=(2,1,0) or started['ticks']<=0:raise ValueError('Next race did not actually start')
        report['targets'][target]=dict(pass_=not differences,differences=differences,racing_image_hashes=hashes,
                                      checkpoint_image_hashes=images,debugger_pad_origin_offset=pad_offset)
    report['pass_']=all(row['pass_'] for row in report['targets'].values())
    sample=picture(args.original,om['race_frames'][1]['prefix'],'.bin',307200)
    negatives=dict(changed_pixel=not exact_bytes(sample,bytes([sample[0]^1])+sample[1:],307200))
    for key,driver,before,result in [
        ('destroyed_regular_finish',dict(om['final_race']['driver'],dead=1),original_before,original_result),
        ('incomplete_laps',dict(om['final_race']['driver'],dead=0,finished_laps=0),original_before,original_result),
        ('zero_player_points',dict(om['final_race']['driver'],dead=0,finished_laps=1),copy.deepcopy(original_before),copy.deepcopy(original_result))]:
        if key=='zero_player_points':
            result['cars'][0]['values'][6]=0;result['cars'][0]['values'][0]=before['cars'][0]['values'][0]
        try:completion_requirements(driver,before,result,True,True)
        except ValueError:negatives[key]=True
        else:negatives[key]=False
    moved=copy.deepcopy(om);moved['driving_inputs'][0]['frame']+=1
    original_events=[json.loads(line) for line in (args.original/'events.jsonl').read_text().splitlines()]
    try:validate_driving_inputs(moved,original_events)
    except ValueError:negatives['moved_driving_transition']=True
    else:negatives['moved_driving_transition']=False
    for key,state in [('retirement',dict(om['final_race'],retired=1)),('unfinished',dict(om['final_race'],finished=14))]:
        try:natural(state)
        except ValueError:negatives[key]=True
        else:negatives[key]=False
    with tempfile.TemporaryDirectory(prefix='natural-champ-invalid-',dir=WORK) as temp:
        root=Path(temp)
        for key,packed in [('truncated_zlib',zlib.compress(sample)[:-1]),('trailing_zlib',zlib.compress(sample)+b'X')]:
            (root/'sample.bin.z').write_bytes(packed)
            try:picture(root,'sample','.bin',307200)
            except (ValueError,zlib.error):negatives[key]=True
            else:negatives[key]=False
    report['negative_cases']=negatives
    if not all(negatives.values()):raise ValueError('Verifier accepted invalid capture evidence')
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS' if report['pass_'] else 'FAIL','natural championship:',report['racing_frames'],'racing pictures, result, four leagues, next actual race')
    if args.clean and report['pass_']:
        opened=open_files();files=[]
        for root in (args.original,args.native,args.browser):
            for pattern in ('race*.bin.z','race*.pal','step*/framebuf.bin','step*/palette.bin'):
                for path in root.glob(pattern):
                    stat=path.stat()
                    if path.is_symlink() or (stat.st_dev,stat.st_ino) in opened:raise RuntimeError('Do not delete captures used by running processes')
                    files.append(path)
        for path in files:path.unlink()
    return 0 if report['pass_'] else 1


if __name__=='__main__':
    raise SystemExit(main())
