#!/usr/bin/env python3
"""Compare card block/directory/duplicate/config logic with unmodified original x86.

Explicit component inputs include empty/full/sparse cards, last free slot,
reserved flags, missing/dormant names, duplicate exemptions and config selection.
Original runs stop before the external file seek or at the fixture return marker
with read-only hardware breakpoints. Real host persistence/UI is checked by the
native/browser replay tests, not claimed by this isolated component comparison.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from artifacts import WORK, prepare_output, run_bounded, discard_frames
from verify_replay import ROOT, EXE_SHA

CASES = 26
CARD_WINDOW = 16 + 0x20000 + 4 + 15 * 36 + 16
CHECKPOINT_BYTES = CARD_WINDOW + 2 * 0x2000 + 8
FUNCTIONS = ('SaveCardFile', 'DeleteFileMC', 'LoadCardFiles', 'LoadCardFile',
             'DupFileCheck', 'FirstSavedGame', 'FUN_0042366c')


def compare(original, actual):
    if len(original) != CHECKPOINT_BYTES or len(actual) != len(original):
        raise RuntimeError('incomplete card checkpoint')
    first = next((i for i, (a, b) in enumerate(zip(original, actual)) if a != b), None)
    return dict(pass_=first is None, first_difference=None if first is None else
                dict(offset=first, original=original[first], port=actual[first]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    WORK.mkdir(parents=True, exist_ok=True)
    out = prepare_output(args.output or tempfile.mkdtemp(prefix='card-', dir=WORK))
    if WORK not in out.parents:
        raise RuntimeError('card verification output must be under /tmp/wasm-dd2')
    out.mkdir(parents=True, exist_ok=args.output is None)
    exe = ROOT / 'DestructionDerby2/dd2h.exe'
    if hashlib.sha256(exe.read_bytes()).hexdigest() != EXE_SHA:
        raise RuntimeError('supported unmodified original required')
    for name in ('ghidra_compat.h', 'dd2_symbols.h'):
        shutil.copyfile(ROOT / 'build' / name, out / name)
    engine = (ROOT / 'build/dd2.c').read_text()
    units = []
    for name in FUNCTIONS:
        matches = re.findall(r'/\* ===== ' + name + r' @ [0-9a-f]+ ===== \*/(.*?)(?=/\* =====|\Z)', engine, re.S)
        if len(matches) != 1:
            raise RuntimeError('exact real engine function missing: ' + name)
        units.append(matches[0])
    (out / 'engine.c').write_text('#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n'
                                 'int FUN_0045623b(int*,int,int);\nint FUN_0042366c(void);\n' + '\n'.join(units))
    binaries = {}
    for target in ('original-x86', 'native', 'wasm'):
        wasm = target == 'wasm'
        binary = out / (target + '.js' if wasm else target)
        compiler = ['emcc', '-mllvm', '-fast-isel=false', '-sNODERAWFS=1', '-sEXIT_RUNTIME=1', '-sGLOBAL_BASE=10485760'] if wasm else ['gcc', '-m32', '-no-pie']
        command = compiler + ['-g', '-O0', '-std=gnu99', '-w', '-fno-strict-aliasing',
                              '-Wno-int-conversion', '-Wno-incompatible-pointer-types', '-I' + str(out),
                              str(ROOT / 'tools/reference/card_fixture.c')]
        if target == 'original-x86':
            command.append('-DDD2_ORIGINAL_CARD')
        else:
            command.append(str(out / 'engine.c'))
        with (out / (target + '-build.log')).open('w') as log:
            run_bounded([*command, '-o', str(binary)], directory=out, timeout=120,
                        stdout=log, stderr=subprocess.STDOUT, check=True)
        binaries[target] = ['node', str(binary)] if wasm else [str(binary)]
    results = []; original_all = []
    for case in range(CASES):
        reference = out / f'original-{case}.bin'
        script = '''set pagination off
set confirm off
set disable-randomization off
starti
hbreak *0x45623b
hbreak *fixture_done
continue
python
import gdb
from pathlib import Path
inferior=gdb.selected_inferior()
pc=int(gdb.parse_and_eval('$pc'))
assert int(gdb.parse_and_eval('sizeof(void*)'))==4, 'actual x86 required'
if pc==0x45623b:
    kind=1; result=0
    arguments=bytes(inferior.read_memory(int(gdb.parse_and_eval('$esp'))+4,12))
    assert [int.from_bytes(arguments[i:i+4],'little') for i in [0,4,8]]==[1,0,0], 'wrong card seek arguments'
else:
    assert pc==int(gdb.parse_and_eval('&fixture_done')), 'wrong original return marker'
    kind=0
    result=int.from_bytes(bytes(inferior.read_memory(int(gdb.parse_and_eval('$esp'))+4,4)),'little')
regions=[(0x754450,CARD_SIZE),(0x93a490,0x2000),(0x94c000,0x2000)]
data=b''.join(bytes(inferior.read_memory(a,n)) for a,n in regions)
data+=kind.to_bytes(4,'little')+result.to_bytes(4,'little')
Path(CAPTURE).write_bytes(data)
print('ORIGINAL_CARD_CHECKPOINT_COMPLETE')
end
kill
quit
'''.replace('CARD_SIZE', str(CARD_WINDOW)).replace('CAPTURE', repr(str(reference)))
        commands = out / f'original-{case}.gdb'; commands.write_text(script)
        with (out / f'original-{case}.log').open('w') as log:
            run_bounded(['gdb', '-q', '-nx', '-batch', '-x', str(commands), '--args',
                         *binaries['original-x86'], str(exe), str(out / f'original-unused-{case}.bin'), str(case)],
                        directory=out, timeout=30, stdout=log, stderr=subprocess.STDOUT, check=True)
        log = (out / f'original-{case}.log').read_text()
        if log.count('ORIGINAL_CARD_CHECKPOINT_COMPLETE') != 1 or 'Python Exception' in log:
            raise RuntimeError('incomplete original hardware observation')
        original = reference.read_bytes(); original_all.append(original)
        if len(original) != CHECKPOINT_BYTES:
            raise RuntimeError('incomplete original card checkpoint')
        for target in ('native', 'wasm'):
            checkpoint = out / f'{target}-{case}.bin'
            with (out / f'{target}-{case}.log').open('w') as log:
                run_bounded([*binaries[target], str(exe), str(checkpoint), str(case)],
                            directory=out, timeout=30, stdout=log, stderr=subprocess.STDOUT, check=True)
            results.append(dict(target=target, case=case, boundary='external seek' if original[-8:-4] == b'\x01\0\0\0' else 'returned',
                                result=int.from_bytes(original[-4:], 'little'), **compare(original, checkpoint.read_bytes())))
    # Sensitivity checks target a guard, a directory byte, a payload and outcome.
    mutations = []
    for offset in [0,16,16+0x2000,16+0x1e000,CHECKPOINT_BYTES-1]:
        changed=bytearray(original_all[0]);changed[offset]^=1
        require=compare(original_all[0],changed)
        if require['pass_'] or require['first_difference']['offset']!=offset:
            raise RuntimeError('changed card byte not localized')
        mutations.append(offset)
    try:
        compare(original_all[0],original_all[0][:-1])
    except RuntimeError:
        pass
    else:
        raise RuntimeError('truncated card checkpoint accepted')
    report=dict(scope=__doc__,pass_=all(row['pass_'] for row in results),cases=CASES,
                checkpoint_bytes=CHECKPOINT_BYTES,functions=FUNCTIONS,original_exe_sha256=EXE_SHA,
                original_sha256=hashlib.sha256(b''.join(original_all)).hexdigest(),results=results,
                changed_guard_directory_payload_outcome_rejected=mutations,negative_truncated_checkpoint_rejected=True)
    (out / 'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    if report['pass_']:
        discard_frames(out);return 0
    return 1


if __name__=='__main__':
    raise SystemExit(main())
