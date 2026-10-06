#!/usr/bin/env python3
"""Verify lossless clock input through production native/ASan/WASM readers.

Every actual supplied original DWORD is emitted and compared literally. Wrap,
large repeats, malformed input and premature termination are independent
transport controls; this does not establish engine or original A/V parity.
"""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import subprocess

from artifacts import WORK, check_space, open_files, prepare_output, run_bounded
from reference.clock_runs import MAGIC, RUN, pack, values

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(args):
    out = prepare_output(args.output)
    if WORK not in out.parents or out.exists():
        raise ValueError('Fresh /tmp/wasm-dd2/ output required')
    out.mkdir(parents=True)
    source = ROOT / 'build/dd2_stubs.c'
    hashes = {str(p): sha(p) for p in (source, Path(__file__), ROOT / 'tools/clock_runs_test.c',
                                      ROOT / 'tools/reference/clock_runs.py')}
    proof = pack(args.original_ticks, out / 'actual.rle')
    expected = args.original_ticks.read_bytes()
    if len(expected) != proof['calls'] * 4:
        raise ValueError('Actual original DWORD extent differs')
    old = out / 'old-close.c'
    text = source.read_text()
    guard = 'dd2_tick_remaining || fgetc(dd2_tick_file)!=EOF'
    if text.count(guard) != 1:
        raise ValueError('Production logical-run leftover guard required')
    old.write_text(text.replace(guard, 'fgetc(dd2_tick_file)!=EOF'))
    common = ['-O2', '-std=gnu99', '-w', '-ffunction-sections', '-fdata-sections',
              '-DDD2_NO_FOPEN_WRAP', '-I' + str(ROOT / 'build'),
              str(ROOT / 'tools/clock_runs_test.c')]
    binaries = {}
    for name, compiler, flags, unit in (
            ('native', 'gcc', ['-m32', '-no-pie', '-Wl,--gc-sections'], source),
            ('native-asan', 'gcc', ['-m32', '-no-pie', '-fsanitize=address', '-Wl,--gc-sections'], source),
            ('wasm', 'emcc', ['-sNODERAWFS=1', '-sEXIT_RUNTIME=1'], source),
            ('native-old-close', 'gcc', ['-m32', '-no-pie', '-Wl,--gc-sections'], old),
            ('wasm-old-close', 'emcc', ['-sNODERAWFS=1', '-sEXIT_RUNTIME=1'], old)):
        binary = out / (name + '.js' if compiler == 'emcc' else name)
        with (out / (name + '-build.log')).open('w') as log:
            run_bounded([compiler, *common, *flags, str(unit), '-o', str(binary), '-lm'],
                        directory=out, timeout=120, check=True, stdout=log, stderr=subprocess.STDOUT)
        binaries[name] = (['node', str(binary)] if compiler == 'emcc' else [str(binary)])
    environment = {k: v for k, v in os.environ.items() if not k.startswith('DD2_')}
    environment['ASAN_OPTIONS'] = 'detect_leaks=0:abort_on_error=1'
    results = []

    def run(target, name, data, calls, error=None, rle=True, emitted=False, fmt=None):
        path = out / (name + '.ticks')
        if data is not None:
            path.write_bytes(data)
        else:
            path = out / 'actual.rle' if rle else args.original_ticks
        env = dict(environment, DD2_TICK_REPLAY=str(path))
        if rle:
            env['DD2_TICK_REPLAY_FORMAT'] = 'DD2TKR1'
        if fmt is not None:
            env['DD2_TICK_REPLAY_FORMAT'] = fmt
        actual = out / (target + '-' + name + '.actual')
        command = [*binaries[target], str(calls), str(path),
                   env.get('DD2_TICK_REPLAY_FORMAT', '-'), *([str(actual)] if emitted else [])]
        stdout_path = out / (target + '-' + name + '.stdout')
        stderr_path = out / (target + '-' + name + '.stderr')
        with stdout_path.open('w') as stdout, stderr_path.open('w') as stderr:
            result = run_bounded(command, directory=out, timeout=120, check=False,
                                 env=env, stdout=stdout, stderr=stderr)
        output_text, error_text = stdout_path.read_text(), stderr_path.read_text()
        (out / (target + '-' + name + '.log')).write_text(output_text + error_text)
        stdout_path.unlink()
        stderr_path.unlink()
        if error:
            if result.returncode != 1 or error not in error_text:
                raise AssertionError('Clock error not detected: ' + target + ' ' + name)
            return dict(target=target, case=name, rejected=True, error=error)
        if result.returncode:
            raise AssertionError(target + ' ' + name + ' failed: ' + error_text)
        report = json.loads(output_text)
        if report['calls'] != calls or f'consumed={calls} complete' not in error_text:
            raise AssertionError('Logical call/close extent differs')
        if emitted and actual.read_bytes() != expected:
            raise AssertionError('Actual original clock return differs on ' + target + ' ' + name)
        return dict(target=target, case=name, pass_=True, observation=report,
                    original_bytes_compared=emitted)

    wrap = [0xfffffff0, 0xfffffff8, 3, 3]
    packed_wrap = MAGIC + b''.join(RUN.pack(value, count)
                                  for value, count in ((wrap[0], 1), (wrap[1], 1), (3, 2)))
    large_calls = 10000001
    large = MAGIC + RUN.pack(0xfffffff0, large_calls - 1) + RUN.pack(3, 1)
    negatives = [('header', b'bad', 1, 'invalid clock run header'),
                 ('empty', MAGIC, 1, 'clock input exhausted'),
                 ('partial-run', MAGIC + b'\x01', 1, 'partial tick run'),
                 ('zero-run', MAGIC + RUN.pack(3, 0), 1, 'invalid tick run count'),
                 ('exhausted', MAGIC + RUN.pack(3, 1), 2, 'clock input exhausted'),
                 ('partial-raw', b'\x01', 1, 'partial tick record'),
                 ('leftover-run', MAGIC + RUN.pack(3, 3), 2, 'unconsumed tick records'),
                 ('leftover-record', MAGIC + RUN.pack(3, 1) + RUN.pack(4, 1), 1, 'unconsumed tick records'),
                 ('overflow', MAGIC + RUN.pack(1, 1) + RUN.pack(2, 0xffffffff), 2, 'invalid tick run count')]
    for target in ('native', 'native-asan', 'wasm'):
        for encoded in (False, True):
            results.append(run(target, 'actual-rle' if encoded else 'actual-raw', None,
                               proof['calls'], rle=encoded, emitted=True))
        result = run(target, 'wrap', packed_wrap, len(wrap))
        if result['observation']['last'] != 3 or int(result['observation']['sum']) != sum(wrap):
            raise AssertionError('DWORD wrap/repeated return lost')
        results.append(result)
        result = run(target, 'large-repeat', large, large_calls)
        if (result['observation']['last'] != 3 or
                int(result['observation']['sum']) != 0xfffffff0 * (large_calls - 1) + 3):
            raise AssertionError('Large logical run lost returns')
        results.append(result)
        # Raw values equal to the format signature remain ordinary DWORDs.
        result = run(target, 'raw-signature', MAGIC, 2, rle=False)
        if int(result['observation']['sum']) != sum(struct.unpack('<II', MAGIC)):
            raise AssertionError('Raw signature-like values reinterpreted')
        results.append(result)
        for name, data, calls, error in negatives:
            results.append(run(target, name, data, calls, error, rle=name != 'partial-raw'))
        results.append(run(target, 'unknown-format', MAGIC, 1, 'unknown clock input format', fmt='bad'))
    for target in ('native-old-close', 'wasm-old-close'):
        accepted = run(target, 'old-leftover-run', MAGIC + RUN.pack(3, 3), 2)
        results.append(dict(target=target, case='old-close-accepts-missing-logical-call',
                            mutant_confirmed=True, observation=accepted['observation']))
    decoder_negatives = 0
    for data in (b'bad', MAGIC, MAGIC + b'\1', MAGIC + RUN.pack(3, 0)):
        try:
            list(values(io.BytesIO(data)))
        except ValueError:
            decoder_negatives += 1
        else:
            raise AssertionError('Malformed Python clock run accepted')
    if any(sha(Path(p)) != h for p, h in hashes.items()):
        raise RuntimeError('Verification sources changed while running')
    report = dict(scope=__doc__.strip(), pass_=True, source_hashes=hashes,
                  actual_original_clock=proof, results=results,
                  python_decoder_negative_cases=decoder_negatives,
                  logical_large_repeat_calls=large_calls)
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    opened = open_files()
    for path in [*out.glob('*.actual'), *out.glob('*.ticks'), old]:
        stat = path.stat()
        if (stat.st_dev, stat.st_ino) in opened:
            raise RuntimeError('Successful comparison output still open')
        path.unlink()
    check_space(out)
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original-ticks', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = verify(args)
    print('Clock runs: PASS;', result['actual_original_clock']['calls'],
          'actual original returns per raw/RLE target;', len(result['results']), 'controls')
