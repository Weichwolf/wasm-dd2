#!/usr/bin/env python3
"""Production-object libc ABI and original Watcom frexp instruction comparison.

This is a CRT component check. It does not establish complete game formatting,
live original gameplay, movie PCM, or synchronized original/port A/V parity.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import random
import re
import struct
import subprocess
import sys

from artifacts import WORK, prepare_output, run_bounded, open_files
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools/reference'))
from reference.capture import EXE_SHA256


def sha(path):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f, 'sha256').hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def routine(exe):
    require(sha(exe) == EXE_SHA256, 'Provisioned original executable changed')
    data = exe.read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    require(data[pe:pe+4] == b'PE\0\0', 'PE executable required')
    sections, optional_size = struct.unpack_from('<H12xH', data, pe+6)
    optional = pe+24
    require(struct.unpack_from('<H', data, optional)[0] == 0x10b, 'PE32 required')
    base = struct.unpack_from('<I', data, optional+28)[0]
    rva = 0x45c0f0-base
    for i in range(sections):
        header = optional+optional_size+40*i
        virtual_size, virtual, size, offset = struct.unpack_from('<4I', data, header+8)
        # Watcom's original PE uses zero VirtualSize in every section. Locate
        # these file-backed instructions through SizeOfRawData, as its loader
        # does, without treating the original's layout as a malformed image.
        if offset and virtual <= rva and rva+123 <= virtual+size:
            code = data[offset+rva-virtual:offset+rva-virtual+123]
            require(code[:4] == bytes.fromhex('5383ec10') and code[-5:] == bytes.fromhex('83c4105bc3'),
                    'Original routine boundaries changed')
            return code
    raise RuntimeError('Original routine not found')


def cases():
    # Every encoded exponent, signs and fraction boundaries, including signed
    # zero/subnormal/normal/infinity/quiet/signalling NaN. Fixed-seed extras
    # exercise nontrivial mantissas without deriving expected Watcom results.
    values = [sign | (e << 52) | m
              for e in range(2048) for sign in (0, 1 << 63)
              for m in (0, 1, 0x7ffffffffffff, 0x8000000000000, 0xfffffffffffff)]
    rng = random.Random(0x45c0f0)
    values.extend(rng.getrandbits(64) for _ in range(1024))
    return values


def records(path, count):
    data = path.read_bytes()
    require(len(data) == count*16, 'Wrong number of frexp observations')
    result = list(struct.iter_unpack('<QiI', data))
    require(all(guard == 1 for _, _, guard in result), 'Exponent write damaged adjacent storage')
    return result


def standard(observed, values):
    for (bits, exponent, _), source in zip(observed, values):
        value = struct.unpack('<d', struct.pack('<Q', source))[0]
        fraction, expected_exponent = math.frexp(value)
        expected_bits = struct.unpack('<Q', struct.pack('<d', fraction))[0]
        if math.isnan(value):
            # A native x87 double return can quiet a signalling NaN. Standard
            # C frexp does not require a particular payload or quiet bit.
            require(math.isnan(struct.unpack('<d', struct.pack('<Q', bits))[0]),
                    'libc frexp lost NaN classification')
        else:
            require(bits == expected_bits, 'libc frexp fraction bits differ')
        # C does not specify the stored exponent for infinity or NaN.
        if math.isfinite(value):
            require(exponent == expected_exponent, 'libc frexp exponent differs')


def watcom(observed, original):
    require(observed == original,
            'Original Watcom instruction bits, exponent or write guard differ')


def run(command, directory, label, timeout=120):
    log = directory/(label+'.log')
    with log.open('w') as f:
        result = run_bounded(command, directory=directory, timeout=timeout, stdout=f,
                             stderr=subprocess.STDOUT, cwd=ROOT)
    return result.returncode, log


def link(directory, objects, target):
    output = directory/target
    output.mkdir()
    inputs = sorted(objects.glob('*.o'))
    probe = ROOT/'tools/frexp_abi_probe.c'
    is_wasm = target.startswith('wasm')
    units = ('dd2 dd2_dispatch dd2_buffers dd2_data dd2_win32 dd2_stubs dd2_com '
             'dd2_filio dd2_input dd2h_stubs dd2_festate dd2_cd dd2_avi dd2_cinepak '
             'dd2_msadpcm dd2_movie dd2_movie_platform dd2_movie_surface dd2_boot').split()
    units += ['dd2_runtime'] if is_wasm else ['main', 'dd2_native']
    require({p.name for p in inputs} == {u+'.o' for u in units},
            'Complete production object set required')
    if is_wasm:
        command = ['emcc', '-std=gnu89', '-O0', '-fno-builtin', '-c', str(probe), '-o', str(output/'probe.o')]
        linked = output/'probe.js'
        flags = ['-sGLOBAL_BASE=10485760', '-sSTACK_SIZE=16777216', '-sINITIAL_MEMORY=268435456',
                 '-sALLOW_MEMORY_GROWTH=1', '-sEXIT_RUNTIME=1', '-sERROR_ON_UNDEFINED_SYMBOLS=0',
                 '-sASYNCIFY', '-sASYNCIFY_STACK_SIZE=131072', '-sNODERAWFS=1', '-sINVOKE_RUN=0',
                 '-sENVIRONMENT=node', '-sEXPORTED_FUNCTIONS=["_main","_dd2_crt_probe"]',
                 '-sEXPORTED_RUNTIME_METHODS=["ccall"]', '--emit-symbol-map']
        linker = ['emcc']
    else:
        symbols = subprocess.check_output(['nm', '-u', *map(str, inputs)], text=True)
        sanitizers = [flag for symbol, flag in (('__asan_', '-fsanitize=address'),
                                               ('__ubsan_', '-fsanitize=undefined'))
                      if symbol in symbols]
        command = ['gcc', '-m32', '-no-pie', *sanitizers, '-std=gnu89', '-O0', '-fno-builtin', '-c', str(probe),
                   '-o', str(output/'probe.o')]
        linked = output/'probe'
        sdl = subprocess.check_output(['python3', str(ROOT/'tools/native_sdl_config.py'), 'libs'], text=True).splitlines()
        flags = ['-lm', *sdl, '-Wl,--wrap=main', '-Wl,--version-script='+str(ROOT/'tools/native_symbols.map')]
        linker = ['gcc', '-m32', '-no-pie', *sanitizers]
    commands = [command, linker+[str(x) for x in inputs]+[str(output/'probe.o'), '-o', str(linked)]+flags]
    for i, command in enumerate(commands):
        status, log = run(command, directory, target+'-build-'+str(i))
        require(status == 0, 'Probe build failed: '+str(log))
    return dict(path=str(linked), sha256=sha(linked.with_suffix('.wasm') if is_wasm else linked),
                production_objects={p.name: sha(p) for p in inputs}, commands=commands)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-before', type=Path, required=True)
    parser.add_argument('--wasm-before', type=Path, required=True)
    parser.add_argument('--native-after', type=Path, required=True)
    parser.add_argument('--wasm-after', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--clean', action='store_true')
    args = parser.parse_args()
    output = prepare_output(args.output)
    require(WORK in output.parents and not output.exists(), 'Use a fresh /tmp/wasm-dd2/ path')
    output.mkdir()
    values = cases()
    inputs = output/'inputs.bin'
    inputs.write_bytes(b''.join(struct.pack('<Q', value) for value in values))
    machine = output/'original-frexp.bin'
    machine.write_bytes(routine(ROOT/'DestructionDerby2/dd2h.exe'))
    runner = output/'invoke.js'
    runner.write_text('''const [binary, mode, input, output, machine] = process.argv.slice(2);
global.Module = { onRuntimeInitialized() {
  try {
    const status = Module.ccall('dd2_crt_probe', 'number',
      ['number','string','string','string'], [Number(mode), input, output, machine]);
    process.exit(status);
  } catch (error) { console.error(error); process.exit(99); }
}};
const fs = require('fs'), vm = require('vm');
const source = fs.readFileSync(binary, 'utf8');
// Preserve Emscripten's actual CommonJS/Node loader and its relative wasm path.
const localRequire = require('module').createRequire(binary);
const loaded = { exports: {} };
vm.runInThisContext('(function(Module,require,module,exports,__dirname,__filename){'+source+'\\n})',
  {filename: binary})(global.Module,localRequire,loaded,loaded.exports,require('path').dirname(binary),binary);
''')
    builds = {}
    observations = {}
    failures = []
    for target in ('native-before', 'wasm-before', 'native-after', 'wasm-after'):
        builds[target] = link(output, getattr(args, target.replace('-', '_')).resolve(), target)
        modes = (0, 1, 2) if target.endswith('after') else (0, 1)
        if target == 'native-after': modes += (3,)
        for mode in modes:
            raw = output/(target+'-'+str(mode)+'.bin')
            tail = [str(mode), str(inputs), str(raw), str(machine)]
            command = (['node', str(runner), builds[target]['path']] if target.startswith('wasm')
                       else [builds[target]['path']])+tail
            status, log = run(command, output, target+'-run-'+str(mode))
            item = dict(exit_status=status, command=command, log=str(log))
            observations[target+':'+str(mode)] = item
            if target.endswith('before'):
                if status:
                    message = ('RuntimeError: unreachable' if mode == 0 else
                               'RuntimeError: null function or function signature mismatch')
                    require(target.startswith('wasm') and status == 99 and
                            message in log.read_text(),
                            'Old failure was not the actual WASM signature trap')
                    if mode == 0:
                        index = re.search(r'wasm-function\[(\d+)\]', log.read_text()).group(1)
                        symbols = Path(builds[target]['path']+'.symbols').read_text().splitlines()
                        require(index+':signature_mismatch:frexp' in symbols,
                                'Formatting failed in a different function')
                    failures.append(target+':'+str(mode))
                elif mode == 1:
                    require(raw.exists(), 'Probe did not execute')
                    try: standard(records(raw, len(values)), values)
                    except RuntimeError: failures.append(target+':'+str(mode))
                else:
                    require('CRT_PROBE_FORMAT_OK' in log.read_text(), 'Formatting probe did not execute')
                continue
            require(status == 0, 'Actual production-object probe failed: '+str(log))
            if mode == 0:
                require('CRT_PROBE_FORMAT_OK' in log.read_text(), 'Formatting probe did not execute')
            if mode:
                item['raw_sha256'] = sha(raw)
                if mode == 1: standard(records(raw, len(values)), values)
    require('native-before:1' in failures and 'wasm-before:0' in failures and
            'wasm-before:1' in failures, 'Old libc shadow did not reproduce')
    original = records(output/'native-after-3.bin', len(values))
    for target in ('native-after', 'wasm-after'):
        watcom(records(output/(target+'-2.bin'), len(values)), original)
    controls = list(failures)
    for field in (0, 1, 2):
        damaged = list(original)
        row = list(damaged[31]); row[field] ^= 1; damaged[31] = tuple(row)
        try: watcom(damaged, original)
        except RuntimeError: controls.append('changed Watcom '+('fraction', 'exponent', 'write guard')[field])
        else: raise RuntimeError('Changed dispatch evidence accepted')
    for field in (0, 1):
        damaged = records(output/'native-after-1.bin', len(values))
        row = list(damaged[5]); row[field] ^= 1; damaged[5] = tuple(row)
        try: standard(damaged, values)
        except RuntimeError: controls.append('changed libc '+('fraction', 'exponent')[field])
        else: raise RuntimeError('Changed libc evidence accepted')
    report = dict(scope=__doc__, pass_=True, cases=len(values), original_exe_sha256=EXE_SHA256,
                  original_va='0x45c0f0', original_instruction_sha256=sha(machine),
                  original_execution='unaltered position-independent i386 instructions, same stack/EDX:EAX ABI',
                  watcom_all_result_bits_checked=True, libc_nan_payload_bits_checked=False,
                  libc_nonfinite_exponents_checked=False,
                  inputs_sha256=sha(inputs), probe_sha256=sha(ROOT/'tools/frexp_abi_probe.c'),
                  verifier_sha256=sha(Path(__file__)), builds=builds, observations=observations,
                  negative_controls=controls, original_port_av_parity='unproven')
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    if args.clean:
        opened = open_files()
        for path in output.glob('*.bin'):
            if path == machine: continue
            stat = path.stat()
            require((stat.st_dev, stat.st_ino) not in opened, 'Raw result still open')
            path.unlink()
    print('PASS production Native/WASM libc ABI and', len(values), 'original instruction cases')


if __name__ == '__main__':
    main()
