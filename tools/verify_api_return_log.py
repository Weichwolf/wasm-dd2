#!/usr/bin/env python3
"""Verify native API return observations against an actual original transcript.

This checks compact observation, caller-visible values and boundary ordering.
It does not establish engine/frame/PCM or physical timing parity.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

from artifacts import WORK, prepare_output, run_bounded
from native_api_returns import HEADER, RECORD, merge, returns
from verify_champ_history import recorded_apis
from verify_champ_season import EXE

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def signature(event):
    return (event['kind'], event['clock_calls'], event['rng_calls'],
            event.get('value'), event.get('before'), event.get('after'), event.get('result'))


def compare(data, expected):
    actual = list(returns(data))
    if [signature(row) for row in actual] != [signature(row) for row in expected]:
        raise ValueError('Actual native return values or mixed API order differ')
    return actual


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--source', type=Path, default=ROOT/'build/dd2_stubs.c')
    parser.add_argument('--before-source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--wasm', action='store_true',
                        help='also check actual WASM caller values with the native observer requested')
    args = parser.parse_args()
    output = prepare_output(args.output.resolve())
    if WORK not in output.parents or output.exists():
        parser.error('Use a fresh /tmp/wasm-dd2/ output directory')
    reference = args.reference.resolve()
    meta_path = reference/('reference.json' if (reference/'reference.json').exists() else 'history.json')
    meta = json.loads(meta_path.read_text())
    if meta['exe_modified'] is not False or meta['exe_sha256'] != EXE or meta['input'] != 'real X11 keys':
        parser.error('Unchanged supported original with actual keys required')
    events = [json.loads(line) for line in (reference/'events.jsonl').read_text().splitlines()]
    ticks, random = [(reference/name).read_bytes() for name in ('ticks.bin', 'random.bin')]
    recorded_apis(meta, events, random, ticks)
    expected = [row for row in events if row['kind'] in ('GetTickCount', 'rand')]
    boundaries = [row for row in events if row['kind'] not in ('GetTickCount', 'rand')]
    output.mkdir()
    schedule = output/'schedule.bin'
    schedule.write_bytes(bytes(1 if row['kind'] == 'GetTickCount' else 2 for row in expected))
    caller_expected = b''.join(struct.pack('<I', row['value'] if row['kind'] == 'GetTickCount' else row['result']) for row in expected)
    env = {k:v for k,v in os.environ.items() if not k.startswith('DD2_')}
    env.update(DD2_TICK_REPLAY=str(reference/'ticks.bin'), DD2_RANDOM_REFERENCE=str(reference/'random.bin'),
               DD2_RANDOM_LEVEL='all', DD2_RANDOM_REQUIRE_INITIAL='1',
               ASAN_OPTIONS='detect_leaks=1:abort_on_error=1', UBSAN_OPTIONS='halt_on_error=1')
    report = dict(scope=__doc__.strip(), pass_=False, original_port_parity='unproven',
        original_exe_sha256=EXE, original_metadata_sha256=sha(meta_path),
        original_events_sha256=sha(reference/'events.jsonl'),
        source_sha256=sha(args.source), before_source_sha256=sha(args.before_source),
        fixture_sha256=sha(ROOT/'tools/api_return_log_test.c'),
        reader_sha256=sha(ROOT/'tools/native_api_returns.py'),
        clock_calls=meta['clock_calls'], calculated_rng_calls=meta['rng_calls'],
        cases=[], negative_controls=[])
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    binaries = {}
    for name, source in [('before',args.before_source), ('current',args.source)]:
        binary = output/name
        command = ['gcc','-m32','-no-pie','-std=gnu99','-w','-g','-fsanitize=address,undefined',
            '-DDD2_NO_FOPEN_WRAP','-ffunction-sections','-fdata-sections',f'-I{ROOT/"build"}',
            *(['-DDD2_HAVE_API_OBSERVER'] if name == 'current' else []),
            str(source),str(ROOT/'tools/api_return_log_test.c'),'-Wl,--gc-sections','-o',str(binary)]
        with (output/(name+'-build.log')).open('w') as log:
            run_bounded(command,directory=output,timeout=120,stdout=log,stderr=log,check=True)
        binaries[name] = binary
    observed = None
    for name, binary, armed in [('before-disabled',binaries['before'],False),
                               ('current-disabled',binaries['current'],False),
                               ('current-enabled',binaries['current'],True)]:
        caller = output/(name+'-caller.bin')
        observed_path = output/(name+'-api.bin')
        case_env = dict(env)
        if armed:
            case_env['DD2_API_OBSERVE'] = str(observed_path)
        run = subprocess.run([str(binary),str(schedule),str(caller)], env=case_env,
                             capture_output=True,text=True,timeout=120)
        (output/(name+'.log')).write_text(run.stdout+run.stderr)
        if run.returncode or caller.read_bytes() != caller_expected:
            raise RuntimeError(f'{name}: actual caller values changed or sanitizer/provider failed')
        if json.loads(run.stdout) != dict(clock_calls=meta['clock_calls'],random_calls=meta['rng_calls'],
                                        schedule_records=len(expected),errno_preserved=True):
            raise RuntimeError('Actual API fixture extent or errno differs')
        if armed:
            data = observed_path.read_bytes()
            observed = compare(data,expected)
            report['api_observations_sha256'] = sha(observed_path)
        elif observed_path.exists():
            raise RuntimeError('Disabled observer created output')
        report['cases'].append(dict(name=name,pass_=True,binary_sha256=sha(binary),
                                   caller_returns_sha256=sha(caller)))
    assert observed is not None
    if args.wasm:
        for name, source in [('before', args.before_source), ('current', args.source)]:
            binary = output/(name+'-wasm.js')
            command = ['emcc', '-std=gnu99', '-w', '-DDD2_NO_FOPEN_WRAP',
                '-ffunction-sections', '-fdata-sections', f'-I{ROOT/"build"}',
                str(source), str(ROOT/'tools/api_return_log_test.c'), '-Wl,--gc-sections',
                '-sNODERAWFS=1', '-sEXIT_RUNTIME=1', '-sGLOBAL_BASE=10485760',
                '-sINITIAL_MEMORY=33554432', '--pre-js', str(ROOT/'tools/node_env.js'),
                '-o', str(binary)]
            with (output/(name+'-wasm-build.log')).open('w') as log:
                run_bounded(command, directory=output, timeout=120, stdout=log, stderr=log, check=True)
            caller = output/(name+'-wasm-caller.bin')
            unused_log = output/(name+'-wasm-api.bin')
            run = subprocess.run(['node', str(binary), str(schedule), str(caller)],
                env={**env, 'DD2_API_OBSERVE': str(unused_log)},
                capture_output=True, text=True, timeout=120)
            (output/(name+'-wasm.log')).write_text(run.stdout+run.stderr)
            if run.returncode or caller.read_bytes() != caller_expected or unused_log.exists():
                raise RuntimeError('WASM actual caller values changed or native observer created output')
            if json.loads(run.stdout) != dict(clock_calls=meta['clock_calls'], random_calls=meta['rng_calls'],
                                             schedule_records=len(expected), errno_preserved=True):
                raise RuntimeError('WASM actual API extent or errno differs')
            report['cases'].append(dict(name=name+'-wasm', pass_=True,
                binary_sha256=sha(binary.with_suffix('.wasm')), caller_returns_sha256=sha(caller),
                native_observer_requested=True, native_observer_output_created=False))
    combined = merge(boundaries,observed)
    if [row['kind'] for row in combined] != [row['kind'] for row in events]:
        raise RuntimeError('Original actual boundary/API ordering changed')
    report['merged_event_count'] = len(combined)
    changes = []
    def changed_word(label, offset, value=None):
        altered = bytearray(data)
        if value is None:
            altered[offset] ^= 1
        else:
            struct.pack_into('<I',altered,offset,value)
        changes.append((label,bytes(altered)))
    changed_word('header',8)
    changes.append(('truncated',data[:-1]))
    changed_word('sequence',len(HEADER)+4,0)
    changed_word('kind',len(HEADER),3)
    changed_word('caller',len(HEADER)+28,0)
    changed_word('clock-count',len(HEADER)+8)
    first_random = next(i for i,row in enumerate(observed) if row['kind'] == 'rand')
    for label,offset in [('rng-before',16),('rng-after',20),('rng-return',24)]:
        changed_word(label,len(HEADER)+first_random*RECORD.size+offset)
    # Reorder adjacent different APIs while keeping a structurally valid log,
    # every RNG triple and each caller value intact. The original order must
    # still reject; a self-consistent log alone cannot establish provenance.
    rows = [list(row) for row in RECORD.iter_unpack(data[len(HEADER):])]
    index = next(i for i in range(len(rows)-1) if rows[i][0] != rows[i+1][0])
    rows[index],rows[index+1] = rows[index+1],rows[index]
    clock_count = random_count = 0
    for index,row in enumerate(rows,1):
        clock_count += row[0] == 1
        random_count += row[0] == 2
        row[1:4] = [index,clock_count,random_count]
    reordered = HEADER+b''.join(RECORD.pack(*row) for row in rows)
    list(returns(reordered))
    changes.append(('mixed-api-order',reordered))
    for label,changed in changes:
        try:
            compare(changed,expected)
        except ValueError as error:
            report['negative_controls'].append(dict(mutation=label,rejected=True,error=str(error)))
        else:
            raise RuntimeError('Changed native API observation accepted: '+label)
    shifted = copy.deepcopy(boundaries)
    shifted[0]['clock_calls'] += 1
    try:
        merge(shifted,observed)
    except ValueError as error:
        report['negative_controls'].append(dict(mutation='boundary-api-position',rejected=True,error=str(error)))
    else:
        raise RuntimeError('Changed boundary accepted')
    sentinel = output/'existing.bin'
    sentinel.write_bytes(b'preserve-existing-output')
    run = subprocess.run([str(binaries['current']),str(schedule),str(output/'exclusive-caller.bin')],
        env={**env,'DD2_API_OBSERVE':str(sentinel)},capture_output=True,text=True,timeout=30)
    if run.returncode != 1 or 'cannot create exclusive return log' not in run.stderr or sentinel.read_bytes() != b'preserve-existing-output':
        raise RuntimeError('Existing observer output was overwritten or wrong failure')
    report['negative_controls'].append(dict(mutation='existing-output',rejected=True))
    report['pass_'] = True
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    # All raw observation/control fixture files are closed and diagnosed.
    for path in output.glob('*.bin'):
        path.unlink()
    print('PASS actual original mixed API returns, unchanged caller values/errno, native ASan/UBSan;',
          len(report['negative_controls']),'changed-observation/order controls rejected',flush=True)


if __name__ == '__main__':
    main()
