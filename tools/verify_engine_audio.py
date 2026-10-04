#!/usr/bin/env python3
"""Compare actual engine PCM using independently observed original services.

Both engines execute their own sound controls and timer callbacks. All original
API values, source cursors and gains are assertions, never injected controls.
Covers this bounded startup/attract input trace, including accepted PCM and its
played prefix. Interactive scheduling, other scenarios and video remain open.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import subprocess

from artifacts import WORK,check_space,open_files,prepare_output
from capture_native_engine_audio import capture
from reference.capture import EXE_SHA256
from reference.engine_audio_services import export
from reference.audio import summarize_audio

ROOT=Path(__file__).resolve().parents[1]
ROW=struct.Struct('<12I')

def digest(raw):return hashlib.sha256(raw).hexdigest()

def validate_target(directory,services,clock,original,accepted,played,wasm=False):
    checkpoint=json.loads((directory/'checkpoint.json').read_text())
    complete=checkpoint.get('audio_services',{})
    if (checkpoint.get('engine_state_writes') is not False or checkpoint['intro']['movie']!=1 or
        checkpoint.get('initial_save_sha256')!=original['initial_save_sha256'] or
        checkpoint.get('game_clock_sha256')!=clock['ticks_sha256'] or
        checkpoint.get('audio_services_sha256')!=services['input_sha256'] or
        complete.get('complete') is not True or complete.get('events')!=services['events'] or
        complete.get('frames')!=services['frames'] or complete.get('epoch')!=2 or
        complete.get('callbacks')!=services['callbacks']):
        raise ValueError('Engine did not consume the verified original inputs with its own controls')
    start,end=checkpoint['start_state'],checkpoint['end_state']
    if (start['level']!=0 or start['movie'] or start['menu']!=dict(poly_list=0x4696b0,restart_cd_audio=0) or
        start['cd']!=dict(enabled=1,playing=1,**{'from':13,'to':14}) or
        end['level']!=original['end_state']['level'] or end['cf']!=original['end_state']['cf'] or
        end['movie'] or end['cd']!=original['end_state']['cd']):
        raise ValueError('Engine did not reach the original menu/race states')
    fmt=json.loads((directory/'mixed.pcm.json').read_text())
    if fmt!=dict(format='FLOAT_LE',rate=44100,channels=2):raise ValueError('Engine PCM format differs')
    pcm=(directory/'mixed.pcm').read_bytes()
    if pcm!=accepted:
        first=next((i for i,(a,b) in enumerate(zip(pcm,accepted)) if a!=b),min(len(pcm),len(accepted)))
        raise ValueError(f'Actual engine PCM differs at byte {first}; extent {len(pcm)} != {len(accepted)}')
    if pcm[:len(played)]!=played:raise ValueError('Played original prefix differs')
    if wasm:
        sink=checkpoint['sink']
        if sink['frames']!=services['frames'] or not sink['buffers'] or any(sink[k] for k in ('mismatches','missing','dropped')):
            raise ValueError('Actual WebAudio delivery differs/drops buffers')
        if checkpoint.get('errors') or checkpoint['accepted_sha256']!=digest(pcm):raise ValueError('Browser runtime/output differs')
    return dict(pass_=True,accepted_bytes=len(pcm),accepted_sha256=digest(pcm),
                played_bytes=len(played),played_sha256=digest(played),services=complete,
                binary_sha256=checkpoint.get('binary_sha256',checkpoint.get('wasm_sha256')),
                webaudio=checkpoint.get('sink'))

def mutations(raw):
    records=[list(ROW.unpack_from(raw,i)) for i in range(40,len(raw),48)]
    def changed(index,field,amount=1):
        rows=[list(row) for row in records];rows[index][field]=(rows[index][field]+amount)&0xffffffff
        return raw[:40]+b''.join(ROW.pack(*row) for row in rows)
    find=lambda kind:next(i for i,row in enumerate(records) if row[0]==kind)
    mono_mix=next(i for i,row in enumerate(records) if row[0]==21 and row[3]==0)
    callback=find(30)
    yield 'header',b'INVALID!'+raw[8:],'invalid service header/rate'
    yield 'truncated',raw[:-1],'incomplete/extra service records'
    yield 'extra',raw+b'\0'*48,'incomplete/extra service records'
    yield 'first-volume',changed(find(6),2),'engine-calculated API argument differs'
    yield 'play-presentation',changed(find(8),8),'engine API clock/presentation position differs'
    yield 'source-cursor',changed(mono_mix,2),'independently advanced source cursor/loop differs'
    yield 'source-gain',changed(mono_mix,4),'engine-calculated mix gain differs'
    callback_end=next(i for i in range(callback+1,len(records)) if records[i][0]==31)
    rows=[list(row) for row in records];wrong_id=rows[callback][1]+1
    rows[callback][1]=wrong_id
    for i in range(callback+1,callback_end+1):
        if rows[i][10]:rows[i][10]=wrong_id
    rows[callback_end][1]=wrong_id
    yield 'callback-id',raw[:40]+b''.join(ROW.pack(*row) for row in rows),'callback has no live engine timer'
    rows=[row for i,row in enumerate(records) if i!=callback]
    header=bytearray(raw[:40]);struct.pack_into('<I',header,12,len(rows))
    reason=('API outside its callback context' if any(row[10] for row in records[callback+1:callback_end])
            else 'unmatched callback end')
    yield 'missing-callback',bytes(header)+b''.join(ROW.pack(*row) for row in rows),reason

def verify(args):
    output=prepare_output(args.output)
    if WORK not in output.parents:raise ValueError('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=False)
    original_dir=args.original.resolve();original=json.loads((original_dir/'checkpoint.json').read_text())
    if original['exe_modified'] or original['exe_sha256']!=EXE_SHA256:raise ValueError('Unmodified original required')
    clock=json.loads((original_dir/'game-clock/report.json').read_text())
    ticks=original_dir/'game-clock/ticks.bin'
    if digest(ticks.read_bytes())!=clock['ticks_sha256']:raise ValueError('Original game clock changed')
    exported=export(original_dir,args.mixer.resolve(),output/'independent-input')
    services=args.services.resolve();raw=services.read_bytes()
    if digest(raw)!=exported['input_sha256']:raise ValueError('Input differs from independently reconstructed original trace')
    audio=summarize_audio(original_dir/'audio',write=False,require_played=True)
    accepted=(original_dir/'audio'/audio['streams'][-1]['file']).read_bytes()
    played=(original_dir/'audio'/audio['played_streams'][-1]['file']).read_bytes()
    if len(accepted)!=exported['frames']*8 or accepted[:len(played)]!=played:raise ValueError('Original accepted/played streams differ')
    targets={name:validate_target(directory.resolve(),exported,clock,original,accepted,played,name=='browser')
             for name,directory in [('native',args.native),('browser',args.browser)]}
    if args.native_asan:
        targets['native-asan']=validate_target(args.native_asan.resolve(),exported,clock,original,accepted,played)
    if digest(args.binary.read_bytes())!=targets['native']['binary_sha256']:
        raise ValueError('Negative runs must use the verified native engine')
    if args.browser_build and digest((args.browser_build/'index.wasm').read_bytes())!=targets['browser']['binary_sha256']:
        raise ValueError('Negative runs must use the verified WASM engine')
    negative=[]
    if args.negative_runs:
        for name,bad,expected in mutations(raw):
            input_file=output/(name+'.bin');input_file.write_bytes(bad);directory=output/('negative-'+name)
            try:capture(args.binary.resolve(),directory,input_file,ticks)
            except RuntimeError:
                text=(directory/'run.log').read_text()
                if 'DD2_AUDIO_SERVICES:' not in text or expected not in text:
                    raise AssertionError('Mutation rejected for a different reason: '+name+'\n'+text)
                negative.append(dict(name=name,rejected=True,reason=expected))
            else:raise AssertionError('Actual engine accepted bad service input: '+name)
            if args.browser_build and name in ('source-gain','callback-id'):
                browser_directory=output/('negative-browser-'+name)
                result=subprocess.run(['node',str(ROOT/'tools/browser/capture_engine_audio.js'),str(args.browser_build.resolve()),
                                       str(browser_directory),str(input_file),str(ticks)],capture_output=True,text=True,timeout=60)
                text=(browser_directory/'browser.log').read_text()
                if not result.returncode or 'DD2_AUDIO_SERVICES:' not in text or expected not in text:
                    raise AssertionError('WASM mutation accepted/rejected for a different reason: '+name+'\n'+result.stdout+result.stderr)
                negative.append(dict(name='browser-'+name,rejected=True,reason=expected))
            input_file.unlink()
    rows=[ROW.unpack_from(raw,i) for i in range(40,len(raw),48)];counts=Counter(row[0] for row in rows)
    report=dict(scope=__doc__.strip(),pass_=True,original_exe_sha256=EXE_SHA256,
                input_sha256=exported['input_sha256'],trace_sha256=exported['trace_sha256'],
                game_clock_sha256=clock['ticks_sha256'],sources=exported['sources'],
                events=exported['events'],event_kinds=dict(counts),targets=targets,negative_cases=negative,
                cd_transport='Original looping MCI ring versus independently loaded finite track; actual flags checked separately',
                primary_tail_frames=next(iter(targets.values()))['services']['rendered_frames']-exported['frames'])
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    # Successful raw output is removed only after the comparison report exists;
    # the original is retained for remaining engine/video diagnoses.
    opened=open_files()
    for directory in [args.native,args.browser,*([args.native_asan] if args.native_asan else [])]:
        file=directory/'mixed.pcm';stat=file.stat()
        if (stat.st_dev,stat.st_ino) in opened:raise RuntimeError('Cannot clean a running capture')
        file.unlink()
    for file in (output/'independent-input').glob('*.bin'):file.unlink()
    for directory in output.glob('negative-*'):
        for file in directory.iterdir():
            if file.is_file() and file.suffix in ('.pcm','.log'):file.unlink()
    check_space(output);return report

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('original','mixer','services','native','browser','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--native-asan',type=Path)
    parser.add_argument('--browser-build',type=Path,help='also run gain/callback mutations against this verified browser build')
    parser.add_argument('--binary',type=Path,default=Path('/tmp/dd2_native'))
    parser.add_argument('--negative-runs',action='store_true')
    report=verify(parser.parse_args())
    print('Actual engine PCM: PASS;',len(report['targets']),'targets;',report['events'],'services;',len(report['negative_cases']),'negative cases')
