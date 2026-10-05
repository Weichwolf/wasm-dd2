#!/usr/bin/env python3
"""Check actual production browser movie clock/drain imports under declared clocks.

Real browser graph nodes preserve every caller sample. Declared render/output
positions exercise missing output, render-ahead, source-ended-before-consumed,
timestamp regression, suspension, source completion, idempotent cleanup and
the no-device 32-bit clock wrap. An old production import must reproduce its
render-clock and premature completion failures. This is a component check;
it does not establish original device PCM or matched A/V timing.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

from artifacts import WORK, check_space, prepare_output, run_bounded
from verify_browser_movie_ready import sha
from verify_configuration_persistence import ROOT, require


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before-platform',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();output=prepare_output(args.output)
    require(WORK in output.parents,'Use /tmp/wasm-dd2/');output.mkdir(parents=True,exist_ok=False)
    snapshot=output/'production';snapshot.mkdir()
    for p in (ROOT/'build').glob('*.h'):shutil.copy2(p,snapshot/p.name)
    current=snapshot/'dd2_movie_platform.c';current.write_bytes((ROOT/'build/dd2_movie_platform.c').read_bytes())
    before=output/'before.c';before.write_bytes(args.before_platform.read_bytes());units=[]
    for label,source in [('production',current),('before',before)]:
        subprocess.run(['emcc','-DDD2_BROWSER','-I'+str(snapshot),str(source),'--no-entry','-sASYNCIFY',
                        '-sEXPORTED_FUNCTIONS=["_malloc","_free","_dd2_movie_now_ms","_dd2_movie_audio_start","_dd2_movie_audio_done","_dd2_movie_audio_stop"]',
                        '-sEXIT_RUNTIME=0','-o',str(output/(label+'.js'))],check=True)
        (output/(label+'.html')).write_text('<button>Play</button><script>var Module={onRuntimeInitialized(){window.fixtureReady=true;}};</script>'
                                           '<script src="'+label+'.js"></script>')
        units.append(dict(program=label,platform_sha256=sha(source),wasm_sha256=sha(output/(label+'.wasm'))))
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    with (output/'run.log').open('w') as log:
        run_bounded(['node',str(ROOT/'tools/browser/qa_movie_clock.js'),str(output)],directory=output,
                    timeout=60,env=env,check=True,stdout=log,stderr=subprocess.STDOUT)
    observed=json.loads((output/'browser-clock.json').read_text())
    require(observed['component_checks_passed'] and len(observed['cases'])==2,'current/old actual imports required')
    report=dict(scope=__doc__,component_checks_passed=True,original_port_parity='unproven',units=units,
                browser_observer_source_sha256=sha(ROOT/'tools/browser/qa_movie_clock.js'),cases=observed['cases'])
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    print('PASS actual production output-clock/drain/cleanup checks; old render-clock and early completion rejected',flush=True)


if __name__=='__main__':main()
