#!/usr/bin/env python3
"""Check calculated Watcom random states/returns against actual original records.

This controlled fixture verifies the reader and arithmetic, not game acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
ROOT=Path(__file__).resolve().parent.parent
# Read-only debugger observations from the supported original's later L6 demo.
RECORDS=[(1038462957,2494844450,5300),(2494844450,1356845235,20703),
         (1356845235,2119472752,32340),(2119472752,1517968873,23162)]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    source=ROOT/'re_out/dd2_stubs.c'
    normal=source.read_text();needle='before*0x41c64e6dU+0x3039U'
    if normal.count(needle)!=1:raise RuntimeError('random arithmetic mutation location is ambiguous')
    mutant=out/'mutant.c';mutant.write_text(normal.replace(needle,'before*0x41c64e6cU+0x3039U'))
    # Only the target random verifier changes; keep the unrelated initial call
    # out of the mutation so failures must come from the reference-state check.
    mutant.write_text(mutant.read_text().replace('before*0x41c64e6cU+0x3039U',
        'before*(verify?0x41c64e6cU:0x41c64e6dU)+0x3039U'))
    def build(target,src,path):
        common=['-std=gnu99','-w','-DDD2_NO_FOPEN_WRAP','-ffunction-sections','-fdata-sections',
            f'-I{ROOT/"re_out"}',str(src),str(ROOT/'tools/random_reference_test.c'),'-Wl,--gc-sections']
        cmd=['gcc','-m32','-no-pie','-fsanitize=address,undefined','-g'] if target=='native' else [
            'emcc','-sNODERAWFS=1','-sEXIT_RUNTIME=1','-sGLOBAL_BASE=10485760',
            '-sINITIAL_MEMORY=33554432','--pre-js',str(ROOT/'tools/node_env.js')]
        subprocess.run([*cmd,*common,'-o',str(path)],check=True)
    payload=b''.join(struct.pack('<III',*row) for row in RECORDS)
    cases=[('exact',payload,None),('exhausted',payload[:12],'random reference exhausted'),
        ('partial',payload[:5],'partial random record'),('leftover',payload+payload[:12],'unconsumed random records')]
    for name,offset in [('before',12),('after',4),('return',8)]:
        changed=bytearray(payload);changed[offset]^=1
        cases.append((f'wrong-{name}',bytes(changed),'calculated random state/result differs'))
    env={k:v for k,v in os.environ.items() if not k.startswith('DD2_')};env.update(
        ASAN_OPTIONS='detect_leaks=1:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1',DD2_RANDOM_LEVEL='6')
    report={'scope':__doc__,'original_records':RECORDS,'sources':{
        name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in
        ['re_out/dd2_stubs.c','tools/random_reference_test.c']},'cases':[]}
    for target in ['native','wasm']:
        binary=out/('native' if target=='native' else 'wasm.js');build(target,source,binary)
        mutated=out/('native-mutant' if target=='native' else 'wasm-mutant.js');build(target,mutant,mutated)
        for name,data,error in [*cases,('multiplier-mutation',payload,'calculated random state/result differs')]:
            path=out/f'{target}-{name}.random';path.write_bytes(data)
            executable=mutated if name=='multiplier-mutation' else binary
            command=[str(executable)] if target=='native' else ['node',str(executable)]
            run=subprocess.run(command,env={**env,'DD2_RANDOM_REFERENCE':str(path)},capture_output=True,text=True,timeout=20)
            (out/f'{target}-{name}.log').write_text(run.stdout+run.stderr)
            if error:
                if run.returncode!=1 or error not in run.stderr:raise RuntimeError(f'{target}/{name}: intended rejection missing')
            elif run.returncode or json.loads(run.stdout)!={'calculated_original_returns':4,'level_gate':True} or '[random-reference] consumed=4 complete' not in run.stderr:
                raise RuntimeError(f'{target}: actual original arithmetic/reference check failed')
            report['cases'].append({'target':target,'case':name,'pass':True})
        # Independently specified boot-seed fixture spans frontend level 0
        # and multiple real levels. Full-history mode must never initialize
        # its seed from a reference, even without REQUIRE_INITIAL being set.
        boot_records=[(1,1103527590,16838),(1103527590,2524885223,5758),
            (2524885223,662824084,10113),(662824084,3295386429,17515)]
        boot=b''.join(struct.pack('<III',*row) for row in boot_records)
        wrong_seed=bytearray(boot);wrong_seed[0]^=1
        boot_cases=[('all-exact',boot,None),('all-wrong-initial',bytes(wrong_seed),'calculated initial seed differs'),
            ('all-exhausted',boot[:12],'random reference exhausted'),
            ('all-partial',boot[:5],'partial random record'),
            ('all-leftover',boot+boot[:12],'unconsumed random records'),
            ('all-multiplier-mutation',boot,'calculated random state/result differs')]
        for name,data,error in boot_cases:
            path=out/f'{target}-{name}.random';path.write_bytes(data)
            executable=mutated if name=='all-multiplier-mutation' else binary
            command=[str(executable)] if target=='native' else ['node',str(executable)]
            run=subprocess.run(command,env={**env,'DD2_RANDOM_LEVEL':'all','DD2_RANDOM_REFERENCE':str(path)},
                capture_output=True,text=True,timeout=20)
            (out/f'{target}-{name}.log').write_text(run.stdout+run.stderr)
            if error:
                if run.returncode!=1 or error not in run.stderr:raise RuntimeError(f'{target}/{name}: intended all-history rejection missing')
            elif run.returncode or json.loads(run.stdout)!={'calculated_boot_returns':4,'all_levels':True} or '[random-reference] consumed=4 complete' not in run.stderr:
                raise RuntimeError(f'{target}: all-history RNG not calculated from boot seed')
            report['cases'].append({'target':target,'case':name,'pass':True})
        print(f'PASS {target}: actual original random states/returns calculated, level gated; malformed inputs and multiplier mutation rejected',flush=True)
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':main()
