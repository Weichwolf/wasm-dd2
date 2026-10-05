#!/usr/bin/env python3
"""Replay actual original observations through old/current default host policies.

This verifies keyboard intentions with the recorded observation as an explicit
input. It does not prove a finish, a tuned policy, engine behavior or A/V parity.
An incomplete original driving diagnosis can supply this host-only regression.
"""
import argparse
import copy
import importlib.util
import json
from pathlib import Path

from artifacts import WORK, check_space, prepare_output
from verify_configuration_persistence import EXE_SHA256, ROOT, digest, require


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--current-source', type=Path, default=ROOT/'tools/natural_champ_driver.py')
    args = parser.parse_args()
    require(WORK in args.reference.resolve().parents, 'Actual original reference under /tmp/wasm-dd2 required')
    out = prepare_output(args.report)
    require(WORK in out.parents and not out.exists(), 'Fresh report under /tmp/wasm-dd2 required')
    capture = json.loads((args.reference/'report.json').read_text())
    require(capture['target']=='original' and capture['binary_sha256']==EXE_SHA256 and
            not capture['engine_state_writes'] and capture['driver']=='hardware' and
            capture['driving_policy']=='steady', 'Actual unmodified original steady-policy observations required')
    source = args.reference/'driver-source/natural_champ_driver.py'
    require(digest(source.read_bytes())==capture['driving_sources'][source.name], 'Immutable recorded policy required')
    current = args.current_source.resolve()
    old, new = module('recorded_road_policy', source), module('current_road_policy', current)
    drivers = [old.KeyboardDriver(steady=True), new.KeyboardDriver(steady=True)]
    raw = args.reference/'turn0/history/driving.jsonl'
    count = 0
    with raw.open() as stream:
        for line in stream:
            row = json.loads(line)
            require(row['level']==1 and row['player']==0 and not row['quit'], 'Actual live first-track observations required')
            # Reconstruct observe(steady=True)'s output before the policy adds
            # reverse/recovery state. Geometry/error/yaw are recorded inputs.
            pre = copy.deepcopy(row)
            pre['target_steering'] = max(-192, min(192, -row['heading_error']*.6+row['yaw_rate']*6))
            pre['wanted'] = ['a'] if row['speed']<250 else []
            if row['steering']>pre['target_steering']+40:
                pre['wanted'].append('Left')
            elif row['steering']<pre['target_steering']-40:
                pre['wanted'].append('Right')
            for policy in [old, new]:
                policy.observe = lambda read, steady, value=pre: copy.deepcopy(value)
            results = [driver.controls(None, row['ticks']) for driver in drivers]
            require(results[0]==results[1] and results[1]['wanted']==row['wanted'],
                    'Default host intentions differ at observation '+str(count))
            count += 1
    require(count>0, 'Actual recorded observations required')
    out.write_text(json.dumps(dict(scope=__doc__, pass_=True, observations=count,
        reference=str(args.reference), old_source_sha256=digest(source.read_bytes()),
        current_source_sha256=digest(current.read_bytes()), input_sha256=digest(raw.read_bytes()),
        verifier_sha256=digest(Path(__file__).read_bytes())), indent=2)+'\n')
    check_space(args.reference)
    print('Default host road-driver intentions: PASS;', count, 'actual observations')


if __name__=='__main__':
    main()
