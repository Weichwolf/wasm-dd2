#!/usr/bin/env python3
"""Compare actual result-menu histories with observed original clock/RNG inputs."""
import argparse
import copy
import json
from pathlib import Path
import struct

from artifacts import WORK, open_files, prepare_output
from race_results_protocol import KEYS, STARTS, TABLES, OVERS, PLAN
from verify_championship_save import fixture
from verify_champ_history import recorded_apis
from verify_configuration_card_ui import match_frame
from verify_configuration_persistence import EXE_SHA256, digest, require

NAMES = [f'step{i:02d}-{key}' for i,key in enumerate(['boot',*KEYS])]
FIELDS = ['level','cf','ticks','quit','poly_list','race_type','race_mode','race','season',
          'num_races','division','stats','actual_season','retire_confirm','cars','saved_state']


def names_and_points(state):
    league = bytes.fromhex(state['league'])
    result = [None]*20
    for index in range(20):
        row = league[index*54:(index+1)*54]
        rank, points = struct.unpack_from('<hh', row, 26)
        require(1 <= rank <= 20 and result[rank-1] is None, 'unique actual race ranks required')
        result[rank-1] = dict(name='%R%JL%T/'+row[:16].split(b'\0')[0].decode('ascii'),
                              points='%R%JL%T/'+str(points))
    return result



def frames(directory, browser):
    directory = Path(directory)
    meta = json.loads((directory/'cycle.json').read_text())
    require(meta['stage'] == ('browser platform present' if browser else 'Draw_All entry / pending presentation'),
            'wrong presentation observation boundary')
    rows = meta['frames']
    require(len(rows) == 64 and {r['phase'] for r in rows} == set(range(64)) and
            {r['level'] for r in rows} == {15} and {r['poly_list'] for r in rows} == {0x46b890} and
            len({r['cf'] for r in rows}) == 1, 'complete settled actual result cycle required')
    result = {}
    for row in rows:
        if browser: require(row['canvas_mismatches'] == 0, 'actual canvas conversion differs')
        pair = [(directory/(row['prefix']+'-'+region+'.bin')).read_bytes() for region in ['framebuf','palette']]
        require([len(p) for p in pair] == [307200,1024], 'complete framebuffer/palette required')
        result[row['phase']] = row,pair
    return result


def load_history(root, target, initial):
    wrapper = json.loads((root/'report.json').read_text())
    require(wrapper['pass_'] and not wrapper['engine_state_writes'] and wrapper['target']==target and
            wrapper['initial_card_sha256']==digest(initial), 'completed actual history required')
    directory = root/'history'; meta = json.loads((directory/'history.json').read_text())
    require(meta['result_tables'] and meta['keys']==KEYS and meta['checkpoints']==NAMES and
            meta['initial_save_sha256']==digest(initial) and meta['acknowledged_keys'] and
            len(meta['held_pad_polls'])==len(KEYS) and min(meta['held_pad_polls'])>=1 and
            meta['binary_sha256']==wrapper['binary_sha256'], 'complete current result route required')
    if target=='original':
        require(meta['exe_modified'] is False and meta['exe_sha256']==EXE_SHA256 and
                meta['input']=='real X11 keys', 'unmodified original X11 history required')
    else: require(meta['input']=='dd2_key_event', 'native actual keyboard bridge required')
    events = [json.loads(line) for line in (directory/'events.jsonl').read_text().splitlines()]
    require([row['index'] for row in events]==list(range(1,len(events)+1)), 'reordered or incomplete observation stream')
    inputs = [row for row in events if row['kind']=='key']
    require([(row['key'],row['down'],row['action']) for row in inputs]==
            [(key,down,i) for i,key in enumerate(KEYS,1) for down in (True,False)], 'actual input sequence differs')
    random,ticks = [(directory/name).read_bytes() for name in ['random.bin','ticks.bin']]
    recorded_apis(meta,events,random,ticks)
    points = [json.loads((directory/name/'checkpoint.json').read_text()) for name in NAMES]
    return dict(wrapper=wrapper,meta=meta,points=points),random,ticks


def match_points(source, actual, browser=False):
    require(len(actual)==52, 'all 52 actual checkpoints required')
    for index,(left,right) in enumerate(zip(source,actual)):
        require(all(left[key]==right[key] for key in FIELDS), 'actual result history state differs at '+str(index))
        expected = dict(clock=left['observed']['clock_calls'],random=left['observed']['rng_calls'])
        if browser:
            require(right['canvas_mismatches']==0 and right['api_calls']==expected and
                    right['rng']['count']==expected['random'], 'actual canvas or API extent differs')
        else:
            require(all(right['observed'][key]==left['observed'][key] for key in ['clock_calls','rng_calls']),
                    'actual native API extent differs')
        if index in TABLES:
            require(right['level']==15 and right['poly_list']==0x46b890 and right['rectangle']==PLAN['rectangle'] and
                    right['result_rows']==names_and_points(right['saved_state'])==left['result_rows'],
                    'displayed result ranks/points or rectangle differ')
    require((actual[-1]['level'],actual[-1]['poly_list'],actual[-1]['race'],actual[-1]['stats'])==(0,0x4696b0,4,1),
            'actual season elimination/title return required')


def compare(args):
    initial,_ = fixture(args.fixture)
    original,random,ticks = load_history(args.original,'original',initial)
    targets,negative,paths = {},[],set()
    for target,directory in [('native',args.native),('native-asan',args.asan),('browser',args.browser),('native-before',args.before)]:
        browser = target=='browser'
        if browser:
            nav = json.loads((directory/'navigation.json').read_text())
            observer = json.loads((directory/'rng-observations.json').read_text())
            require(nav['keys']==KEYS and nav['checkpoints']==NAMES and nav['result_tables'] and nav['input_synthetic'] and
                    nav['input']=='browser keyboard events' and nav['initial_save_sha256']==digest(initial) and
                    nav['wasm_sha256']==observer['layout']['wasm_sha256'], 'actual browser route/file/binary differs')
            require(nav['api_reference']==dict(clock_calls=original['meta']['clock_calls'],computed_rng_calls=original['meta']['rng_calls'],
                    api_sha256=dict(clock=digest(ticks),random=digest(random))), 'browser original API streams differ')
            codes=dict(Return='Enter',Escape='Escape',Up='ArrowUp',Down='ArrowDown',Left='ArrowLeft',Right='ArrowRight')
            require([(e['action'],e['code'],e['down']) for e in nav['input_observations']]==
                    [(i,codes[key],down) for i,key in enumerate(KEYS,1) for down in (True,False)], 'browser actual DOM input differs')
            seeds=[1]+[after for _,after,_ in struct.iter_unpack('<III',random)]
            observations=observer['observations']
            require(observations and observations[0]['count']==0 and observations[0]['seed']==1, 'actual initial browser RNG required')
            previous=0
            for row in observations:
                require(previous<=row['count']<len(seeds) and row['seed']==seeds[row['count']], 'calculated browser RNG differs')
                previous=row['count']
            points=[json.loads((directory/name/'checkpoint.json').read_text()) for name in NAMES]
            data=dict(navigation=nav,observer=observer,points=points)
            binary=nav['wasm_sha256']; root=directory
        else:
            data,r,t=load_history(directory,'native',initial)
            require((r,t)==(random,ticks), 'calculated native RNG or observed clock stream differs')
            points=data['points'];binary=data['meta']['binary_sha256'];root=directory/'history'
        match_points(original['points'],points,browser)
        if target=='native-before':
            require(binary!=targets['native']['binary_sha256'], 'distinct actual old executable required')
        hashes=[]
        for step in TABLES:
            name=NAMES[step];a=frames(args.original/'history'/name/'cycle',False);b=frames(root/name/'cycle',browser)
            paths.update([args.original/'history'/name/'cycle',root/name/'cycle'])
            for phase in range(64):
                require(all(a[phase][0][key]==b[phase][0][key] for key in ['cf','ticks','level','race','season']),
                        'aligned result cycle state differs')
                if target=='native-before':
                    require(a[phase][1][1]==b[phase][1][1], 'old palette differs')
                    try: match_frame(*a[phase],*b[phase],False)
                    except RuntimeError: pass
                    else: raise RuntimeError('actual old background accepted')
                else: match_frame(*a[phase],*b[phase],False)
                hashes.append(dict(checkpoint=name,phase=phase,framebuffer_sha256=digest(b[phase][1][0]),palette_sha256=digest(b[phase][1][1])))
            if target=='native-before':negative.append(dict(target=target,case=name+'-actual-old-background-64-frames'));continue
            for region in range(2):
                bad=[bytearray(p) for p in b[0][1]];bad[region][0]^=1
                try: match_frame(*a[0],b[0][0],bad,False)
                except RuntimeError: negative.append(dict(target=target,case=name+'-'+str(region)))
                else: raise RuntimeError('altered result output accepted')
        if target!='native-before':
            for field in ['name','points','clock','random']:
                changed=copy.deepcopy(points)
                if field in ['name','points']:changed[TABLES[0]]['result_rows'][0][field]+='X'
                elif browser:changed[TABLES[0]]['api_calls'][field]+=1
                else:changed[TABLES[0]]['observed']['clock_calls' if field=='clock' else 'rng_calls']+=1
                try:match_points(original['points'],changed,browser)
                except RuntimeError:negative.append(dict(target=target,case=field))
                else:raise RuntimeError('altered actual result/API state accepted')
        targets[target]=dict(pass_=True,binary_sha256=binary,capture=data,frames=hashes,
                            method='actual old regression rejected' if target=='native-before' else 'literal complete indexed/palette comparison')
    report=dict(scope=PLAN['scope'],pass_=True,original=original,targets=targets,negative_cases=negative,
                checked_states=52,frames_per_target=384,clock_calls=original['meta']['clock_calls'],rng_calls=original['meta']['rng_calls'])
    destination=prepare_output(args.report);require(WORK in destination.parents,'report belongs in /tmp/wasm-dd2')
    destination.write_text(json.dumps(report,indent=2)+'\n')
    if args.clean:
        opened=open_files()
        for path in paths:
            require(WORK in path.resolve().parents and not path.is_symlink(),'unexpected raw cycle path')
            for raw in path.glob('*.bin'):
                st=raw.stat();require((st.st_dev,st.st_ino) not in opened,'capture is still open');raw.unlink()
        for directory in [args.original/'history',args.native/'history',args.asan/'history',args.before/'history',args.browser]:
            for raw in directory.glob('step*/*.bin'):
                require(not raw.is_symlink(),'unexpected raw image symlink');st=raw.stat()
                require((st.st_dev,st.st_ino) not in opened,'image is still open');raw.unlink()
    print('Actual aligned result menus: 52 states, 384 exact frames per target;',len(negative),'negative checks rejected',flush=True)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ['fixture','original','native','asan','browser','before','report']:parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--clean',action='store_true');args=parser.parse_args()
    for name in ['original','native','asan','browser','before','report']:
        require(WORK in getattr(args,name).resolve().parents,'all captures/reports belong in /tmp/wasm-dd2')
    compare(args)


if __name__=='__main__':main()
