#!/usr/bin/env python3
"""Check real track-menu navigation and every complete rendered highlight phase.

The road/bowl cases cover all seven/four previews at the supplied default unlock
state, locked confirmation, both wrap directions, cancel/confirm/reopen and main
menu F1/F2 shortcuts. Pair by the observed highlight counter, never image fitting.
This excludes transition chronology, live timing, racing, audio and earned unlocks.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path
import subprocess
import sys

from artifacts import WORK, check_space, open_files, prepare_output, run_bounded

ROOT = Path(__file__).resolve().parents[1]
EXE = '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
MAIN, SELECT = 0x4696b0, 0x46a890
# (key, track, screen, locked). Escape restores the track, but the original
# retains the locked byte: reopening then Enter still stays in Select_Track.
# These two independent runtime observations are required, not corrected here.
ROAD = [
    (None,0,MAIN,0), ('Right',0,MAIN,0), ('Right',0,MAIN,0),
    ('Return',0,SELECT,0), ('Left',6,SELECT,1), ('Return',6,SELECT,1),
    ('Right',0,SELECT,0), ('Right',1,SELECT,0), ('Return',1,MAIN,0),
    ('Return',1,SELECT,0), ('Right',2,SELECT,0), ('Right',3,SELECT,0),
    ('Right',4,SELECT,1), ('Return',4,SELECT,1), ('Right',5,SELECT,1),
    ('Return',5,SELECT,1), ('Right',6,SELECT,1), ('Return',6,SELECT,1),
    ('Right',0,SELECT,0), ('Left',6,SELECT,1), ('Escape',1,MAIN,1),
    ('Return',1,SELECT,1), ('Return',1,SELECT,1), ('Right',2,SELECT,0),
    ('Return',2,MAIN,0), ('F2',3,MAIN,0), ('Return',3,SELECT,0),
    ('Return',3,MAIN,0), ('F1',2,MAIN,0),
]
BOWL = [
    (None,0,MAIN,0), ('F1',7,MAIN,0), ('F1',7,MAIN,0),
    ('Right',7,MAIN,0), ('Right',7,MAIN,0), ('Return',7,SELECT,0),
    ('Left',10,SELECT,1), ('Return',10,SELECT,1), ('Right',7,SELECT,0),
    ('Right',8,SELECT,1), ('Return',8,SELECT,1), ('Right',9,SELECT,1),
    ('Return',9,SELECT,1), ('Right',10,SELECT,1), ('Return',10,SELECT,1),
    ('Right',7,SELECT,0), ('Return',7,MAIN,0), ('F1',7,MAIN,0),
    ('F2',7,MAIN,0), ('Return',7,SELECT,0), ('Right',8,SELECT,1),
    ('Escape',7,MAIN,1), ('Return',7,SELECT,1), ('Return',7,SELECT,1),
    ('Left',10,SELECT,1), ('Right',7,SELECT,0), ('Return',7,MAIN,0),
    ('Return',7,SELECT,0), ('Escape',7,MAIN,0),
]
CASES = {'road': ROAD, 'bowl': BOWL}
FIELDS = ('race_mode','race_type','race_track','playable_tracks','playable_bowls',
          'track_locked','saved_track','poly_list')
INPUTS = {'original':'real X11 keys', 'native':'dd2_key_event', 'browser':'browser keyboard events'}


def path_in_work(path):
    path = path.resolve()
    if WORK not in path.parents:
        raise ValueError('Verification output must be under /tmp/wasm-dd2/')
    return path


def load(root, target, case):
    root = path_in_work(root)
    check_space(root)
    nav = json.loads((root/'navigation.json').read_text())
    expected = CASES[case]
    names = [f"step{i:02d}-{step[0] or 'boot'}" for i,step in enumerate(expected)]
    if nav['keys'] != [step[0] for step in expected[1:]] or nav['checkpoints'] != names:
        raise ValueError('Incomplete or wrong track navigation')
    if nav['input'] != INPUTS[target] or (target == 'original' and not nav.get('acknowledged_keys')):
        raise ValueError('Incorrect keyboard provenance')
    if not re.fullmatch('[0-9a-f]{64}',nav.get('initial_save_sha256','')):
        raise ValueError('Missing initial save input hash')
    if target != 'original' and not re.fullmatch('[0-9a-f]{64}',nav.get('binary_sha256',nav.get('wasm_sha256',''))):
        raise ValueError('Missing captured binary hash')
    stages = {'browser':'browser platform present'}
    cycles = {}
    for index, name in enumerate(names):
        directory = root/name/'cycle'
        if target == 'original':
            checkpoint = json.loads((root/name/'checkpoint.json').read_text())
            if checkpoint.get('exe_modified') is not False or checkpoint.get('exe_sha256') != EXE:
                raise ValueError('Supported unmodified original required')
        meta = json.loads((directory/'cycle.json').read_text())
        if meta['stage'] != stages.get(target,'Draw_All entry / pending presentation'):
            raise ValueError('Incorrect presentation stage')
        rows = meta['frames']
        if len(rows) != 64 or [row['index'] for row in rows] != list(range(64)):
            raise ValueError('Need 64 consecutive presentations')
        if {row['phase'] for row in rows} != set(range(64)) or any(
                row['phase'] != (rows[i-1]['phase']+1)%64 for i,row in enumerate(rows) if i):
            raise ValueError('Missing/repeated/reordered highlight phases')
        _, track, screen, locked = expected[index]
        mode, race_type = (0,0) if case == 'road' or index == 0 else (2,2 if index == 1 else 0)
        for row in rows:
            wanted = dict(race_track=track,poly_list=screen,track_locked=locked,
                          race_mode=mode,race_type=race_type,playable_tracks=4,playable_bowls=1)
            if row['cf'] != 0 or row['level'] != 0 or any(row.get(k) != v for k,v in wanted.items()):
                raise ValueError(f'{target} {name}: unexpected track navigation state: {row}')
            if target == 'browser' and row.get('canvas_mismatches') != 0:
                raise ValueError('Canvas pixels differ from indexed capture')
            if tuple(row.get(k) for k in FIELDS) != tuple(rows[0].get(k) for k in FIELDS):
                raise ValueError('Track selection changed during cycle')
            if row['prefix'] != f"frame{row['index']:03d}":
                raise ValueError('Invalid framebuffer prefix')
        cycles[name] = {row['phase']: row for row in rows}
    return root, nav, cycles


def read_frame(root, name, row):
    files = [root/name/'cycle'/f"{row['prefix']}-{region}.bin" for region in ('framebuf','palette')]
    values = [file.read_bytes() for file in files]
    if list(map(len,values)) != [307200,1024]:
        raise ValueError('Incomplete framebuffer/palette')
    return values


def state_difference(a,b):
    return {key: [a.get(key),b.get(key)] for key in FIELDS if a.get(key) != b.get(key)}


def compare(reference, actual):
    failures, hashes = [], []
    for name, phases in reference[2].items():
        for phase, row in phases.items():
            other = actual[2][name][phase]
            different = state_difference(row,other)
            if different:
                failures.append(dict(checkpoint=name,phase=phase,state=different))
            original, port = read_frame(reference[0],name,row), read_frame(actual[0],name,other)
            for i,region in enumerate(('framebuffer','palette')):
                if original[i] != port[i]:
                    failures.append(dict(checkpoint=name,phase=phase,region=region,
                        differing_bytes=sum(a!=b for a,b in zip(original[i],port[i]))))
            hashes.append(dict(checkpoint=name,phase=phase,state={key:row[key] for key in FIELDS},
                original=[hashlib.sha256(v).hexdigest() for v in original],
                actual=[hashlib.sha256(v).hexdigest() for v in port]))
    return failures, hashes


def negatives(reference, actual, case):
    # Damage actual on-disk captures and run the same loader/comparator. Restore
    # exactly in finally, so failed negative checks do not poison later proof.
    root, _, cycles = actual
    name = next(name for name,rows in cycles.items() if next(iter(rows.values()))['poly_list'] == SELECT)
    row = next(iter(cycles[name].values()))
    accepted = []
    for region, position in [('framebuf',200*640+200),('palette',17)]:
        file = root/name/'cycle'/f"{row['prefix']}-{region}.bin"
        original = file.read_bytes(); damaged = bytearray(original); damaged[position] ^= 1
        try:
            file.write_bytes(damaged)
            if not compare(reference,actual)[0]:
                raise AssertionError('Damaged '+region+' accepted')
        finally:
            file.write_bytes(original)
        accepted.append('damaged-'+region)
    return accepted + metadata_negatives(actual,case)


def metadata_negatives(actual,case):
    root, nav, cycles = actual
    name = next(name for name,rows in cycles.items() if next(iter(rows.values()))['poly_list'] == SELECT)
    target = next(target for target, provenance in INPUTS.items() if nav['input'] == provenance)
    accepted = []
    meta = root/name/'cycle'/'cycle.json'; original = meta.read_bytes()
    # Wrong selection/ignored cancellation cannot pass just on image equality.
    for fault in ('race_track','poly_list','track_locked','phase-order','missing-presentation'):
        damaged = json.loads(original)
        if fault == 'phase-order':
            damaged['frames'][0]['phase'],damaged['frames'][1]['phase'] = damaged['frames'][1]['phase'],damaged['frames'][0]['phase']
        elif fault == 'missing-presentation':
            damaged['frames'].pop()
        else:
            for frame in damaged['frames']:
                frame[fault] ^= 1
        try:
            meta.write_text(json.dumps(damaged))
            try:
                load(root,target,case)
            except ValueError:
                pass
            else:
                raise AssertionError('Damaged '+fault+' accepted')
        finally:
            meta.write_bytes(original)
        accepted.append('damaged-'+fault if fault in FIELDS else fault)
    return accepted


def capture(args):
    output = prepare_output(path_in_work(args.output))
    if output.exists() and any(output.iterdir()):
        raise ValueError('Use a fresh output directory')
    keys = [step[0] for step in CASES[args.case][1:]]
    if args.target == 'browser':
        output.mkdir(parents=True,exist_ok=True)
        run_bounded(['node',str(ROOT/'tools/browser/capture_menu_cycle.js'),str(ROOT/'web/dd2'),str(output),*keys],
                    directory=output,timeout=300,check=True,cwd=ROOT)
    else:
        script = 'reference/capture.py' if args.target == 'original' else 'capture_native_menu.py'
        command = [sys.executable,str(ROOT/'tools'/script),'--output',str(output),
                   '--menu-cycle','64','--timeout','300','--keys',*keys]
        if args.target == 'original':
            command += ['--mode','menu','--acknowledged-key']
        subprocess.run(command,check=True,cwd=ROOT)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command',required=True)
    cap = commands.add_parser('capture'); cap.add_argument('--target',choices=INPUTS,required=True)
    cap.add_argument('--case',choices=CASES,required=True); cap.add_argument('--output',type=Path,required=True)
    check = commands.add_parser('compare'); check.add_argument('--case',choices=CASES,required=True)
    for target in INPUTS:
        check.add_argument('--'+target,type=Path,required=True)
    check.add_argument('--report',type=Path,required=True)
    check.add_argument('--negative',action='store_true'); check.add_argument('--clean',action='store_true')
    args = parser.parse_args()
    if args.command == 'capture':
        capture(args); return 0
    report = prepare_output(path_in_work(args.report))
    data = {target:load(getattr(args,target),target,args.case) for target in INPUTS}
    reference = data['original']
    result = dict(scope=__doc__.strip(),case=args.case,original_exe_sha256=EXE,
        initial_save_sha256=reference[1]['initial_save_sha256'],targets={},
        captures={target:str(value[0]) for target,value in data.items()})
    for target in ('native','browser'):
        actual = data[target]
        if actual[1]['initial_save_sha256'] != result['initial_save_sha256']:
            raise ValueError('Initial save inputs differ')
        failures, hashes = compare(reference,actual)
        result['targets'][target] = dict(pass_=not failures,frames=len(hashes),failures=failures,hashes=hashes,
            binary_sha256=actual[1].get('binary_sha256',actual[1].get('wasm_sha256')),
            negative_cases=negatives(reference,actual,args.case) if args.negative and not failures else [])
    result['pass_'] = all(value['pass_'] for value in result['targets'].values())
    report.parent.mkdir(parents=True,exist_ok=True); report.write_text(json.dumps(result,indent=2)+'\n')
    if args.clean and result['pass_']:
        opened = open_files()
        files = [file for root,_,_ in data.values() for file in root.rglob('*')
                 if file.is_file() and not file.is_symlink() and file.suffix in ('.bin','.ppm','.png')]
        if any((file.stat().st_dev,file.stat().st_ino) in opened for file in files):
            raise RuntimeError('Report saved; capture files are still open')
        for file in files:
            file.unlink()
    print(json.dumps({**result,'targets':{target:{**{k:v for k,v in value.items() if k not in ('hashes','failures')},
                       'failure_count':len(value['failures']),'failures':value['failures'][:5]}
                       for target,value in result['targets'].items()}},indent=2))
    return 0 if result['pass_'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
