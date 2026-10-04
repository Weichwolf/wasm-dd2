#!/usr/bin/env python3
"""Record real original replay audio, controls, clocks and callback executions.

Load an actual original-produced card through genuine X11 keys without debugger
stops. Observe natural replay completion and unchanged card bytes. Forwarding
timer/ALSA/CD observers and Wine tracing change timing; this records explicit
comparison inputs, not original/port parity or physical timing acceptance.
"""
import argparse
import fcntl
import json
from pathlib import Path
import time

from artifacts import check_space
from verify_original_replay import fixture,metadata,SCRIPT
from verify_championship_save import setup
from verify_configuration_persistence import (ROOT,OriginalUI,original_args,run_original,
    WINE_WORK,EXE_SHA256,digest,require,settings)
from verify_native_replay import NativeUI
from reference.audio import summarize_audio
from reference.game_clock import export_clock
from reference.timer_callbacks import original_report


class RealtimeUI(OriginalUI):
    # The normal X11/pad bridge acknowledges keys through read-only pad masks.
    # OriginalUI.key uses a hardware debugger acknowledgement; audio must keep
    # the original running, so use the shared real-window acknowledgement.
    def key(self,code):NativeUI.key(self,code)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--trace-keyboard',action='store_true',help='record original window-procedure keyboard messages with Wine +msg')
    parser.add_argument('--keep-movie',action='store_true',help='let the original intro finish naturally before menu input')
    parser.add_argument('--trace-video',action='store_true',help='record every successful original DirectDraw presentation without debugger stops')
    parser.add_argument('--video-archive',action='store_true',help='losslessly archive closed presentation blocks; requires --trace-video')
    parser.add_argument('--video-max-frames',type=int,default=4096,help='at most 60000 with --video-archive, otherwise 4096')
    args=parser.parse_args();initial,payload,producer=fixture(args.fixture)
    if (args.video_archive and not args.trace_video) or not 1<=args.video_max_frames<=(60000 if args.video_archive else 4096):
        parser.error('video archival requires --trace-video and a bounded 1..60000 frame count; raw limit is 4096')
    out,game=setup(args,initial)
    report=dict(scope=__doc__,pass_=False,scenario='original-replay',engine_state_writes=False,
                debugger=False,original_exe_sha256=EXE_SHA256,initial_save_sha256=digest(initial),input_keys=[])
    options=original_args()
    options.mode='audio';options.audio=True;options.audio_rate=44100;options.audio_device='clock'
    options.trace_cd=True;options.trace_game_clock=True;options.trace_timer_callbacks=True
    # Flip records are required to order sound services against completed
    # presentations. Relay and dsound alone record clocks/PCM but omit Flips.
    options.wine_debug='-all,+timestamp,+dsound,+ddraw,+relay,+debugstr';options.timeout=150
    if args.trace_keyboard:options.wine_debug+=',+msg'
    options.keep_movie=args.keep_movie
    options.trace_video=args.trace_video
    options.video_archive=args.video_archive;options.video_max_frames=args.video_max_frames
    report['capture_options']=dict(wine_debug=options.wine_debug,keep_movie=options.keep_movie,
                                  startup_escape=not options.keep_movie,trace_video=options.trace_video,
                                  video_archive=options.video_archive,video_max_frames=options.video_max_frames)
    def driver(pid,output,env,deadline,rundir):
        ui=RealtimeUI(pid,rundir,output,env,deadline)
        try:
            report['start_state']=settings(ui)
            for key in ['Right','Right','Right','Return','Return']:
                ui.key(key);report['input_keys'].append(key)
            ui.wait(lambda:'Select' in ui.text(0x46725c) and ui.integer(0x774680)==0)
            ui.key('Return');report['input_keys'].append('Return')
            ui.wait(lambda:ui.integer(0x467074)==1 and ui.integer(0x7746ac)==0 and ui.integer(0x7746c0)>0,60)
            require(ui.read(0x9376b0,SCRIPT)==payload[18:18+SCRIPT] and
                    ui.read(0x795c28,20)==payload[18+SCRIPT:],'original audio replay tape/order differ')
            rows=[];end=time.monotonic()+60
            with (out/'replay.jsonl').open('w') as log:
                while ui.integer(0x467074)==1 and time.monotonic()<end:
                    check_space(out)
                    state=dict(metadata(ui),tick=ui.integer(0x7746c0),countdown=ui.integer(0x784298),
                        pedal=ui.integer(0x792a86),quit=ui.integer(0x7746ac),actual_level=ui.integer(0x936ff4),
                        replay=ui.integer(0x467074),script_cursor=ui.integer(0x9392b4),
                        observed_monotonic_ns=time.monotonic_ns())
                    if state['replay']==1 and state['quit']==0 and state['countdown']<1:
                        rows.append(state);log.write(json.dumps(state)+'\n');log.flush()
                    time.sleep(.02)
            require(ui.integer(0x467074)==0 and ui.integer(0x7746ac)==1 and
                    ui.integer(0x9392b4)==producer['recorded']['end'] and ui.integer(0x9392b0)==1,
                    'original audio replay did not complete the actual tape naturally')
            require(len(rows)>=10 and any(r['pedal']>0 for r in rows),'actual recorded acceleration required')
            ui.wait(lambda:ui.integer(0x936ff4)==0 and settings(ui)==report['start_state'] and
                    'File Manager' in ui.text(0x46975c),30)
            require((rundir/'SaveGames').read_bytes()==initial and ui.read(0x754460,len(initial))==initial,
                    'original audio run changed its card')
            report['completion']=dict(script_cursor=ui.integer(0x9392b4),first_time=ui.integer(0x9392b0))
            report['restored']=settings(ui);report['observed_replay_samples']=len(rows)
            from reference.capture import state
            checkpoint=dict(mode='audio',scenario='original-replay',phase='real X11 original replay; no debugger',
                exe_modified=False,exe_sha256=EXE_SHA256,initial_save_sha256=digest(initial),
                end_state=state(pid),input_keys=report['input_keys'],complete_original_replay=True,
                engine_state_writes=False,debugger=False,audio_comparison='pending')
            (out/'checkpoint.json').write_text(json.dumps(checkpoint,indent=2)+'\n')
        finally:ui.stop()
    try:
        WINE_WORK.mkdir(parents=True,exist_ok=True)
        with (WINE_WORK/'capture.lock').open('w') as lock:
            fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
            run_original(game,out,options,on_menu=driver)
        audio=summarize_audio(out/'audio',require_played=True)
        audio.update(exe_sha256=EXE_SHA256,exe_modified=False,virtual_device_rate=44100,
            virtual_device='clock',wine_debug=options.wine_debug,movie_autoskip=not options.keep_movie)
        (out/'audio/summary.json').write_text(json.dumps(audio,indent=2)+'\n')
        clock=export_clock(out/'wine.log',game/'dd2h.exe',out/'game-clock',allow_terminal_entry=True)
        callbacks=original_report(out,game/'dd2h.exe',allow_terminal=True)
        (out/'timer-callbacks.json').write_text(json.dumps(callbacks,indent=2)+'\n')
        report.update(pass_=True,clock_calls=clock['calls'],completed_callbacks=len(callbacks['completed_callbacks']))
    finally:
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(out)
    print('Actual original replay audio inputs captured; port PCM comparison pending',flush=True)
    if args.trace_keyboard:
        from reference.keyboard_messages import observe
        (out/'keyboard-input.json').write_text(json.dumps(observe(out,args.fixture),indent=2)+'\n')


if __name__=='__main__':main()
