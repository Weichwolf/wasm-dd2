#!/usr/bin/env python3
"""Compare production modifier messages/state with a genuine Wine USER32 oracle.

Scope: high-bit pressed states, generic modifier VKs, event direction/count and
KEY/SYSKEY classes for declared transitions (scan input for mapped keys; virtual input for
F13-F24, which lack scans in the Wine reference layout). This is not physical
Windows layout, scan-code/lparam, character, AltGr or original full-game proof.
"""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
from verify_sound_cursor import wine_probe
from verify_native_sdl import config

ROOT=Path(__file__).resolve().parent.parent

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mingw',default='i686-w64-mingw32-gcc')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    env.update(DD2_WINDOW='1',SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',ASAN_OPTIONS='detect_leaks=1:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
    exe=out/'user32.exe'
    subprocess.run([args.mingw,'-Wall','-Wextra','-Werror',str(ROOT/'tools/keyboard_win32_probe.c'),'-o',str(exe)],check=True)
    reference=wine_probe(exe,out,env)
    count=len(re.findall(r'\{0x[0-9a-f]+,[01],"[^"]+"\}',(ROOT/'tools/keyboard_events.h').read_text()))
    if count!=104 or len(reference)!=count or [row['step'] for row in reference]!=list(range(count)):
        raise RuntimeError('USER32 reference lost/duplicated physical transitions')
    if reference[2]['message']!=257 or reference[2]['shift']!=[1,0,1]:
        raise RuntimeError('USER32 oracle did not exercise release with the other Shift held')
    (out/'user32.json').write_text(json.dumps(reference,indent=2)+'\n')
    print(f'PASS actual Wine USER32: {count} transitions, modifiers/Alt/Ctrl/repeat, F1-F24 and eight editing/navigation keys',flush=True)
    report={'scope':__doc__,'wine_version':subprocess.check_output(['wine','--version'],env=env,text=True).strip(),
            'reference_exe_sha256':sha(exe),'sources':{},'records':len(reference),'targets':[]}
    for name in ['tools/keyboard_events.h','tools/keyboard_win32_probe.c','tools/keyboard_port_probe.c','re_out/dd2_input.c','re_out/dd2_native.c']:
        report['sources'][name]=sha(ROOT/name)
    common=['-std=gnu99','-Wall','-Wextra','-Werror','-ffunction-sections','-fdata-sections',
            f'-I{ROOT/"re_out"}',str(ROOT/'re_out/dd2_input.c'),str(ROOT/'tools/keyboard_port_probe.c'),'-Wl,--gc-sections']
    native,wasm=out/'native',out/'wasm.js'
    native_flags=['gcc','-m32','-no-pie','-DDD2_NATIVE_SDL','-DDD2_KEYBOARD_SDL',*config('cflags')]
    subprocess.run([*native_flags,*common,str(ROOT/'re_out/dd2_native.c'),*config('libs'),'-o',str(native)],check=True)
    # Separate sanitized direct bridge fixture avoids attributing system SDL
    # library/global allocations to the C input bridge.
    sanitizer=out/'native-sanitized'
    subprocess.run(['gcc','-m32','-no-pie','-fsanitize=address,undefined','-g',*common,'-o',str(sanitizer)],check=True)
    subprocess.run(['emcc',*common,'-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760','-o',str(wasm)],check=True)
    targets=[('native-sdl',[str(native),'sdl']),('native-vk',[str(sanitizer),'vk']),
             ('native-browser-code',[str(sanitizer),'browser']),('wasm-vk',['node',str(wasm),'vk']),
             ('wasm-browser-code',['node',str(wasm),'browser'])]
    for target,command in targets:
        run=subprocess.run(command,env=env,capture_output=True,text=True,timeout=30)
        (out/f'{target}.log').write_text(run.stdout+run.stderr);run.check_returncode()
        actual=json.loads(run.stdout)
        if actual!=reference:
            raise RuntimeError(f'{target}: window messages or physical/generic pressed states differ from actual USER32')
        report['targets'].append(target)
        print(f'PASS {target}: all {count} USER32 records exact',flush=True)
    # Reproduce the pre-fix platform+bridge, rather than a hypothetical change:
    # the very first physical modifier produces two engine messages.
    old_input,old_native=out/'old-input.c',out/'old-native.c'
    for path,name in [(old_input,'re_out/dd2_input.c'),(old_native,'re_out/dd2_native.c')]:
        path.write_bytes(subprocess.check_output(['git','show',f'941d609:{name}'],cwd=ROOT))
    bad=out/'negative'
    negative_common=[str(old_input) if part==str(ROOT/'re_out/dd2_input.c') else part for part in common]
    subprocess.run([*native_flags,*negative_common,str(old_native),*config('libs'),'-o',str(bad)],check=True)
    run=subprocess.run([str(bad),'sdl'],env=env,capture_output=True,text=True,timeout=30)
    (out/'negative.log').write_text(run.stdout+run.stderr)
    if run.returncode!=1 or 'one window message per transition' not in run.stderr:
        raise RuntimeError('pre-fix double-dispatch was not rejected at the runtime message boundary')
    report['pre_fix_double_dispatch_rejected']=True
    print('PASS negative control: pre-fix double modifier dispatch rejected at runtime',flush=True)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':main()
