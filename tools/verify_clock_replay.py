#!/usr/bin/env python3
"""Verify exact recorded game-clock return values and strict input extent.

This is a clock-transport fixture, not original engine video/audio acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
ROOT=Path(__file__).resolve().parent.parent

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    common=['-std=gnu99','-w','-DDD2_NO_FOPEN_WRAP','-ffunction-sections','-fdata-sections',
        f'-I{ROOT/"re_out"}',str(ROOT/'re_out/dd2_stubs.c'),str(ROOT/'tools/clock_replay_test.c'),'-Wl,--gc-sections']
    native,wasm=out/'native',out/'wasm.js'
    subprocess.run(['gcc','-m32','-no-pie','-fsanitize=address,undefined','-g',*common,'-o',str(native)],check=True)
    subprocess.run(['emcc',*common,'-sNODERAWFS=1','-sEXIT_RUNTIME=1','--pre-js',str(ROOT/'tools/node_env.js'),'-o',str(wasm)],check=True)
    payload=struct.pack('<IIII',0xfffffff0,0xfffffff8,3,3)
    expected={'exact_clock_returns':4,'uint32_wrap':True,'repeat_retained':True}
    cases=[('exact',payload,None),('exhausted',payload[:4],'clock input exhausted'),
        ('partial',payload[:3],'partial tick record'),('leftover',payload+payload[:4],'unconsumed tick records')]
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')};env.update(ASAN_OPTIONS='detect_leaks=1:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
    report={'scope':__doc__,'sources':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ['re_out/dd2_stubs.c','tools/clock_replay_test.c']},'cases':[]}
    for target,command in [('native',[str(native)]),('wasm',['node',str(wasm)])]:
        for name,data,error in cases:
            path=out/f'{target}-{name}.ticks';path.write_bytes(data)
            run=subprocess.run(command,env={**env,'DD2_TICK_REPLAY':str(path)},capture_output=True,text=True,timeout=20)
            (out/f'{target}-{name}.log').write_text(run.stdout+run.stderr)
            if error:
                if run.returncode!=1 or error not in run.stderr:raise RuntimeError(f'{target}/{name}: intended clock extent failure not detected')
            elif run.returncode or json.loads(run.stdout)!=expected or '[clock-replay] consumed=4 complete' not in run.stderr:raise RuntimeError(f'{target}: exact clock replay failed')
            report['cases'].append({'target':target,'case':name,'pass':True})
        print(f'PASS {target}: exact wrapped/repeated clock returns, exhausted/partial/leftover inputs rejected',flush=True)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':main()
