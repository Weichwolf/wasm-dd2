#!/usr/bin/env python3
"""Verify lossless clock/observation exports from a complete actual trace.

Both storage formats retain every original call and every observation byte.
Independent audio-service exports must remain identical. This is input storage
and scheduling validation, not full original/port engine audio/video parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, open_files, prepare_output
from reference.game_clock import EXE, export_clock, observation_records, tick_values
from reference.engine_audio_services import export as export_services
from reference.trace_log import read_trace, write_trace


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def verify(args):
    capture = args.capture.resolve()
    source = json.loads((capture/'report.json').read_text())
    old_clock = json.loads((capture/'game-clock/report.json').read_text())
    if (not source['pass_'] or source['debugger'] or source['engine_state_writes'] or
            source['original_exe_sha256'] != EXE or not old_clock['pass_']):
        raise ValueError('Verified unchanged original without debugger/state writes required')
    trace = capture/'wine.log'
    if not trace.exists():
        trace = Path(str(trace)+'.zst')
    # Keep the authoritative trace open across stale-log cleanup, then make a
    # bounded private copy. No producer is restarted or reconstructed here.
    with trace.open('rb') as actual:
        output = prepare_output(args.output)
        if WORK not in output.parents or output.exists():
            raise ValueError('Fresh /tmp/wasm-dd2/ output required')
        output.mkdir(parents=True)
        private_trace = output/trace.name
        with private_trace.open('wb') as target:
            shutil.copyfileobj(actual, target)
    sources = [Path(__file__), Path(__file__).with_name('game_clock.py'),
               Path(__file__).with_name('engine_audio_services.py'),
               Path(__file__).with_name('clock_runs.py'), private_trace]
    frozen = {str(p): sha(p) for p in sources}
    reports = {}
    for variant, encoding, compressed in [('raw', 'raw', False), ('compact', 'DD2TKR1', True)]:
        root = output/variant
        root.mkdir()
        reports[variant] = export_clock(private_trace, args.executable, root/'game-clock',
            allow_terminal_entry=True, ticks_encoding=encoding,
            compress_observations=compressed, max_calls=0xffffffff)
        clock = reports[variant]
        for key in ('trace_sha256', 'calls', 'call_sites', 'first', 'last',
                    'observations_sha256', 'terminal_unreturned_entry'):
            if clock[key] != old_clock[key]:
                raise AssertionError('Actual original clock evidence differs: '+key)
        if clock['logical_ticks_sha256'] != old_clock.get('logical_ticks_sha256', old_clock['ticks_sha256']):
            raise AssertionError('Actual logical clock returns differ')
        original_values = tick_values(capture/'game-clock', old_clock)
        for value in tick_values(root/'game-clock', clock):
            if value != next(original_values, None):
                raise AssertionError('Literal original DWORD differs')
        if next(original_values, None) is not None:
            raise AssertionError('Original DWORD extent differs')
        with read_trace(capture/'game-clock/observations.bin') as original:
            if original.read(16) != struct.pack('<8sII', b'DD2GC01\0', 1, 32):
                raise AssertionError('Actual original observation header differs')
            with observation_records(root/'game-clock', clock) as records:
                for row in records:
                    if struct.pack('<QQIIII', *row) != original.read(32):
                        raise AssertionError('Literal original observation differs')
            if original.read(1):
                raise AssertionError('Actual original observation extent differs')
        os.link(private_trace, root/private_trace.name)
        (root/'timer-callbacks.json').symlink_to(capture/'timer-callbacks.json')
        services = export_services(root, args.mixer, output/(variant+'-services'))
        baseline = (args.services/'services.bin').read_bytes()
        if (output/(variant+'-services')/'services.bin').read_bytes() != baseline:
            raise AssertionError('Actual event/flip/logical-clock service bytes differ')
        clock['service_events'] = services['events']
        clock['services_sha256'] = services['input_sha256']
    with read_trace(capture/'game-clock/observations.bin') as original:
        original_observations = original.read(16+4*32)
    control_report = dict(calls=4, observations_sha256=hashlib.sha256(original_observations).hexdigest())
    negatives = []
    damaged = bytearray(original_observations)
    damaged[-1] ^= 1
    cases = [('partial', original_observations[:-1], False, False),
             ('extra', original_observations+original_observations[-32:], False, False),
             ('changed', bytes(damaged), False, False),
             ('changed-tail-early-exit', bytes(damaged), False, True),
             ('changed-compressed-tail', bytes(damaged), True, True),
             ('truncated-zstd', original_observations, True, False),
             ('truncated-zstd-early-exit', original_observations, True, True)]
    for name, raw, compressed, early in cases:
        directory = output/('negative-'+name)
        directory.mkdir()
        path = directory/'observations.bin'
        with write_trace(path, compressed=compressed) as stream:
            stream.write(raw)
        if name.startswith('truncated-zstd'):
            path = Path(str(path)+'.zst')
            with path.open('r+b') as stream:
                stream.truncate(path.stat().st_size-1)
        try:
            with observation_records(directory, control_report) as rows:
                if early:
                    next(rows)
                else:
                    list(rows)
        except ValueError as error:
            negatives.append(dict(case=name, rejected=True, error=str(error)))
        else:
            raise AssertionError('Damaged observation accepted: '+name)
    try:
        export_clock(private_trace, args.executable, output/'over-bound',
                     ticks_encoding='DD2TKR1', compress_observations=True, max_calls=1)
    except ValueError as error:
        if list((output/'over-bound').glob('*.bin*')):
            raise AssertionError('Failed bounded export left replay inputs')
        negatives.append(dict(case='explicit-call-bound', rejected=True, error=str(error)))
    else:
        raise AssertionError('Explicit call bound ignored')
    if any(sha(Path(p)) != h for p, h in frozen.items()):
        raise RuntimeError('Verification sources changed while running')
    compact = output/'compact/game-clock'
    result = dict(scope=__doc__.strip(), pass_=True, source_hashes=frozen,
                  actual_calls=old_clock['calls'], actual_trace_sha256=old_clock['trace_sha256'],
                  formats=reports, negative_cases=negatives,
                  original_clock_bytes=old_clock['calls']*4,
                  encoded_clock_bytes=(compact/'ticks.bin').stat().st_size,
                  original_observation_bytes=16+old_clock['calls']*32,
                  encoded_observation_bytes=(compact/'observations.bin.zst').stat().st_size)
    (output/'report.json').write_text(json.dumps(result, indent=2)+'\n')
    opened = open_files()
    for path in [private_trace, output/'raw'/trace.name, output/'compact'/trace.name,
                 *output.rglob('*.bin'), *output.rglob('*.bin.zst')]:
        if path.is_symlink():
            continue
        st = path.stat()
        if (st.st_dev, st.st_ino) in opened:
            raise RuntimeError('Successful comparison output still open')
        path.unlink()
    check_space(output)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('capture', 'executable', 'mixer', 'services', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    result = verify(parser.parse_args())
    print('Lossless clock/observations: PASS;', result['actual_calls'],
          'actual returns and observations; service inputs identical;',
          len(result['negative_cases']), 'rejected controls')
