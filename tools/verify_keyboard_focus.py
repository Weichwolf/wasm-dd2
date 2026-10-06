#!/usr/bin/env python3
"""Compare actual X11 focus/key transport with an independent Wine USER32 oracle.

Checks held/released Left, both Shift sides, left Control and an outside A press across an actual
cross-process foreground change, including message counts. This is a native X11
input/API check, not original game activation, timers, video, PCM or full parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import select
import signal
import subprocess
import time
from artifacts import WORK, check_space, open_files
from verify_native_sdl import config
from verify_sound_cursor import wine_probe

ROOT=Path(__file__).resolve().parents[1]

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def read_row(process):
    if not select.select([process.stdout],[],[],10)[0]:
        raise RuntimeError('Actual X11 fixture did not respond')
    line=process.stdout.readline()
    if not line:raise RuntimeError(f'Actual X11 fixture exited: {process.poll()}')
    return json.loads(line)

def capture(executable,out):
    out.mkdir();rows=[];display=process=sink=None
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    env.update(DD2_WINDOW='1',SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy')
    with (out/'run.log').open('wb') as log:
        try:
            display=subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','800x600x24'],stdout=subprocess.PIPE,stderr=log)
            number=display.stdout.readline().decode().strip()
            if not number:raise RuntimeError('Actual X11 display failed')
            env['DISPLAY']=':'+number
            process=subprocess.Popen([str(executable)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=log,env=env,text=True,bufsize=1)
            if read_row(process)!={'ready':True}:raise RuntimeError('Fixture failed to initialize')
            def x(*args):return subprocess.check_output(['xdotool',*map(str,args)],env=env,stderr=log,text=True,timeout=5).strip()
            window=x('search','--name','^Destruction Derby 2$').splitlines()[-1]
            x('windowfocus','--sync',window)
            def row(label):
                process.stdin.write(label+'\n');process.stdin.flush();value=read_row(process)
                if value['label']!=label:raise RuntimeError('Fixture row order differs')
                rows.append(value)
            row('baseline');x('keydown','Left','Shift_L','Shift_R','Control_L');row('down-active')
            sink=subprocess.Popen(['xmessage','-title','DD2 keyboard focus sink','-timeout','20','focus target'],env=env,stdout=log,stderr=log)
            deadline=time.monotonic()+5
            while True:
                found=subprocess.run(['xdotool','search','--name','^DD2 keyboard focus sink$'],env=env,capture_output=True,text=True)
                if not found.returncode:break
                if time.monotonic()>deadline:raise RuntimeError('Separate focus window did not start')
                time.sleep(.01)
            other=found.stdout.splitlines()[-1];x('windowfocus','--sync',other);row('inactive-held')
            x('keyup','Left');row('released-outside')
            x('windowfocus','--sync',window);row('reactivated-left-up-shifts-held')
            x('keydown','Left');x('keyup','Left','Shift_L','Shift_R','Control_L');row('released-active')
            x('windowfocus','--sync',other);x('keydown','a');row('pressed-outside')
            x('windowfocus','--sync',window);row('reactivated-outside-key-held')
            x('keyup','a');row('outside-key-released-active')
            # Queue the genuine release immediately before FocusOut, while the
            # fixture is stopped. Both are processed by one SDL event drain;
            # a blanket lookahead filter would incorrectly discard KEYUP.
            x('keydown','Left');row('down-before-switch')
            process.send_signal(signal.SIGSTOP)
            try:x('keyup','Left','windowfocus','--sync',other)
            finally:process.send_signal(signal.SIGCONT)
            row('release-before-focus-loss')
            process.stdin.write('quit\n');process.stdin.flush();process.wait(timeout=5)
            if process.returncode:raise RuntimeError('Actual X11 fixture failed')
            (out/'rows.json').write_text(json.dumps(rows,indent=2)+'\n')
            return rows
        finally:
            if process and process.poll() is None:
                process.send_signal(signal.SIGCONT);process.terminate();process.wait(timeout=5)
            if sink and sink.poll() is None:sink.terminate();sink.wait(timeout=5)
            if display:display.terminate();display.wait(timeout=5)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--mingw',default='i686-w64-mingw32-gcc')
    parser.add_argument('--negative-controls',action='store_true')
    parser.add_argument('--clean',action='store_true')
    args=parser.parse_args();out=args.output.resolve()
    if WORK not in out.parents or out.exists():parser.error('Use a fresh directory under /tmp/wasm-dd2/')
    out.mkdir(parents=True);check_space(out)
    names=['tools/keyboard_focus_win32_probe.c','tools/keyboard_focus_port_probe.c','tools/verify_keyboard_focus.py','re_out/dd2_input.c','re_out/dd2_native.c']
    report=dict(scope=__doc__,original_port_full_parity='unproven',pass_=False,sources={n:sha(ROOT/n) for n in names})
    exe=out/'user32.exe'
    subprocess.run([args.mingw,'-Wall','-Wextra','-Werror',str(ROOT/names[0]),'-o',str(exe)],check=True)
    report['wine_version']=subprocess.check_output(['wine','--version'],text=True).strip()
    report['reference_exe_sha256']=sha(exe)
    reference=wine_probe(exe,out,{k:v for k,v in os.environ.items() if not k.startswith('DD2_')})
    (out/'user32.json').write_text(json.dumps(reference,indent=2)+'\n')
    if len(reference)!=9 or reference[2]['active']!=0 or reference[2]['left']!=1 or reference[3]['left']!=0 or reference[4]['shift']!=[1,1,1] or reference[6]['a']!=1 or reference[6]['active']!=0 or reference[7]['down']!=5 or reference[-1]['up']!=5:
        raise RuntimeError('USER32 did not exercise the declared focus/physical-state transitions')
    def build(native,path):
        subprocess.run(['gcc','-m32','-no-pie','-DDD2_NATIVE_SDL','-Wall','-Wextra','-Werror',*config('cflags'),
                        f'-I{ROOT/"re_out"}',str(native),str(ROOT/'re_out/dd2_input.c'),str(ROOT/names[1]),*config('libs'),'-o',str(path)],check=True)
    fixture=out/'native';build(ROOT/'re_out/dd2_native.c',fixture)
    actual=capture(fixture,out/'native-capture')
    report.update(reference=reference,actual=actual,native_sha256=sha(fixture))
    report['match_']=actual[:9]==reference
    report['genuine_release_before_focus_loss']=actual[-2]['active']==1 and actual[-2]['left']==1 and actual[-2]['down']==6 and actual[-2]['up']==5 and actual[-1]==dict(label='release-before-focus-loss',active=0,left=0,a=0,shift=[0,0,0],control=[0,0,0],down=6,up=6)
    if args.negative_controls:
        old=out/'old-native.c';old.write_bytes(subprocess.check_output(['git','show','ce18e7e:re_out/dd2_native.c'],cwd=ROOT))
        bad=out/'negative';build(old,bad);wrong=capture(bad,out/'negative-capture')
        report['negative_controls']=[dict(case='actual-pre-fix-SDL-reset-and-focus-cleanup',rejected=wrong[:9]!=reference,actual=wrong)]
    if any(sha(ROOT/n)!=digest for n,digest in report['sources'].items()):raise RuntimeError('Source changed during verification')
    report['pass_']=report['match_'] and report['genuine_release_before_focus_loss'] and all(c['rejected'] for c in report.get('negative_controls',[]))
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(out)
    if not report['pass_']:raise RuntimeError(f'Actual X11 input differs from USER32; see {out/"report.json"}')
    if args.clean:
        import shutil
        opened=open_files();removed=[]
        for path in out.iterdir():
            if path.name=='report.json':continue
            files=list(path.rglob('*')) if path.is_dir() else [path]
            size=0
            for f in files:
                if f.is_symlink() or not f.is_file():continue
                s=f.stat()
                if (s.st_dev,s.st_ino) in opened:raise RuntimeError('Completed output is still open')
                size+=s.st_size
            removed.append(dict(name=path.name,bytes=size))
            if path.is_dir():shutil.rmtree(path)
            else:path.unlink()
        (out/'cleanup.json').write_text(json.dumps(dict(removed=removed),indent=2)+'\n')
    print('PASS actual native X11: USER32 key states/counts across focus; real pre-focus release preserved',flush=True)

if __name__=='__main__':main()
