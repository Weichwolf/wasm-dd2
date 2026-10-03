#!/usr/bin/env python3
"""Replay actual browser clock/DOM input diagnostics in the native engine.

No original A/V acceptance: this compares source/port states and verifies exact
clock/LCG transport before diagnosing a native/WASM engine disagreement.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile

from artifacts import WORK,prepare_output,run_bounded

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--binary',type=Path,default=Path('/tmp/dd2_native'))
    args=parser.parse_args();reference=args.reference.resolve();output=prepare_output(args.output)
    if WORK not in reference.parents or WORK not in output.parents or output.exists():
        parser.error('Use an existing /tmp/wasm-dd2/ reference and a fresh output directory')
    meta=json.loads((reference/'api-record.json').read_text())
    diagnosis_path=reference/('diagnosis.json' if (reference/'diagnosis.json').exists() else 'controls.json')
    diagnosis=json.loads(diagnosis_path.read_text())
    for name,size,hash_key in [('actual-ticks.bin',meta['clock_calls']*4,'clock_sha256'),('calculated-random.bin',meta['rng_calls']*12,'random_sha256')]:
        data=(reference/name).read_bytes()
        if len(data)!=size or hashlib.sha256(data).hexdigest()!=meta[hash_key]:raise ValueError('Incomplete or changed actual API tape')
    if meta['error'] is not None or diagnosis['wasm_sha256']!=meta['layout']['wasm_sha256']:
        raise ValueError('Actual source binary/observer provenance differs')
    output.mkdir(parents=True)
    env={key:value for key,value in os.environ.items() if not key.startswith('DD2_')}
    env.update(DD2_FE='1',DD2_SOUND='1',DD2_NOSEGV='1',DD2_TICK_REPLAY=str(reference/'actual-ticks.bin'),
               DD2_RANDOM_REFERENCE=str(reference/'calculated-random.bin'),DD2_RANDOM_LEVEL='all',DD2_RANDOM_REQUIRE_INITIAL='1')
    script=output/'replay.gdb'
    script.write_text('set pagination off\nset confirm off\nset auto-solib-add off\nset disable-randomization off\nstarti\n'+
        'hbreak *PutDispEnv\ncondition 1 *(int*)0x936ff4 == 0 && *(unsigned*)0x940010 == 0x4696b0\ncontinue\ndelete 1\npython\n'+
        f'import sys\nsys.path.insert(0,{str(ROOT/"tools")!r})\nfrom native_api_replay_gdb import replay\nreplay({str(reference)!r},{str(output)!r})\nend\nkill\nquit\n')
    with tempfile.TemporaryDirectory(prefix='native-api-diagnosis-assets-',dir=WORK) as temp:
        game=Path(temp)
        for asset in (ROOT/'DestructionDerby2').iterdir():
            if asset.name=='SaveGames':shutil.copyfile(asset,game/asset.name)
            else:(game/asset.name).symlink_to(asset,target_is_directory=asset.is_dir())
        save_sha=hashlib.sha256((game/'SaveGames').read_bytes()).hexdigest()
        if save_sha!=diagnosis['initial_save_sha256']:raise ValueError('Initial save differs from actual browser')
        with (output/'gdb.log').open('w') as log:
            run_bounded(['gdb','--nx','-q','-batch','-x',str(script),str(args.binary.resolve())],directory=output,
                        cwd=game,env=env,stdout=log,stderr=log,timeout=900,check=True)
    report=json.loads((output/'report.json').read_text())
    report.update(native_sha256=hashlib.sha256(args.binary.read_bytes()).hexdigest(),source_wasm_sha256=meta['layout']['wasm_sha256'],
                  source_clock_sha256=meta['clock_sha256'],source_random_sha256=meta['random_sha256'],initial_save_sha256=save_sha)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS' if report['pass_'] else 'FAIL',output)
    return 0 if report['pass_'] else 1


if __name__=='__main__':raise SystemExit(main())
