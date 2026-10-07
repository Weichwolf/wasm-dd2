#!/usr/bin/env python3
"""Verify original 20-slot grids and coupled vehicle motion on three targets.

Read source road graphs, lane geometry and the special SCA grid independently.
Arena grids retain the original ring/half-ring layout using continuous trig;
original integer sine/height rounding is not claimed. Three-second reverse or
turning drives compare all bodies and exercise actual car-to-car response.
Every fixed step checks bounded per-contact reports, source obstacle IDs, partner
identity, chronology and agreement with each body's aggregate impact. Driving
decisions are disabled here; damage and race rules remain pending.
"""
import argparse
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
from rewrite.verify_barriers import source_barriers
from rewrite.verify_levels import assets
from rewrite.verify_roads import reference, CORNERS

CODES = '123456789AB'
STATE_TOLERANCES = [0.1]*3 + [0.2]*3 + [1e-4]*4 + [1e-3]*3 + [1e-4]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def surface_height(decoded, xpos, zpos):
    heights = []
    for cell in decoded['cells'].values():
        for triangle, corners in enumerate(CORNERS):
            if not cell[4] & (1 << triangle): continue
            points = [decoded['vertices'][cell[5+c]] for c in corners]
            if not min(p[0] for p in points)-1e-6 <= xpos <= max(p[0] for p in points)+1e-6: continue
            if not min(p[2] for p in points)-1e-6 <= zpos <= max(p[2] for p in points)+1e-6: continue
            signs = [(b[0]-a[0])*(zpos-a[2])-(b[2]-a[2])*(xpos-a[0])
                     for a,b in zip(points,points[1:]+points[:1])]
            tolerance = 1e-6*max(math.hypot(b[0]-a[0],b[2]-a[2])
                                for a,b in zip(points,points[1:]+points[:1]))
            if min(signs) < -tolerance and max(signs) > tolerance: continue
            a,b,c = points
            edge = [b[i]-a[i] for i in range(3)]
            other = [c[i]-a[i] for i in range(3)]
            normal = [edge[1]*other[2]-edge[2]*other[1],
                      edge[2]*other[0]-edge[0]*other[2],
                      edge[0]*other[1]-edge[1]*other[0]]
            if normal[1] == 0: continue
            heights.append(a[1]-(normal[0]*(xpos-a[0])+normal[2]*(zpos-a[2]))/normal[1])
    if not heights: raise ValueError('Source grid slot is outside the arena')
    return max(heights)


def original_grid(decoded, code, sca):
    if code in '1234567':
        strips = decoded['strips']
        numbered = {s[4]:offset for offset,s in strips.items() if s[4] != 0xffffffff}
        counter, current = len(numbered), 0
        for _ in range(len(strips)):
            if strips[current][5] == 8:
                current = strips[current][3]
                for _ in range(len(strips)):
                    if strips[current][5] == 9: break
                    numbered[counter] = current; counter += 1
                    current = strips[current][1]
            current = strips[current][1]
            if current == 0: break
        index = int(code)-1
        numbers = (266,656,535,614,22,242)
        lanes = (2,1,3,2,2,2)
        directions = (1,1,-1,-1,1,1)
        current = None if code == '7' else numbered[numbers[index]]
        starts = []
        for slot in range(20):
            if code == '7': current,lane = numbered[sca[slot][0]],sca[slot][1]
            else: lane = lanes[index] + directions[index]*(slot % 2)
            cell = decoded['cells'][current,lane]
            points = [decoded['vertices'][v] for v in cell[5:]]
            position = [sum(p[i] for p in points)/4 for i in range(3)]
            position[1] += 190
            starts.append([*position, ((192-strips[current][8])%256)*math.tau/256])
            current = strips[current][2]
        return starts
    starts = []
    for slot in range(20):
        angle = slot*math.pi/20-math.pi/2 if code == 'B' else slot*math.tau/20
        xpos,zpos = math.sin(angle)*12000,-math.cos(angle)*12000
        if slot == 0: xpos,zpos = (-12000,0) if code == 'B' else (0,-12000)
        starts.append([xpos,surface_height(decoded,xpos,zpos)+190,zpos,-angle])
    return starts


def contact_reports(samples, decoded, barriers):
    """Independently reduce selected contacts to the legacy per-body summary."""
    kinds = [0, 0, 0]; positive = [0, 0, 0]; repairs = 0; previous = 0
    for row in samples:
        if not previous < row['step'] <= 600 or not 0 < len(row['events']) <= 64:
            raise ValueError('Contact step/order/budget differs')
        previous = row['step']; expected = [[0,0,0,0,0,0,0] for _ in range(20)]; time = 0; pairs = 0
        for event in row['events']:
            if len(event)!=19 or not all(math.isfinite(value) for value in event):
                raise ValueError('Incomplete or nonfinite contact')
            kind,first,second,obstacle = event[:4]
            bodies_in_probe=1 if row['world'] else 20
            if kind not in (0,1,2) or first not in range(bodies_in_probe) or not time <= event[4] <= 1:
                raise ValueError('Contact identity/chronology differs')
            if row['world'] and kind!=row['step']-1:
                raise ValueError('Original world probe returned the wrong contact kind')
            time = event[4]; speed,impulse = event[5:7]; normal = event[10:13]
            if speed < 0 or impulse < 0 or bool(speed)!=bool(impulse) or not math.isclose(sum(n*n for n in normal),1,abs_tol=1e-8):
                raise ValueError('Invalid collision response/normal')
            if kind==2:
                if not first < second < 20 or obstacle!=0xffffffff:
                    raise ValueError('Car partner differs')
                bodies=(first,second); pairs+=1
            else:
                limit=len(decoded['cells']) if kind==0 else max(1,len(barriers['segments']))
                if second!=20 or not 0<=obstacle<limit or event[16:]!=[0,0,0]:
                    raise ValueError('Source obstacle/unused partner differs')
                bodies=(first,)
            for body in bodies:
                target=expected[body];target[0]+=1;target[1]+=int(kind==2)
                if speed>target[2]:target[2:]=event[5:10]
            kinds[kind]+=1;positive[kind]+=int(impulse>0);repairs+=int(impulse==0)
        if row['pairs']!=pairs or row['impacts']!=expected:
            raise ValueError('Contact stream does not reproduce aggregate impacts')
    return dict(reported_steps=len(samples),events=sum(kinds),kinds=kinds,positive=positive,zero_impulse=repairs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK/'rewrite-fleet-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents: parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=True); check_space(output)
    archive = (ROOT/'DestructionDerby2/Dirinfo').resolve()
    if digest(archive) != ORIGINAL_SHA256: raise ValueError('Provision supported unmodified original Dirinfo')
    files = assets(archive.read_bytes())
    image = (ROOT/'DestructionDerby2/dd2_image.bin').read_bytes()
    row_table = list(struct.iter_unpack('<2i',image[0x63dcc:0x63e2c]))
    sca = [struct.unpack_from('<HBx',image,0x66da0+i*4) for i in range(20)]
    barrier_inventory=source_barriers(files,image)
    calls = []
    def run(command,label):
        path = output/(label+'.log')
        with path.open('wb') as log:
            result = run_bounded(command,directory=output,timeout=120,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        content = path.read_text(errors='replace')
        calls.append(dict(label=label,returncode=result.returncode,log_sha256=digest(path)))
        if result.returncode or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label+' failed: '+content[-6000:])
        return [json.loads(s) for s in content.splitlines() if s.startswith('{')]
    flags = ['-std=c11','-O1','-g','-I',str(ROOT/'src'),'-Wall','-Wextra','-Wpedantic',
             '-Wno-unused-parameter','-Wno-unused-function','-fno-strict-aliasing','-ffast-math',
             '-Werror','-Wshadow','-Wconversion','-Wstrict-prototypes','-Wmissing-prototypes','-Wformat=2',
             '-fsanitize=address,undefined','-fno-omit-frame-pointer']
    units = [ROOT/f'src/assets/{name}.c' for name in ('archive','level','road','barriers')]
    units += [ROOT/f'src/physics/{name}.c' for name in
              ('road_contact','road_surface','vehicle','barrier_world','car_contact','contact_group','vehicle_collision','damage')]
    units += [ROOT/f'src/game/{name}.c' for name in ('starting_grid','driving','accidents','course','laps','race','recovery','sound_events')]
    units += [ROOT/f'src/ai/{name}.c' for name in ('path','driver')]
    units += [ROOT/'src/platform/file.c']
    sanitized, synthetic, ground, barrier = (output/name for name in ('export-sanitized','test-sanitized','ground-sanitized','barrier-sanitized'))
    for name,binary in (('fleet_export',sanitized),('car_pair_test',synthetic),('ground_test',ground),('barrier_test',barrier)):
        run([tool('clang'),*flags,*map(str,units),str(ROOT/f'tests/{name}.c'),'-lm','-o',str(binary)],name+'-build')
    run([str(synthetic)],'synthetic-sanitized')
    run([str(ground)],'ground-sanitized')
    run([str(barrier)],'barrier-sanitized')
    commands = {'native':[str(WORK/'rewrite-native/dd2_fleet_export')],
                'wasm':['node',str(WORK/'rewrite-wasm/dd2_fleet_export.js')], 'sanitized':[str(sanitized)]}
    levels = []
    for code in CODES:
        decoded=reference(files[f'LEV{code}\\LEVEL.DAT'],code,row_table)
        expected = original_grid(decoded,code,sca)
        targets = {}
        errors = {}
        contact_metrics={}
        world_metrics={}
        for platform,command in commands.items():
            samples = run([*command,str(archive),code],code+'-'+platform)
            contacts=[row for row in samples if row.get('contact_report') and not row['world']]
            contact_metrics[platform]=contact_reports(contacts,decoded,barrier_inventory[code])
            worlds=[row for row in samples if row.get('contact_report') and row['world']]
            if [row['step'] for row in worlds]!=[1,2]:raise ValueError('Incomplete original landing/wall probes')
            world_metrics[platform]=contact_reports(worlds,decoded,barrier_inventory[code])
            if any(world_metrics[platform]['positive'][kind]==0 for kind in (0,1)):
                raise ValueError('Missing impulse-bearing original world contact')
            samples=[row for row in samples if not row.get('contact_report')]
            if len(samples) != 161 or not samples[140].get('summary') or samples[140]['vehicles'] != 20:
                raise ValueError('Incomplete starter-field coverage: '+code)
            initial, reset = samples[:20],samples[141:]
            if initial != reset: raise ValueError('Reset did not restore every body: '+code)
            for slot,row in enumerate(initial):
                if row['slot'] != slot or row['frame'] != 0 or row['steps'] != 200 or row['grounded'] != 4:
                    raise ValueError('Grid state ordering differs')
                for first,second in zip(expected[slot],row['spawn']):
                    if not math.isclose(first,second,abs_tol=1e-8,rel_tol=1e-12):
                        raise ValueError('Original grid layout differs: '+str((code,slot,expected[slot],row)))
            targets[platform] = samples
            if platform == 'native': baseline = samples
            if samples[140] != baseline[140]: raise ValueError('Fleet contact summary differs: '+code)
            maximum = [0]*14
            for sample,(first,second) in enumerate(zip(baseline[:140],samples[:140])):
                frame,slot = (sample // 20)*20, sample % 20
                if second['frame'] != frame or second['slot'] != slot or second['steps'] != 200+frame*5:
                    raise ValueError('Fleet sample inventory differs')
                if first['slot'] != second['slot'] or first['frame'] != second['frame'] or first['steps'] != second['steps']:
                    raise ValueError('Fleet state ordering differs')
                if first['grounded'] != second['grounded']: raise ValueError('Fleet support differs: '+code)
                if len(first['state']) != 14 or len(second['state']) != 14: raise ValueError('Incomplete body state')
                for index,(left,right) in enumerate(zip(first['state'],second['state'])):
                    if not math.isfinite(left) or not math.isfinite(right): raise ValueError('Nonfinite body state')
                    maximum[index] = max(maximum[index],abs(left-right))
                    if not math.isclose(left,right,abs_tol=STATE_TOLERANCES[index],rel_tol=1e-8):
                        raise ValueError('Fleet state differs: '+str((code,platform,first['slot'],first['frame'],index,left,right)))
            errors[platform] = maximum
        summary = baseline[140]
        for platform,metrics in contact_metrics.items():
            if code in '1234567' and metrics['positive'][2]==0:
                raise ValueError('Missing impulse-bearing car contact: '+str((code,platform)))
        if code in '1234567' and summary['pairs'] == 0: raise ValueError('Reverse drive never touched another car: '+code)
        # On level 1, the player reverses into the same-lane third grid car.
        if code == '1':
            first,second = baseline[2],baseline[122]
            if math.dist(first['state'][:3],second['state'][:3]) < 100:
                raise ValueError('Other car did not move after the player impact')
        levels.append(dict(code=code,grid=expected,summary=summary,maximum_component_errors=errors,
                           summaries={p:s[140] for p,s in targets.items()},contact_reports=contact_metrics,
                           world_reports=world_metrics))
        print(json.dumps(dict(code=code,pass_=True,summary=summary,maximum_error=max(max(e) for e in errors.values()))),flush=True)
    sources = [p for p in (ROOT/'src').rglob('*') if p.suffix in ('.c','.h')]
    sources += [ROOT/'tests/fleet_export.c',ROOT/'tests/car_pair_test.c',ROOT/'tests/ground_test.c',ROOT/'tests/barrier_test.c',ROOT/'tests/surface_fixture.h',ROOT/'tests/asset_fixture.h',ROOT/'CMakeLists.txt',Path(__file__).resolve()]
    binaries = [WORK/'rewrite-native/dd2_fleet_export',WORK/'rewrite-wasm/dd2_fleet_export.wasm',sanitized,synthetic,ground,barrier]
    report = dict(pass_=True,scope=__doc__.strip(),verified_at=datetime.now(timezone.utc).isoformat(),
                  original_sha256=ORIGINAL_SHA256,levels=levels,calls=calls,
                  state_tolerances=STATE_TOLERANCES,relative_tolerance=1e-8,
                  source_sha256={str(p.relative_to(ROOT)):digest(p) for p in sources},
                  binary_sha256={str(p):digest(p) for p in binaries})
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    sanitized.unlink();synthetic.unlink();ground.unlink();barrier.unlink()
    for call in calls: (output/(call['label']+'.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True,grid_slots=220,steps_per_target=132000,report=str(output/'report.json'))))


if __name__ == '__main__': main()
