#!/usr/bin/env python3
"""Capture the real native replay engine with observed audio/clock services.

An original-produced replay file and six genuine X11 keys load the replay.
With --keyboard-input, all key edges come from verified original window-procedure
entry/return observations, cross-checked against frontend sound triggers.
Without it, sound triggers and an explicit release hypothesis supply diagnostic
scheduling. Hardware stops preserve the schedule without engine writes.
API arguments/status/cursors/gains remain assertions of the native engine's
own calculations. Exact PCM acceptance is separate; physical timing is open.
"""
import argparse
import json
from pathlib import Path
import subprocess

from artifacts import WORK,check_space
from verify_original_replay import fixture
from verify_original_race_audio import validate_capture
from verify_championship_save import setup
from verify_configuration_persistence import ROOT,ConfigUI,settings,require,digest


def input_schedule(original,services,fixture_directory,keyboard_input=None):
    checkpoint=validate_capture(original,fixture_directory)
    proof=json.loads((services/'report.json').read_text())
    component=Path(proof['mixer_component_report']).parent
    timeline=json.loads((component/'timeline.json').read_text())
    clock=json.loads((original/'game-clock/report.json').read_text())
    require(proof['trace_sha256']==timeline['trace_sha256']==clock['trace_sha256'] and
            proof['clock_sha256']==clock['ticks_sha256'] and
            digest((services/'services.bin').read_bytes())==proof['input_sha256'],
            'original service/clock provenance differs')
    mono=[s for s in timeline['sources'] if s['format']['channels']==1 and 'duplicate_of' not in s]
    require(len(mono)==90,'two actual original sound bank generations required')
    bank=mono[-45:];ids={bank[41]['id']:'Right',bank[44]['id']:'Return'}
    events=json.loads((services/'events.json').read_text())
    selected=[e for e in events if e['kind']==8 and e['source'] in ids]
    planned=[dict(key=ids[e['source']],flip=e['flip'],clock_calls=e['clock_calls'],
                  original_trace_line=e['line'],source=e['source']) for e in selected]
    require([p['key'] for p in planned]==checkpoint['input_keys'] and
            all(p['clock_calls']==0 and p['flip']>0 for p in planned) and
            all(a['flip']<b['flip'] for a,b in zip(planned,planned[1:])),
            'six actual original frontend key-trigger positions required')
    if keyboard_input is not None:
        from reference.keyboard_messages import load
        messages=load(original,fixture_directory,keyboard_input)
        for p,k in zip(planned,messages['schedule']):
            require(p['key']==k['key'] and p['flip']==k['flip'] and p['clock_calls']==k['clock_calls'],
                    'actual original key handler and sound-trigger positions differ')
            p.update(k)
    return planned


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ['binary','fixture','original','services','output']:
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--first-return-release-flip',type=int,
        help='explicit diagnostic key-release presentation; original OS release time is not recorded')
    parser.add_argument('--keyboard-input',type=Path,help='verified actual original window-procedure key-down/up observations')
    parser.add_argument('--diagnostic-release-offset',type=int,choices=(-1,1),
        help='deliberately perturb the first observed Enter release for an actual engine rejection test')
    parser.add_argument('--trace-video',action='store_true',help='also capture each indexed presentation and device palette')
    parser.add_argument('--video-reference',type=Path,help='compare original bytes incrementally and discard only matched, closed frames; requires --trace-video')
    args=parser.parse_args();initial,payload,producer=fixture(args.fixture)
    require(args.video_reference is None or args.trace_video,'incremental video comparison requires --trace-video')
    original=args.original.resolve();services=args.services.resolve()
    planned=input_schedule(original,services,args.fixture,args.keyboard_input)
    if args.first_return_release_flip is not None:
        require(args.keyboard_input is None,'observed original keyboard input cannot be overridden by a release hypothesis')
        first=next(p for p in planned if p['key']=='Return')
        require(first['flip']<args.first_return_release_flip<planned[4]['flip'],
            'diagnostic release must follow first confirmation and precede next input')
        first['release_flip']=args.first_return_release_flip
        first['release_scope']='explicit diagnostic hypothesis; not observed original OS release'
    if args.diagnostic_release_offset is not None:
        require(args.keyboard_input is not None,'release perturbation requires actual original keyboard observations')
        first=next(p for p in planned if p['key']=='Return')
        first['release_flip']+=args.diagnostic_release_offset
        first['release_scope']='diagnostic perturbation of observed window-procedure release'
    out,game=setup(args,initial);binary=args.binary.resolve();clock=original/'game-clock/ticks.bin'
    clock_proof=json.loads((original/'game-clock/report.json').read_text())
    clock_encoding=clock_proof.get('ticks_encoding','raw')
    require(clock_encoding in ('raw','DD2TKR1'),'explicit supported original clock encoding required')
    report=dict(scope=__doc__,pass_=False,scenario='original-replay',engine_state_writes=False,
        binary_sha256=digest(binary.read_bytes()),initial_save_sha256=digest(initial),schedule=planned,
        game_clock_encoding=clock_encoding,
        game_clock_sha256=digest(clock.read_bytes()),audio_services_sha256=digest((services/'services.bin').read_bytes()))
    if args.keyboard_input:report['keyboard_input_sha256']=digest(args.keyboard_input.read_bytes())
    if args.diagnostic_release_offset is not None:report['diagnostic_release_offset']=args.diagnostic_release_offset
    stream_source=None
    if args.video_reference:
        from reference.video_frames import observe
        stream_source=observe(original)
        require(stream_source==json.loads(args.video_reference.read_text()),'original streaming video reference differs')
        require(stream_source['trace_sha256']==json.loads((services/'report.json').read_text())['trace_sha256'] and
                stream_source['game_clock_sha256']==report['game_clock_sha256'],
                'streaming video and audio must use the same original run')
    display=ui=video_compare=None
    try:
        with (out/'xvfb.log').open('wb') as log:
            display=subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','1280x1024x24'],
                stdout=subprocess.PIPE,stderr=log)
            number=display.stdout.readline().decode().strip();require(number,'Xvfb failed')
            overrides=dict(DD2_REALTIME=None,DD2_TICK_REPLAY=str(clock),
                DD2_TICK_REPLAY_FORMAT=clock_encoding,
                DD2_AUDIO_SERVICES=str(services/'services.bin'),DD2_MIXPCM=str(out/'mixed.pcm'),
                DD2_AUDIO_SERVICE_REPORT=str(out/'clock-complete.json'),DD2_SNDLOG=str(out/'sound.log'),
                DD2_RACE_STREAM=str(out/'race-stream.jsonl'),ASAN_OPTIONS='detect_leaks=0:abort_on_error=1')
            if args.trace_video:
                limit=json.loads((services/'report.json').read_text())['completion_position']['flip']
                require(0<limit<=(60000 if args.video_reference else 4096),'bounded video capture requires incremental comparison beyond 4096 presentations')
                (out/'video').mkdir()
                overrides.update(DD2_FRAMEDIR=str(out/'video'),DD2_PALDUMP='1',
                    DD2_PRESENT_LOG=str(out/'video/presentations.jsonl'),
                    DD2_VIDEO_CAPTURE_LIMIT=None if args.video_reference else str(limit))
            ui=ConfigUI(binary,game,out,':'+number,1,180,env_override=overrides)
            if args.video_reference:
                from replay_video_stream import Comparison
                video_compare=Comparison(out/'video',original,stream_source,limit,ui.process.terminate).start()
            ui.wait(lambda:ui.integer(0x462cd4)==1)
            report['intro']=dict(movie=1)
            subprocess.run(['xdotool','search','--name','^Destruction Derby 2$','windowfocus','keydown','Escape'],
                env=ui.env,check=True,timeout=5)
            ui.wait(lambda:ui.integer(0x462cd4)==0);ui.edge('Escape',False)
            from reference.capture import state,menu_ready
            ui.wait(lambda:menu_ready(state(ui.process.pid)))
            report['start_state']=settings(ui)
            script=out/'input.gdb'
            script.write_text('set pagination off\nset confirm off\nset auto-solib-add off\n'+
                f'attach {ui.process.pid}\npython\nimport sys\nsys.path.insert(0,{str(ROOT/"tools")!r})\n'+
                f'sys.path.insert(0,{str(ROOT/"tools/reference")!r})\n'+
                f'from replay_audio_input_gdb import drive\ndrive({str(out)!r},{planned!r},{ui.table["ds_service_error"]})\nend\ndetach\nquit\n')
            with (out/'input.log').open('w') as gdb_log:
                subprocess.run(['gdb','--nx','-q','-batch','-x',str(script)],env=ui.env,cwd=out,
                    stdout=gdb_log,stderr=subprocess.STDOUT,check=True,timeout=170)
            history=json.loads((out/'input-history.json').read_text())
            require(history['pass_'] and history['hardware_only'] and not history['engine_state_writes'],
                    'completed read-only hardware input history required')
            terminal=history['terminal'];endpoint=history['endpoint']
            require(terminal['script_cursor']==producer['recorded']['end'] and terminal['first_time']==1,
                    'actual native tape completion required')
            require(endpoint['script_sha256']==digest(payload[18:-20]) and
                    endpoint['order_sha256']==digest(payload[-20:]),'actual loaded native script/order differs')
            require((game/'SaveGames').read_bytes()==initial and endpoint['card_sha256']==digest(initial),
                    'native replay changed original card')
            require(endpoint['settings']==report['start_state'],'native replay did not restore frontend settings')
            report.update(pass_=True,end_state=endpoint['scene'],restored=endpoint['settings'],
                completion=dict(script_cursor=terminal['script_cursor'],first_time=terminal['first_time']),
                audio_services=json.loads((out/'clock-complete.json').read_text()),input_history=history)
    finally:
        if ui:ui.stop()
        if display and display.poll() is None:display.terminate();display.wait(timeout=5)
        video_error=None
        if video_compare:
            try:report['video_stream']=video_compare.finish()
            except Exception as error:
                report.update(pass_=False,video_error=str(error));video_error=error
        (out/'checkpoint.json').write_text(json.dumps(report,indent=2)+'\n');check_space(out)
        if video_error:raise video_error
    print('Actual native replay engine captured; literal PCM comparison pending',flush=True)


if __name__=='__main__':main()
