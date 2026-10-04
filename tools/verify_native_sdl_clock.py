#!/usr/bin/env python3
"""Verify actual SDL startup with recorded or live production clock inputs.

Clock transport only; full original engine PCM/video comparison is separate.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

from artifacts import WORK, check_space, prepare_output, run_bounded
from verify_native_sdl import config

ROOT = Path(__file__).resolve().parents[1]


def verify(output):
    output = prepare_output(output)
    if WORK not in output.parents:raise ValueError('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    current = ROOT/'build/dd2_native.c'
    source = current.read_text()
    line = '    if(!getenv("DD2_TICK_REPLAY"))setenv("DD2_REALTIME","1",1);'
    if source.count(line) != 1:raise ValueError('Native recorded clock correction required')
    old = output/'old-native.c';old.write_text(source.replace(line,'    setenv("DD2_REALTIME","1",1);'))
    common = ['gcc','-m32','-no-pie','-std=gnu99','-w','-DDD2_NATIVE_SDL','-DDD2_NO_FOPEN_WRAP',
              '-ffunction-sections','-fdata-sections','-I'+str(ROOT/'build'),*config('cflags'),
              str(ROOT/'build/dd2_stubs.c'),str(ROOT/'tools/native_sdl_clock_test.c'),
              '-Wl,--gc-sections']
    binaries = {}
    for name, native in [('fixed',current),('old',old)]:
        binary = output/name
        with (output/(name+'-build.log')).open('w') as log:
            run_bounded(common+[str(native),*config('libs'),'-o',str(binary)],directory=output,timeout=60,
                        check=True,stdout=log,stderr=subprocess.STDOUT)
        binaries[name] = binary
    payload = struct.pack('<IIII',0xfffffff0,0xfffffff8,3,3)
    env = {key:value for key,value in os.environ.items() if not key.startswith('DD2_')}
    env.update(DD2_WINDOW='1',SDL_AUDIODRIVER='dummy')
    cases = [('exact',payload,None),('exhausted',payload[:4],'clock input exhausted'),
             ('partial',payload[:3],'partial tick record'),('leftover',payload+payload[:4],'unconsumed tick records'),
             ('explicit-realtime-conflict',payload,'requires headless clock mode'),
             ('live',None,None),('old-override',payload,'requires headless clock mode')]
    results = []
    with (output/'xvfb.log').open('w') as log:
        display = subprocess.Popen(['Xvfb','-displayfd','1','-screen','0','640x480x24'],stdout=subprocess.PIPE,stderr=log)
        try:
            number = display.stdout.readline().decode().strip()
            if not number:raise RuntimeError('Xvfb did not start')
            env['DISPLAY'] = ':'+number
            for name, data, error in cases:
                actual_env = dict(env)
                if data is not None:
                    clock = output/(name+'.ticks');clock.write_bytes(data)
                    actual_env['DD2_TICK_REPLAY'] = str(clock)
                if name == 'explicit-realtime-conflict':actual_env['DD2_REALTIME'] = '1'
                run = subprocess.run([str(binaries['old' if name=='old-override' else 'fixed'])],env=actual_env,
                                     capture_output=True,text=True,timeout=20)
                (output/(name+'.log')).write_text(run.stdout+run.stderr)
                if error:
                    if run.returncode != 1 or error not in run.stderr:raise AssertionError('Clock failure was not detected: '+name)
                else:
                    expected = dict(live_clock=True,native_window=True) if name=='live' else dict(exact_clock_returns=4,uint32_wrap=True,repeat_retained=True)
                    if run.returncode or json.loads(run.stdout) != expected:raise AssertionError('Native SDL clock failed: '+name)
                results.append(dict(case=name,pass_=True))
        finally:display.terminate();display.wait(timeout=5)
    report = dict(scope=__doc__.strip(),pass_=True,cases=results,
                  native_source_sha256=hashlib.sha256(current.read_bytes()).hexdigest())
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    for p in output.glob('*.ticks'):p.unlink()
    old.unlink();check_space(output)
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    print(json.dumps(verify(args.output),indent=2))
