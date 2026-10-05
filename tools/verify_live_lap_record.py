#!/usr/bin/env python3
"""Verify actual lap/name/save/restart captures and selected original dialog pixels.

Each target drives independently. Matching highlight phases establishes these
menu rasters only; it cannot establish identical chronological racing or audio.
The initial card contains exactly one declared saved-minute WORD edit.
"""
import argparse
import copy
import json
from pathlib import Path
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from artifacts import WORK, check_space, open_files, prepare_output
from verify_championship_save import pack
from verify_configuration_card_ui import match_frame
from verify_configuration_persistence import EXE_SHA256, ROOT, digest, require
from verify_statistics_ui import fixture as statistics_fixture


def load(directory):
    return json.loads((directory/'report.json').read_text())


def actual_time(value):
    require(len(value)==3 and all(isinstance(x,int) for x in value), 'Complete actual lap time required')
    require(0<=value[0]<0x100 and 0<=value[1]<60 and 0<=value[2]<=0xffff, 'Actual lap time outside bounds')
    return (value[0]<<24)|(value[1]<<16)|value[2]


def validate(directory, initial, target, data=None):
    data=load(directory) if data is None else data
    require(data['target']==target and data['pass_'] and not data['engine_state_writes'],
            'Successful real lap/name/save/restart capture required: '+target)
    require(data['initial_card_sha256']==digest(initial), 'Identical declared initial card required')
    require(data['engine_matches_file'] and data['configuration_restored'] and
            data['reloaded_state']==data['saved_state'], 'Actual complete saved configuration must survive restart')
    if target!='browser':
        require(set(data['observer_sources'])=={'capture_live_lap_record.py','live_lap_record_gdb.py','live_lap_record_driver.py'},
                'Complete immutable observer sources required')
        for name,recorded in data['observer_sources'].items():
            require(recorded==digest((directory/'observer-source'/name).read_bytes()) and
                    recorded==digest((ROOT/'tools'/name).read_bytes()), 'Current immutable observer source required')
        require(data['drive']['driver_sha256']==digest((ROOT/'tools/live_lap_record_driver.py').read_bytes()),
                'Current read-only driver required')
    else:
        for key,file in [('observer_sha256','browser/capture_live_lap_record.js'),
                         ('driver_sha256','live_lap_record_driver.py'),('rpc_sha256','live_lap_record_rpc.py'),
                         ('helper_sha256','driver_name_input.py')]:
            require(data[key]==digest((ROOT/'tools'/file).read_bytes()), 'Current actual browser observer required')
    if target=='original':
        require(data['binary_sha256']==EXE_SHA256, 'Unmodified supported original required')
    drive=data['drive']
    require(not drive['engine_state_writes'], 'Read-only driver required')
    if target!='browser':
        require(drive['pass_'] and drive['hardware_slots']==1 and drive['input']=='real X11 keys',
                'Actual read-only hardware observer / real X11 keys required')
    else:
        events=data['trusted_keyboard_events']
        require(events and all(e['trusted'] for e in events), 'Actual trusted browser input required')
    final=drive['final']; vehicle=final['driver']; runtime=final['runtime']
    require(vehicle['lap']==2 and vehicle['dead']==0 and actual_time(runtime)<actual_time([2,25,0]),
            'Actual alive completed lap improving the declared initial best required')
    if target=='browser':
        require(final['level']==1 and final['type']==1 and final['quit']==0,
                'Actual first-track time trial required')
    else:
        require(drive['level']==1 and drive['track']==0 and drive['record_before']==[2,25,0],
                'Actual first-track time trial required')
    before=bytes.fromhex(data['before']['fastest']);after=bytes.fromhex(data['after']['fastest'])
    require(before==initial[0x4000+5948:0x4000+6508], 'Loaded actual initial record banks differ')
    require(data['before']['type']==1 and data['before']['car']==1 and data['before']['track']==0 and
            data['before']['cars']==1, 'Actual selected Amateur / one-car / first-track trial required')
    expected=bytearray(before)
    expected[16:80]=before[:64]
    expected[:2]=b'D\0'
    struct.pack_into('<HHH',expected,10,*runtime)
    require(after==expected, 'Actual name, lap WORDs, history or other track banks differ')
    require(data['saved_state']['fastest']==after.hex(), 'Actual saved record state differs')
    packed=bytearray(pack(data['saved_state']));struct.pack_into('<H',packed,0,0x1010)
    require(digest(packed)==data['payload_sha256'], 'Actual full configuration serialization differs')
    raw=(directory/'saved.card').read_bytes()
    require(len(raw)==0x20000 and digest(raw)==data['card_sha256'] and raw[0x4000:0x597e]==packed,
            'Actual saved file/card payload differs')
    require(raw[:0x200]==initial[:0x200] and raw[0x2000:0x4000]==initial[0x2000:0x4000],
            'Original championship slot changed')
    if target=='browser':
        points=data['checkpoints']
        require([p['name'] for p in points]==['record-name-empty','record-name-typed'], 'Both actual dialog checkpoints required')
        for p,entered in zip(points,['','D']):
            require(p['ui']['menu']==0x469f70 and p['ui']['title']=='%R%JC%T/Fastest Lap' and
                    p['ui']['entered']==entered, 'Actual fastest-lap title/name differs')
        require(points[0]['ui']['cursor']==[48,123] and points[1]['ui']['cursor']==[256,191],
                'Actual empty/accept cursor differs')
        cycles=[p['cycle'] for p in points]
    else:
        require(data['name_dialog']['menu']==0x469f70 and data['name_dialog']['title']=='%R%JC%T/Fastest Lap',
                'Actual fastest-lap name dialog required')
        cycles=[data['name_dialog'][name] for name in ['empty_cycle','typed_cycle']]
    return data,cycles


def dialog_frames(directory, browser):
    """The original retains a nonzero, constant race frame counter in this dialog."""
    directory=Path(directory)
    require(WORK in directory.resolve().parents, 'Capture must be under /tmp/wasm-dd2')
    metadata=json.loads((directory/'cycle.json').read_text())
    require(metadata['stage']==('browser platform present' if browser else 'Draw_All entry / pending presentation'),
            'Actual dialog presentation capture required')
    rows=metadata['frames']
    require(len(rows)==64 and {r['phase'] for r in rows}==set(range(64)), 'All 64 dialog highlight phases required')
    require(all(r['level']==0 and r['poly_list']==0x469f70 for r in rows) and len({r['cf'] for r in rows})==1,
            'Actual stable post-race name dialog required')
    result={};raw_files=set()
    for row in rows:
        if browser:require(row['canvas_mismatches']==0, 'Actual browser canvas differs from indexed raster')
        pair=[]
        for region,size in [('framebuf',307200),('palette',1024)]:
            file=directory/(row['prefix']+'-'+region+'.bin')
            require(file.parent==directory and not file.is_symlink(), 'Unexpected raster path')
            raw=file.read_bytes();require(len(raw)==size, 'Incomplete actual raster/palette')
            pair.append(raw);raw_files.add(file)
        result[row['phase']]=(row,pair)
    return result,raw_files


def compare(args):
    out=prepare_output(args.report)
    require(WORK in out.parents and not out.exists(), 'Fresh report under /tmp/wasm-dd2 required')
    producer=load(args.original)
    source,unused=statistics_fixture(Path(producer['original_source_fixture']))
    initial=bytearray(source);struct.pack_into('<H',initial,0x4000+5948+10,2);initial=bytes(initial)
    require((args.original/'initial.card').read_bytes()==initial and producer['original_source_card_sha256']==digest(source),
            'Authentic original-generated source and exact one-WORD file edit required')
    edits=[dict(offset=0x4000+5948+10,field='fastest[track][0].minutes',track=0,
                before=source[0x4000+5948+10:0x4000+5948+12].hex(),after='0200')]
    captures={};cycles={};negatives=[];raw_files=set()
    for target,directory in [('original',args.original),('native',args.native),('browser',args.browser)]:
        data,paths=validate(directory,initial,target)
        require(data['initial_file_edits']==edits and data['original_source_card_sha256']==digest(source),
                'Actual declared initial input differs')
        captures[target]=data;cycles[target]=[]
        for path in paths:
            frames,files=dialog_frames(path,target=='browser');cycles[target].append(frames);raw_files.update(files)
        for field in ['after','reloaded_state']:
            altered=copy.deepcopy(data)
            if field=='after':altered[field]['fastest']='00'+altered[field]['fastest'][2:]
            else:altered[field]['car']^=1
            try:validate(directory,initial,target,altered)
            except RuntimeError:negatives.append(dict(target=target,case=field))
            else:raise RuntimeError('Altered actual record/reload accepted')
    pairs={}
    for target in ['native','browser']:
        rows=[]
        for index in range(2):
            ref,got=cycles['original'][index],cycles[target][index]
            for phase in range(64):
                match_frame(*ref[phase],*got[phase],False)
                rows.append(dict(checkpoint=index,phase=phase,framebuffer_sha256=digest(got[phase][1][0]),
                                 palette_sha256=digest(got[phase][1][1])))
            for region in range(2):
                altered=[bytearray(x) for x in got[0][1]];altered[region][0]^=1
                try:match_frame(*ref[0],got[0][0],altered,False)
                except RuntimeError:negatives.append(dict(target=target,case='dialog-'+str(index)+'-region-'+str(region)))
                else:raise RuntimeError('Altered actual dialog raster/palette accepted')
        pairs[target]=rows
    result=dict(scope=__doc__,pass_=True,captures=captures,frames=pairs,negative_cases=negatives,
                initial_file_edits=edits,verifier_sha256=digest(Path(__file__).read_bytes()))
    out.write_text(json.dumps(result,indent=2)+'\n')
    if args.clean:
        opened=open_files()
        for file in raw_files:
            stat=file.stat();require((stat.st_dev,stat.st_ino) not in opened, 'Capture still open')
        for file in raw_files:file.unlink()
    for directory in [args.original,args.native,args.browser]:check_space(directory)
    print('Live lap/name/save/restart: PASS; 128 exact original dialog pairs per port;',len(negatives),'negative controls')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ['original','native','browser','report']:parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--clean',action='store_true')
    compare(parser.parse_args())


if __name__=='__main__':main()
