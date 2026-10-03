#!/usr/bin/env python3
"""Compare isolated replay recording/decoding with actual unmodified x86 code.

Execute supported dd2h.exe functions at their real VAs in a Linux fixture, with
explicit inputs. Compare every captured script/state byte with reconstructed
Native/WASM C. This component proof does not establish whole-game acceptance.
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

ROOT = Path(__file__).resolve().parents[1]
EXE_SHA = '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2'
FUNCTIONS = ('Record_Event', 'Terminate_Replay', 'Terminate_Replay_Bodge', 'Control_Car_Replay')
CHECKPOINT_BYTES = 0x1c00+24+20+20*0x1b2+4


def compare_checkpoints(original, actual, names):
    if not names or len(original) != len(names)*CHECKPOINT_BYTES or len(actual) != len(original):
        raise RuntimeError('incomplete replay component checkpoints')
    different = [i for i in range(len(names)) if original[i*CHECKPOINT_BYTES:(i+1)*CHECKPOINT_BYTES] != actual[i*CHECKPOINT_BYTES:(i+1)*CHECKPOINT_BYTES]]
    first = None
    if different:
        i=different[0];begin=i*CHECKPOINT_BYTES
        offset=next(j for j in range(CHECKPOINT_BYTES) if original[begin+j] != actual[begin+j])
        first=dict(checkpoint=names[i],offset=offset,original=original[begin+offset],port=actual[begin+offset])
    return dict(pass_=not different,different_checkpoints=len(different),first_difference=first)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    WORK.mkdir(parents=True, exist_ok=True)
    out = prepare_output(args.output or tempfile.mkdtemp(prefix='replay-', dir=WORK))
    out.mkdir(parents=True, exist_ok=True)
    for variant in ('fixed', 'old'):
        headers=out / (variant+'-headers');headers.mkdir()
        for name in ('ghidra_compat.h', 'dd2_symbols.h'):
            shutil.copyfile(ROOT / 'build' / name, headers / name)
        if variant=='old':
            with (ROOT / 'patches/847-replay-word-cursors.diff').open() as patch:
                subprocess.run(['patch','-R','-F0','-p1'],cwd=headers,stdin=patch,stdout=subprocess.DEVNULL,check=True)
    exe = ROOT / 'DestructionDerby2/dd2h.exe'
    if hashlib.sha256(exe.read_bytes()).hexdigest() != EXE_SHA:
        raise RuntimeError('supported unmodified original required')
    engine = (ROOT / 'build/dd2.c').read_text()
    units = []
    for name in FUNCTIONS:
        matches = re.findall(r'/\* ===== '+re.escape(name)+r' @ [0-9a-f]+ ===== \*/(.*?)(?=/\* =====|\Z)', engine, re.S)
        if len(matches) != 1:
            raise RuntimeError(f'exact real engine function missing: {name}')
        units.append(matches[0])
    (out / 'engine.c').write_text('#include "ghidra_compat.h"\n#include "dd2_symbols.h"\nvoid Record_Event(unsigned,int);\n'+'\n'.join(units))
    fixture = str(ROOT / 'tools/reference/replay_fixture.c')
    common = ['-O0', '-std=gnu99', '-w', '-fno-strict-aliasing', '-Wno-int-conversion',
              '-Wno-incompatible-pointer-types']
    cases = []
    for target in ('original-x86', 'native', 'wasm', 'native-old', 'wasm-old'):
        wasm=target.startswith('wasm')
        binary = out / (target+'.js' if wasm else target)
        if wasm:
            compile_cmd = ['emcc', '-mllvm', '-fast-isel=false', '-sNODERAWFS=1', '-sEXIT_RUNTIME=1', '-sGLOBAL_BASE=10485760']
        else:
            compile_cmd = ['gcc', '-m32', '-no-pie']
        if target == 'original-x86':
            compile_cmd += ['-DDD2_ORIGINAL_REPLAY']
        headers=out / ('old-headers' if target.endswith('-old') else 'fixed-headers')
        compile_cmd += common+['-I'+str(headers),fixture]+([] if target=='original-x86' else [str(out / 'engine.c')])+['-o', str(binary)]
        with (out / f'{target}-build.log').open('w') as log:
            run_bounded(compile_cmd, directory=out, timeout=120, stdout=log, stderr=subprocess.STDOUT, check=True)
        command = ['node', str(binary)] if wasm else [str(binary)]
        with (out / f'{target}.log').open('w') as log:
            run_bounded([*command, str(exe), str(out / f'{target}.bin')], directory=out, timeout=60,
                        stdout=log, stderr=subprocess.STDOUT, check=True)
        cases.append((out / f'{target}.log').read_text().splitlines())
    if not cases[0] or any(case != cases[0] for case in cases):
        raise RuntimeError('fixture checkpoints differ or are empty')
    original = (out / 'original-x86.bin').read_bytes()
    if len(original) != len(cases[0])*CHECKPOINT_BYTES:
        raise RuntimeError('incomplete original component checkpoints')
    results = []
    for target in ('native', 'wasm'):
        actual = (out / f'{target}.bin').read_bytes()
        compared=compare_checkpoints(original,actual,cases[0])
        negative=compare_checkpoints(original,(out / f'{target}-old.bin').read_bytes(),cases[0])
        if negative['pass_'] or negative['first_difference']['checkpoint']!='2 record-change' or negative['first_difference']['offset']!=2:
            raise RuntimeError(f'{target}: old DWORD cursors were not rejected at the first skipped WORD')
        results.append(dict(target=target,**compared,negative_old_cursors=negative))
    if (out/'native-old.bin').read_bytes()!=(out/'wasm-old.bin').read_bytes():
        raise RuntimeError('old-cursor controls differ across targets')
    changed=bytearray(original);changed[0]^=1
    if compare_checkpoints(original,changed,cases[0])['first_difference']['offset']!=0:
        raise RuntimeError('changed first replay byte was not localized')
    try:
        compare_checkpoints(original,original[:-1],cases[0])
    except RuntimeError:
        pass
    else:
        raise RuntimeError('truncated replay checkpoints were accepted')
    report = dict(scope=__doc__,original_exe_sha256=EXE_SHA,checkpoints=len(cases[0]),checkpoint_bytes=CHECKPOINT_BYTES,
                  original_sha256=hashlib.sha256(original).hexdigest(),functions=FUNCTIONS,targets=results,
                  negative_first_byte_rejected=True,negative_truncated_checkpoint_rejected=True)
    (out / 'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
    if all(row['pass_'] for row in results):
        discard_frames(out)
        return 0
    return 1


if __name__ == '__main__':
    raise SystemExit(main())
