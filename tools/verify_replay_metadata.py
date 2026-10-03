#!/usr/bin/env python3
"""Compare replay packing/load setup with actual unmodified original x86.

Original execution stops at read-only hardware breakpoints before the external
file browser/player; Native/WASM observe the same API boundary with fixtures.
This checks complete packed/script/order bytes, guards and metadata with explicit
inputs, not original whole-game save/load, rendered output or audio acceptance.
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
from verify_replay import EXE_SHA, ROOT

PACKED_BYTES = 18 + 0x1c00 + 20
CHECKPOINT_BYTES = PACKED_BYTES + 32 + 0x1c00 + 20 + 9 * 4
FUNCTIONS = ('FUN_0044ab98', 'FUN_0044ac60')
CASES = 8


def compare(original, actual):
    if len(original) != CHECKPOINT_BYTES or len(actual) != len(original):
        raise RuntimeError('incomplete replay metadata checkpoint')
    first = next((i for i, (a, b) in enumerate(zip(original, actual)) if a != b), None)
    return dict(pass_=first is None, first_difference=None if first is None else
                dict(offset=first, original=original[first], port=actual[first]))


def original_capture(binary, exe, out, phase, case):
    name = f'original-{phase}-{case}'
    capture = out / (name + '.bin')
    boundary = 0x4494e0 if phase == 'save' else 0x44b62c
    # Breakpoint may be installed before the fixture maps the PE, since hardware
    # instruction breakpoints do not write any original code bytes.
    script = '''set pagination off
set confirm off
set disable-randomization off
set breakpoint always-inserted on
starti
hbreak *@BOUNDARY@
continue
python
import gdb
from pathlib import Path
inferior = gdb.selected_inferior()
assert int(gdb.parse_and_eval('$pc')) == @BOUNDARY@, 'wrong original boundary'
assert int(gdb.parse_and_eval('sizeof(void*)')) == 4, 'original must run as x86'
if PHASE == 'save':
    stack = int(gdb.parse_and_eval('$esp'))
    arguments = bytes(inferior.read_memory(stack + 4, 8))
    assert int.from_bytes(arguments[:4], 'little') == 5, 'wrong save operation'
    assert int.from_bytes(arguments[4:], 'little') == 0x93a490, 'wrong save buffer'
regions = [(0x93a480, PACKED_SIZE + 32), (0x9376b0, 0x1c00), (0x795c28, 20)]
regions += [(a, 4) for a in [0x467400, 0x4673f8, 0x4673f4, 0x93dec0,
                           0x936ff4, 0x9392bc, 0x467078, 0x46765c, 0x9392c4]]
Path(CAPTURE).write_bytes(b''.join(bytes(inferior.read_memory(a, n)) for a, n in regions))
print('ORIGINAL_METADATA_BOUNDARY_COMPLETE')
end
kill
quit
'''.replace('@BOUNDARY@', hex(boundary)).replace('PHASE', repr(phase)).replace(
        'PACKED_SIZE', str(PACKED_BYTES)).replace('CAPTURE', repr(str(capture)))
    marker = 'ORIGINAL_METADATA_BOUNDARY_COMPLETE'
    commands = out / (name + '.gdb')
    commands.write_text(script)
    with (out / (name + '.log')).open('w') as log:
        run_bounded(['gdb', '-q', '-nx', '-batch', '-x', str(commands), '--args',
                     str(binary), str(exe), str(out / (name + '-unused.bin')), phase, str(case)],
                    directory=out, timeout=30, stdout=log, stderr=subprocess.STDOUT, check=True)
    log = (out / (name + '.log')).read_text()
    if log.count(marker) != 1 or 'Python Exception' in log or 'Error in sourced command file' in log:
        raise RuntimeError(f'{name}: original hardware breakpoint observation incomplete')
    return capture.read_bytes()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    WORK.mkdir(parents=True, exist_ok=True)
    out = prepare_output(args.output or tempfile.mkdtemp(prefix='replay-metadata-', dir=WORK))
    out.mkdir(parents=True, exist_ok=True)
    exe = ROOT / 'DestructionDerby2/dd2h.exe'
    if hashlib.sha256(exe.read_bytes()).hexdigest() != EXE_SHA:
        raise RuntimeError('supported unmodified original required')
    sources = {}
    for variant in ('fixed', 'old'):
        directory = out / (variant + '-source')
        directory.mkdir()
        for name in ('ghidra_compat.h', 'dd2_symbols.h', 'dd2.c'):
            shutil.copyfile(ROOT / 'build' / name, directory / name)
        if variant == 'old':
            with (ROOT / 'patches/848-replay-metadata-word-fields.diff').open() as patch:
                subprocess.run(['patch', '-R', '-F0', '-p1'], cwd=directory, stdin=patch,
                               stdout=subprocess.DEVNULL, check=True)
        engine = (directory / 'dd2.c').read_text()
        units = []
        for name in FUNCTIONS:
            matches = re.findall(r'/\* ===== ' + name + r' @ [0-9a-f]+ ===== \*/(.*?)(?=/\* =====|\Z)', engine, re.S)
            if len(matches) != 1:
                raise RuntimeError(f'exact actual engine function missing: {name}')
            units.append(matches[0])
        (directory / 'engine.c').write_text('#include "ghidra_compat.h"\n#include "dd2_symbols.h"\n'
            'unsigned LoadSave(unsigned,void*);\nunsigned View_Frontend_Replay(void);\n' + '\n'.join(units))
        (directory / 'dd2.c').unlink()
        sources[variant] = directory
    binaries = {}
    for target in ('original-x86', 'native', 'wasm', 'native-old', 'wasm-old'):
        wasm = target.startswith('wasm')
        binary = out / (target + '.js' if wasm else target)
        compiler = ['emcc', '-mllvm', '-fast-isel=false', '-sNODERAWFS=1', '-sEXIT_RUNTIME=1', '-sGLOBAL_BASE=10485760'] if wasm else ['gcc', '-m32', '-no-pie']
        directory = sources['old' if target.endswith('-old') else 'fixed']
        command = compiler + ['-O0', '-std=gnu99', '-w', '-fno-strict-aliasing',
                              '-Wno-int-conversion', '-Wno-incompatible-pointer-types', '-I' + str(directory),
                              str(ROOT / 'tools/reference/replay_metadata_fixture.c')]
        if target == 'original-x86':
            command.append('-DDD2_ORIGINAL_REPLAY')
        else:
            command.append(str(directory / 'engine.c'))
        command += ['-o', str(binary)]
        with (out / (target + '-build.log')).open('w') as log:
            run_bounded(command, directory=out, timeout=120, stdout=log, stderr=subprocess.STDOUT, check=True)
        binaries[target] = ['node', str(binary)] if wasm else [str(binary)]
    results = []
    originals = []
    for phase in ('save', 'load'):
        for case in range(CASES):
            original = original_capture(Path(binaries['original-x86'][0]), exe, out, phase, case)
            if len(original) != CHECKPOINT_BYTES:
                raise RuntimeError('incomplete original metadata checkpoint')
            originals.append(original)
            for target in ('native', 'wasm', 'native-old', 'wasm-old'):
                name = f'{target}-{phase}-{case}'
                checkpoint = out / (name + '.bin')
                with (out / (name + '.log')).open('w') as log:
                    run_bounded([*binaries[target], str(exe), str(checkpoint), phase, str(case)],
                                directory=out, timeout=30, stdout=log, stderr=subprocess.STDOUT, check=True)
                results.append(dict(target=target, phase=phase, case=case,
                                    **compare(original, checkpoint.read_bytes())))
    fixed = [r for r in results if not r['target'].endswith('-old')]
    negatives = [r for r in results if r['target'].endswith('-old')]
    for target in ('native-old', 'wasm-old'):
        first = next(r for r in negatives if r['target'] == target and r['phase'] == 'save' and r['case'] == 0)
        if first['pass_'] or first['first_difference'] != dict(offset=18, original=7, port=0):
            raise RuntimeError('old magic DWORD did not overwrite the nonzero saved car')
        if all(r['pass_'] for r in negatives if r['target'] == target and r['phase'] == 'load'):
            raise RuntimeError('old widened metadata loads were not rejected')
    changed = bytearray(originals[0]); changed[0] ^= 1
    if compare(originals[0], changed)['first_difference']['offset'] != 0:
        raise RuntimeError('changed guard byte was not localized')
    try:
        compare(originals[0], originals[0][:-1])
    except RuntimeError:
        pass
    else:
        raise RuntimeError('truncated metadata checkpoint accepted')
    report = dict(scope=__doc__, pass_=all(r['pass_'] for r in fixed), original_exe_sha256=EXE_SHA,
                  original_sha256=hashlib.sha256(b''.join(originals)).hexdigest(), functions=FUNCTIONS,
                  cases=CASES, phases=['save', 'load'], checkpoint_bytes=CHECKPOINT_BYTES,
                  boundaries=dict(save='0x4494e0', load='0x44b62c'), results=fixed, negative_old_metadata=negatives,
                  negative_guard_byte_rejected=True, negative_truncated_checkpoint_rejected=True)
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    if report['pass_']:
        discard_frames(out)
        return 0
    return 1


if __name__ == '__main__':
    raise SystemExit(main())
