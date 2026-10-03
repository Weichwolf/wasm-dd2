#!/usr/bin/env python3
"""Exercise normal startup, cancel Quit and confirm Quit through real X11 keys.

Require the original user WinMain return1 and actual closure of every opened
SDL stream. Read-only observer/state checks; the user's save is copied first.
Complete original A/V timing and physical hardware remain unproven.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import time
from verify_native_sdl import config
from verify_native_window import symbols

ROOT=Path(__file__).resolve().parent.parent

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();binary=args.binary.resolve();output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=False)
    table=symbols(binary)
    display=process=memory=None
    with tempfile.TemporaryDirectory(prefix='quit-fixture-',dir=output) as temp:
        directory=Path(temp);game=directory/'game';game.mkdir();observer=directory/'observer.so'
        subprocess.run(['gcc','-m32','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror',*config('cflags'),
                        str(ROOT/'tools/native_movie_observer.c'),*config('libs'),'-ldl','-o',str(observer)],check=True)
        for asset in (ROOT/'DestructionDerby2').iterdir():
            if asset.name=='SaveGames':shutil.copyfile(asset,game/asset.name)
            else:(game/asset.name).symlink_to(asset,target_is_directory=asset.is_dir())
        with (output/'run.log').open('w') as log:
            try:
                display=subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','640x480x24'],stdout=subprocess.PIPE,stderr=log)
                number=display.stdout.readline().decode().strip()
                if not number:raise RuntimeError('Xvfb failed')
                env={k:v for k,v in os.environ.items() if not k.startswith('DD2_') and k!='LD_PRELOAD'}
                env['DISPLAY']=':'+number
                process=subprocess.Popen([str(binary)],cwd=game,env={**env,'DD2_WINDOW':'1','SDL_AUDIODRIVER':'dummy',
                    'LD_PRELOAD':str(observer),'DD2_NATIVE_MOVIE_OBSERVE':str(output)},stdout=log,stderr=log)
                def read(address,count):
                    nonlocal memory
                    if memory is None:
                        if Path(f'/proc/{process.pid}/exe').resolve()!=binary:raise OSError('Child exec pending')
                        memory=open(f'/proc/{process.pid}/mem','rb',buffering=0)
                    return os.pread(memory.fileno(),count,address)
                def integer(address):return struct.unpack('<I',read(address,4))[0]
                def text(address):
                    pointer=integer(address)
                    return read(pointer,64).split(b'\0',1)[0].decode('ascii') if 0x400000<pointer<0x980400 else ''
                def wait(predicate):
                    for _ in range(1000):
                        if process.poll() is not None:raise RuntimeError(f'Unexpected early exit: {process.returncode}')
                        try:
                            if predicate():return
                        except (OSError,struct.error):pass
                        time.sleep(0.02)
                    raise RuntimeError('Normal Quit state timed out')
                def edge(key,down):subprocess.run(['xdotool','keydown' if down else 'keyup',key],env=env,check=True,timeout=5)
                wait(lambda:integer(0x462cd4)==1)
                subprocess.run(['xdotool','search','--name','^Destruction Derby 2$','windowfocus'],env=env,check=True)
                edge('Escape',True);wait(lambda:integer(0x462cd4)==0);edge('Escape',False)
                wait(lambda:integer(table['g_frameno'])>=200 and 'Wrecking' in text(0x46975c))
                def tap(key,vk):
                    bits=(1,8,0x10,0x20,0x40,0x80,0x100,0x200,0x400,0x800,0x1000,0x2000,0x4000,0x8000)
                    mapping=read(0x46302c,14)
                    mask=next(bits[i] for i in (0,1,2,3,4,5,8,6,9,7,10,11,12,13) if mapping[i]==vk)
                    edge(key,True);wait(lambda:struct.unpack('<H',read(0x754448,2))[0]&mask)
                    edge(key,False);wait(lambda:not(struct.unpack('<H',read(0x754448,2))[0]&mask))
                    time.sleep(0.2)
                tap('Escape',0x1b);wait(lambda:'Quit DD2?' in text(0x4697ac))
                tap('Return',0x0d);wait(lambda:'Quit DD2?' not in text(0x4697ac) and 'Wrecking' in text(0x46975c))
                before=integer(table['g_frameno']);wait(lambda:integer(table['g_frameno'])>=before+5)
                tap('Escape',0x1b);wait(lambda:'Quit DD2?' in text(0x4697ac))
                tap('Left',0x25)
                edge('Return',True)
                try:status=process.wait(timeout=10)
                finally:edge('Return',False)
                if status!=1:raise RuntimeError(f'Normal Quit did not return original user WinMain status1: {status}')
            finally:
                if memory:memory.close()
                if process and process.poll() is None:process.terminate();process.wait(timeout=5)
                if display:display.terminate();display.wait(timeout=5)
    events=[json.loads(line) for line in (output/'events.jsonl').read_text().splitlines()]
    opened=[e for e in events if e['event']=='open' and e['device']]
    closed=[e for e in events if e['event']=='close']
    if len(opened)!=len(closed) or len({e['stream'] for e in closed})!=len(opened):raise RuntimeError('Quit left SDL audio stream lifetimes open')
    for e in closed:
        if (output/f"stream{e['stream']}.pcm").stat().st_size!=e['bytes']:raise RuntimeError('Closed audio stream extent differs from accepted journal')
    frames=sum(e['event']=='present' for e in events)
    report={'scope':__doc__,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'intro_skipped':True,
            'quit_cancelled':True,'quit_confirmed':True,'exit_status':status,'closed_streams':len(closed),'exact_renderer_pixels':frames*307200}
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS native normal Quit: cancel stays in menu; confirm returns1; {len(closed)} actual streams closed; {frames*307200} renderer pixels exact',flush=True)

if __name__=='__main__':main()
