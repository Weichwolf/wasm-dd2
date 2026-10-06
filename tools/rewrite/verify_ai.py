#!/usr/bin/env python3
"""Check source-linked AI guidance and sixty-second twenty-car driving.

An independent reader reconstructs lane/triangle centers and forward polylines
from every original racing strip, including alternate branches and lap closure.
Native, Node/WASM and ASan/UBSan exercise traffic, pursuit and timed reversing
on all eleven levels. This is driving AI coverage, not race rules, original
personality/command parity, damage-aware tactics or automatic off-road recovery.
"""
import argparse
from bisect import bisect_left
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256
from rewrite.verify_levels import assets
from rewrite.verify_roads import reference, CORNERS


def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()


def cell_center(decoded, key):
    cell=decoded['cells'][key]
    corners=range(4) if cell[4]==3 else CORNERS[0 if cell[4]==1 else 1]
    points=[decoded['vertices'][cell[5+i]] for i in corners]
    return [sum(p[i] for p in points)/len(points) for i in range(3)]


def center(decoded, strip, lane):
    count=decoded['strips'][strip][6]
    coordinate=min(count-1,max(0,lane*count-0.5))
    first=math.floor(coordinate); second=min(first+1,count-1)
    a,b=cell_center(decoded,(strip,first)),cell_center(decoded,(strip,second))
    return [a[i]+(b[i]-a[i])*(coordinate-first) for i in range(3)]


def oracle(decoded, query):
    strip=query['source']; lane=query['lane']
    links=[strip]
    for _ in range(128): links.append(decoded['strips'][links[-1]][1])
    points=[center(decoded,link,lane) for link in links]
    lengths=[math.hypot(b[0]-a[0],b[2]-a[2]) for a,b in zip(points,points[1:])]
    directions=[[(b[0]-a[0])/length,(b[2]-a[2])/length] if length>1e-8 else [0,0]
                for a,b,length in zip(points,points[1:],lengths)]
    offset=max(0,min(lengths[0],sum((query['position'][i]-points[0][i])*directions[0][j]
                                   for j,i in enumerate((0,2)))))
    cumulative=[]; distance=0
    for length in lengths:
        distance+=length; cumulative.append(distance)
    wanted=offset+query['distance']; segment=bisect_left(cumulative,wanted)
    if segment==len(lengths): return None
    begin=0 if segment==0 else cumulative[segment-1]
    fraction=(wanted-begin)/lengths[segment]
    point=[a+(b-a)*fraction for a,b in zip(points[segment],points[segment+1])]
    curvature=0
    for index in range(1,segment+1):
        a,b=directions[index-1:index+1]
        angle=abs(math.atan2(a[0]*b[1]-a[1]*b[0],a[0]*b[0]+a[1]*b[1]))
        curvature=max(curvature,angle/((lengths[index-1]+lengths[index])/2))
    at=links[segment]; count=decoded['strips'][at][6]
    first,last=decoded['cells'][at,0],decoded['cells'][at,count-1]
    v=decoded['vertices']
    width=math.hypot(*[(v[last[6]][i]+v[last[7]][i]-v[first[5]][i]-v[first[8]][i])/2 for i in (0,2)])
    return dict(hit=at,values=[*point,*directions[segment],width,curvature])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=WORK/'rewrite-ai-verification')
    args=parser.parse_args(); output=prepare_output(args.output)
    if WORK not in output.parents: parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=True);check_space(output)
    archive=(ROOT/'DestructionDerby2/Dirinfo').resolve()
    if digest(archive)!=ORIGINAL_SHA256: raise ValueError('Provision supported unmodified Dirinfo')
    files=assets(archive.read_bytes()); image=(ROOT/'DestructionDerby2/dd2_image.bin').read_bytes()
    rows=list(struct.iter_unpack('<2i',image[0x63dcc:0x63e2c])); calls=[]
    def run(command,label):
        path=output/(label+'.log')
        with path.open('wb') as log:
            # A dense arena completes about 50 simulation seconds in 180 wall
            # seconds under ASan. Keep the full 60-second scenario and allow
            # bounded instrumentation overhead; native/WASM retain their bound.
            timeout=360 if str(sanitized) in command else 180
            result=run_bounded(command,directory=output,timeout=timeout,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        content=path.read_text(errors='replace')
        calls.append(dict(label=label,returncode=result.returncode,log_sha256=digest(path)))
        if result.returncode or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label+' failed: '+content[-3000:])
        return [json.loads(s) for s in content.splitlines() if s.startswith('{')]
    units=[ROOT/f'src/assets/{name}.c' for name in ('archive','level','road','barriers')]
    units += [ROOT/f'src/physics/{name}.c' for name in ('road_contact','road_surface','vehicle','barrier_world','car_contact','vehicle_collision','damage')]
    units += [ROOT/f'src/game/{name}.c' for name in ('starting_grid','driving')]
    units += [ROOT/f'src/ai/{name}.c' for name in ('path','driver')]+[ROOT/'src/platform/file.c']
    flags=['-std=c11','-O1','-g','-I',str(ROOT/'src'),'-I',str(ROOT/'tests/rewrite'),
           '-Wall','-Wextra','-Wpedantic','-Wno-unused-parameter','-Wno-unused-function',
           '-fno-strict-aliasing','-ffast-math','-Werror','-Wshadow','-Wconversion',
           '-Wstrict-prototypes','-Wmissing-prototypes','-Wformat=2',
           '-fsanitize=address,undefined','-fno-omit-frame-pointer']
    sanitized,synthetic=output/'export-sanitized',output/'test-sanitized'
    for name,binary in (('ai_export',sanitized),('ai_test',synthetic)):
        run([tool('clang'),*flags,*map(str,units),str(ROOT/f'tests/rewrite/{name}.c'),'-lm','-o',str(binary)],name+'-build')
    run([str(synthetic)],'synthetic-sanitized')
    commands={'native':[str(WORK/'rewrite-native/dd2_ai_export')],
              'wasm':['node',str(WORK/'rewrite-wasm/dd2_ai_export.js')],'sanitized':[str(sanitized)]}
    levels=[]; path_queries=0
    for code in '123456789AB':
        decoded=reference(files[f'LEV{code}\\LEVEL.DAT'],code,rows); targets={}
        def probe(platform,command):
            paths=run([*command,str(archive),code,'path'],code+'-path-'+platform) if code in '1234567' else []
            states=run([*command,str(archive),code,'drive'],code+'-drive-'+platform)
            return paths,states
        # Each target owns its process and output files; the source archive is
        # immutable. Validate all results after these independent probes finish.
        with ThreadPoolExecutor(max_workers=3) as executor:
            futures={platform:executor.submit(probe,platform,command) for platform,command in commands.items()}
            probes={platform:future.result() for platform,future in futures.items()}
        for platform,command in commands.items():
            maximum=0; queries=0
            samples,states=probes[platform]
            if code in '1234567':
                inventory={(s['source'],s['lane'],s['distance']) for s in samples}
                expected={(key,lane,distance) for key in decoded['strips'] for lane in (.25,.5,.75) for distance in (0,1000,5000)}
                if inventory!=expected or len(samples)!=len(expected): raise ValueError('Incomplete source path coverage')
                for sample in samples:
                    cell=decoded['cells'][sample['source'],0]
                    points=[decoded['vertices'][index] for index in cell[5:]]
                    position=[sum(p[axis] for p in points)/4 for axis in range(3)]
                    if sample['position']!=position: raise ValueError('Source query position differs')
                    wanted=oracle(decoded,sample)
                    if bool(wanted)!=bool(sample['found']) or (wanted and wanted['hit']!=sample['hit']):
                        raise ValueError('Source path selection differs: '+str((code,platform,sample,wanted)))
                    actual=[*sample['point'],*sample['direction'],sample['width'],sample['curvature']]
                    for left,right in zip(wanted['values'],actual):
                        maximum=max(maximum,abs(left-right))
                        if not math.isfinite(right) or not math.isclose(left,right,abs_tol=1e-8,rel_tol=1e-10):
                            raise ValueError('Source guidance differs: '+str((code,platform,sample,wanted)))
                queries=len(samples)
            if len(states)!=300 or states[:20]!=states[280:]: raise ValueError('AI reset or field coverage differs')
            for sample,row in enumerate(states[:260]):
                frame=(sample//20)*200;slot=sample%20
                if row['slot']!=slot or row['frame']!=frame or row['decisions']!=(frame*5 if slot else 0):
                    raise ValueError('AI decision cadence differs')
                if row['cell']>=len(decoded['cells']) or not all(math.isfinite(n) for n in row['position']+row['velocity']):
                    raise ValueError('Invalid AI pose or route state')
            summaries=states[260:280]
            for slot,summary in enumerate(summaries):
                if not summary.get('summary') or summary['slot']!=slot: raise ValueError('Incomplete AI summaries')
                if slot and (summary['travel']<10000 or summary['supported']<2280):
                    raise ValueError('AI failed sustained supported motion: '+str((code,platform,summary)))
            targets[platform]=dict(path_queries=queries,path_error=maximum,summaries=summaries)
        path_queries+=targets['native']['path_queries']
        levels.append(dict(code=code,targets=targets))
        print(json.dumps(dict(code=code,pass_=True,min_travel={p:min(s['travel'] for s in t['summaries'][1:]) for p,t in targets.items()})),flush=True)
    sources=[p for p in (ROOT/'src').rglob('*') if p.suffix in ('.c','.h')]
    sources += [ROOT/'tests/rewrite/ai_export.c',ROOT/'tests/rewrite/ai_test.c',ROOT/'tests/rewrite/surface_fixture.h',ROOT/'CMakeLists.txt',Path(__file__).resolve()]
    binaries=[WORK/'rewrite-native/dd2_ai_export',WORK/'rewrite-wasm/dd2_ai_export.wasm',sanitized,synthetic]
    report=dict(pass_=True,scope=__doc__.strip(),verified_at=datetime.now(timezone.utc).isoformat(),
                original_sha256=ORIGINAL_SHA256,calls=calls,levels=levels,path_queries_per_target=path_queries,
                seconds_per_level=60,vehicle_steps_per_target=2640000,
                source_sha256={str(p.relative_to(ROOT)):digest(p) for p in sources},binary_sha256={str(p):digest(p) for p in binaries})
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    sanitized.unlink();synthetic.unlink()
    for call in calls:(output/(call['label']+'.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True,path_queries_per_target=path_queries,report=str(output/'report.json'))))


if __name__=='__main__':main()
