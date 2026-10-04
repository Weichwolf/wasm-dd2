#!/usr/bin/env python3
"""Compare actual keyboard-binding menus and saved/cancelled key maps.

Normal navigation, five distinct bindings, four duplicate refusals, commit,
partial cancellation and reopening are compared with the unmodified original.
Every indexed pixel and palette byte is compared over 64 highlight phases at
each checkpoint. Audio, transition video, physical timing and racing use of
the changed bindings are separate requirements.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference'))
from artifacts import WORK,check_space,open_files,prepare_output,run_bounded
from reference.capture import EXE_SHA256
from verify_menu_cycles import cycles

KEYS=['Down','Right','Right','Return','Return','Return',
      'B','B','C','C','D','D','E','E','F','Return','Return','Return',
      'G','Escape','Escape','Return','Return','Escape','Escape','Escape']
POLYS=[0x4696b0]*4+[0x4690c4,0x46a134]+[0x469298]*10+[
       0x4690c4,0x46a134,0x469298,0x469298,0x46a134,0x4690c4,
       0x46a134,0x469298,0x46a134,0x4690c4,0x4696b0]
BINDINGS={7:(5,'B'),9:(3,'C'),11:(8,'D'),13:(12,'E'),15:(13,'F'),19:(5,'G')}
ATTEMPTS={7:(0,0),8:(1,-1),9:(1,0),10:(2,-1),11:(2,0),12:(3,-1),13:(3,0),14:(4,-1),15:(4,1),19:(0,0)}

def digest(raw):return hashlib.sha256(raw).hexdigest()

def capture_target(args):
    output=prepare_output(args.output)
    if WORK not in output.parents or output.exists():raise ValueError('Fresh output under /tmp/wasm-dd2/ required')
    if args.target=='original':
        command=['python3',str(ROOT/'tools/reference/capture.py'),'--mode','menu','--acknowledged-key',
                 '--menu-cycle','64','--timeout','600','--output',str(output),'--keys',*KEYS]
    elif args.target=='native':
        command=['python3',str(ROOT/'tools/capture_native_menu.py'),'--binary',str(args.binary.resolve()),
                 '--menu-cycle','64','--timeout','600','--output',str(output),'--keys',*KEYS]
    else:
        command=['node',str(ROOT/'tools/browser/capture_menu_cycle.js'),str(args.build.resolve()),str(output),*KEYS]
    output.mkdir()
    run_bounded(command,directory=output,timeout=650,check=True)
    check_space(output)

def image_state(directory):
    raw=(directory/'image.bin').read_bytes()
    if len(raw)!=0x580400:raise ValueError('Incomplete menu checkpoint')
    at=lambda va,size:list(raw[va-0x400000:va-0x400000+size])
    return dict(saved=at(0x46757a,18),working=at(0x93fd90,18),active=at(0x46302c,14),
                pad_option=struct.unpack_from('<I',raw,0x467414-0x400000)[0]),struct.unpack_from('<I',raw,0x940010-0x400000)[0]

def expected_states(initial,states):
    if len(states)!=len(KEYS)+1 or len(POLYS)!=len(states):raise ValueError('Incomplete keyboard path')
    saved=initial['saved'][:];working=None
    for index,row in enumerate(states):
        state=row['binding']
        if index in (6,18,23):working=saved[:]
        if index in BINDINGS:
            offset,key=BINDINGS[index];working[offset]=ord(key)
            if index==11:working[9]=ord(key)
        if index==16:saved=working[:]
        if row['poly_list']!=POLYS[index] or state['saved']!=saved:
            raise ValueError(f'Menu/saved bindings differ at checkpoint {index}')
        if working is not None and state['working']!=working:
            raise ValueError(f'Working bindings or duplicate refusal differ at checkpoint {index}')
        if index>=16 and state['pad_option']!=0:raise ValueError('Commit did not select the original keyboard method')

def load_states(directory,navigation,browser):
    states=[]
    for index,name in enumerate(navigation['checkpoints']):
        checkpoint=directory/name
        if browser:
            frames=json.loads((checkpoint/'cycle/cycle.json').read_text())['frames']
            row=frames[0];state=row['keyboard_binding'];poly=row['poly_list']
            if any(frame['keyboard_binding']!=state or frame['poly_list']!=poly for frame in frames):
                raise ValueError('Key map changed during a settled menu cycle')
        else:
            state,poly=image_state(checkpoint)
        states.append(dict(index=index,poly_list=poly,binding=state))
    expected_states(states[0]['binding'],states);return states

def compare_states(original,actual):
    if original!=actual:raise ValueError('Port saved/working/active key maps or menu states differ')

def compare_frame(original,actual):
    if original!=actual:raise ValueError('Menu framebuffer/palette differs')

def compare(args):
    report_path=prepare_output(args.report)
    if WORK not in report_path.parents or report_path.exists():raise ValueError('Fresh report under /tmp/wasm-dd2/ required')
    original=args.original.resolve();nav,reference=cycles(original,'real X11 keys')
    if nav['keys']!=KEYS or not nav.get('acknowledged_keys'):raise ValueError('Actual acknowledged keyboard binding path required')
    for name in nav['checkpoints']:
        metadata=json.loads((original/name/'checkpoint.json').read_text())
        if metadata['exe_modified'] or metadata['exe_sha256']!=EXE_SHA256:raise ValueError('Unmodified original required')
    original_states=load_states(original,nav,False)
    for index,wanted in ATTEMPTS.items():
        observation=json.loads((original/nav['checkpoints'][index]/'binding-input.json').read_text())
        if (observation['slot'],observation['result'])!=wanted or observation['engine_state_writes'] is not False:
            raise ValueError('Original actual binding attempt differs')
    targets={};negative=[]
    for target,directory,input_type in [('native',args.native.resolve(),'dd2_key_event'),('browser',args.browser.resolve(),'browser keyboard events')]:
        actual_nav,actual=cycles(directory,input_type)
        if actual_nav['keys']!=KEYS or actual_nav['initial_save_sha256']!=nav['initial_save_sha256'] or set(actual)!=set(reference):
            raise ValueError('Port navigation/save input differs')
        states=load_states(directory,actual_nav,target=='browser');compare_states(original_states,states)
        frames=[]
        for key,pair in reference.items():
            try:compare_frame(pair,actual[key])
            except ValueError:raise ValueError(f'{target} menu pixels/palette differ at {key}') from None
            frames.append(dict(checkpoint=key[0],phase=key[1],framebuffer_sha256=digest(pair[0]),palette_sha256=digest(pair[1])))
        # Reject saved-map, duplicate, cancellation and reopened-state defects.
        for label,index,region,offset in [('saved-map',16,'saved',5),('duplicate',8,'working',3),
            ('cancelled-map',20,'saved',5),('reopened-map',23,'working',5),('active-map',16,'active',0)]:
            damaged=copy.deepcopy(states);damaged[index]['binding'][region][offset]^=1
            try:compare_states(original_states,damaged)
            except ValueError:negative.append(target+'-'+label)
            else:raise AssertionError('Damaged binding accepted: '+label)
        for region in (0,1):
            pair=next(iter(actual.values()))[:];damaged=bytearray(pair[region]);damaged[0]^=1;pair[region]=bytes(damaged)
            try:compare_frame(next(iter(reference.values())),pair)
            except ValueError:negative.append(target+'-'+('framebuffer' if region==0 else 'palette'))
            else:raise AssertionError('Changed pixel/palette accepted')
        targets[target]=dict(pass_=True,frames=len(frames),states=states,frame_hashes=frames,
            binary_sha256=actual_nav.get('binary_sha256',actual_nav.get('wasm_sha256')))
    report=dict(scope=__doc__.strip(),pass_=True,original_exe_sha256=EXE_SHA256,
                initial_save_sha256=nav['initial_save_sha256'],keys=KEYS,targets=targets,negative_cases=negative,
                original_binding_attempts={index:dict(slot=slot,result=result) for index,(slot,result) in ATTEMPTS.items()})
    report_path.parent.mkdir(parents=True,exist_ok=True);report_path.write_text(json.dumps(report,indent=2)+'\n')
    if args.clean:
        opened=open_files()
        for directory in [original,args.native.resolve(),args.browser.resolve()]:
            for file in directory.rglob('*'):
                if file.is_file() and not file.is_symlink() and file.suffix in ('.bin','.ppm','.png','.log'):
                    stat=file.stat()
                    if (stat.st_dev,stat.st_ino) in opened:raise RuntimeError('Cannot clean a running menu capture')
                    file.unlink()
    return report

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);commands=parser.add_subparsers(dest='command',required=True)
    record=commands.add_parser('capture');record.add_argument('--target',choices=['original','native','browser'],required=True)
    record.add_argument('--output',type=Path,required=True);record.add_argument('--binary',type=Path,default=Path('/tmp/dd2_native'))
    record.add_argument('--build',type=Path,default=ROOT/'web/dd2')
    check=commands.add_parser('compare')
    for name in ('original','native','browser','report'):check.add_argument('--'+name,type=Path,required=True)
    check.add_argument('--clean',action='store_true');args=parser.parse_args()
    if args.command=='capture':capture_target(args)
    else:
        proof=compare(args);print('Keyboard binding menus: PASS;',len(KEYS)+1,'checkpoints;',len(proof['negative_cases']),'rejected mutations')
