#!/usr/bin/env python3
"""Check host keyboard policy against retained actual original observations.

Recorded road geometry, heading errors and car states are explicit inputs to
the old/current host controller. Check every old/default key intention against
the actual trace, then require the opt-in recovery turn to stay outside the
steering dead zone. No new engine run, lap finish or original/port A/V parity
is established by this counterfactual policy regression.
"""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path

from artifacts import WORK, prepare_output
from verify_configuration_persistence import ROOT, EXE_SHA256, require


def sha(path):
    return hashlib.file_digest(path.open('rb'), 'sha256').hexdigest()


def controller(path, label):
    spec = importlib.util.spec_from_file_location(label, path)
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module


def intentions(module, driver, row, speed_limit):
    def observed(read, steady):
        require(steady, 'Recorded steady controller required')
        state = copy.deepcopy(row)
        # Road geometry already supplied the recorded target/heading error.
        # Reconstruct only the ordinary accelerator intention; controls()
        # computes its own steering and manoeuvre from the explicit state.
        state['wanted'] = ['a'] if row['speed'] < 250 else []
        return state
    module.observe = observed
    state = driver.controls(None, row['ticks'])
    if state['speed'] >= speed_limit and 'a' in state['wanted']:
        state['wanted'].remove('a')
    return state


def defaults(state, recorded):
    require(state['wanted'] == recorded['wanted'] and state['manoeuvre'] == recorded['manoeuvre'],
            'Default host keys differ from actual original trace')


def recovery(state, row):
    if row['manoeuvre'] == 'reverse' and (row['speed'] < -5 or abs(row['speed']) <= 5):
        require(abs(state['target_steering']) >= 96,
                'Reverse recovery cancelled its minimum turn')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True, help='original capture root with frozen driver and events')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = prepare_output(args.output); require(WORK in output.parents, 'Use /tmp/wasm-dd2/')
    require(not output.exists(), 'Use a fresh report path'); output.parent.mkdir(parents=True, exist_ok=True)
    reference = args.reference.resolve()
    original = json.loads((reference/'report.json').read_text())
    require(original['engine_state_writes'] is False and original['binary_sha256'] == EXE_SHA256,
            'Actual unmodified original observer required')
    old_source = reference/'driver-source/natural_champ_driver.py'
    require(sha(old_source) == original['observer_sources'][old_source.name], 'Frozen original driver source changed')
    current_source = ROOT/'tools/natural_champ_driver.py'
    old = controller(old_source, 'recorded_driver'); current = controller(current_source, 'current_driver')
    options = dict(steady=True, movement_distance=100, progress_ticks=1200,
                   slow_ticks=75 if original['driving_slow_recovery'] else 0)
    before = old.KeyboardDriver(**options); after = current.KeyboardDriver(**options)
    escape = current.KeyboardDriver(**options, escape_steering=96)
    observations = changed = reverse_turns = 0; examples = []; checks = []
    events = reference/'history/events.jsonl'
    for line in events.open():
        row = json.loads(line)
        if row['kind'] != 'driver': continue
        require(row['player'] == 0, 'This diagnosis contains the first human driver only')
        observations += 1
        a = intentions(old, before, row, original['driving_speed_limit'])
        b = intentions(current, after, row, original['driving_speed_limit'])
        c = intentions(current, escape, row, original['driving_speed_limit'])
        defaults(a, row); defaults(b, row)
        require(a['wanted'] == b['wanted'] and a['target_steering'] == b['target_steering'],
                'Optional policy changed the default steering')
        recovery(c, row)
        if c['wanted'] != b['wanted']:
            changed += 1
            require(row['manoeuvre'] == c['manoeuvre'] == 'reverse' and
                    {k for k in c['wanted'] if k not in ('Left','Right')} ==
                    {k for k in b['wanted'] if k not in ('Left','Right')},
                    'Wall recovery changed ordinary acceleration or a forward manoeuvre')
            if len(examples) < 5:
                examples.append(dict(tick=row['ticks'],lap=row['lap'],speed=row['speed'],
                                     old_keys=b['wanted'],new_keys=c['wanted'],
                                     old_steering=b['target_steering'],new_steering=c['target_steering']))
        if row['manoeuvre'] == 'reverse' and (row['speed'] < -5 or abs(row['speed']) <= 5):
            reverse_turns += 1
            if not checks and abs(b['target_steering']) < 24:
                try: recovery(b, row)
                except RuntimeError: checks.append('old cancelled turn')
                else: raise RuntimeError('Old dead-zone steering unexpectedly passed')
    require(observations > 1000 and reverse_turns > 100 and changed > 100 and checks,
            'Actual wall diagnosis did not exercise the opt-in correction')
    damaged = dict(row, wanted=['unexpected'])
    try: defaults(b, damaged)
    except RuntimeError: checks.append('changed default key')
    else: raise RuntimeError('Changed default intention accepted')
    report = dict(scope=__doc__,pass_=True,observations=observations,
                  exact_default_intentions=observations, reverse_recovery_observations=reverse_turns,
                  changed_opt_in_intentions=changed,examples=examples,negative_controls=checks,
                  reference=str(reference),input_sha256=sha(events),old_source_sha256=sha(old_source),
                  current_source_sha256=sha(current_source),verifier_sha256=sha(Path(__file__)),
                  original_port_av_parity='unproven')
    output.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS retained original host intentions:',observations,'unchanged defaults;',changed,'opt-in recovery changes')


if __name__ == '__main__':
    main()
