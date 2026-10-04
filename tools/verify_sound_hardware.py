#!/usr/bin/env python3
"""Compare real Wine hardware rejection with the production native/WASM API.

Checks HRESULT, cleared output pointer, the engine-style software retry and
primary-buffer exemption. Does not establish full engine audio/video parity.
"""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from artifacts import WORK,check_space,prepare_output,run_bounded

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference'))
from capture import original_pid
from timer_observer import KERNEL


def verify(output):
    output=prepare_output(output)
    if WORK not in output.parents:raise ValueError('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=False)
    fixture=ROOT/'tools/sound_hardware_test.c';unit=ROOT/'build/dd2h_stubs.c'
    expected=dict(hardware_result=0x80004001,hardware_pointer_null=True,software_retry=True,primary_exempt=True)
    common=['-O2','-std=gnu99','-w','-DDD2_NO_FOPEN_WRAP','-ffunction-sections','-fdata-sections',
            '-I'+str(ROOT/'build'),str(unit),str(fixture),'-Wl,--gc-sections']
    targets=[]
    for name,compiler in [('native',['gcc','-m32','-no-pie']),
                          ('native-asan',['gcc','-m32','-no-pie','-fsanitize=address','-g']),
                          ('wasm',['emcc','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760',
                                   '--pre-js',str(ROOT/'tools/node_env.js')])]:
        binary=output/(name+'.js' if name=='wasm' else name)
        with (output/(name+'-build.log')).open('wb') as log:
            run_bounded([*compiler,*common,'-o',str(binary)],directory=output,timeout=120,check=True,stdout=log,stderr=subprocess.STDOUT)
        env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
        env.update(DD2_SOUND='1',ASAN_OPTIONS='detect_leaks=1:abort_on_error=1')
        result=subprocess.run((['node',str(binary)] if name=='wasm' else [str(binary)]),env=env,capture_output=True,text=True,timeout=20)
        (output/(name+'.log')).write_text(result.stdout+result.stderr)
        if result.returncode or json.loads(result.stdout)!=expected:raise AssertionError('Hardware API probe failed: '+name)
        targets.append(dict(target=name,pass_=True,observed=expected))
    # SDK-free Win32 build uses the same fixture and actual COM vtables.
    (output/'kernel32.def').write_text('LIBRARY kernel32.dll\nEXPORTS\n'+''.join(f'  {name}@{size}\n' for name,size in KERNEL.items()))
    (output/'dsound.def').write_text('LIBRARY dsound.dll\nEXPORTS\n  DirectSoundCreate@12\n')
    for name in ('kernel32','dsound'):
        subprocess.run(['llvm-dlltool','-m','i386','-k','-d',str(output/(name+'.def')),'-l',str(output/(name+'.lib'))],check=True)
    subprocess.run(['clang','--target=i686-windows-gnu','-ffreestanding','-fno-builtin','-fno-stack-protector','-O2',
                    '-Wall','-Wextra','-Werror','-c',str(fixture),'-o',str(output/'probe.obj')],check=True)
    subprocess.run(['lld-link','/nodefaultlib','/timestamp:0','/entry:start','/machine:x86','/subsystem:console',
                    '/out:'+str(output/'probe.exe'),str(output/'probe.obj'),str(output/'kernel32.lib'),str(output/'dsound.lib')],check=True)
    prefix=WORK/'wine-reference/prefix'
    with (prefix.parent/'capture.lock').open('w') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        if original_pid(prefix) is not None:raise RuntimeError('Original still running in private prefix')
        env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
        (output/'asound.conf').write_text('pcm.!default { type null }\n')
        env.update(WINEPREFIX=str(prefix),WINEARCH='win32',WINEDEBUG='-all',
                   WINEDLLOVERRIDES='dsound=b;winmm=b',ALSA_CONFIG_PATH=str(output/'asound.conf'))
        with (output/'wine.log').open('wb') as log:
            display=subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','640x480x16'],stdout=subprocess.PIPE,stderr=log)
            try:
                number=display.stdout.readline().decode().strip()
                if not number:raise RuntimeError('Xvfb did not start')
                env['DISPLAY']=':'+number
                result=subprocess.run(['wine',str(output/'probe.exe')],cwd=output,env=env,stdout=subprocess.PIPE,stderr=log,text=True,timeout=30)
                if result.returncode or json.loads(result.stdout)!=expected:raise AssertionError('Actual Wine hardware probe failed')
                targets.append(dict(target='wine',pass_=True,observed=json.loads(result.stdout)))
            finally:
                subprocess.run(['wineserver','-k'],env=env,stdout=log,stderr=log,timeout=10)
                subprocess.run(['wineserver','-w'],env=env,stdout=log,stderr=log,timeout=10)
                display.terminate();display.wait(timeout=5)
    mutant=output/'old.c';text=unit.read_text();anchor='if(desc && !(desc[1]&1) && (desc[1]&4))'
    if text.count(anchor)!=1:raise ValueError('Production hardware rejection required')
    mutant.write_text(text.replace(anchor,'if(0)'))
    command=['gcc','-m32','-no-pie',*common,'-o',str(output/'old')]
    command=[str(mutant) if item==str(unit) else item for item in command]
    subprocess.run(command,check=True)
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')};env['DD2_SOUND']='1'
    result=subprocess.run([str(output/'old')],env=env,capture_output=True,text=True,timeout=20)
    if result.returncode!=2 or 'hardware unsupported result/pointer' not in result.stderr:
        raise AssertionError('Old hardware acceptance was not rejected')
    report=dict(scope=__doc__.strip(),pass_=True,targets=targets,negative_old_hardware_acceptance=True,
                source_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (unit,fixture)},
                wine_version=subprocess.check_output(['wine','--version'],text=True).strip())
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    mutant.unlink()
    for file in output.glob('*.log'):file.unlink()
    check_space(output)
    return report


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();report=verify(args.output)
    print('Hardware/software buffer fallback: PASS;',len(report['targets']),'targets; old hardware acceptance rejected')
