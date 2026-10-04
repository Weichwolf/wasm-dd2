#!/usr/bin/env python3
"""Save a real original replay and load the same file through original/port menus.

Only actual keyboard input, normal startup and read-only observations are used.
The complete packed metadata, script and car order must match the original live
recording. Loading must apply recorded acceleration, end naturally and restore
frontend choices. Selected complete card-menu cycles are compared separately.
This does not prove chronological racing video/PCM or every replay scenario.
"""
import argparse
import copy
import json
from pathlib import Path
import struct
import time

from artifacts import WORK, check_space, open_files, prepare_output
from verify_championship_save import setup, execute
from verify_configuration_persistence import ROOT, EXE_SHA256, digest, require, settings
from verify_configuration_card_ui import file_ui, rendered_cycle, cycle_frames, match_frame

HEADER = struct.Struct('<HhIhhhhh')
SCRIPT = 0x1c00
PACKED = HEADER.size + SCRIPT + 20
POINTS = ('manager', 'selected')
SCOPE = __doc__


def metadata(ui):
    return {key:ui.integer(address) for key,address in dict(car=0x467400,mode=0x4673f8,
        type=0x4673f4,season=0x93dec0,level=0x9392bc,pad=0x467078,end=0x9392c4).items()}


def pack(state, script, order):
    return HEADER.pack(0x2020, *(state[k] for k in ['car','end','mode','type','season','level','pad'])) + script + order


def fixture(directory):
    producer = json.loads((directory/'report.json').read_text())
    require(producer['operation']=='generate' and producer['pass_'] and
            producer['binary_sha256']==EXE_SHA256 and not producer['engine_state_writes'],
            'completed actual original replay producer required')
    raw = (directory/'original.card').read_bytes()
    require(len(raw)==0x20000 and digest(raw)==producer['card_sha256'], 'replay card changed')
    require(raw[:6]==b'\x01\0\0\0A\0' and
            all(struct.unpack_from('<I',raw,i*0x200)[0]==0 for i in range(1,15)),
            'one named replay entry required')
    payload = raw[0x2000:0x2000+PACKED]
    script, order = bytes.fromhex(producer['script']),bytes.fromhex(producer['order'])
    state = producer['recorded']
    require(payload==pack(state,script,order), 'saved replay differs from original live recording')
    require((state['car'],state['mode'],state['type'],state['season'],state['level'])==(1,1,0,0,1),
            'actual car 1 / Stock Car practice on first track required')
    require(state['pad'] in (0,1) and 0x9376b0<state['end']<0x9376b0+SCRIPT and
            (state['end']-0x9376b0)%2==0 and sorted(order)==list(range(20)),
            'valid nontrivial recorded tape/order required')
    end = state['end']-0x9376b0
    require(struct.unpack_from('<H',script,end)[0]==(0xa001 if state['pad']==1 else 0x8080),
            'original replay terminal differs')
    events = struct.unpack('<'+'H'*(end//2),script[:end])
    require(any(e & (0x1000 if state['pad']==1 else 0x80) for e in events),
            'genuine recorded acceleration required')
    return raw, payload, producer


def generate(args):
    args.target='original'
    out,game = setup(args,(ROOT/'DestructionDerby2/SaveGames').read_bytes())
    report=dict(scope=SCOPE,operation='generate',pass_=False,engine_state_writes=False,
                binary_sha256=EXE_SHA256,input_keys=[])
    def action(ui):
        require(all(struct.unpack_from('<I',(ui.game/'SaveGames').read_bytes(),i*0x200)[0]==0
                    for i in range(15)), 'empty provisioned card required')
        # Select nonzero car, then Stock Car / Practice through genuine menus.
        for code in ['Right','Return','Right','Return','Left','Return','Right',
                     'Return','Right','Return','Down','Down']:
            ui.key(code);report['input_keys'].append(code)
        require((ui.integer(0x467400),ui.integer(0x4673f8),ui.integer(0x4673f4))==(1,1,0)
                and 'Go!' in ui.text(0x46975c),'selected actual Stock Car practice required')
        ui.key('Return');report['input_keys'].append('Return')
        ui.wait(lambda: ui.integer(0x7746c0)>=8 and ui.integer(0x7746ac)==0 and
                ui.integer(0x936ff4)==1,60)
        ui.wait(lambda: ui.integer(0x784298)<1,60)
        ui.edge('a',True)
        try:
            ui.wait(lambda:ui.integer(0x792a86)>0)
            report['acceleration']={'seconds':3,'start_tick':ui.integer(0x7746c0)}
            time.sleep(3)
            report['acceleration']['end_tick']=ui.integer(0x7746c0)
        finally:ui.edge('a',False)
        for code in ['Escape','Down','Down','Down','Return','Up','Return']:
            ui.key(code);report['input_keys'].append(code)
        ui.wait(lambda:'View Replay' in ui.text(0x46a508) and ui.integer(0x7746ac)==1)
        report['recorded']=metadata(ui)
        script,order=ui.read(0x9376b0,SCRIPT),ui.read(0x795c28,20)
        report['script'],report['order']=script.hex(),order.hex()
        ui.key('Right');require('Save Replay' in ui.text(0x46a508),'actual Save Replay required')
        ui.key('Return');ui.wait(lambda:ui.integer(0x93a318)==5)
        require(ui.read(0x93a490,PACKED)==pack(report['recorded'],script,order), 'packed replay differs')
        ui.key('Return');ui.wait(lambda:'Select' in ui.text(0x46725c) and ui.integer(0x774680)==0)
        ui.key('Return');ui.enter_name('A')
        ui.wait(lambda:(ui.game/'SaveGames').read_bytes()[0]==1)
        raw=(ui.game/'SaveGames').read_bytes()
        require(raw==ui.read(0x754460,len(raw)),'original replay card disk/RAM differ')
        require(raw[0x2000:0x2000+PACKED]==pack(report['recorded'],script,order),'saved tape differs')
        (out/'original.card').write_bytes(raw)
        report.update(card_sha256=digest(raw),payload_sha256=digest(raw[0x2000:0x2000+PACKED]),pass_=True)
    try:execute(args,out,game,action)
    finally:
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(out)
    fixture(out)
    print('Actual original replay recording/saving: PASS',flush=True)


def validate(data,payload):
    require(data['pass_'] and not data['engine_state_writes'],'completed read-only replay capture required')
    magic,car,end,mode,race_type,season,level,pad=HEADER.unpack(payload[:18])
    require(magic==0x2020 and (data['initial']['car'],data['initial']['mode'],data['initial']['type'])==(0,0,0),
            'replay incorrectly auto-loaded as configuration')
    require([p['name'] for p in data['checkpoints']]==list(POINTS),'incomplete replay menu capture')
    require(bytes.fromhex(data['loaded_script'])==payload[18:18+SCRIPT] and
            bytes.fromhex(data['loaded_order'])==payload[18+SCRIPT:],'loaded tape/order differ')
    rows=data['playback']
    require(len(rows)>=10 and len({r['tick'] for r in rows})>=10 and any(r['pedal']>0 for r in rows),
            'actual replay physics with recorded acceleration required')
    for row in rows:
        require((row['car'],row['end'],row['mode'],row['type'],row['season'],row['level'],row['pad'])==
                (car,end,mode,race_type,season,level,pad) and row['actual_level']==level and
                row['replay']==1 and row['quit']==0 and row['cars']==20,'live replay metadata differs')
        # The original initializes the decoder one WORD before the first event.
        require(row['countdown']<1 and 0x9376ae<=row['script_cursor']<=end and row['script_cursor']%2==0,
                'actual tape decoding after the countdown required')
    require(data['completion']==dict(script_cursor=end,first_time=1),
            'replay returned before decoding the tape terminal')
    require(data['natural_end'] and data['restored']==data['initial'] and data['final_level']==0 and
            'File Manager' in data['final_label'],'replay did not naturally restore the frontend')
    require(data['card_unchanged'] and data['engine_matches_file'],'loading changed original replay card')
    if data['target']=='browser':
        require(data['trusted_keyboard_events'] and all(e['trusted'] for e in data['trusted_keyboard_events']),
                'trusted browser keyboard input required')


def capture(args):
    initial,payload,producer=fixture(args.fixture)
    out,game=setup(args,initial)
    report=dict(scope=SCOPE,operation='capture',target=args.target,pass_=False,engine_state_writes=False,
        initial_save_sha256=digest(initial),checkpoints=[],playback=[],
        binary_sha256=EXE_SHA256 if args.target=='original' else digest(args.binary.read_bytes()))
    source=json.loads((args.reference/'report.json').read_text()) if args.reference else None
    if source:validate(source,payload);require(source['binary_sha256']==EXE_SHA256,'original reference required')
    def checkpoint(ui,name):
        ui.settled()
        row=dict(name=name,file_ui=file_ui(ui),menu=ui.integer(0x940010),
                 file_mode=ui.integer(0x93a318),file_slot=ui.integer(0x774680))
        wanted=None
        if source:
            saved=next(row for row in source['checkpoints'] if row['name']==name)
            frames=json.loads((Path(saved['cycle'])/'cycle.json').read_text())['frames']
            wanted=[(f['phase'],f['card_phase']) for f in frames]
        row['cycle']=rendered_cycle(ui,{'checkpoint':name},args.target=='original',wanted)
        report['checkpoints'].append(row)
    def action(ui):
        report['initial']=settings(ui)
        for code in ['Right','Right','Right','Return']:ui.key(code)
        checkpoint(ui,'manager');ui.key('Return')
        ui.wait(lambda:'Select' in ui.text(0x46725c) and ui.integer(0x774680)==0)
        checkpoint(ui,'selected');ui.key('Return')
        ui.wait(lambda:ui.integer(0x467074)==1 and ui.integer(0x7746ac)==0 and ui.integer(0x7746c0)>0,60)
        report['loaded_script']=ui.read(0x9376b0,SCRIPT).hex()
        report['loaded_order']=ui.read(0x795c28,20).hex()
        deadline=time.monotonic()+60;seen=set()
        while ui.integer(0x467074)==1 and time.monotonic()<deadline:
            check_space(out)
            tick=ui.integer(0x7746c0)
            row={**metadata(ui),'tick':tick,'pedal':ui.integer(0x792a86),
                 'actual_level':ui.integer(0x936ff4),'cars':ui.integer(0x46765c),
                 'countdown':ui.integer(0x784298),'script_cursor':ui.integer(0x9392b4),
                 'replay':ui.integer(0x467074),'quit':ui.integer(0x7746ac)}
            # Reject torn transitions rather than recording frontend loading as gameplay.
            if (tick==ui.integer(0x7746c0) and row['replay']==1 and row['quit']==0 and
                    row['actual_level']==producer['recorded']['level'] and row['countdown']<1 and tick not in seen):
                seen.add(tick);report['playback'].append(row)
            time.sleep(.005)
        report['natural_end']=ui.integer(0x467074)==0 and ui.integer(0x7746ac)==1
        report['completion']=dict(script_cursor=ui.integer(0x9392b4),first_time=ui.integer(0x9392b0))
        ui.wait(lambda:ui.integer(0x936ff4)==0 and 'File Manager' in ui.text(0x46975c) and
                settings(ui)==report['initial'],30)
        report['restored']=settings(ui);report['final_level']=ui.integer(0x936ff4)
        report['final_label']=ui.text(0x46975c)
        raw=(ui.game/'SaveGames').read_bytes()
        report['card_unchanged']=raw==initial;report['engine_matches_file']=raw==ui.read(0x754460,len(raw))
        validate({**report,'pass_':True},payload);report['pass_']=True
    try:execute(args,out,game,action)
    finally:
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(out)
    print('Actual '+args.target+' original-file replay loading/playback: PASS',flush=True)


def compare(args):
    initial,payload,producer=fixture(args.fixture)
    source=json.loads((args.original/'report.json').read_text());validate(source,payload)
    require(source['target']=='original' and source['binary_sha256']==EXE_SHA256,'actual original reference required')
    results,negatives,paths={},[],[]
    for target,directory in [('native',args.native),('asan',args.asan),('browser',args.browser)]:
        if directory is None:continue
        actual=json.loads((directory/'report.json').read_text());validate(actual,payload)
        require(actual['target']==('browser' if target=='browser' else 'native'),'wrong capture target')
        require(actual['initial_save_sha256']==source['initial_save_sha256']==digest(initial) and
                actual['initial']==source['initial'] and actual['restored']==source['restored'],
                'original file input/frontend settings differ')
        frames=[]
        for left,right in zip(source['checkpoints'],actual['checkpoints']):
            require({k:v for k,v in left.items() if k!='cycle'}=={k:v for k,v in right.items() if k!='cycle'},
                    'actual replay card UI state differs')
            a,b=cycle_frames(left['cycle'],False),cycle_frames(right['cycle'],target=='browser')
            for phase in range(64):
                match_frame(*a[phase],*b[phase],True)
                frames.append(dict(checkpoint=right['name'],phase=phase,
                    framebuffer_sha256=digest(b[phase][1][0]),palette_sha256=digest(b[phase][1][1])))
            paths.extend([Path(left['cycle']),Path(right['cycle'])])
            for region in range(2):
                altered=[bytearray(v) for v in b[0][1]];altered[region][0]^=1
                try:match_frame(*a[0],b[0][0],altered,True)
                except RuntimeError:negatives.append(dict(target=target,case=right['name']+'-'+str(region)))
                else:raise RuntimeError('altered image accepted')
        for field in ['car','end','mode','type','season','level','pad','actual_level','cars','replay','quit']:
            changed=copy.deepcopy(actual);changed['playback'][0][field]+=1
            try:validate(changed,payload)
            except RuntimeError:negatives.append(dict(target=target,case='playback-'+field))
            else:raise RuntimeError('altered replay metadata accepted')
        for field in ['loaded_script','loaded_order','natural_end','restored','card_unchanged','engine_matches_file']:
            changed=copy.deepcopy(actual)
            if isinstance(changed[field],str):changed[field]=('00' if changed[field][:2]!='00' else '01')+changed[field][2:]
            elif isinstance(changed[field],dict):changed[field]['car']+=1
            else:changed[field]=False
            try:validate(changed,payload)
            except RuntimeError:negatives.append(dict(target=target,case=field))
            else:raise RuntimeError('altered replay state accepted')
        for field in ['script_cursor','first_time']:
            changed=copy.deepcopy(actual);changed['completion'][field]+=1
            try:validate(changed,payload)
            except RuntimeError:negatives.append(dict(target=target,case='completion-'+field))
            else:raise RuntimeError('altered replay completion accepted')
        for field,value in [('countdown',1),('script_cursor',0x9376b0+SCRIPT)]:
            changed=copy.deepcopy(actual);changed['playback'][0][field]=value
            try:validate(changed,payload)
            except RuntimeError:negatives.append(dict(target=target,case='decoder-'+field))
            else:raise RuntimeError('altered replay decoder accepted')
        changed=copy.deepcopy(actual)
        for row in changed['playback']:row['pedal']=0
        try:validate(changed,payload)
        except RuntimeError:negatives.append(dict(target=target,case='missing-recorded-acceleration'))
        else:raise RuntimeError('missing recorded acceleration accepted')
        results[target]=dict(pass_=True,capture=actual,frames=frames)
    report=dict(scope=SCOPE,pass_=True,fixture=producer,original=source,targets=results,negative_cases=negatives)
    path=prepare_output(args.report);require(WORK in path.parents and not path.exists(),'fresh report required')
    path.write_text(json.dumps(report,indent=2)+'\n')
    if args.clean:
        opened=open_files()
        for directory in set(paths):
            for file in directory.glob('*.bin'):
                stat=file.stat();require((stat.st_dev,stat.st_ino) not in opened,'capture still in use');file.unlink()
    print('Actual original-file replay comparison: PASS;',len(results)*128,
          'complete menu frame/palette pairs;',len(negatives),'negative cases rejected')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest='operation',required=True)
    gen=sub.add_parser('generate');gen.add_argument('--output',type=Path,required=True)
    cap=sub.add_parser('capture');cap.add_argument('--target',choices=['original','native'],required=True)
    cap.add_argument('--binary',type=Path,default=Path('/tmp/dd2_native'))
    cap.add_argument('--output',type=Path,required=True);cap.add_argument('--reference',type=Path)
    cmp=sub.add_parser('compare')
    for arg in ['original','native','browser','report']:cmp.add_argument('--'+arg,type=Path,required=True)
    cmp.add_argument('--asan',type=Path);cmp.add_argument('--clean',action='store_true')
    for command in [cap,cmp]:command.add_argument('--fixture',type=Path,required=True)
    args=parser.parse_args();{'generate':generate,'capture':capture,'compare':compare}[args.operation](args)


if __name__=='__main__':main()
