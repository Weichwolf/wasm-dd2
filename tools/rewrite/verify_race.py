#!/usr/bin/env python3
"""Check race phases/results against independent original data and rule oracles.

Physical twenty-car scenarios cover countdown, six seconds of driving, voluntary
DNF, result freeze and reset on every track/arena. Ordered source-cell scenarios
cover complete circuit races in both scoring modes; they are not physically driven
complete races. A separate full Stockcar race on original circuit 5 drives all
required laps through physics, using AI only to supply player control inputs.
Time Trial short runs cover all seven circuits with one physical vehicle and
restoration of finite twenty-car rules. A long Time Trial on circuit 5 completes
nine laps, beyond the source eight-lap race limit, with independent geometry and
lap-time checks. Total Destruction short sessions cover all four arenas; physical
destructive steering/shunting sessions retain all NPC pursuit targets and run to natural engine
retirement/coasting results. Supported overturns are observed separately from engine retirement.
Native, Node/WASM and instrumented C are verified separately.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
import math
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
        self.count = 1 if mode == 2 else COUNT
        self.phase = self.end = self.steps = self.elapsed = self.coast = 0
        self.finishers, self.alive = 0, self.count
        self.survival = 0
        self.drivers = [[0]*10 for _ in range(self.count)]
        for slot, lap in enumerate(initial_laps):
            self.drivers[slot][2:4] = lap[2:4]
        self.order = self.ranked()
        self.recovery = [[0,0,0] for _ in range(self.count)]
        self.results = [0]*self.count
        self.assign_places()

    def ranked(self):
        def priority(slot):
            d = self.drivers[slot]
            if d[5]:
                return 0, d[5], 0, slot
            if self.length:
                return 1, -d[2], -d[3], slot
            return 1+d[9], -d[1], 0, slot
        return sorted(range(self.count), key=priority)

    def assign_places(self):
        for place, slot in enumerate(self.order, 1):
            self.drivers[slot][4] = place

    def tick(self, laps, cars, recovery=None):
        if self.phase == 3:
            return
        self.steps += 1
        if self.phase == 0:
            if self.steps == 400:
                self.phase = 1
            return
        self.elapsed += 1
        if recovery is not None:
            if len(recovery) != self.count:
                raise ValueError('Incomplete physical recovery field')
            for slot,(current,prior) in enumerate(zip(recovery,self.recovery)):
                if len(current)!=3 or not 0<=current[0]<=400 or current[2]!=int(current[0]!=0):
                    raise ValueError('Invalid temporary overturn state')
                if current[1] not in (prior[1],prior[1]+1):
                    raise ValueError('Recovery counter decreases or skips')
                if current[1]!=prior[1] and (prior[0] not in (399,400) or cars[slot][-2]):
                    raise ValueError('Early recovery or repaired engine-retired wreck')
                if current[0] not in (0,min(400,prior[0]+1)):
                    raise ValueError('Recovery rest deadline loses physical cadence')
            self.recovery = [state[:] for state in recovery]
        if self.mode == 3 and not cars[0][-2]:
            self.survival = min(99*60*200, self.survival+1)
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
        self.alive = sum(not d[9] and (recovery is None or not recovery[slot][2])
                         for slot,d in enumerate(self.drivers))
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
            d[7] = self.tables[self.mode][d[4]-1] if self.length and self.mode != 2 else 0
            d[8] = min(999, d[7] + (d[6] if self.mode == 0 else 0))
        self.results = sorted(range(self.count), key=lambda slot: (-self.drivers[slot][8], self.drivers[slot][4]))
        self.phase = 3

    def withdraw(self):
        if not self.end: self.end = 4
        self.publish()

    def compare(self, row):
        expected = dict(state=[self.phase, self.end, self.steps, self.elapsed, self.coast, self.finishers, self.alive],
                        drivers=self.drivers, order=self.order, results=self.results, survival=self.survival)
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
    original_laps = laps
    if meta['mode'] == 2: laps = 0
    if (meta['length'], meta['laps']) != (length, laps):
        raise ValueError('Original circuit rules differ')
    if {offset:number for offset,number,_ in meta['strips']} != numbers:
        raise ValueError('Original progress equivalents differ')
    course = dict(length=length, laps=laps, original_laps=original_laps, relative={}, keys={}, first={})
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
        prior_times = [[0,0,0,0] for _ in range(race.count)]
        for line in stream:
            row = json.loads(line)
            rows += 1
            if row['kind'] == 'restored':
                if mode != 2 or row['rules'] != [COUNT, course['length'], course['original_laps']]:
                    raise ValueError('Time Trial exit loses original finite rules')
                if row['state'] != [0,0,0,0,0,0,COUNT] or len(row['cars']) != COUNT or len(row['laps']) != COUNT:
                    raise ValueError('Time Trial exit fails to restore the twenty-car grid')
                if any(car[0] != 200 or car[4:] != [0,0,0] for car in row['cars']):
                    raise ValueError('Time Trial exit fails to reset the field')
                continue
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
                        physical += race.count
                        for slot, car in enumerate(row['cars']):
                            if course:
                                cells = surface.trace(previous['cars'][slot][1:4], car[1:4], lap_oracles[slot].values[5])
                                max_samples = max(max_samples, len(cells))
                                lap_oracles[slot].tick(cells, bool(car[-2]))
                        race.tick(row['laps'], row['cars'], row.get('recovery'))
                    else:
                        if row['cars'] != previous['cars'] or row['laps'] != previous['laps']:
                            raise ValueError('Countdown/results advances the physical field')
                        race.tick(row['laps'], row['cars'], row.get('recovery'))
                for slot, car in enumerate(row['cars']):
                    if car[0] != 200+race.elapsed or car[4] != race.elapsed:
                        raise ValueError('Race and physical/accident clocks differ')
                    if course and row['laps'][slot] != projected(lap_oracles[slot]):
                        raise ValueError('Physical circuit progress differs')
                previous = last_live = row
            else:
                raise ValueError('Unexpected race output')
            race.compare(row)
            if mode == 3 and any(driver != [race.elapsed,0] for driver in row['ai'][1:]):
                raise ValueError('Total Destruction opponent abandons the player or loses cadence')
            if len(row['drivers']) != race.count or (kind == 'live' and len(row['cars']) != race.count):
                raise ValueError('Race contains inactive vehicles')
            if course:
                expected_times = []
                for slot, oracle in enumerate(lap_oracles):
                    values = oracle.values
                    current = prior_times[slot][0]
                    if not values[10]:
                        current = 0 if not values[8] else (values[2] if values[11] else values[0]-values[1])
                    expected_times.append([current, values[2], values[3], values[8]])
                if row['times'] != expected_times: raise ValueError('Race lap-time snapshot differs')
                prior_times = expected_times
        if kind == 'route' and (race.phase != 3 or race.end != 1 or not race.drivers[0][5]):
            raise ValueError('Source rules route does not complete the race')
        if kind == 'live' and (physical != 1200*race.count or frozen is None):
            raise ValueError('Physical/countdown/result lifecycle incomplete')
    return dict(pass_=True, rows=rows, physical_vehicle_steps=physical, source_rule_vehicle_steps=route_steps,
                finishers=race.finishers, end=race.end, max_progress_samples=max_samples,
                scope='Ordered source-cell rules, not physical completion' if kind == 'route' else 'Physical short race session and DNF results')


def check_auto(path, decoded, image, code, mode, tables):
    with path.open() as stream:
        meta = json.loads(next(stream))
        course = course_from_source(meta, decoded, image, code)
        initial = json.loads(next(stream))
        count = 1 if mode == 2 else COUNT
        if initial['kind'] != 'auto-start' or initial['state'] != [0,0,0,0,0,0,count]:
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
                    if mode == 2 and oracle.values[9] > course['original_laps']+1: end = 4
                    elif oracle.values[11]: end = 1
                    elif oracle.values[10]: end = 2
                    if end: phase = 3 if mode == 2 and end == 4 else 2
                else:
                    coast += 1
                    if coast == 600: phase = 3
            oracle.compare(row['lap'], 'physical full-race player')
            if row['state'][:5] != [phase,end,ticks,elapsed,coast]:
                raise ValueError('Physical full-race phase/clock differs')
            position = row['position']
        if mode == 2:
            if final is None or phase != 3 or end != 4 or oracle.values[9] != course['original_laps']+2:
                raise ValueError('Time Trial does not continue past the original race limit')
            if final['state'] != [3,4,ticks,elapsed,0,0,1] or len(final['cars']) != 1 or len(final['laps']) != 1:
                raise ValueError('Time Trial field/ending differs')
            if final['drivers'][0][4:6] != [1,0] or final['drivers'][0][7:9] != [0,0]:
                raise ValueError('Time Trial fabricates finish places/points')
            values = oracle.values
            if final['times'] != [[values[0]-values[1], values[2], values[3], values[8]]]:
                raise ValueError('Time Trial result loses lap records')
            if final['cars'][0][0] != 200+elapsed or final['laps'][0] != projected(oracle):
                raise ValueError('Time Trial final physical clocks/laps differ')
            return dict(pass_=True, level=code, physical_time_trial=True, vehicles=1,
                        original_race_laps=course['original_laps'], completed_laps=values[9]-1,
                        elapsed_seconds=elapsed*.005, physical_vehicle_steps=elapsed,
                        independent_player_geometry_queries=surface.queries,
                        best_lap_seconds=values[3]*.005, last_lap_seconds=values[2]*.005,
                        max_progress_samples=max_samples, scope='Actual physics past the finite race lap limit; AI supplies control inputs')
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


def check_survive(path, decoded, image, code, tables):
    with path.open() as stream:
        meta = json.loads(next(stream))
        if meta['mode'] != 3 or course_from_source(meta, decoded, image, code) is not None:
            raise ValueError('Invalid Total Destruction arena rules')
        race = RaceOracle(3, 0, [], tables)
        initial = json.loads(next(stream))
        race.compare(initial)
        previous = initial
        physical = 0
        for line in stream:
            row = json.loads(line)
            if row['kind'] != 'survive' or row['rules'] != [20,0,0] or row['laps']:
                raise ValueError('Total Destruction field/rules changed')
            active = race.phase in (1,2)
            if active: physical += COUNT
            elif row['cars'] != previous['cars']:
                raise ValueError('Arena countdown moves the field')
            race.tick([], row['cars'], row.get('recovery'))
            race.compare(row)
            if len(row['cars']) != COUNT or len(row['ai']) != COUNT or len(row['damage']) != COUNT:
                raise ValueError('Arena field is incomplete')
            for slot, car in enumerate(row['cars']):
                zones = row['damage'][slot]
                if len(zones) != 6 or any(not math.isfinite(z) or not 0 <= z <= 1 for z in zones):
                    raise ValueError('Invalid arena damage zones')
                if any(z < old for z,old in zip(zones, previous['damage'][slot])):
                    raise ValueError('Arena damage decreases without reset')
                if car[-2] != int(zones[0] == 1 or zones[1] == 1):
                    raise ValueError('Arena retirement does not come from front engine damage')
                if car[0] != 200+race.elapsed or car[4] != race.elapsed:
                    raise ValueError('Arena physical/accident clock differs')
                if slot and row['ai'][slot] != [race.elapsed,0]:
                    raise ValueError('Arena opponent abandons the player')
                if any(not math.isfinite(value) for value in car[1:4]):
                    raise ValueError('Nonfinite arena body')
            previous = row
        if race.phase != 3 or race.end not in (2,3) or race.coast != 600:
            raise ValueError('Physical arena fails to reach natural coasting results')
        if not race.survival or any(driver[0] or driver[5] or driver[7] or driver[8] for driver in race.drivers):
            raise ValueError('Total Destruction timing/scoring invalid')
        if race.end == 2 and (not previous['cars'][0][-2] or race.survival != race.drivers[0][1]-1):
            raise ValueError('Physical engine retirement does not stop the survival timer')
    return dict(pass_=True, level=code, physical_vehicle_steps=physical, ticks=race.steps,
        survival_steps=race.survival, survival_seconds=race.survival/200, alive=race.alive,
        end=race.end, all_opponents_pursue_player=True,
        input_profile={'8':'constant gas/right steering','9':'constant gas/right steering',
            'A':'constant gas/six-second alternating steering','B':'twenty-second shunts/one-second alternating steering'}[code]
            + '; after 30,000 fixture ticks including countdown, twenty-second shunts/one-second alternating steering',
        scope='Actual arena physics/contact damage to natural engine retirement or last survivor; overturn recovery pending')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK/'rewrite-race-verification')
    cases = []
    for code in CODES:
        for mode_name, mode in (('wreck',0), ('stock',1), ('trial',2), ('total',3)):
            if (code not in '1234567' and mode in (1,2)) or (code in '1234567' and mode == 3): continue
            for kind in ('live','route') if code in '1234567' and mode != 2 else ('live',):
                cases.append((code,mode_name,mode,kind))
    cases += [(code,'total',3,'survive') for code in '89AB']
    cases += [('5',name,mode,'auto') for name,mode in (('stock',1), ('trial',2))]
    parser.add_argument('--case', action='append', choices=['-'.join((code,name,kind)) for code,name,_,kind in cases],
                        help='Select individual scenarios for diagnosis; omitted means the complete suite')
    args = parser.parse_args()
    selected = [case for case in cases if not args.case or '-'.join((case[0],case[1],case[3])) in args.case]
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
            with path.open('rb') as tail:
                tail.seek(max(0, path.stat().st_size-3000))
                message = tail.read().decode(errors='replace')
            raise RuntimeError(label+' failed: '+message)
        if any('Sanitizer:' in line or 'runtime error:' in line for line in path.open(errors='replace')):
            raise ValueError('Sanitizer finding '+label)
        return path
    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT/'src'), '-Wall', '-Wextra', '-Wpedantic',
             '-Wno-unused-parameter', '-Wno-unused-function', '-fno-strict-aliasing', '-ffast-math',
             '-Werror', '-Wshadow', '-Wconversion', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wformat=2', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    units = [ROOT/f'src/assets/{name}.c' for name in ('archive','level','road','barriers')]
    units += [ROOT/f'src/physics/{name}.c' for name in ('road_contact','road_surface','vehicle','barrier_world','car_contact','contact_group','vehicle_collision','damage')]
    units += [ROOT/f'src/game/{name}.c' for name in ('starting_grid','driving','accidents','course','laps','race','recovery','sound_events')]
    units += [ROOT/f'src/ai/{name}.c' for name in ('path','driver')]
    units += [ROOT/'src/platform/file.c']
    sanitized, synthetic = output/'export-sanitized', output/'rules-sanitized'
    for name, binary in (('race_export', sanitized), ('race_test', synthetic)):
        run([tool('clang'), *flags, *map(str, units), str(ROOT/f'tests/rewrite/{name}.c'), '-lm', '-o', str(binary)], name+'-build').unlink()
    run([str(synthetic)], 'sanitized-rules').unlink()
    commands = {'native':[str(WORK/'rewrite-native/dd2_race_export')],
                'wasm':['node',str(WORK/'rewrite-wasm/dd2_race_export.js')], 'sanitized':[str(sanitized)]}
    sources = [path for folder in ('src','tests/rewrite') for path in (ROOT/folder).rglob('*') if path.suffix in ('.c','.h')]
    sources += [Path(__file__).resolve(), ROOT/'CMakeLists.txt', ROOT/'tools/rewrite/verify_laps.py']
    identity = dict(source_sha256={str(p.relative_to(ROOT)):digest(p) for p in sources},
                    binary_sha256={str(p):digest(p) for p in (WORK/'rewrite-native/dd2_race_export',WORK/'rewrite-wasm/dd2_race_export.wasm',sanitized,synthetic)},
                    original_sha256=digest(archive), image_sha256=hashlib.sha256(image).hexdigest(),
                    selected_scenarios=['-'.join((code,name,kind)) for code,name,_,kind in selected],
                    full_suite=not args.case)
    (output/'identity.json').write_text(json.dumps(identity,indent=2)+'\n')
    def target_checks(executor, code, mode_name, kind, checker, timeout=180):
        def target_check(item):
            target, command = item
            label = f'{code}-{mode_name}-{kind}-{target}'
            receipt = dict(identity='identity.json', level=code, mode=mode_name, scenario=kind, target=target)
            try:
                check_space(output)
                path = run([*command,str(archive),code,mode_name,kind], label, timeout=timeout)
                result = checker(path)
                path.unlink()
            except Exception as error:
                receipt.update(pass_=False, error=str(error))
                (output/(label+'.json')).write_text(json.dumps(receipt,indent=2)+'\n')
                raise
            receipt.update(pass_=True, result=result)
            (output/(label+'.json')).write_text(json.dumps(receipt,indent=2)+'\n')
            return target, result
        # Each target owns its process, oracle and file. Three maximum-length
        # arena traces fit the output budget; run_bounded monitors it throughout.
        return dict(executor.map(target_check, commands.items()))

    with ThreadPoolExecutor(max_workers=len(commands)) as executor:
        for code,mode_name,mode,kind in selected:
            decoded = reference(files[f'LEV{code}\\LEVEL.DAT'], code, rows)
            if kind == 'survive':
                checker = lambda path: check_survive(path, decoded, image, code, tables)
                timeout = 1800
            elif kind == 'auto':
                checker = lambda path: check_auto(path, decoded, image, code, mode, tables)
                timeout = 600
            else:
                checker = lambda path: check(path, decoded, image, code, mode, kind, tables)
                timeout = 180
            targets = target_checks(executor, code, mode_name, kind, checker, timeout=timeout)
            results.append(dict(level=code, mode=mode_name, scenario=kind, targets=targets))
            print(json.dumps(results[-1]), flush=True)
    sources = [path for folder in ('src','tests/rewrite') for path in (ROOT/folder).rglob('*') if path.suffix in ('.c','.h')]
    sources += [Path(__file__).resolve(), ROOT/'CMakeLists.txt', ROOT/'tools/rewrite/verify_laps.py']
    if identity['source_sha256'] != {str(p.relative_to(ROOT)):digest(p) for p in sources}:
        raise ValueError('Race verification sources changed during the run')
    report = dict(pass_=True, full_suite=not args.case, scope='Selected original-data race scenarios; not full-suite acceptance' if args.case else __doc__.strip(), verified_at=datetime.now(timezone.utc).isoformat(),
                  original_sha256=digest(archive), image_sha256=hashlib.sha256(image).hexdigest(), tables=tables,
                  scenarios=results, calls=calls, source_sha256={str(p.relative_to(ROOT)):digest(p) for p in sources},
                  binary_sha256={str(p):digest(p) for p in (WORK/'rewrite-native/dd2_race_export',WORK/'rewrite-wasm/dd2_race_export.wasm',sanitized,synthetic)})
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    sanitized.unlink(); synthetic.unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, full_suite=not args.case, report=str(output/'report.json'), scenarios=len(results))))


if __name__ == '__main__':
    main()
