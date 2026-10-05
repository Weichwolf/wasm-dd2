#!/usr/bin/env python3
"""Actual driver-name UI checks; selected menu frames, not full original A/V."""
import argparse
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from artifacts import check_space
from driver_name_input import enter_driver_name, name_actions
from verify_championship_save import setup, execute, snapshot
from verify_configuration_persistence import ROOT, EXE_SHA256, digest, require
from verify_configuration_card_ui import rendered_cycle


def input_plan():
    return {name: name_actions(value, **options) for name,value,options in [
        ('limit','MACSrPOOQ',{}), ('deletion','Az09!?-.',{'delete_last':True}),
        ('cancel','Cancel',{'confirm':False}), ('default','',{})]}


def checkpoints():
    plan=input_plan()
    groups=[
        ('wrong-file-grid',['Return']*3+['Right']*3+['Return','Down','Down','Right','Return'],'Dg',True),
        ('empty-grid',['Escape']*3+['Return']*3,'',True),
        ('eight-character-limit',[a['key'] for a in plan['limit'][:-1]],'MACSrPOO',True),
        ('accepted-limit-name',['Return'],'MACSrPOO',False),
        ('accepted-deletion',['Return']*3+[a['key'] for a in plan['deletion']],'Az09!?-',False),
        ('cancelled-name',['Return']*3+[a['key'] for a in plan['cancel']]+['Escape']*2,'Cancel',False),
        ('accepted-default',['Return']*3+[a['key'] for a in plan['default']],'',False)]
    keys=[];points=[]
    for name,inputs,entered,cycle in groups:
        keys+=inputs
        points.append(dict(name=name,input_end=len(keys),entered=entered,cycle=cycle))
    return keys,points


def validate(report):
    require(report['pass_'] and not report['engine_state_writes'] and
            report['card_unchanged'] and report['engine_matches_file'],
            'Completed real-input name capture with unchanged card required')
    keys,points=checkpoints()
    require(report['input_keys']==keys and len(report['checkpoints'])==len(points),
            'Complete original name input sequence required')
    require(report['helper_sha256']==digest((ROOT/'tools/driver_name_input.py').read_bytes()),
            'Current actual name helper required')
    source=ROOT/'tools/browser/capture_driver_name_ui.js' if report['target']=='browser' else Path(__file__)
    require(report['observer_sha256']==digest(source.read_bytes()), 'Current actual observer required')
    require(report['initial_card_sha256']==digest((ROOT/'DestructionDerby2/SaveGames').read_bytes()),
            'Same unchanged original provisioned card input required')
    if report['target']=='original':
        require(report['binary_sha256']==EXE_SHA256,'Unmodified supported original required')
    if report['target']=='browser':
        require(report['trusted_keyboard_events'] and all(e['trusted'] for e in report['trusted_keyboard_events']),
                'Actual trusted browser inputs required')
    for row,point in zip(report['checkpoints'],points):
        require(row['name']==point['name'] and row['input_end']==point['input_end'] and
                row['ui']['entered']==point['entered'] and ('cycle' in row)==point['cycle'],
                'Actual name checkpoint differs: '+point['name'])
        name=row['name']; ui=row['ui']
        if point['cycle']:
            require(ui['menu']==0x469f70,'Actual driver-name dialog required')
        else:
            require(ui['menu']==0x4696b0,'Actual return to title menu required')
        if name=='accepted-limit-name':
            require(bytes.fromhex(ui['names'])[:9]==b'MACSrPOO\0' and
                    ui['playable_tracks']==7 and ui['playable_bowls']==4,
                    'Actual accepted eight-character unlock name required')
        if name in ('accepted-deletion','cancelled-name'):
            require(bytes.fromhex(ui['names'])[:8]==b'Az09!?-\0',
                    'Actual deletion/cancellation result differs')
        if name=='accepted-default':
            require(bytes.fromhex(ui['names'])[:9]==b'Player 1\0', 'Actual empty-name default differs')


def compare(args):
    import copy
    from artifacts import WORK, open_files, prepare_output
    from verify_configuration_card_ui import cycle_frames,match_frame
    out=prepare_output(args.report)
    require(WORK in out.parents and not out.exists(),'Fresh report under /tmp/wasm-dd2 required')
    original=json.loads((args.original/'report.json').read_text());validate(original)
    require(original['target']=='original','Actual original reference required')
    targets={}; raw_files=set(); negatives=[]
    for target,directory in [('native',args.native),('browser',args.browser)]:
        actual=json.loads((directory/'report.json').read_text());validate(actual)
        require(actual['target']==target,'Actual target differs')
        frames=[]
        for left,right in zip(original['checkpoints'],actual['checkpoints']):
            require({k:v for k,v in left.items() if k!='cycle'}==
                    {k:v for k,v in right.items() if k!='cycle'},
                    target+' name/menu state differs at '+left['name'])
            if 'cycle' not in left:continue
            source=cycle_frames(left['cycle'],False)
            candidate=cycle_frames(right['cycle'],target=='browser')
            for phase in range(64):
                match_frame(*source[phase],*candidate[phase],False)
                frames.append(dict(checkpoint=left['name'],phase=phase,
                    framebuffer_sha256=digest(candidate[phase][1][0]),
                    palette_sha256=digest(candidate[phase][1][1])))
            for region in range(2):
                changed=[bytearray(v) for v in candidate[0][1]];changed[region][0]^=1
                try:match_frame(*source[0],candidate[0][0],changed,False)
                except RuntimeError:negatives.append(dict(target=target,case=left['name']+'-'+str(region)))
                else:raise RuntimeError('Altered name dialog pixels/palette accepted')
            for cycle in [left['cycle'],right['cycle']]:raw_files.update(Path(cycle).glob('*.bin'))
        changed=copy.deepcopy(actual);changed['checkpoints'][2]['ui']['entered']+='Q'
        try:validate(changed)
        except RuntimeError:negatives.append(dict(target=target,case='ninth-character'))
        else:raise RuntimeError('Accepted ninth character not rejected')
        changed=copy.deepcopy(actual);changed['checkpoints'][5]['ui']['names']='00'*120
        try:validate(changed)
        except RuntimeError:negatives.append(dict(target=target,case='cancelled-name-overwrite'))
        else:raise RuntimeError('Cancelled name overwrite not rejected')
        targets[target]=dict(pass_=True,capture=actual,frames=frames)
    result=dict(scope='Actual championship driver-name dialog: real input, complete selected 64-phase framebuffer/palette cycles, eight-character limit, deletion/cancel/default/unlock name. No fastest-lap title/record update, persistence write, race or chronological original A/V acceptance.',
        pass_=True,original=original,targets=targets,negative_cases=negatives,
        verifier_sha256=digest(Path(__file__).read_bytes()))
    out.write_text(json.dumps(result,indent=2)+'\n')
    if args.clean:
        opened=open_files()
        for file in raw_files:
            require(WORK in file.resolve().parents and not file.is_symlink(),'Unexpected raw capture path')
            stat=file.stat();require((stat.st_dev,stat.st_ino) not in opened,'Capture still open')
            file.unlink()
    print('Driver-name UI: PASS; 192 original framebuffer/palette pairs per port;',len(negatives),'negative controls')


def visible(ui):
    pointer = int.from_bytes(ui.read(0x469fd4,4),'little')
    return dict(menu=ui.integer(0x940010), cursor=list(struct.unpack('<hh',ui.read(0x469f34,4))),
                entered=ui.read(pointer+8,10).split(b'\0')[0].decode('ascii'),
                title=ui.text(0x46a024), names=ui.read(0x93e318,120).hex(),
                type=ui.integer(0x4673f4), playable_tracks=ui.integer(0x467404),
                playable_bowls=ui.integer(0x467408))


def capture(args):
    args.timeout=650
    initial=(ROOT/'DestructionDerby2/SaveGames').read_bytes()
    out,game=setup(args,initial)
    report=dict(scope='Real original/native driver-name UI through actual keys. Original hardware-only selected 64-phase raster/palette cycles, eight-character limit, deletion/cancel/default/real unlock-name behavior. No new lap record, persistence write, race or whole original A/V acceptance.',
        target=args.target,pass_=False,engine_state_writes=False, input_keys=[],checkpoints=[],
        binary_sha256=EXE_SHA256 if args.target=='original' else digest(args.binary.read_bytes()),
        observer_sha256=digest(Path(__file__).read_bytes()),
        helper_sha256=digest((ROOT/'tools/driver_name_input.py').read_bytes()),initial_card_sha256=digest(initial))
    def action(ui):
        def keys(codes):
            for key in codes:ui.key(key);report['input_keys'].append(key)
        def opened():
            keys(['Return','Return','Return'])
            ui.wait(lambda: ui.integer(0x940010)==0x469f70)
        def checkpoint(name,cycle=False):
            ui.settled()
            row=dict(name=name,input_end=len(report['input_keys']),ui=visible(ui),state=snapshot(ui))
            if cycle:row['cycle']=rendered_cycle(ui,dict(checkpoint=name),args.target=='original')
            report['checkpoints'].append(row)
        def enter(name,**kwargs):
            actions=enter_driver_name(ui,name,**kwargs)
            report['input_keys'] += [a['key'] for a in actions]
        opened()
        # A real negative case: File Manager's name keys do not accept this grid.
        ui.enter_name('D')
        report['input_keys'] += ['Right']*3+['Return','Down','Down','Right','Return']
        require(ui.integer(0x940010)==0x469f70 and visible(ui)['entered']=='Dg',
                'Real old-grid mismatch must be rejected')
        checkpoint('wrong-file-grid',True)
        keys(['Escape','Escape','Escape'])
        ui.wait(lambda:ui.integer(0x940010)==0x4696b0)
        opened()
        checkpoint('empty-grid',True)
        actions=name_actions('MACSrPOOQ')
        for a in actions:
            if a.get('accepts'):break
            keys([a['key']])
            require(visible(ui)['cursor']==a['cursor'] and visible(ui)['entered']==a['entered'],
                    'Actual mixed-case name or ninth-character limit differs')
        checkpoint('eight-character-limit',True)
        require(visible(ui)['entered']=='MACSrPOO','The actual ninth letter must be ignored')
        keys(['Return'])
        ui.wait(lambda:ui.integer(0x940010)==0x4696b0)
        require(ui.read(0x93e318,9)==b'MACSrPOO\0' and ui.integer(0x467404)==7 and
                ui.integer(0x467408)==4,'Actual original unlock-name behavior required')
        checkpoint('accepted-limit-name')
        opened()
        enter('Az09!?-.',delete_last=True)
        ui.wait(lambda:ui.integer(0x940010)==0x4696b0)
        require(ui.read(0x93e318,8)==b'Az09!?-\0','Actual deletion and accepted punctuation differ')
        checkpoint('accepted-deletion')
        opened()
        enter('Cancel',confirm=False)
        ui.wait(lambda:ui.integer(0x940010)==0x46a624)
        require(ui.read(0x93e318,8)==b'Az09!?-\0','Cancelled name changed actual saved player name')
        keys(['Escape','Escape'])
        ui.wait(lambda:ui.integer(0x940010)==0x4696b0)
        checkpoint('cancelled-name')
        opened()
        enter('')
        ui.wait(lambda:ui.integer(0x940010)==0x4696b0)
        require(ui.read(0x93e318,9)==b'Player 1\0','Actual empty-name default differs')
        checkpoint('accepted-default')
        raw=(ui.game/'SaveGames').read_bytes()
        require(raw==initial and raw==ui.read(0x754460,len(raw)), 'Name UI changed actual disk/card data')
        report.update(pass_=True,card_unchanged=True,engine_matches_file=True)
    try:execute(args,out,game,action)
    except Exception as error:report['error']=str(error);raise
    finally:
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(out)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    commands=parser.add_subparsers(dest='operation',required=True)
    cap=commands.add_parser('capture')
    cap.add_argument('--target',choices=['original','native'],required=True)
    cap.add_argument('--binary',type=Path,default=Path('/tmp/dd2_native'))
    cap.add_argument('--output',type=Path,required=True)
    commands.add_parser('plan')
    comp=commands.add_parser('compare')
    for name in ['original','native','browser','report']:comp.add_argument('--'+name,type=Path,required=True)
    comp.add_argument('--clean',action='store_true')
    args=parser.parse_args()
    if args.operation=='capture':capture(args)
    elif args.operation=='compare':compare(args)
    else:print(json.dumps(input_plan()))


if __name__=='__main__':main()
