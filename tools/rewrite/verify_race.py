#!/usr/bin/env python3
"""Check race phases/results against independent original data and rule oracles.

Physical twenty-car scenarios cover countdown, six seconds of driving, voluntary
DNF, result freeze and reset on every track/arena. Ordered source-cell scenarios
cover complete circuit races in both scoring modes; they are not physically driven
complete races. A separate full Stockcar race on original circuit 5 drives all
required laps through physics, using AI only to supply player control inputs.
Native, Node/WASM and instrumented C are verified separately.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256
from rewrite.verify_levels import assets
from rewrite.verify_roads import reference
from rewrite.verify_fleet import original_grid
from rewrite.verify_laps import course_numbers, LapOracle, SurfaceOracle

COUNT = 20
CODES = '123456789AB'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class RaceOracle:
    def __init__(self, mode, length, initial_laps, tables):
        self.mode, self.length, self.tables = mode, length, tables
        self.phase = self.end = self.steps = self.elapsed = self.coast = 0
        self.finishers, self.alive = 0, COUNT
        self.drivers = [[0]*10 for _ in range(COUNT)]
        for slot, lap in enumerate(initial_laps):
            self.drivers[slot][2:4] = lap[2:4]
        self.order = self.ranked()
        self.results = [0]*COUNT
        self.assign_places()

    def ranked(self):
        def priority(slot):
            d = self.drivers[slot]
            if d[5]:
                return 0, d[5], 0, slot
            if self.length:
                return 1, -d[2], -d[3], slot
            return 1+d[9], -d[1], 0, slot
        return sorted(range(COUNT), key=priority)

    def assign_places(self):
        for place, slot in enumerate(self.order, 1):
            self.drivers[slot][4] = place

    def tick(self, laps, cars):
        if self.phase == 3:
            return
        self.steps += 1
        if self.phase == 0:
            if self.steps == 400:
                self.phase = 1
            return
        self.elapsed += 1
        for slot, d in enumerate(self.drivers):
            retired, points = cars[slot][-2:]
            if retired and not d[9]:
                d[1] = self.elapsed
            d[6], d[9] = points, retired
            if self.length:
                d[0], d[2], d[3] = laps[slot][1:4]
        fresh = [slot for slot, d in enumerate(self.drivers) if d[0] and not d[5]]
        fresh.sort(key=lambda slot: (self.drivers[slot][0], self.drivers[slot][4]))
        for slot in fresh:
            self.finishers += 1
            self.drivers[slot][5] = self.finishers
        self.alive = sum(not d[9] for d in self.drivers)
        self.order = self.ranked()
        self.assign_places()
        if self.phase == 1:
            if self.drivers[0][5]: self.end = 1
            elif self.drivers[0][9]: self.end = 2
            elif not self.length and self.alive < 2: self.end = 3
            if self.end: self.phase = 2
        else:
            self.coast += 1
            if self.coast == 600: self.publish()

    def publish(self):
        for d in self.drivers:
            d[7] = self.tables[self.mode][d[4]-1] if self.length else 0
            d[8] = min(999, d[7] + (d[6] if self.mode == 0 else 0))
        self.results = sorted(range(COUNT), key=lambda slot: (-self.drivers[slot][8], self.drivers[slot][4]))
        self.phase = 3

    def withdraw(self):
        if not self.end: self.end = 4
        self.publish()

    def compare(self, row):
        expected = dict(state=[self.phase, self.end, self.steps, self.elapsed, self.coast, self.finishers, self.alive],
                        drivers=self.drivers, order=self.order, results=self.results)
        for key, value in expected.items():
            if row[key] != value:
                raise ValueError(f'{row["kind"]} tick {self.steps} {key} differs: {row[key]} != {value}')


def course_from_source(meta, decoded, image, code):
    if code not in '1234567':
        if meta['length'] or meta['laps'] or meta['strips'] or meta['starts']:
            raise ValueError('Arena has circuit progress')
        return None
    numbers, length = course_numbers(decoded)
    finish, _, laps = struct.unpack_from('<3H', image, 0x66df0+int(code)*6)
    if (meta['length'], meta['laps']) != (length, laps):
        raise ValueError('Original circuit rules differ')
    if {offset:number for offset,number,_ in meta['strips']} != numbers:
        raise ValueError('Original progress equivalents differ')
    course = dict(length=length, laps=laps, relative={}, keys={}, first={})
    for offset, number, first in meta['strips']:
        course['first'][offset] = first
        for lane in range(decoded['strips'][offset][6]):
            course['keys'][first+lane] = offset, lane
            course['relative'][first+lane] = (number-finish) % length
    if set(course['keys']) != set(range(len(decoded['cells']))):
        raise ValueError('Cell inventory incomplete')
    sca = [struct.unpack_from('<HBx', image, 0x66da0+i*4) for i in range(COUNT)]
    grid = original_grid(decoded, code, sca)
    for slot, cell in enumerate(meta['starts']):
        points = [decoded['vertices'][index] for index in decoded['cells'][course['keys'][cell]][5:]]
        center = [sum(point[axis] for point in points)/4 for axis in range(3)]
        if center != [grid[slot][0], grid[slot][1]-190, grid[slot][2]]:
            raise ValueError('Race starts outside the source grid cell')
    return course


def projected(oracle):
    v = oracle.values
    return [v[0], v[4], v[9], v[6], v[10], v[11]]


def check(path, decoded, image, code, mode, kind, tables):
    with path.open() as stream:
        meta = json.loads(next(stream))
        if meta['kind'] != 'meta' or meta['mode'] != mode:
            raise ValueError('Race metadata invalid')
        course = course_from_source(meta, decoded, image, code)
        initial = [projected(LapOracle(course, cell)) for cell in meta['starts']] if course else []
        race = RaceOracle(mode, meta['length'], initial, tables)
        lap_oracles = [LapOracle(course, cell) for cell in meta['starts']] if course else []
        surface = SurfaceOracle(decoded, course) if course and kind == 'live' else None
        strips = [course['keys'][cell][0] for cell in meta['starts']] if course else []
        previous = baseline = last_live = frozen = None
        physical = route_steps = rows = max_samples = 0
        for line in stream:
            row = json.loads(line)
            rows += 1
            if row['kind'] == 'part':
                reference_row = {key:value for key,value in last_live.items() if key != 'kind'}
                if {key:value for key,value in row.items() if key != 'kind'} != reference_row:
                    raise ValueError('25 ms and 5 ms race frames differ')
                continue
            if row['kind'] == 'pause':
                if {key:value for key,value in row.items() if key != 'kind'} != {
                        key:value for key,value in last_live.items() if key != 'kind'}:
                    raise ValueError('Partial timing/pause/rejection changes the race')
                continue
            if row['kind'] == 'withdraw':
                race.withdraw()
                frozen = {key:value for key,value in row.items() if key != 'kind'}
            elif row['kind'] == 'frozen':
                if {key:value for key,value in row.items() if key != 'kind'} != frozen:
                    raise ValueError('Published results or physical field changes')
            elif row['kind'] == 'reset':
                if {key:value for key,value in row.items() if key != 'kind'} != baseline:
                    raise ValueError('Reset does not restore complete race grid')
                continue
            elif row['kind'] == 'route':
                active = race.phase != 0
                for slot, oracle in enumerate(lap_oracles):
                    if active:
                        cadence = 3 if slot == 0 else slot % 5+1
                        if race.elapsed % cadence == 0:
                            strips[slot] = decoded['strips'][strips[slot]][1]
                        oracle.tick([course['first'][strips[slot]]])
                        route_steps += 1
                    if row['cells'][slot] != strips[slot] or row['laps'][slot] != projected(oracle):
                        raise ValueError('Ordered source route/lap data differs')
                cars = [[0, (slot*10 if active else 0)] for slot in range(COUNT)]
                race.tick(row['laps'], cars)
            elif row['kind'] == 'live':
                if baseline is None:
                    baseline = {key:value for key,value in row.items() if key != 'kind'}
                else:
                    active = race.phase in (1, 2)
                    if active:
                        physical += COUNT
                        for slot, car in enumerate(row['cars']):
                            if course:
                                cells = surface.trace(previous['cars'][slot][1:4], car[1:4], lap_oracles[slot].values[5])
                                max_samples = max(max_samples, len(cells))
                                lap_oracles[slot].tick(cells, bool(car[-2]))
                        race.tick(row['laps'], row['cars'])
                    else:
                        if row['cars'] != previous['cars'] or row['laps'] != previous['laps']:
                            raise ValueError('Countdown/results advances the physical field')
                        race.tick(row['laps'], row['cars'])
                for slot, car in enumerate(row['cars']):
                    if car[0] != 200+race.elapsed or car[4] != race.elapsed:
                        raise ValueError('Race and physical/accident clocks differ')
                    if course and row['laps'][slot] != projected(lap_oracles[slot]):
                        raise ValueError('Physical circuit progress differs')
                previous = last_live = row
            else:
                raise ValueError('Unexpected race output')
            race.compare(row)
        if kind == 'route' and (race.phase != 3 or race.end != 1 or not race.drivers[0][5]):
            raise ValueError('Source rules route does not complete the race')
        if kind == 'live' and (physical != 24000 or frozen is None):
            raise ValueError('Physical/countdown/result lifecycle incomplete')
    return dict(pass_=True, rows=rows, physical_vehicle_steps=physical, source_rule_vehicle_steps=route_steps,
                finishers=race.finishers, end=race.end, max_progress_samples=max_samples,
                scope='Ordered source-cell rules, not physical completion' if kind == 'route' else 'Physical short race session and DNF results')


def check_auto(path, decoded, image, code, mode, tables):
    with path.open() as stream:
        meta = json.loads(next(stream))
        course = course_from_source(meta, decoded, image, code)
        initial = json.loads(next(stream))
        if initial['kind'] != 'auto-start' or initial['state'] != [0,0,0,0,0,0,COUNT]:
            raise ValueError('Invalid physical full-race start')
        oracle = LapOracle(course, meta['starts'][0])
        surface = SurfaceOracle(decoded, course)
        position = initial['cars'][0][1:4]
        phase = end = elapsed = coast = ticks = max_samples = 0
        final = None
        for line in stream:
            row = json.loads(line)
            if row['kind'] == 'auto-final':
                final = row
                break
            if row['kind'] != 'auto': raise ValueError('Invalid full-race trace')
            ticks += 1
            if phase == 0:
                if row['position'] != position: raise ValueError('Countdown moves the full-race player')
                if ticks == 400: phase = 1
            else:
                elapsed += 1
                trace = surface.trace(position, row['position'], oracle.values[5])
                max_samples = max(max_samples, len(trace))
                oracle.tick(trace, bool(row['lap'][10]))
                if phase == 1:
                    if oracle.values[11]: end = 1
                    elif oracle.values[10]: end = 2
                    if end: phase = 2
                else:
                    coast += 1
                    if coast == 600: phase = 3
            oracle.compare(row['lap'], 'physical full-race player')
            if row['state'][:5] != [phase,end,ticks,elapsed,coast]:
                raise ValueError('Physical full-race phase/clock differs')
            position = row['position']
        if final is None or phase != 3 or end != 1 or oracle.values[9] != course['laps']+1:
            raise ValueError('Physical race does not complete all required laps')
        if final['kind'] != 'auto-final' or final['state'][:5] != [phase,end,ticks,elapsed,coast]:
            raise ValueError('Final physical race snapshot differs')
        drivers = final['drivers']
        finishers = [slot for slot,d in enumerate(drivers) if d[5]]
        if sorted(drivers[slot][5] for slot in finishers) != list(range(1,len(finishers)+1)):
            raise ValueError('Physical finish places are not unique/consecutive')
        by_place = sorted(finishers, key=lambda slot: drivers[slot][5])
        if [drivers[slot][0] for slot in by_place] != sorted(drivers[slot][0] for slot in finishers):
            raise ValueError('Physical finish order contradicts crossing ticks')
        expected_order = sorted(range(COUNT), key=lambda slot:
            (0,drivers[slot][5],0,slot) if drivers[slot][5] else
            (1,-drivers[slot][2],-drivers[slot][3],slot))
        if final['order'] != expected_order: raise ValueError('Physical final road order differs')
        for slot,d in enumerate(drivers):
            lap, car = final['laps'][slot], final['cars'][slot]
            if d[0] != lap[1] or d[2:4] != lap[2:4] or d[9] != car[-2] or d[6] != car[-1]:
                raise ValueError('Physical result does not use the simulated driver state')
            if d[4] != expected_order.index(slot)+1 or d[7] != tables[mode][d[4]-1]:
                raise ValueError('Physical placement/source bonus differs')
            if d[8] != min(999,d[7]+(d[6] if mode == 0 else 0)):
                raise ValueError('Physical full-race total points differ')
            if car[0] != 200+elapsed or car[4] != elapsed or lap[0] != elapsed:
                raise ValueError('Physical full field clock differs')
        expected_results = sorted(range(COUNT),key=lambda slot:(-drivers[slot][8],drivers[slot][4]))
        if final['results'] != expected_results or final['state'][5] != len(finishers):
            raise ValueError('Physical full-race results/finish count differs')
    return dict(pass_=True, full_physical_race=True, level=code, required_laps=course['laps'],
                completed_laps=oracle.values[9]-1, elapsed_seconds=elapsed*.005, ticks=ticks,
                physical_vehicle_steps=elapsed*COUNT, independent_player_geometry_queries=surface.queries,
                max_progress_samples=max_samples, player_place=drivers[0][4],player_points=drivers[0][8],
                best_lap_seconds=oracle.values[3]*.005, finishers=len(finishers),
                scope='Complete physical twenty-car Stockcar race on original circuit 5; AI supplies player controls')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK/'rewrite-race-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents: parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    archive = (ROOT/'DestructionDerby2/Dirinfo').resolve()
    if digest(archive) != ORIGINAL_SHA256: raise ValueError('Unsupported original archive')
    files = assets(archive.read_bytes())
    image = (ROOT/'DestructionDerby2/dd2_image.bin').read_bytes()
    rows = list(struct.iter_unpack('<2i', image[0x63dcc:0x63e2c]))
    tables = {0:list(struct.unpack_from('<20H', image, 0x6762c)),
              1:list(struct.unpack_from('<20H', image, 0x67604))}
    calls, results = [], []
    def run(command, label, timeout=180):
        path = output/(label+'.log')
        with path.open('wb') as log:
            result = run_bounded(command, directory=output, cwd=ROOT, timeout=timeout,
                                 stdout=log, stderr=subprocess.STDOUT)
        calls.append(dict(label=label, returncode=result.returncode, sha256=digest(path)))
        if result.returncode:
            raise RuntimeError(label+' failed: '+path.read_text(errors='replace')[-3000:])
        if any('Sanitizer:' in line or 'runtime error:' in line for line in path.open(errors='replace')):
            raise ValueError('Sanitizer finding '+label)
        return path
    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT/'src'), '-Wall', '-Wextra', '-Wpedantic',
             '-Wno-unused-parameter', '-Wno-unused-function', '-fno-strict-aliasing', '-ffast-math',
             '-Werror', '-Wshadow', '-Wconversion', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wformat=2', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    units = [ROOT/f'src/assets/{name}.c' for name in ('archive','level','road','barriers')]
    units += [ROOT/f'src/physics/{name}.c' for name in ('road_contact','road_surface','vehicle','barrier_world','car_contact','vehicle_collision','damage')]
    units += [ROOT/f'src/game/{name}.c' for name in ('starting_grid','driving','accidents','course','laps','race')]
    units += [ROOT/f'src/ai/{name}.c' for name in ('path','driver')]
    units += [ROOT/'src/platform/file.c']
    sanitized, synthetic = output/'export-sanitized', output/'rules-sanitized'
    for name, binary in (('race_export', sanitized), ('race_test', synthetic)):
        run([tool('clang'), *flags, *map(str, units), str(ROOT/f'tests/rewrite/{name}.c'), '-lm', '-o', str(binary)], name+'-build').unlink()
    run([str(synthetic)], 'sanitized-rules').unlink()
    commands = {'native':[str(WORK/'rewrite-native/dd2_race_export')],
                'wasm':['node',str(WORK/'rewrite-wasm/dd2_race_export.js')], 'sanitized':[str(sanitized)]}
    for code in CODES:
        decoded = reference(files[f'LEV{code}\\LEVEL.DAT'], code, rows)
        for mode_name, mode in (('wreck',0), ('stock',1)):
            if code not in '1234567' and mode: continue
            for kind in ('live','route') if code in '1234567' else ('live',):
                targets = {}
                for target, command in commands.items():
                    check_space(output)
                    label = f'{code}-{mode_name}-{kind}-{target}'
                    path = run([*command,str(archive),code,mode_name,kind], label)
                    targets[target] = check(path, decoded, image, code, mode, kind, tables)
                    path.unlink()
                item = dict(level=code, mode=mode_name, scenario=kind, targets=targets)
                results.append(item)
                print(json.dumps(item), flush=True)
    decoded = reference(files['LEV5\\LEVEL.DAT'], '5', rows)
    targets = {}
    for target, command in commands.items():
        path = run([*command,str(archive),'5','stock','auto'], '5-stock-auto-'+target, timeout=360)
        targets[target] = check_auto(path, decoded, image, '5', 1, tables)
        path.unlink()
    results.append(dict(level='5', mode='stock', scenario='auto', targets=targets))
    print(json.dumps(results[-1]), flush=True)
    sources = [path for folder in ('src','tests/rewrite') for path in (ROOT/folder).rglob('*') if path.suffix in ('.c','.h')]
    sources += [Path(__file__).resolve(), ROOT/'CMakeLists.txt', ROOT/'tools/rewrite/verify_laps.py']
    report = dict(pass_=True, scope=__doc__.strip(), verified_at=datetime.now(timezone.utc).isoformat(),
                  original_sha256=digest(archive), image_sha256=hashlib.sha256(image).hexdigest(), tables=tables,
                  scenarios=results, calls=calls, source_sha256={str(p.relative_to(ROOT)):digest(p) for p in sources},
                  binary_sha256={str(p):digest(p) for p in (WORK/'rewrite-native/dd2_race_export',WORK/'rewrite-wasm/dd2_race_export.wasm',sanitized,synthetic)})
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    sanitized.unlink(); synthetic.unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, report=str(output/'report.json'), scenarios=len(results))))


if __name__ == '__main__':
    main()
