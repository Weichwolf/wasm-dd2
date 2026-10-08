#!/usr/bin/env python3
"""Check physical roof-down recovery at every original grid position.

Drop one partially damaged player car at each of 220 source positions, let world
physics settle it, then verify the two-second righting deadline and real tire
support against independently decoded original triangles. Exercise acceleration
after righting. Native, Node/WASM and ASan/UBSan are separate executions. These
controlled single-car scenarios do not establish natural arena completion or
original handling parity. Synthetic tests additionally cover NPC distance,
retired engines, side/nose poses, bridge levels and transactional rejection.
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
from rewrite.verify_fleet import original_grid
from rewrite.verify_ground import original_planes, BUCKET
from rewrite.verify_levels import assets
from rewrite.verify_roads import reference

CODES = '123456789AB'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def cross(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]


def rotate(rotation, vector):
    arm = cross(rotation[:3], vector)
    second = cross(rotation[:3], arm)
    return [v + 2*(rotation[3]*a+b) for v, a, b in zip(vector, arm, second)]


def contacts(buckets, point, low, high):
    xpos, _, zpos = point
    found = []
    for plane in buckets[math.floor(xpos/BUCKET), math.floor(zpos/BUCKET)]:
        vertices = plane['points']
        edges = list(zip(vertices, vertices[1:]+vertices[:1]))
        signs = [(b[0]-a[0])*(zpos-a[2])-(b[2]-a[2])*(xpos-a[0]) for a, b in edges]
        tolerance = 1e-6*max(math.hypot(b[0]-a[0], b[2]-a[2]) for a, b in edges)
        if min(signs) < -tolerance and max(signs) > tolerance:
            continue
        base, normal = vertices[0], plane['normal']
        height = base[1]-(normal[0]*(xpos-base[0])+normal[2]*(zpos-base[2]))/normal[1]
        if low <= height <= high:
            found.append((height, plane))
    return found


def check(rows, expected, buckets, corners):
    if len(rows) != 20 or [row['slot'] for row in rows] != list(range(20)):
        raise ValueError('Incomplete original grid recovery inventory')
    for row, spawn in zip(rows, expected):
        if any(not math.isclose(a,b,abs_tol=1e-8,rel_tol=1e-12) for a,b in zip(row['spawn'],spawn)):
            raise ValueError('Recovery uses a different source grid')
        tick, before, landed, driven = (row[key] for key in ('tick','before','landed','driven'))
        if not 400 <= tick <= 2000 or row['prior_rest'] != 399 or row['state'] != [0,1,0]:
            raise ValueError('Recovery deadline/state differs')
        if before['steps'] != tick or landed['steps'] != tick or driven['steps'] != tick+100:
            raise ValueError('Righting advances the physical clock')
        for pose in (before,landed,driven):
            if any(not math.isfinite(v) for key in ('position','velocity','rotation','angular') for v in pose[key]):
                raise ValueError('Recovery publishes a nonfinite pose')
            if not math.isclose(dot(pose['rotation'],pose['rotation']),1,abs_tol=1e-8):
                raise ValueError('Recovery publishes a nonunit rotation')
        if rotate(before['rotation'],[0,1,0])[1] >= .2 or dot(before['angular'],before['angular']) > .25:
            raise ValueError('Car did not physically settle upside down')
        support = None
        upward_before = rotate(before['rotation'], [0,1,0])
        resting = []
        for corner in corners:
            point = [a+b for a,b in zip(before['position'],rotate(before['rotation'],corner))]
            candidates = contacts(buckets,point,point[1]-2,point[1]+2)
            if candidates:
                highest = max(height for height,_ in candidates)
                # Inclusive shared-edge ties use original cell/triangle order.
                height,plane = min((p for p in candidates if p[0] >= highest-1e-6),
                                   key=lambda p:(p[1]['index'],p[1]['triangle']))
                if plane['unit'][1] >= .2 and abs(dot(before['velocity'],plane['unit'])) <= 50:
                    resting.append((-dot(upward_before,plane['unit']),height,plane,point))
        if resting:
            # Roof-facing support from independently decoded source triangles;
            # equal alignments preserve the original box-corner order.
            _,height,plane,point = max(resting,key=lambda item:item[0])
            rest_normal = plane['unit']
            expected_height = height-(rest_normal[0]*(before['position'][0]-point[0])+
                                      rest_normal[2]*(before['position'][2]-point[2]))/rest_normal[1]
            center = contacts(buckets,before['position'],expected_height-190,expected_height+190)
            if center:
                highest_center = max(value for value,_ in center)
                _,landing_plane = min((p for p in center if p[0]>=highest_center-1e-6),
                    key=lambda p:(p[1]['index']!=plane['index'],p[1]['index'],p[1]['triangle']))
                support = landing_plane['unit']
        if support is None:
            raise ValueError('Recovery lacks real original roof/side support')
        if landed['angular'] != [0,0,0] or abs(dot(landed['velocity'],support)) > 1e-8:
            raise ValueError('Righting retains spin or vertical collision velocity')
        projection = [v-n*dot(before['velocity'],support) for v,n in zip(before['velocity'],support)]
        if any(not math.isclose(a,b,abs_tol=1e-8) for a,b in zip(landed['velocity'],projection)):
            raise ValueError('Righting changes road-tangent velocity')
        upward = rotate(landed['rotation'],[0,1,0])
        if dot(upward,support) < 1-1e-8:
            raise ValueError('Car was not aligned to the supporting source plane')
        heading = rotate(landed['rotation'],[0,0,1])
        prior = rotate(before['rotation'],[0,0,1])
        yaw = math.atan2(prior[0],prior[2])
        tilt_length = math.sqrt(2*(1+support[1]))
        tilt = [support[2]/tilt_length,0,-support[0]/tilt_length,(1+support[1])/tilt_length]
        expected_heading = rotate(tilt,[math.sin(yaw),0,math.cos(yaw)])
        if any(abs(a-b)>1e-8 for a,b in zip(heading,expected_heading)):
            raise ValueError('Righting loses the incoming horizontal heading')
        grounded = 0
        for wheel in row['wheels']:
            if not wheel['grounded']:
                continue
            grounded += 1
            query = [mount-70*axis for mount,axis in zip(wheel['mount'],upward)]
            source = [item for item in contacts(buckets,query,-math.inf,math.inf)
                      if list(item[1]['key']) == wheel['key'] and item[1]['triangle'] == wheel['triangle']]
            if not source:
                raise ValueError('Recovered wheel is outside the source contact triangle')
            height,plane = source[0]
            if abs(height-wheel['height']) > 1e-8 or any(abs(a-b)>1e-8 for a,b in zip(plane['unit'],wheel['normal'])):
                raise ValueError('Recovered wheel source height/normal differs')
            if abs(wheel['center'][1]-height-60)>1e-7:
                raise ValueError('Recovered wheel fails to touch the road')
        if grounded == 0:
            raise ValueError('Recovery publishes a car without tire support')
        displacement = [b-a for a,b in zip(landed['position'],driven['position'])]
        if dot(driven['velocity'],heading) <= dot(landed['velocity'],heading)+10 or dot(displacement,heading)<=1:
            raise ValueError('Recovered player cannot accelerate/drive')
    return dict(cases=len(rows),physical_steps=sum(row['tick']+100 for row in rows),
                first_recovery_tick=min(row['tick'] for row in rows),
                last_recovery_tick=max(row['tick'] for row in rows),
                interrupted_rest_intervals=sum(row['restarts'] for row in rows))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=WORK/'rewrite-recovery-verification')
    parser.add_argument('--build-root',type=Path,default=WORK)
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=True)
    check_space(output)
    archive = (ROOT/'DestructionDerby2/Dirinfo').resolve()
    if digest(archive) != ORIGINAL_SHA256:
        raise ValueError('Provision unmodified supported original Dirinfo')
    image_path = (ROOT/'DestructionDerby2/dd2_image.bin').resolve()
    image = image_path.read_bytes()
    files = assets(archive.read_bytes())
    row_table = list(struct.iter_unpack('<2i',image[0x63dcc:0x63e2c]))
    sca = [struct.unpack_from('<HBx',image,0x66da0+i*4) for i in range(20)]
    corners = [list(struct.unpack_from('<3h',image,0x66a98+i*8)) for i in range(8)]
    units = [ROOT/f'src/assets/{name}.c' for name in ('archive','level','road','barriers')]
    units += [ROOT/f'src/physics/{name}.c' for name in
              ('road_contact','road_surface','vehicle','barrier_world','car_contact','contact_group','vehicle_collision')]
    units += [ROOT/f'src/game/{name}.c' for name in ('starting_grid','recovery')]
    units += [ROOT/'src/platform/file.c']
    sources = [*units,ROOT/'tests/recovery_export.c',ROOT/'tests/recovery_test.c',
               ROOT/'src/game/recovery.h',ROOT/'src/physics/vehicle.h',Path(__file__)]
    hashes = {str(path.relative_to(ROOT)):digest(path) for path in sources}
    calls = []
    def run(command,label,timeout=120):
        check_space(output)
        path = output/(label+'.log')
        with path.open('wb') as log:
            result = run_bounded(command,directory=output,timeout=timeout,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        content = path.read_text(errors='replace')
        calls.append(dict(label=label,returncode=result.returncode,log_sha256=digest(path)))
        if result.returncode or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label+' failed: '+content[-3000:])
        return [json.loads(line) for line in content.splitlines() if line.startswith('{')]
    flags = ['-std=c11','-O1','-g','-I',str(ROOT/'src'),'-I',str(ROOT/'tests'),
             '-Wall','-Wextra','-Wpedantic','-Wno-unused-parameter','-Wno-unused-function',
             '-fno-strict-aliasing','-ffast-math','-Werror','-Wshadow','-Wconversion',
             '-Wstrict-prototypes','-Wmissing-prototypes','-Wformat=2',
             '-fsanitize=address,undefined','-fno-omit-frame-pointer']
    sanitized,synthetic = output/'export-sanitized',output/'test-sanitized'
    for fixture,binary in (('recovery_export',sanitized),('recovery_test',synthetic)):
        run([tool('clang'),*flags,*map(str,units),str(ROOT/f'tests/{fixture}.c'),'-lm','-o',str(binary)],fixture+'-build')
    commands = {'native':[str(args.build_root/'rewrite-native/dd2_recovery_export')],
                'wasm':['node',str(args.build_root/'rewrite-wasm/dd2_recovery_export.js')],
                'sanitized':[str(sanitized)]}
    binaries = {target:[dict(path=part,sha256=digest(Path(part))) for part in command if Path(part).is_file()]
                for target,command in commands.items()}
    wasm = args.build_root/'rewrite-wasm/dd2_recovery_export.wasm'
    binaries['wasm'].append(dict(path=str(wasm),sha256=digest(wasm)))
    run([str(synthetic)],'synthetic-sanitized')
    levels = []
    for code in CODES:
        decoded = reference(files[f'LEV{code}\\LEVEL.DAT'],code,row_table)
        expected = original_grid(decoded,code,sca)
        _,buckets = original_planes(decoded)
        targets = {}
        for target,command in commands.items():
            rows = run([*command,str(archive),code],code+'-'+target)
            targets[target] = check(rows,expected,buckets,corners)
        levels.append(dict(level=code,targets=targets))
        print('recovery '+code+': PASS (20 physical drops on each target)',flush=True)
    if any(digest(ROOT/path) != sha for path,sha in hashes.items()):
        raise ValueError('Recovery source changed during verification')
    report = dict(pass_=True,utc=datetime.now(timezone.utc).isoformat(),
                  scope=__doc__,archive_sha256=digest(archive),image_sha256=digest(image_path),
                  source_sha256=hashes,binaries=binaries,levels=levels,calls=calls,
                  cases_per_target=220,targets=list(commands),original_rest_seconds=2,
                  rewrite_rest_steps=400,npc_distance=8192,original_parity=False)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    for path in [*output.glob('*.log'),sanitized,synthetic]:
        path.unlink()
    print('Recovery verification: PASS; report:',output/'report.json')


if __name__ == '__main__':
    main()
