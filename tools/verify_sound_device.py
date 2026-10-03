#!/usr/bin/env python3
"""Check actual Wine default DirectSound interface lifetimes and port idle epochs."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
from verify_sound_cursor import wine_probe

ROOT=Path(__file__).resolve().parent.parent
SOURCE=ROOT/'tools/sound_device_test.c'

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mingw',default='i686-w64-mingw32-gcc')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    expected={'device_addref':2,'device_release_retained':1,'interface_final_release':0,
              'shared_device_survived':True,'reopened':True}
    pcm=struct.pack('<ff',1234/32768,1234/32768)*528
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    env.update(DD2_SND_RATE='44100',ASAN_OPTIONS='detect_leaks=1:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
    report={'scope':__doc__,'fixture_sha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
            'backend_sha256':hashlib.sha256((ROOT/'re_out/dd2h_stubs.c').read_bytes()).hexdigest(),
            'api':expected,'controlled_pcm_bytes':len(pcm),'targets':[]}
    with tempfile.TemporaryDirectory(prefix='device-fixture-',dir=args.output) as temp:
        directory=Path(temp)
        exe=directory/'device.exe'
        subprocess.run([args.mingw,'-Wall','-Wextra','-Werror',str(SOURCE),'-ldsound','-o',str(exe)],check=True)
        actual=wine_probe(exe,directory,env)
        if actual!=expected:raise RuntimeError(f'actual Wine device lifecycle differs: {actual}')
        report['wine']=actual
        report['wine_exe_sha256']=hashlib.sha256(exe.read_bytes()).hexdigest()
        report['wine_version']=subprocess.check_output(['wine','--version'],env=env,text=True).strip()
        print('PASS actual Wine: device AddRef/Release, shared default interface lifetime and reopen',flush=True)
        common=['-std=gnu99','-w','-DDD2_NO_FOPEN_WRAP','-ffunction-sections','-fdata-sections',
                f'-I{ROOT/"re_out"}',str(ROOT/'re_out/dd2h_stubs.c'),str(SOURCE),'-Wl,--gc-sections']
        native,wasm=directory/'native',directory/'wasm.js'
        subprocess.run(['gcc','-m32','-no-pie','-DDD2_NATIVE_SDL','-fsanitize=address,undefined','-g',*common,'-o',str(native)],check=True)
        subprocess.run(['emcc',*common,'-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760',
                        '--pre-js',str(ROOT/'tools/node_env.js'),'-o',str(wasm)],check=True)
        for target,command in [('native',[str(native)]),('wasm',['node',str(wasm)])]:
            output=directory/f'{target}.pcm'
            run=subprocess.run([*command,str(output)],env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
            (args.output/f'{target}.log').write_text(run.stdout+run.stderr)
            run.check_returncode()
            if json.loads(run.stdout)!=expected or output.read_bytes()!=pcm:
                raise RuntimeError(f'{target}: API, PCM extent or bytes crosses the 70-second device gap')
            report['targets'].append({'target':target,'pcm_bytes':len(pcm),'idle_gap_ms':69991,'mci_music_survived_ds_release':True})
            print(f'PASS {target}: {len(pcm)} exact PCM bytes over two epochs, no movie-gap/fractional catchup',flush=True)
        # Deliberately retain the old clock across final device Release and
        # reopen. This must fail the same runtime/source test, not compilation.
        broken=directory/'stale-clock.c'
        broken.write_text((ROOT/'re_out/dd2h_stubs.c').read_text().replace('ds_clock_init=0;','/* negative: stale device clock */'))
        bad=directory/'negative'
        broken_common=[str(broken) if item==str(ROOT/'re_out/dd2h_stubs.c') else item for item in common]
        subprocess.run(['gcc','-m32','-no-pie','-DDD2_NATIVE_SDL',*broken_common,'-o',str(bad)],check=True)
        rejected=subprocess.run([str(bad),str(directory/'negative.pcm')],env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30)
        (args.output/'negative.log').write_text(rejected.stdout+rejected.stderr)
        if rejected.returncode!=1 or 'queued sample crosses the closed device epoch' not in rejected.stderr:
            raise RuntimeError('stale device clock was not rejected by the intended PCM/sink comparison')
        report['negative_stale_clock_rejected']=True
        print('PASS negative control: retained old clock is rejected by the actual PCM/sink test',flush=True)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':main()
