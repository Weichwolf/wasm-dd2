#!/usr/bin/env python3
"""Capture/compare actual replay racing video with explicit original API inputs.

The file input is an actual original-produced replay card. Genuine X11 keys
load it, with read-only hardware observations through natural completion.
Ports replay captured clock returns and verify their independently calculated
RNG triples. Every racing pending presentation is compared byte for byte;
chronological audio, loading/menu video and physical-clock parity remain open.
"""
import argparse
import copy
import json
from pathlib import Path
import struct
import subprocess
import zlib

from artifacts import WORK,check_space,prepare_output,open_files
from verify_original_replay import fixture
from verify_championship_save import setup,execute
from verify_configuration_persistence import ROOT,ConfigUI,EXE_SHA256,digest,require


def capture(args):
    initial,_,_=fixture(args.fixture)
    out,game=setup(args,initial)
    report=dict(scope=__doc__,pass_=False,target=args.target,engine_state_writes=False,
        binary_sha256=EXE_SHA256 if args.target=='original' else digest(args.binary.read_bytes()),
        initial_save_sha256=digest(initial))
    def action(ui):
        pid=ui.pid if args.target=='original' else ui.process.pid
        script=out/'history.gdb'
        script.write_text('set pagination off\nset confirm off\nset auto-solib-add off\n'+f'attach {pid}\npython\n'+
            f'import sys\nsys.path.insert(0,{str(ROOT/"tools")!r})\n'+
            'from replay_history_gdb import record_replay_history\n'+
            f'record_replay_history({str(out)!r},{args.target!r},diagnostic_frames={args.diagnostic_frames!r})\nend\n'+
            ('printf "NATIVE_CLOCK_CONSUMED=%u\\n", dd2_tick_calls\nprintf "NATIVE_RANDOM_CONSUMED=%u\\n", dd2_random_calls\n' if args.target=='native' else '')+
            'detach\nquit\n')
        with (out/'gdb.log').open('wb') as log:
            subprocess.run(['gdb','--nx','-q','-batch','-x',str(script)],env=ui.env,
                stdout=log,stderr=subprocess.STDOUT,check=True,timeout=680)
        history=json.loads((out/'history/history.json').read_text())
        raw=(ui.game/'SaveGames').read_bytes()
        require(raw==initial and raw==ui.read(0x754460,len(raw)),'replay history changed its input card')
        if args.target=='native':
            source=json.loads((args.reference/'history.json').read_text())
            require(history['clock_calls']==source['clock_calls'] and history['rng_calls']==source['rng_calls'],
                    'complete original API input extent differs')
            text=(out/'gdb.log').read_text()
            require(f'NATIVE_CLOCK_CONSUMED={source["clock_calls"]}\n' in text and
                    f'NATIVE_RANDOM_CONSUMED={source["rng_calls"]}\n' in text,'strict production API counters differ')
            require((out/'history/ticks.bin').read_bytes()==(args.reference/'ticks.bin').read_bytes() and
                    (out/'history/random.bin').read_bytes()==(args.reference/'random.bin').read_bytes(),
                    'observed actual API results differ')
        report.update(pass_=True,card_unchanged=True,history=str(out/'history'))
    try:
        if args.target=='original':execute(args,out,game,action)
        else:
            source=json.loads((args.reference/'history.json').read_text())
            source_report=json.loads((args.reference.parent/'report.json').read_text())
            require(source['pass_'] and source['target']=='original' and source_report['pass_'] and
                    source_report['binary_sha256']==EXE_SHA256 and source_report['initial_save_sha256']==digest(initial),
                    'completed actual original replay history required')
            for name,n in [('ticks.bin',source['clock_calls']*4),('random.bin',source['rng_calls']*12)]:
                require((args.reference/name).stat().st_size==n,'incomplete original API inputs')
            with (out/'xvfb.log').open('wb') as log:
                display=subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','1280x1024x24'],
                    stdout=subprocess.PIPE,stderr=log)
                ui=None
                try:
                    number=display.stdout.readline().decode().strip();require(number,'Xvfb failed')
                    overrides=dict(DD2_TICK_REPLAY=str((args.reference/'ticks.bin').resolve()),
                        DD2_RANDOM_REFERENCE=str((args.reference/'random.bin').resolve()),
                        DD2_RANDOM_LEVEL='all',DD2_RANDOM_REQUIRE_INITIAL='1',DD2_REALTIME=None,
                        ASAN_OPTIONS='detect_leaks=0:abort_on_error=1')
                    ui=ConfigUI(args.binary.resolve(),game,out,':'+number,1,700,env_override=overrides)
                    ui.boot();action(ui)
                finally:
                    if ui:ui.stop()
                    display.terminate();display.wait(timeout=5)
    finally:
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(out)
    print(args.target,'replay racing history captured; exact video comparison pending',flush=True)


FIELDS = ['level','cf','ticks','countdown','frame_skip','quit','replay','car','mode','type',
          'end','script_cursor','first_time','pedal','clock_calls','rng_calls','regions']


def match_frame(left,right,a,b):
    for name in FIELDS:
        require(left[name]==right[name],f'racing frame {left["index"]} differs: {name}')
    for name,expected,n in [('framebuffer',left['framebuffer_sha256'],307200),('palette',left['palette_sha256'],1024)]:
        index=0 if name=='framebuffer' else 1
        require(len(a[index])==len(b[index])==n and a[index]==b[index] and
                digest(b[index])==expected==right[name+'_sha256'],f'racing frame {left["index"]} differs: {name}')


def compare(args):
    folders={'original':args.original,'native':args.native}
    if args.asan:folders['asan']=args.asan
    if args.browser:folders['browser']=args.browser
    reports={k:json.loads((p/'report.json').read_text()) for k,p in folders.items()}
    histories={k:json.loads((p/'history/history.json').read_text()) for k,p in folders.items()}
    source=histories['original'];original=reports['original'];results={};negatives=[]
    require(original['pass_'] and original['binary_sha256']==EXE_SHA256 and source['pass_'] and
            not original['engine_state_writes'] and not source['engine_state_writes'] and source['hardware_only'],
            'complete unmodified original capture required')
    for target,folder in folders.items():
        if target=='original':continue
        actual=histories[target];report=reports[target]
        require(report['pass_'] and actual['pass_'] and not report['engine_state_writes'] and
                report['initial_save_sha256']==original['initial_save_sha256'],
                'complete port capture with same file input required')
        require(actual['clock_calls']==source['clock_calls'] and actual['rng_calls']==source['rng_calls'] and
                len(actual['frames'])==len(source['frames']),'actual API/video extents differ')
        for name in ['ticks.bin','random.bin']:
            filename='input-'+name if target=='browser' else name
            require((folder/'history'/filename).read_bytes()==(args.original/'history'/name).read_bytes(),
                    'actual API history differs: '+name)
        if target=='browser':
            require(report['trusted_keyboard_events'] and all(e['trusted'] for e in report['trusted_keyboard_events']),
                    'trusted browser inputs required')
            require(report['api_inputs']['clock_sha256']==digest((args.original/'history/ticks.bin').read_bytes()) and
                    report['api_inputs']['random_sha256']==digest((args.original/'history/random.bin').read_bytes()),
                    'browser explicit API input provenance differs')
            require(report['completion']==dict(script_cursor=source['terminal']['end'],first_time=1),
                    'browser tape did not naturally complete')
        frames=[]
        buffer_diagnostics=[]
        for left,right in zip(source['frames'],actual['frames']):
            # Frontend blink phase is not a racing state; the input driver does
            # not align earlier loading/menu animation. Racing engine fields,
            # API extents and all pending indexed pixels are checked explicitly.
            a,b=[],[]
            for suffix,n in [('.bin.z',307200),('.pal',1024)]:
                x=(args.original/'history'/(left['prefix']+suffix)).read_bytes()
                y=(folder/'history'/(right['prefix']+suffix)).read_bytes()
                if suffix=='.bin.z':x,y=zlib.decompress(x),zlib.decompress(y)
                a.append(x);b.append(y)
            match_frame(left,right,a,b)
            if target=='browser':
                require(right['canvas_mismatches']==0,'browser canvas differs from indexed presentation')
                random=(args.original/'history/random.bin').read_bytes()
                seed=struct.unpack_from('<I',random,(right['rng_calls']-1)*12+4)[0] if right['rng_calls'] else 1
                require(right['rng_seed']==seed,'browser independently calculated RNG seed differs')
            if left['primitive_storage_sha256']!=right['primitive_storage_sha256']:
                buffer_diagnostics.append(dict(index=left['index'],original_buffer=left['render_buffer'],
                    actual_buffer=right['render_buffer'],scope='physical primitive storage differs; all vehicle pose/state/FD hashes and actual pixels/palette match'))
            if left['index']==0:
                for field in FIELDS:
                    changed=copy.deepcopy(right)
                    if field=='regions':changed['regions']['car_pose']='0'*64
                    else:changed[field]+=1
                    try:match_frame(left,changed,a,b)
                    except RuntimeError:negatives.append(dict(target=target,case='frame-'+field))
                    else:raise RuntimeError('changed replay field accepted')
                for region in range(2):
                    changed=b[:];changed[region]=bytes([b[region][0]^1])+b[region][1:]
                    try:match_frame(left,right,a,changed)
                    except RuntimeError:negatives.append(dict(target=target,case='pixel' if region==0 else 'palette'))
                    else:raise RuntimeError('changed replay image accepted')
            frames.append(dict(index=left['index'],framebuffer_sha256=right['framebuffer_sha256'],
                               palette_sha256=right['palette_sha256'],regions=right['regions']))
        results[target]=dict(pass_=True,capture=report,frames=frames,physical_buffer_diagnostics=buffer_diagnostics)
    path=prepare_output(args.report);require(WORK in path.parents and not path.exists(),'fresh /tmp report required')
    path.write_text(json.dumps(dict(scope=__doc__,pass_=True,original=original,
        clock_calls=source['clock_calls'],rng_calls=source['rng_calls'],
        api_input_sha256={name:digest((args.original/'history'/name).read_bytes()) for name in ['ticks.bin','random.bin']},
        targets=results,negative_cases=negatives),indent=2)+'\n')
    if args.clean:
        opened=open_files()
        for folder in folders.values():
            for file in (folder/'history').iterdir():
                if file.suffix not in ['.z','.pal'] and not file.name.endswith('-dynamics.bin'):continue
                stat=file.stat();require((stat.st_dev,stat.st_ino) not in opened,'capture still open');file.unlink()
    print('Actual original replay racing API/video comparison: PASS',len(source['frames']),'frames per target')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest='operation',required=True)
    cap=sub.add_parser('capture');cap.add_argument('--target',choices=['original','native'],required=True)
    cap.add_argument('--fixture',type=Path,required=True);cap.add_argument('--output',type=Path,required=True)
    cap.add_argument('--binary',type=Path,default=Path('/tmp/dd2_native'));cap.add_argument('--reference',type=Path)
    cap.add_argument('--diagnostic-frames',type=int,nargs='*',default=[],help='dump only these selected vehicle dynamics checkpoints')
    cmp=sub.add_parser('compare')
    for name in ['original','native','report']:cmp.add_argument('--'+name,type=Path,required=True)
    cmp.add_argument('--asan',type=Path)
    cmp.add_argument('--browser',type=Path);cmp.add_argument('--clean',action='store_true')
    args=parser.parse_args()
    if args.operation=='capture' and args.target=='native' and not args.reference:parser.error('native requires original --reference history directory')
    {'capture':capture,'compare':compare}[args.operation](args)


if __name__=='__main__':main()
