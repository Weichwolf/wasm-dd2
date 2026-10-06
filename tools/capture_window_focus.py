#!/usr/bin/env python3
"""Observe unchanged original focus loss with real X11 input and four hardware slots.

Capture only activation, held input flags and message waits. This bounded
original observation does not accept port behavior, resumption, video or PCM.
"""
import sys,json,os,time,subprocess,hashlib,argparse,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--mingw',default=shutil.which('i686-w64-mingw32-gcc') or str(root/'third_party/mingw-sdk/usr/bin/i686-w64-mingw32-gcc-win32'))
args=parser.parse_args();out=args.output.resolve()
if Path('/tmp/wasm-dd2') not in out.parents or out.exists():parser.error('Use a fresh directory under /tmp/wasm-dd2/')
out.mkdir(parents=True)
sys.path.insert(0,str(root/'tools'));sys.path.insert(0,str(root/'tools/reference'))
from reference import capture
from verify_configuration_persistence import original_args
from artifacts import check_space,open_files
capture.WORK=out/'work';capture.WORK.mkdir()
subprocess.run([args.mingw,'-Wall','-Wextra','-Werror',str(root/'tools/reference/window_focus_probe.c'),'-o',str(out/'window.exe')],check=True)
report=dict(scope=__doc__.strip(),operation='original-window-focus-capture',engine_state_writes=False,original_port_full_parity='unproven',reactivation='not captured',pass_=False,
            exe_sha256=hashlib.sha256((root/'DestructionDerby2/dd2h.exe').read_bytes()).hexdigest(),
            focus_probe_sha256=hashlib.sha256((out/'window.exe').read_bytes()).hexdigest(),
            sources={name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in ('tools/capture_window_focus.py','tools/window_focus_observer.py','tools/reference/window_focus_probe.c','tools/reference/capture.py')},actions=[],samples=[])
def driver(pid,output,env,deadline,rundir):
    def token(pid):return Path(f'/proc/{pid}/stat').read_text().split(') ',1)[1].split()[19]
    report['original_process']=dict(pid=pid,start_token=token(pid),display=env['DISPLAY'])
    memory=open(f'/proc/{pid}/mem','rb',buffering=0)
    def sample(label):
        def word(a):return int.from_bytes(os.pread(memory.fileno(),4,a),'little')
        row=dict(label=label,host_ns=time.monotonic_ns(),active=word(0x46042c),timer=word(0x460474),
                 timer_fires=word(0x460484),phase=word(0x4699cc),cf=word(0x462ff0),
                 flags=os.pread(memory.fileno(),17,0x46303f).hex())
        report['samples'].append(row);return row
    def wait(test,seconds=10):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            if test():return
            check_space(out);time.sleep(.04)
        raise TimeoutError('Original focus condition did not occur')
    def action(*args):
        subprocess.run(['xdotool',*map(str,args)],env=env,check=True,timeout=5,stdout=subprocess.DEVNULL)
        report['actions'].append(dict(command=list(map(str,args)),host_ns=time.monotonic_ns()))
    focus_serial=0
    def focus(target):
        nonlocal focus_serial
        focus_serial+=1
        (out/'focus-request.tmp').write_text(f'{focus_serial} {target}\n');(out/'focus-request.tmp').replace(out/'focus-request')
        report['actions'].append(dict(focus_target=target,host_ns=time.monotonic_ns()))
        time.sleep(.15)
    window=subprocess.check_output(['xdotool','search','--name','PC-DD2'],env=env,text=True).splitlines()[-1]
    script=out/'observer.gdb'
    script.write_text('set pagination off\nset confirm off\nset auto-solib-add off\n'+f'attach {pid}\npython\nimport sys\nsys.path.insert(0,{str(root/'tools')!r})\nfrom window_focus_observer import run\nrun({str(out)!r})\nend\ndetach\nquit\n')
    with (out/'gdb.log').open('wb') as log,(out/'sink.log').open('wb') as sinklog:
        debugger=subprocess.Popen(['gdb','--nx','-q','-batch','-x',str(script)],env=env,stdout=log,stderr=subprocess.STDOUT)
        report['gdb_process']=dict(pid=debugger.pid,start_token=token(debugger.pid))
        sink=None
        try:
            wait(lambda:(out/'ready').exists()); action('windowfocus','--sync',window);sample('baseline')
            sink=subprocess.Popen(['wine',str(out/'window.exe'),'Z:'+str(out/'focus-request').replace('/','\\')],env=env,stdout=sinklog,stderr=sinklog)
            wait(lambda:'\"kind\":\"ready\"' in (out/'sink.log').read_text())
            action('keydown','Left');wait(lambda:os.pread(memory.fileno(),1,0x463045)!=b'\0')
            sample('key-down-active')
            focus('sink');wait(lambda:sample('await-deactivation')['active']==0)
            sample('inactive-start');time.sleep(1.3);sample('inactive-held')
            action('keyup','Left');time.sleep(.3);sample('released-in-sink')
            rows={r['label']:r for r in report['samples']}
            assert rows['key-down-active']['flags'][12:14]=='01'
            for label in ('inactive-start','inactive-held','released-in-sink'):
                assert rows[label]['active']==rows[label]['timer']==0
                assert rows[label]['flags'][12:14]=='01'
            assert rows['inactive-held']['timer_fires']>rows['inactive-start']['timer_fires']
            report['bounded_conditions_observed']=True
        finally:
            (out/'stop').touch()
            if debugger.poll() is None:
                try:debugger.wait(timeout=1)
                except subprocess.TimeoutExpired:
                    debugger.send_signal(__import__('signal').SIGINT);debugger.wait(timeout=5)
                    report['observer_stop']='Exact owned GDB interrupted for diagnostic closure; game state was not changed by debugger writes'
            if debugger.returncode!=0:raise RuntimeError('Original observer failed')
            if sink and sink.poll() is None:sink.terminate();sink.wait(timeout=5)
            memory.close()
options=original_args();options.timeout=180;completed=False
try:
    capture.run(root/'DestructionDerby2',out,options,on_menu=driver)
    completed=True
finally:
    events=out/'events.jsonl'
    if events.exists():report['observer_events']=[json.loads(line) for line in events.read_text().splitlines()]
    if completed and report.get('bounded_conditions_observed'):
        observed=report['observer_events']
        assert any(r['kind']=='activation-return' and r['requested_active']==0 and r['active']==r['timer']==0 and r['input_flags'][12:14]=='01' for r in observed)
        assert any(r['kind']=='wait-enter' and r['active']==0 for r in observed)
        assert any(r.get('message')==0x100 and r['wparam']==0x25 for r in observed)
        report['pass_']=True
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(out)
if report['pass_']:
    # Capture.run has waited for this dedicated Wine server and all clients.
    # Keep the processed observations and hashes; remove closed raw output.
    opened=open_files();removed=[]
    candidates=[out/name for name in ('events.jsonl','wine.log','gdb.log','sink.log','xvfb.log','window.exe')]
    candidates.extend(p for p in capture.WORK.rglob('*') if p.is_file() and not p.is_symlink())
    for path in candidates:
        stat=path.stat()
        if (stat.st_dev,stat.st_ino) in opened:raise RuntimeError('Capture output remains open: '+str(path))
    for path in candidates:
        with path.open('rb') as source:sha=hashlib.file_digest(source,'sha256').hexdigest()
        removed.append(dict(path=str(path.relative_to(out)),bytes=path.stat().st_size,sha256=sha))
    (out/'cleanup.json').write_text(json.dumps(dict(report_sha256=hashlib.sha256((out/'report.json').read_bytes()).hexdigest(),removed_bytes=sum(p['bytes'] for p in removed),removed=removed),indent=2)+'\n')
    for path in candidates:
        if capture.WORK not in path.parents:path.unlink()
    shutil.rmtree(capture.WORK)
print('Original focus observation completed',flush=True)
