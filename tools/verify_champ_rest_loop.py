#!/usr/bin/env python3
"""Diagnose the original's post-Retire rest loop and production-browser Steam calls.

The first-race original capture deliberately delays real draws; actual clock
returns, RNG callers and computed native replay identify timing-sensitive
original behavior. This partial diagnosis does not prove full browser A/V parity.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

from artifacts import open_files, prepare_output
from verify_champ_history import recorded_apis, work
from verify_champ_season import EXE, KEYS


def partial(root, target):
    meta = json.loads((root / 'history.json').read_text())
    events = [json.loads(line) for line in (root / 'events.jsonl').read_text().splitlines()]
    if meta['keys'] != KEYS[:17] or meta['complete_retirement_season'] or not meta['acknowledged_keys']:
        raise ValueError('Actual acknowledged first-result diagnosis required')
    if target == 'original' and (meta['exe_modified'] is not False or meta['exe_sha256'] != EXE or meta['input'] != 'real X11 keys' or meta['observed_play_draw_delay_ms'] != 160):
        raise ValueError('Supported actual paced original required')
    if target == 'native' and (meta['input'] != 'dd2_key_event' or 'api_inputs' not in meta):
        raise ValueError('Actual native computed API replay required')
    random = (root / 'random.bin').read_bytes()
    ticks = (root / 'ticks.bin').read_bytes()
    recorded_apis(meta, events, random, ticks)
    return meta, events, random, ticks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('original', 'native', 'browser', 'layout', 'report'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--clean', action='store_true')
    args = parser.parse_args()
    original, native, browser = (work(p) for p in (args.original, args.native, args.browser))
    a, b = partial(original, 'original'), partial(native, 'native')
    if a[2:] != b[2:] or a[0]['initial_save_sha256'] != b[0]['initial_save_sha256']:
        raise ValueError('Actual native clock/RNG inputs/save differ')
    if (a[0]['clock_calls'], a[0]['rng_calls']) != (8, 524):
        raise ValueError('Original adaptive rest-loop diagnosis differs')
    steam = [row for row in a[1] if row['kind'] == 'rand' and row['caller'] in ('0x436ae4', '0x436afa')]
    if [(row['caller'], row['rng_calls']) for row in steam] != [('0x436ae4', 485), ('0x436afa', 486)]:
        raise ValueError('Actual original Steam call pair missing/reordered')
    if any((row['quit'], row['frame_skip'], row['ticks'], row['countdown']) != (1, 8, 16, 100) for row in steam):
        raise ValueError('Original Steam was not in the post-Retire rest loop')
    result = 'step17-Return'
    original_state = json.loads((original / result / 'checkpoint.json').read_text())
    native_state = json.loads((native / result / 'checkpoint.json').read_text())
    browser_state = json.loads((browser / result / 'checkpoint.json').read_text())
    if original_state['cars'] != native_state['cars'] or original_state['cars'] != browser_state['cars']:
        raise ValueError('Complete first-race standings differ')
    trace = json.loads((browser / 'rng-call-trace.json').read_text())
    nav = json.loads((browser / 'navigation.json').read_text())
    layout = json.loads(work(args.layout).read_text())
    if trace['error'] is not None or trace['wasm_sha256'] != layout['wasm_sha256'] or nav['wasm_sha256'] != layout['wasm_sha256'] or nav['initial_save_sha256'] != a[0]['initial_save_sha256']:
        raise ValueError('Actual production browser trace provenance differs')
    rand_name = '$func' + str(trace['rand_definition_ordinal'] + layout['imported_function_count'])
    steam_name = '$func' + str(layout['steam_function_ordinal'] + layout['imported_function_count'])
    call_counts = {}
    for race, first in [(1, 484), (2, 1008)]:
        calls = [row for row in trace['calls'] if row['race'] == race]
        if len(calls) != 40 or [row['count'] for row in calls] != list(range(first, first + 40)):
            raise ValueError('Browser confirmation-to-result calls incomplete/reordered')
        if any(row['stack'][0]['function'] != rand_name for row in calls):
            raise ValueError('Actual production rand stack differs')
        callers = Counter(row['stack'][1]['function'] for row in calls)
        if callers[steam_name] != 2 or sorted(callers.values()) != [2, 19, 19]:
            raise ValueError('Browser extra Steam/result/finish call counts differ')
        for row in calls[:2]:
            if row['stack'][1]['function'] != steam_name or row['quit'] != 1 or row['frame_skip'] != 8 or row['ticks'] % 8 or row['countdown'] != 100:
                raise ValueError('Browser Steam was not after Retire in the rest loop')
        call_counts[race] = dict(callers)
    first = [row for row in trace['calls'] if row['race'] == 1][:2]
    if [row['seed'] for row in first] != [row['before'] for row in steam]:
        raise ValueError('Actual first-race Steam pre-seeds differ')
    images = []
    for filename, size in [('framebuf.bin', 307200), ('palette.bin', 1024)]:
        data = [(root / result / filename).read_bytes() for root in (original, native, browser)]
        if any(len(raw) != size for raw in data) or data[0] != data[1]:
            raise ValueError('Native paced-original result image differs/incomplete')
        images.append(dict(region=filename, native_literal_match=True,
            live_browser_literal_match=data[0] == data[2], live_browser_differing_bytes=sum(x != y for x, y in zip(data[0], data[2])),
            sha256={name:hashlib.sha256(raw).hexdigest() for name, raw in zip(('original', 'native', 'browser'), data)}))
    report = dict(scope=__doc__.strip(), diagnosis_pass=True, original_exe_sha256=EXE,
        native_sha256=b[0]['binary_sha256'], wasm_sha256=trace['wasm_sha256'],
        original_clock_calls=8, original_rng_calls=524, original_draw_delay_ms=160,
        original_steam=steam, browser_call_counts=call_counts, complete_first_race_standings_match=True,
        browser_first_steam_pre_seeds=[row['seed'] for row in first], result_images=images,
        full_browser_original_av_acceptance=False)
    output = prepare_output(work(args.report)); output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2) + '\n')
    if args.clean:
        opened = open_files()
        files = [p for root in (original, native) for p in root.glob('step*/*.bin') if not p.is_symlink()]
        if any((p.stat().st_dev, p.stat().st_ino) in opened for p in files):
            raise RuntimeError('Completed first-race capture still open')
        for p in files:
            p.unlink()
    print('PASS diagnosis: real original also runs Steam twice after Retire; native reproduces all API inputs; 20 first-race standings match live browser. No full A/V claim.')


if __name__ == '__main__':
    main()
