#!/usr/bin/env python3
"""Verify source barrier inventory, exhaustive swept contacts and body response.

This checks rewrite collision geometry/behavior, not original fixed-point
response, damage, other cars or complete race coverage.
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
from rewrite.verify_levels import assets
from rewrite.verify_roads import reference


def digest(data):
    return hashlib.sha256(data).hexdigest()


def source_barriers(files, image):
    rows = list(struct.iter_unpack('<2i', image[0x63dcc:0x63e2c]))
    right = list(struct.iter_unpack('<2i', image[0x65dd0:0x65e30]))
    left = list(struct.iter_unpack('<2i', image[0x65e30:0x65e90]))
    radii = struct.unpack_from('<12i', image, 0x65da0)
    result = {}
    for number, code in enumerate('123456789AB', 1):
        road = reference(files[f'LEV{code}\\LEVEL.DAT'], code, rows)
        segments = {}
        for offset, strip in road['strips'].items():
            kind, lanes, first = strip[5], strip[6], strip[10]
            row_a = first + rows[kind][0]
            row_b = first + rows[kind][1] + lanes + 1
            # Original FD first/last-lane collision offsets; no runtime decoder.
            endpoints = ((row_a + right[kind][0], row_b + right[kind][1]),
                         (row_b + lanes + left[kind][1], row_a + lanes + left[kind][0]))
            for side, (start, end) in enumerate(endpoints):
                segments[offset, side] = [*road['vertices'][start], *road['vertices'][end]]
        result[code] = dict(radius=radii[number] if number > 7 else 0, segments=segments)
    return result


def component_entry(events, inside):
    # Independently enumerate all boundary crossing times, sort them, then test
    # the geometric region. The C implementation intersects parameter intervals.
    candidates = sorted({0., 1., *(t for t in events if 0 <= t <= 1)})
    for time in candidates:
        if inside(time): return time
    return None


def segment_hit(segment, query):
    ax, ay, az, bx, by, bz = segment
    start, end = query['start'], query['end']
    dx, dy, dz = [b-a for a,b in zip(start,end)]
    radius, half = query['radius'], query['half_height']
    length = math.hypot(bx-ax,bz-az)
    components = []
    def events_for(position, delta, boundaries):
        return [(bound-position)/delta for bound in boundaries] if delta else []
    if length:
        tx, tz = (bx-ax)/length, (bz-az)/length
        along = (start[0]-ax)*tx+(start[2]-az)*tz
        across = (start[0]-ax)*tz-(start[2]-az)*tx
        move_along, move_across = dx*tx+dz*tz, dx*tz-dz*tx
        slope = (by-ay)/length
        vertical = start[1]-ay-along*slope
        move_vertical = dy-move_along*slope
        events = (events_for(along,move_along,(0,length)) +
                  events_for(across,move_across,(-radius,radius)) +
                  events_for(vertical,move_vertical,(-half,400+half)))
        def line_inside(time):
            return (-1e-7 <= along+move_along*time <= length+1e-7 and
                    abs(across+move_across*time) <= radius+1e-7 and
                    -half-1e-7 <= vertical+move_vertical*time <= 400+half+1e-7)
        time = component_entry(events,line_inside)
        if time is not None:
            components.append((time,vertical,move_vertical))
    for cx,cy,cz in ((ax,ay,az),(bx,by,bz)):
        px,pz = start[0]-cx,start[2]-cz
        speed = math.hypot(dx,dz)
        events = events_for(start[1]-cy,dy,(-half,400+half))
        if speed:
            # Ray coordinates, avoiding the quadratic discriminant cancellation
            # of b*b-a*c when the starting point is far from an endpoint.
            along = (px*dx+pz*dz)/speed
            perpendicular = (px*dz-pz*dx)/speed
            if abs(perpendicular) <= radius:
                distance = math.sqrt(max(0,radius*radius-perpendicular*perpendicular))
                events += [(-along-distance)/speed,(-along+distance)/speed]
        def circle_inside(time):
            return (math.hypot(px+dx*time,pz+dz*time) <= radius+1e-7 and
                    -half-1e-7 <= start[1]+dy*time-cy <= 400+half+1e-7)
        time = component_entry(events,circle_inside)
        if time is not None:
            components.append((time,start[1]-cy,dy))
    if not components: return None
    time, vertical, move_vertical = min(components,key=lambda item:item[0])
    position = [a+(b-a)*time for a,b in zip(start,end)]
    fraction = max(0,min(1,((position[0]-ax)*(bx-ax)+(position[2]-az)*(bz-az))/(length*length))) if length else 0
    nearest = [ax+(bx-ax)*fraction,ay+(by-ay)*fraction,az+(bz-az)*fraction]
    distance = math.hypot(position[0]-nearest[0],position[2]-nearest[2])
    if time > 0 and move_vertical and (
        (vertical < -half and abs(vertical+move_vertical*time+half)<1e-7) or
        (vertical > 400+half and abs(vertical+move_vertical*time-(400+half))<1e-7)):
        normal = [0, -1 if move_vertical>0 else 1, 0]
        penetration = 0
    else:
        normal = [(position[0]-nearest[0])/distance,0,(position[2]-nearest[2])/distance] if distance>1e-6 else ([ (bz-az)/length,0,-(bx-ax)/length] if length else [1,0,0])
        penetration = max(0,radius-distance)
    if time == 0 and penetration <= 1e-6 and sum(a*b for a,b in zip((dx,dy,dz),normal)) >= -1e-6:
        return None
    point = [nearest[0],position[1]-normal[1]*half if normal[1] else max(nearest[1],min(nearest[1]+400,position[1])),nearest[2]]
    return dict(time=time,normal=normal,penetration=penetration,point=point)


def reference_query(source, query, bounds):
    if source['radius']:
        start,end=query['start'],query['end'];radius=source['radius']-query['radius']
        first,last=math.hypot(start[0],start[2]),math.hypot(end[0],end[2])
        if first < radius and last < radius: return None
        low,high=0.,1.
        for _ in range(60):
            mid=(low+high)/2
            point=[a+(b-a)*mid for a,b in zip(start,end)]
            if math.hypot(point[0],point[2])>=radius:high=mid
            else:low=mid
        time=high if first<radius else 0
        point=[a+(b-a)*time for a,b in zip(start,end)]
        length=math.hypot(point[0],point[2]);normal=[-point[0]/length,0,-point[2]/length]
        return dict(time=time,normal=normal,penetration=max(0,first-radius),
                    point=[-normal[0]*source['radius'],point[1],-normal[2]*source['radius']],id=[0,0])
    start,end=query['start'],query['end']; radius=query['radius'];half=query['half_height']
    query_low=[min(a,b)-extent for a,b,extent in zip(start,end,(radius,half,radius))]
    query_high=[max(a,b)+extent for a,b,extent in zip(start,end,(radius,half,radius))]
    found=[]
    for key,segment,low,high in bounds:
        if any(a>b+1e-6 or c<d-1e-6 for a,b,c,d in zip(query_low,high,query_high,low)):continue
        candidate=segment_hit(segment,query)
        if candidate is not None:found.append({**candidate,'id':list(key)})
    if not found:return None
    earliest=min(item['time'] for item in found)
    return min((item for item in found if item['time']<=earliest+1e-10),key=lambda item:item['id'])


def compare_export(path, expected):
    actual=json.loads(path.read_text());reports=[]
    if [level['level'] for level in actual]!=list('123456789AB'):raise ValueError('Level inventory differs')
    for level in actual:
        code=level['level'];source=expected[code]
        geometry={tuple(row[:2]):row[2:] for row in level['segments']}
        if geometry!=source['segments'] or level['radius']!=source['radius']:
            raise ValueError('Source barrier geometry differs: '+code)
        bounds=[]
        for key,segment in source['segments'].items():
            first,last=segment[:3],segment[3:]
            low=[min(a,b) for a,b in zip(first,last)];high=[max(a,b) for a,b in zip(first,last)];high[1]+=400
            bounds.append((key,segment,low,high))
        matches=0;maximum_error=0;checks=0
        for query in level['queries']:
            wanted=reference_query(source,query,bounds)
            if bool(query['found'])!=(wanted is not None):raise ValueError('Sweep presence differs: '+str((code,query,wanted)))
            checks+=query['work'][1]
            if wanted is None:continue
            matches+=1
            # IDs are source offsets/sides; original reader walks source records
            # in another order. These sweeps avoid ambiguity at exact shared ends.
            if query['id']!=wanted['id']:raise ValueError('Sweep ID differs: '+str((code,query,wanted)))
            for field in ('time','penetration','normal','point'):
                first=query[field] if isinstance(query[field],list) else [query[field]]
                second=wanted[field] if isinstance(wanted[field],list) else [wanted[field]]
                maximum_error=max(maximum_error,*(abs(a-b) for a,b in zip(first,second)))
                if not all(math.isclose(a,b,abs_tol=1e-5,rel_tol=1e-8) for a,b in zip(first,second)):
                    raise ValueError('Sweep value differs: '+str((code,field,query,wanted)))
        reports.append(dict(level=code,segments=len(geometry),queries=len(level['queries']),hits=matches,
                            segment_checks=checks,max_error=maximum_error))
    return reports


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=WORK/'rewrite-barrier-verification')
    args=parser.parse_args();output=prepare_output(args.output)
    if WORK not in output.parents:parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=True);check_space(output)
    archive=(ROOT/'DestructionDerby2/Dirinfo').resolve();data=archive.read_bytes()
    if digest(data)!=ORIGINAL_SHA256:raise ValueError('Provision supported unmodified Dirinfo')
    image=(ROOT/'DestructionDerby2/dd2_image.bin').read_bytes()
    expected=source_barriers(assets(data),image);calls=[]
    def run(command,label):
        log=output/(label+'.log')
        with log.open('wb') as stream:
            result=run_bounded(command,directory=output,timeout=60,stdout=stream,stderr=subprocess.STDOUT,cwd=ROOT)
        content=log.read_text(errors='replace')
        calls.append(dict(label=label,returncode=result.returncode,log_sha256=digest(log.read_bytes())))
        if result.returncode or 'Sanitizer:' in content or 'runtime error:' in content:raise RuntimeError(label+' failed: '+content[-4000:])
        return log
    flags=['-std=c11','-O1','-g','-I',str(ROOT/'src'),'-Wall','-Wextra','-Wpedantic',
           '-Wno-unused-parameter','-Wno-unused-function','-fno-strict-aliasing','-ffast-math',
           '-Werror','-Wshadow','-Wconversion','-Wstrict-prototypes','-Wmissing-prototypes','-Wformat=2',
           '-fsanitize=address,undefined','-fno-omit-frame-pointer']
    units=[ROOT/f'src/assets/{name}.c' for name in ('archive','level','road','barriers')]
    units += [ROOT/f'src/physics/{name}.c' for name in ('road_contact','road_surface','vehicle','barrier_world','car_contact','contact_group','vehicle_collision','damage')]
    sanitized=output/'export-sanitized';test=output/'test-sanitized'
    for name,binary in (('barrier_export',sanitized),('barrier_test',test)):
        run([tool('clang'),*flags,*map(str,units),str(ROOT/f'tests/{name}.c'),'-lm','-o',str(binary)],name+'-build')
    run([str(test)],'sanitized-response')
    commands={'native':[str(WORK/'rewrite-native/dd2_barrier_export')],
              'wasm':['node',str(WORK/'rewrite-wasm/dd2_barrier_export.js')],'sanitized':[str(sanitized)]}
    reports={};exports={}
    for platform,command in commands.items():
        export=run([*command,str(archive)],platform+'-export');exports[platform]=json.loads(export.read_text())
        reports[platform]=compare_export(export,expected)
        print(json.dumps(dict(platform=platform,pass_=True)),flush=True)
    for platform in ('wasm','sanitized'):
        for native,candidate in zip(exports['native'],exports[platform]):
            for first,second in zip(native['queries'],candidate['queries']):
                if (first['found'],first['id'],first['work'])!=(second['found'],second['id'],second['work']):
                    raise ValueError('Cross-target discrete sweep state differs')
    sources=units+[ROOT/'src/game/driving.c',ROOT/'tests/barrier_export.c',ROOT/'tests/barrier_test.c',Path(__file__).resolve()]
    report=dict(pass_=True,scope=__doc__.strip(),verified_at=datetime.now(timezone.utc).isoformat(),
                original_sha256=ORIGINAL_SHA256,reference_image_sha256=digest(image),comparisons=reports,calls=calls,
                source_sha256={str(p.relative_to(ROOT)):digest(p.read_bytes()) for p in sources},
                binary_sha256={str(p):digest(p.read_bytes()) for p in (sanitized,test,WORK/'rewrite-native/dd2_barrier_export',WORK/'rewrite-wasm/dd2_barrier_export.wasm')})
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    sanitized.unlink();test.unlink()
    for path in output.glob('*.log'):path.unlink()
    check_space(output);print(json.dumps(dict(pass_=True,report=str(output/'report.json'))))


if __name__=='__main__':main()
