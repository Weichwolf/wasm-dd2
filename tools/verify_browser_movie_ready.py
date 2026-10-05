#!/usr/bin/env python3
"""Verify actual browser movie PCM preparation before real-time context creation.

The unchanged production platform import runs in Chromium with all S16 caller
values, real buffers/contexts, required activation, and declared allocation,
open and negotiated-rate failures. The earlier production import must fail
the ready-before-context invariant while preserving caller samples. This does
not establish original output PCM, A/V timing or physical device parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

from artifacts import WORK, check_space, prepare_output, run_bounded
from verify_configuration_persistence import ROOT, require


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before-platform',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();output=prepare_output(args.output)
    require(WORK in output.parents,'Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=False)
    snapshot=output/'production';snapshot.mkdir()
    for p in (ROOT/'build').glob('*.h'):shutil.copy2(p,snapshot/p.name)
    current=snapshot/'dd2_movie_platform.c';current.write_bytes((ROOT/'build/dd2_movie_platform.c').read_bytes())
    before=output/'before.c';before.write_bytes(args.before_platform.read_bytes())
    units=[]
    for label,source in [('production',current),('before',before)]:
        binary=output/(label+'.js')
        subprocess.run(['emcc','-DDD2_BROWSER','-I'+str(snapshot),str(source),'--no-entry','-sASYNCIFY',
                        '-sEXPORTED_FUNCTIONS=["_malloc","_free","_dd2_movie_audio_start","_dd2_movie_audio_stop"]',
                        '-sEXIT_RUNTIME=0','-o',str(binary)],check=True)
        (output/(label+'.html')).write_text('<button>Play</button><script>var Module={onRuntimeInitialized(){window.__fixtureResult=window.__startReadyFixture();window.fixtureReady=true;}};</script>'
                                           '<script src="'+label+'.js"></script>')
        units.append(dict(program=label,platform_sha256=sha(source),wasm_sha256=sha(output/(label+'.wasm'))))
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    with (output/'run.log').open('w') as log:
        run_bounded(['node',str(ROOT/'tools/browser/qa_movie_ready.js'),str(output)],directory=output,
                    timeout=60,env=env,check=True,stdout=log,stderr=subprocess.STDOUT)
    observed=json.loads((output/'browser-ready.json').read_text())
    require(observed['observations_valid'] and len(observed['cases'])==6,'complete readiness/error/activation cases required')
    report=dict(scope=__doc__,pass_=True,original_port_parity='unproven',units=units,
                browser_observer_source_sha256=sha(ROOT/'tools/browser/qa_movie_ready.js'),cases=observed['cases'])
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    print('PASS actual production readiness, all S16 caller values, activation and failure cleanup; old order rejected',flush=True)


if __name__=='__main__':main()
