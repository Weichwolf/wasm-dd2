#!/usr/bin/env python3
"""Replay source-track accident scoring independently on Native, WASM and ASan.

Six-second twenty-car scenarios on every original playable level supply physical
poses, retirement flags and responding pair contacts. A Python angle-history
oracle checks attribution deadlines, source 10/25/50 point thresholds, destruction
credit and the 999 cap at every fixed step. Controlled side impacts on original
arena 8 additionally exercise actual 180/360-degree awards and retirement credit
through the physical solver and regional damage. Their assigned initial speeds
are stress inputs, not normal driving or evidence of original handling parity.
This is rewrite rule verification,
not original collision/damage parity or race/championship coverage.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256

CODES = '123456789AB'
STEPS = 1200
COUNT = 20
WINDOW = 300  # Original 75 * 20 ms, expressed in fixed 5 ms steps.


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def observations(row, previous):
    result = []
    for slot, (qx, qy, qz, qw, retired) in enumerate(row['observations']):
        x, z = 2 * (qx * qz + qw * qy), 1 - 2 * (qx * qx + qy * qy)
        heading = math.atan2(x, z) if math.hypot(x, z) > 1e-10 else previous[slot]
        if not math.isfinite(heading) or retired not in (0, 1):
            raise ValueError('Invalid physical observation')
        result.append((heading, bool(retired)))
    if len(result) != COUNT:
        raise ValueError('Incomplete field')
    return result


def check_rows(rows, steps=STEPS):
    traces = [row for row in rows if row.get('accident_step')]
    if [row['step'] for row in traces] != [0, *range(1, steps + 1), 0]:
        raise ValueError('Missing fixed-step/reset observations')
    initial = observations(traces[0], [0] * COUNT)
    points, kills = [0] * COUNT, [0] * COUNT
    windows = [None] * COUNT
    previous = [heading for heading, _ in initial]
    retired_before = [False] * COUNT
    metrics = dict(armed=0, quarter_awards=0, half_awards=0, full_awards=0,
                   destruction_awards=0, cancelled=0, peak_points=0, peak_windows=0,
                   retired=0, steps=steps, vehicles=COUNT, reset_clears=True)

    def validate(row, inputs):
        if len(row['scores']) != COUNT:
            raise ValueError('Incomplete scoring field')
        for slot, actual in enumerate(row['scores']):
            window = windows[slot]
            if window is None:
                remaining, partner, quarters, rotation = 0, COUNT, 0, 0.0
            else:
                remaining = window['deadline'] - row['step']
                partner = window['partner']
                rotation = math.fsum(window['deltas'])
                quarters = 2 if window['peak'] >= math.pi - 1e-10 else int(
                    window['peak'] >= math.pi / 2 - 1e-10)
            expected = [points[slot], kills[slot], remaining, partner, quarters,
                        inputs[slot][0], rotation, int(inputs[slot][1]), row['step']]
            if len(actual) != len(expected):
                raise ValueError('Incorrect exported scoring fields')
            for field, (got, want) in enumerate(zip(actual, expected)):
                close = math.isclose(got, want, abs_tol=1e-8, rel_tol=1e-10) if field in (5, 6) else got == want
                if not close:
                    raise ValueError(f"step {row['step']} slot {slot} field {field}: {got} != {want}")

    validate(traces[0], initial)
    for row in traces[1:-1]:
        current = observations(row, previous)
        if any(before and not now[1] for before, now in zip(retired_before, current)):
            raise ValueError('Wreck resurrects without reset')
        for first, second, speed, impulse in row['events']:
            if not (0 <= first < COUNT and 0 <= second < COUNT and first != second):
                raise ValueError('Invalid physical contact partner')
            if speed <= 500 or impulse <= 250 or retired_before[first] or retired_before[second]:
                continue
            for victim, instigator in ((first, second), (second, first)):
                if windows[victim] is None:
                    windows[victim] = dict(partner=instigator, deadline=row['step'] + WINDOW - 1,
                                           deltas=[], peak=0.0)
                    metrics['armed'] += 1
        for victim, window in enumerate(windows):
            if window is None:
                continue
            instigator = window['partner']
            award = None
            if current[instigator][1]:
                metrics['cancelled'] += 1
                windows[victim] = None
                continue
            if current[victim][1]:
                kills[instigator] += 1
                award = 25
                metrics['destruction_awards'] += 1
            else:
                change = current[victim][0] - previous[victim]
                # Principal argument avoids copying the C branch-based unwrap.
                delta = math.atan2(math.sin(change), math.cos(change))
                window['deltas'].append(delta)
                rotation = math.fsum(window['deltas'])
                window['peak'] = max(window['peak'], abs(rotation))
                if abs(rotation) >= math.tau - 1e-10:
                    award = 50
                    metrics['full_awards'] += 1
                elif row['step'] == window['deadline']:
                    award = 25 if window['peak'] >= math.pi - 1e-10 else 10 if window['peak'] >= math.pi / 2 - 1e-10 else 0
                    if award:
                        metrics['half_awards' if award == 25 else 'quarter_awards'] += 1
            if award is not None:
                points[instigator] = min(999, points[instigator] + award)
                windows[victim] = None
        validate(row, current)
        metrics['peak_points'] = max(metrics['peak_points'], *points)
        metrics['peak_windows'] = max(metrics['peak_windows'], sum(window is not None for window in windows))
        previous = [heading for heading, _ in current]
        retired_before = [retired for _, retired in current]
    metrics['retired'] = sum(retired_before)
    points, kills, windows = [0] * COUNT, [0] * COUNT, [None] * COUNT
    reset = observations(traces[-1], [0] * COUNT)
    validate(traces[-1], reset)
    return metrics


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-accidents-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (ROOT / 'DestructionDerby2/Dirinfo').resolve()
    if digest(archive) != ORIGINAL_SHA256:
        raise ValueError('Provision supported unmodified Dirinfo')
    calls = []

    def run(command, label):
        path = output / (label + '.log')
        with path.open('wb') as log:
            result = run_bounded(command, directory=output, timeout=180, cwd=ROOT,
                                 stdout=log, stderr=subprocess.STDOUT)
        content = path.read_text(errors='replace')
        calls.append(dict(label=label, returncode=result.returncode, log_sha256=digest(path)))
        if result.returncode or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label + ' failed: ' + content[-3000:])
        return [json.loads(line) for line in content.splitlines() if line.startswith('{')]

    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT / 'src'), '-Wall', '-Wextra', '-Wpedantic',
             '-Wno-unused-parameter', '-Wno-unused-function', '-fno-strict-aliasing', '-ffast-math',
             '-Werror', '-Wshadow', '-Wconversion', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wformat=2', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    units = [ROOT / f'src/assets/{name}.c' for name in ('archive', 'level', 'road', 'barriers')]
    units += [ROOT / f'src/physics/{name}.c' for name in
              ('road_contact', 'road_surface', 'vehicle', 'barrier_world', 'car_contact', 'vehicle_collision', 'damage')]
    units += [ROOT / f'src/game/{name}.c' for name in ('starting_grid', 'driving', 'accidents', 'course', 'laps', 'race')]
    units += [ROOT / f'src/ai/{name}.c' for name in ('path', 'driver')]
    units += [ROOT / 'src/platform/file.c']
    sanitized, synthetic, impact = (output / name for name in ('export-sanitized', 'rules-sanitized', 'impact-sanitized'))
    run([tool('clang'), *flags, *map(str, units), str(ROOT / 'tests/rewrite/fleet_export.c'),
         '-lm', '-o', str(sanitized)], 'export-build')
    run([tool('clang'), *flags, str(ROOT / 'src/game/accidents.c'),
         str(ROOT / 'tests/rewrite/accidents_test.c'), '-lm', '-o', str(synthetic)], 'rules-build')
    run([str(synthetic)], 'rules-sanitized')
    run([tool('clang'), *flags, *map(str, units), str(ROOT / 'tests/rewrite/accidents_impact.c'),
         '-lm', '-o', str(impact)], 'impact-build')
    commands = {'native': [str(WORK / 'rewrite-native/dd2_fleet_export')],
                'wasm': ['node', str(WORK / 'rewrite-wasm/dd2_fleet_export.js')],
                'sanitized': [str(sanitized)]}
    levels = []
    for code in CODES:
        targets = {}
        for target, command in commands.items():
            targets[target] = check_rows(run([*command, str(archive), code, 'accidents'], code + '-' + target))
        levels.append(dict(level=code, targets=targets))
        print(json.dumps(dict(level=code, targets=targets, pass_=True)), flush=True)
    for target in commands:
        if sum(level['targets'][target]['armed'] for level in levels) == 0:
            raise ValueError('No actual pair impact attribution exercised')
    impact_commands = {'native': [str(WORK / 'rewrite-native/dd2_accidents_impact')],
                       'wasm': ['node', str(WORK / 'rewrite-wasm/dd2_accidents_impact.js')],
                       'sanitized': [str(impact)]}
    controlled = {}
    for profile, arguments in (('spin', []), ('destruction', ['destruction'])):
        controlled[profile] = {}
        for target, command in impact_commands.items():
            metrics = check_rows(run([*command, str(archive), *arguments], profile + '-' + target), steps=400)
            if profile == 'spin' and (metrics['half_awards'] == 0 or metrics['full_awards'] == 0):
                raise ValueError('Controlled collision does not exercise 180/360-degree awards')
            if profile == 'destruction' and (metrics['destruction_awards'] == 0 or metrics['retired'] == 0):
                raise ValueError('Controlled collision does not exercise physical destruction credit')
            controlled[profile][target] = metrics
    sources = [path for path in (ROOT / 'src').rglob('*') if path.suffix in ('.c', '.h')]
    sources += [ROOT / 'tests/rewrite/fleet_export.c', ROOT / 'tests/rewrite/accidents_test.c',
                ROOT / 'tests/rewrite/accidents_impact.c', ROOT / 'tests/rewrite/accident_trace.h',
                ROOT / 'CMakeLists.txt', Path(__file__).resolve()]
    report = dict(pass_=True, scope=__doc__.strip(), verified_at=datetime.now(timezone.utc).isoformat(),
                  original_sha256=digest(archive), levels=levels, controlled_impact=controlled, calls=calls,
                  source_sha256={str(path.relative_to(ROOT)): digest(path) for path in sources},
                  binary_sha256={str(path): digest(path) for path in
                                 (WORK / 'rewrite-native/dd2_fleet_export', WORK / 'rewrite-wasm/dd2_fleet_export.wasm',
                                  WORK / 'rewrite-native/dd2_accidents_impact', WORK / 'rewrite-wasm/dd2_accidents_impact.wasm',
                                  sanitized, synthetic, impact)})
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    for path in output.iterdir():
        if path.suffix == '.log' or path in (sanitized, synthetic, impact):
            path.unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, levels=len(levels), vehicle_steps=STEPS * COUNT * len(levels),
                         controlled_vehicle_steps=400 * COUNT * len(controlled), report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
