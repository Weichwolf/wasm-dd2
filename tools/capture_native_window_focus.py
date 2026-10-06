#!/usr/bin/env python3
"""Record actual native window focus with normal startup and real X11 input.

A completed capture is a diagnosis, not Original video, PCM or input parity.
"""
from pathlib import Path
import json,sys,subprocess,time,shutil,hashlib,argparse
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--binary',type=Path,required=True)
args=parser.parse_args();out=args.output.resolve()
if Path('/tmp/wasm-dd2') not in out.parents or out.exists():parser.error('Use a fresh directory under /tmp/wasm-dd2/')
out.mkdir(parents=True)
sys.path.insert(0,str(root/'tools'))
from verify_native_replay import NativeUI
from artifacts import check_space
binary=args.binary.resolve()
game=out/'game';game.mkdir(exist_ok=False)
for asset in (root/'DestructionDerby2').iterdir():
    if asset.name=='SaveGames':shutil.copyfile(asset,game/asset.name)
    else:(game/asset.name).symlink_to(asset,target_is_directory=asset.is_dir())
report=dict(scope=__doc__.strip(),target='native',operation='window-focus-capture',engine_state_writes=False,original_port_full_parity='unproven',pass_=False,samples=[],binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),sources={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in ('tools/capture_native_window_focus.py','tools/verify_native_replay.py','re_out/dd2_native.c','re_out/dd2_input.c')})
ui=sink=server=None
with (out/'xvfb.log').open('wb') as log:
    try:
        server=subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','1280x1024x24'],stdout=subprocess.PIPE,stderr=log)
        display=':'+server.stdout.readline().decode().strip()
        ui=NativeUI(binary,game,out,display,1,limit=90)
        report['process']=dict(pid=ui.process.pid,start_token=Path(f'/proc/{ui.process.pid}/stat').read_text().split(') ',1)[1].split()[19])
        ui.boot()
        def sample(label):
            row=dict(label=label,active=ui.integer(0x46042c),timer=ui.integer(0x460474),timer_fires=ui.integer(0x460484),
                     phase=ui.integer(0x4699cc),cf=ui.integer(0x462ff0),flags=ui.read(0x46303f,17).hex(),
                     flips=ui.integer(ui.table['g_frameno']),host_ns=time.monotonic_ns())
            report['samples'].append(row);return row
        sample('baseline');ui.edge('Left',True);ui.wait(lambda:ui.read(0x463045,1)==b'\1');sample('key-down-active')
        with (out/'sink.log').open('wb') as sinklog:
            sink=subprocess.Popen(['xmessage','-title','DD2 native focus sink','-timeout','30','focus target'],env=ui.env,stdout=sinklog,stderr=sinklog)
            ui.wait(lambda:subprocess.run(['xdotool','search','--name','^DD2 native focus sink$'],env=ui.env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode==0)
            subprocess.run(['xdotool','search','--name','^DD2 native focus sink$','windowfocus','--sync'],env=ui.env,check=True,timeout=5)
            time.sleep(.2);sample('inactive-start');time.sleep(1.3);sample('inactive-held')
            ui.edge('Left',False);time.sleep(.3);sample('released-in-sink')
            window=subprocess.check_output(['xdotool','search','--name','^Destruction Derby 2$'],env=ui.env,text=True).splitlines()[-1]
            subprocess.run(['xdotool','windowfocus','--sync',window],env=ui.env,check=True,timeout=5)
            time.sleep(.2);sample('reactivated-after-outside-release')
        report['pass_']=True
    finally:
        if sink and sink.poll() is None:sink.terminate();sink.wait(timeout=5)
        if ui:ui.stop()
        if server:server.terminate();server.wait(timeout=5)
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(out)
print('Actual native focus diagnosis completed')
