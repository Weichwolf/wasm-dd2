#!/usr/bin/env python3
"""Verify the native movie QPC provider against actual Wine and clock inputs.

Actual Wine QPC values must match independently forwarded Linux RAW readings
at 100-ns resolution. The production native provider must select RAW, match
its own forwarded reading at millisecond resolution, and fall back to MONOTONIC
when RAW is unavailable. Declared inputs cover nanosecond and DWORD boundaries.
No original dd2h.exe playback, browser clock or whole A/V parity is accepted.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

from artifacts import WORK, check_space, prepare_output
from verify_configuration_persistence import ROOT, require
from verify_sound_cursor import wine_probe


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_rows(rows):
    require(len(rows)==22, 'complete declared clock matrix required')
    for index, row in enumerate(rows):
        require(row['case']==index and row['raw_failed']==(index>=11), 'clock input identity differs')
        calls=row['calls']
        expected=[4,1] if row['raw_failed'] else [4]
        require([c['clock'] for c in calls]==expected and calls[-1]['result']==0 and
                (not row['raw_failed'] or calls[0]['result']==-1), 'movie QPC clock selection differs')
        require(row['movie_ms']==(calls[-1]['ns']//1000000)&0xffffffff, 'movie clock conversion differs')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--mingw',default='i686-w64-mingw32-gcc')
    parser.add_argument('--before-source',type=Path)
    args=parser.parse_args();output=prepare_output(args.output)
    require(WORK in output.parents,'Use /tmp/wasm-dd2/');output.mkdir(parents=True,exist_ok=False)
    source=output/'movie_qpc_test.c';source.write_bytes((ROOT/'tools/movie_qpc_test.c').read_bytes())
    header=output/'dd2_movie_platform.h';header.write_bytes((ROOT/'build'/header.name).read_bytes())
    (output/'dd2_native.h').write_bytes((ROOT/'build/dd2_native.h').read_bytes())
    platform=output/'dd2_movie_platform.c';platform.write_bytes((ROOT/'build'/platform.name).read_bytes())
    observer_source=ROOT/'tools/reference/movie_qpc_observer.c';libraries=[]
    for bits in (32,64):
        directory=output/f'lib{bits}';directory.mkdir();libraries.append(directory)
        subprocess.run(['gcc',f'-m{bits}','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror',
                        str(observer_source),'-ldl','-o',str(directory/'dd2_qpc.so')],check=True)
    executable=output/'movie-qpc.exe'
    subprocess.run([args.mingw,'-O2','-Wall','-Wextra','-Werror',str(source),'-o',str(executable)],check=True)
    directory=output/'wine';directory.mkdir();journal=directory/'clocks.jsonl'
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    env.update(LD_LIBRARY_PATH=':'.join(map(str,libraries)),LD_PRELOAD='dd2_qpc.so',
               DD2_QPC_PROCESS=executable.name,DD2_QPC_CAPTURE=str(journal))
    wine=wine_probe(executable,directory,env)
    shutil.rmtree(directory/'wine-prefix')
    observations=[json.loads(s) for s in journal.read_text().splitlines()]
    require(wine['frequency']==10000000 and len(wine['counters'])==16, 'actual Wine QPC frequency/counter count')
    raw={r['ns']//100 for r in observations if r['clock']==4}
    require(all(c in raw for c in wine['counters']), 'actual Wine QPC differs from forwarded RAW readings')
    targets=[];negative=[]
    for case in ('native',*(['before'] if args.before_source else [])):
        selected=platform
        if case=='before':
            selected=output/'before-platform.c';selected.write_bytes(args.before_source.read_bytes())
        binary=output/case
        subprocess.run(['gcc','-m32','-O2','-no-pie','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                        '-ffunction-sections','-fdata-sections','-I'+str(output),str(source),str(selected),
                        '-Wl,--gc-sections','-Wl,--wrap=clock_gettime','-o',str(binary)],check=True)
        run=subprocess.run([str(binary)],capture_output=True,text=True,check=True,timeout=5)
        require('runtime error:' not in run.stderr,'clock provider sanitizer error')
        (output/f'{case}.jsonl').write_text(run.stdout)
        rows=[json.loads(s) for s in run.stdout.splitlines()]
        if case=='before':
            try: verify_rows(rows)
            except RuntimeError as error:
                require(str(error)=='movie QPC clock selection differs','unexpected previous-provider failure')
            else:raise RuntimeError('previous provider unexpectedly selects RAW')
        else:verify_rows(rows)
        live=subprocess.run([str(binary),'live'],capture_output=True,text=True,check=True,timeout=5)
        live_row=json.loads(live.stdout)
        require(len(live_row['calls'])==1 and live_row['calls'][0]['clock']==(1 if case=='before' else 4) and
                live_row['movie_ms']==(live_row['calls'][0]['ns']//1000000)&0xffffffff,'live production clock differs')
        targets.append(dict(case=case,binary_sha256=sha(binary),platform_sha256=sha(selected),cases=rows,
                            live=live_row,previous_provider_rejected=case=='before'))
    rows=targets[0]['cases']
    for label in ('clock-id','conversion','missing-case','fallback-result'):
        changed=json.loads(json.dumps(rows))
        if label=='clock-id':changed[0]['calls'][0]['clock']=1
        elif label=='conversion':changed[4]['movie_ms']^=1
        elif label=='missing-case':changed.pop()
        else:changed[11]['calls'][0]['result']=0
        try:verify_rows(changed)
        except RuntimeError:negative.append(label)
        else:raise RuntimeError('accepted altered clock evidence')
    report=dict(scope=__doc__,pass_=True,original_game_av_parity='unproven',wine=wine,
                wine_version=subprocess.check_output(['wine','--version'],text=True).strip(),
                forwarded_clock_observations=len(observations),wine_clock_journal_sha256=sha(journal),
                fixture_sha256=sha(source),observer_source_sha256=sha(observer_source),
                verifier_sha256=sha(Path(__file__)),targets=targets,negative_controls=negative)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n');check_space(output)
    print('PASS real Wine QPC and sanitized native RAW/fallback/wrap clock provider;',len(negative),'negative controls')


if __name__=='__main__':main()
