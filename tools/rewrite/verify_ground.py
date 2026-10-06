#!/usr/bin/env python3
"""Verify body ground contacts on original roads across native/WASM/ASan.

Independent original-data planes check short and fast vertical sweeps at both
triangle centroids of every cell, including missing triangles and stacked roads.
Four original-grid drops per level exercise upright, inverted, sliding and
spinning bodies for three seconds. This is not original handling/race parity.
"""
import argparse
from collections import defaultdict
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
from rewrite.verify_roads import reference, CORNERS

CODES = '123456789AB'
BUCKET = 4096
# Iterative friction at a spinning body's near-simultaneous contacts amplifies
# fast-math rounding. These bounds remain below the source coordinate resolution;
# analytic sweep geometry/selection uses the separate strict plane checks.
STATE_TOLERANCES = [0.1]*3 + [0.2]*3 + [1e-4]*4 + [1e-3]*3


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def original_planes(decoded):
    # Independently assign the owner cell order from the source graph, so earliest
    # time ties can be checked without trusting the exporter's selected IDs.
    if decoded['strips']:
        order = [0]
        seen = {0}
        for offset in order:
            strip = decoded['strips'][offset]
            links = strip[1:3] + ([strip[3]] if strip[5] in (8, 9) else [])
            for link in links:
                if link not in seen:
                    seen.add(link); order.append(link)
        keys = [(offset, lane) for offset in order
                for lane in range(decoded['strips'][offset][6])]
    else:
        keys = list(decoded['cells'])
    triangles = []
    buckets = defaultdict(list)
    for index, key in enumerate(keys):
        cell = decoded['cells'][key]
        for triangle, corners in enumerate(CORNERS):
            if not cell[4] & (1 << triangle): continue
            points = [decoded['vertices'][cell[5 + corner]] for corner in corners]
            a, b, c = points
            edge = [b[i] - a[i] for i in range(3)]
            other = [c[i] - a[i] for i in range(3)]
            normal = [edge[1]*other[2]-edge[2]*other[1],
                      edge[2]*other[0]-edge[0]*other[2],
                      edge[0]*other[1]-edge[1]*other[0]]
            if normal[1] == 0: continue
            length = math.sqrt(sum(n*n for n in normal))
            unit = [n * (1 if normal[1] > 0 else -1) / length for n in normal]
            item = dict(key=key, index=index, triangle=triangle, points=points, normal=normal, unit=unit)
            triangles.append(item)
            for xpos in range(math.floor(min(p[0] for p in points)/BUCKET),
                              math.floor(max(p[0] for p in points)/BUCKET)+1):
                for zpos in range(math.floor(min(p[2] for p in points)/BUCKET),
                                  math.floor(max(p[2] for p in points)/BUCKET)+1):
                    buckets[xpos, zpos].append(item)
    return triangles, buckets


def oracle(decoded, buckets, row):
    cell = decoded['cells'][tuple(row['source'])]
    points = [decoded['vertices'][cell[5+c]] for c in CORNERS[row['triangle']]]
    center = [sum(p[i]/3 for p in points) for i in range(3)]
    half = (25, 2000)[row['window']]
    candidates = []
    for item in buckets[math.floor(center[0]/BUCKET), math.floor(center[2]/BUCKET)]:
        a, b, c = item['points']
        signs = []
        for first, last in ((a, b), (b, c), (c, a)):
            signs.append((last[0]-first[0])*(center[2]-first[2]) -
                         (last[2]-first[2])*(center[0]-first[0]))
        # Original centroid queries are interior, except shared degenerate geometry.
        tolerance = 1e-6 * max(math.hypot(last[0]-first[0], last[2]-first[2])
                             for first, last in ((a,b),(b,c),(c,a)))
        if min(signs) < -tolerance and max(signs) > tolerance: continue
        normal = item['normal']
        height = a[1] - (normal[0]*(center[0]-a[0]) + normal[2]*(center[2]-a[2]))/normal[1]
        time = (center[1] + half - height)/(2*half)
        if time < 0 or time > 1: continue
        candidates.append((time, item, height))
    if not candidates: return None
    earliest = min(c[0] for c in candidates)
    return min((c for c in candidates if c[0] <= earliest+1e-10),
               key=lambda c: (c[1]['index'], c[1]['triangle']))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK/'rewrite-ground-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents: parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (ROOT/'DestructionDerby2/Dirinfo').resolve()
    if digest(archive) != ORIGINAL_SHA256: raise ValueError('Provision supported original Dirinfo')
    files = assets(archive.read_bytes())
    image = (ROOT/'DestructionDerby2/dd2_image.bin').read_bytes()
    rows = list(struct.iter_unpack('<2i', image[0x63dcc:0x63e2c]))
    corners = [list(struct.unpack_from('<3h', image, 0x66a98 + index*8)) for index in range(8)]
    calls = []

    def run(command, label):
        log = output/(label+'.log')
        with log.open('wb') as stream:
            result = run_bounded(command, directory=output, timeout=120, cwd=ROOT,
                                 stdout=stream, stderr=subprocess.STDOUT)
        content = log.read_text(errors='replace')
        calls.append(dict(label=label, returncode=result.returncode, log_sha256=digest(log)))
        if result.returncode or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label+' failed: '+content[-6000:])
        return [json.loads(line) for line in content.splitlines() if line.startswith('{')]

    flags = ['-std=c11','-O1','-g','-I',str(ROOT/'src'),
             '-Wall','-Wextra','-Wpedantic','-Wno-unused-parameter','-Wno-unused-function',
             '-fno-strict-aliasing','-ffast-math','-Werror','-Wshadow','-Wconversion',
             '-Wstrict-prototypes','-Wmissing-prototypes','-Wformat=2',
             '-fsanitize=address,undefined','-fno-omit-frame-pointer']
    units = [ROOT/f'src/assets/{name}.c' for name in ('archive','level','road','barriers')]
    units += [ROOT/f'src/physics/{name}.c' for name in
              ('road_contact','road_surface','vehicle','barrier_world','car_contact','vehicle_collision','damage')]
    units += [ROOT/'src/game/driving.c', ROOT/'src/game/starting_grid.c', ROOT/'src/game/accidents.c', ROOT/'src/game/course.c', ROOT/'src/game/laps.c', ROOT/'src/game/race.c', ROOT/'src/platform/file.c']
    units += [ROOT/f'src/ai/{name}.c' for name in ('path','driver')]
    sanitized, synthetic = output/'export-sanitized', output/'test-sanitized'
    for source, binary in (('ground_export', sanitized), ('ground_test', synthetic)):
        run([tool('clang'), *flags, *map(str, units), str(ROOT/f'tests/rewrite/{source}.c'),
             '-lm','-o',str(binary)], source+'-build')
    run([str(synthetic)], 'synthetic-sanitized')
    commands = {'native': [str(WORK/'rewrite-native/dd2_ground_export')],
                'wasm': ['node', str(WORK/'rewrite-wasm/dd2_ground_export.js')],
                'sanitized': [str(sanitized)]}
    levels = []
    for code in CODES:
        decoded = reference(files[f'LEV{code}\\LEVEL.DAT'], code, rows)
        planes, buckets = original_planes(decoded)
        comparisons = {}
        for platform, command in commands.items():
            sweep = run([*command, str(archive), code, 'sweep'], code+'-sweep-'+platform)
            if [row['values'] for row in sweep[:8]] != corners:
                raise ValueError('Source body corner geometry differs')
            queries = sweep[8:]
            if len(queries) != 4*len(decoded['cells']):
                raise ValueError('Incomplete sweep query coverage')
            coverage = {(tuple(r['source']), r['triangle'], r['window']) for r in queries}
            expected = {(key, triangle, window) for key in decoded['cells']
                        for triangle in range(2) for window in range(2)}
            if coverage != expected: raise ValueError('Sweep inventory differs')
            error, hits = 0, 0
            for query in queries:
                wanted = oracle(decoded, buckets, query)
                if bool(wanted) != bool(query['found']):
                    raise ValueError('Sweep selection differs: '+str((code, platform, query, wanted)))
                if wanted is None: continue
                time, item, height = wanted
                if tuple(query['key']) != item['key'] or query['hit_triangle'] != item['triangle']:
                    raise ValueError('Earliest source triangle differs: '+str((code, platform, query, wanted)))
                values = [(time, query['time']), (height, query['height']), *zip(item['unit'], query['normal'])]
                for first, second in values:
                    error = max(error, abs(first-second))
                    if not math.isclose(first, second, abs_tol=1e-8, rel_tol=1e-10):
                        raise ValueError('Sweep contact differs')
                hits += 1
            states = run([*command, str(archive), code, 'drive'], code+'-drive-'+platform)
            if len(states) != 120 or {r['seed'] for r in states} != set(range(4)):
                raise ValueError('Incomplete body drop coverage')
            for seed in (1,2,3):
                if next(r for r in states if r['seed'] == seed and r['step'] == 600)['contacts'] == 0:
                    raise ValueError('Missing original-road body impact')
            if platform == 'native': baseline = states
            pose_error = 0
            for first, second in zip(baseline, states):
                if first['seed'] != second['seed'] or first['step'] != second['step']:
                    raise ValueError('Body state ordering differs')
                # A spinning body can gain/lose a grazing solver response due to
                # rounding. Require the same support outcome and at most two
                # responses' difference; other three drops remain exact.
                contact_tolerance = 2 if first['seed'] == 3 else 0
                if abs(first['contacts'] - second['contacts']) > contact_tolerance or \
                        bool(first['contacts']) != bool(second['contacts']):
                    raise ValueError('Body contact count differs')
                for index, (left, right) in enumerate(zip(first['values'], second['values'])):
                    if not math.isfinite(left) or not math.isfinite(right): raise ValueError('Nonfinite body state')
                    pose_error = max(pose_error, abs(left-right))
                    tolerance = STATE_TOLERANCES[index]
                    if not math.isclose(left, right, abs_tol=tolerance, rel_tol=1e-8):
                        raise ValueError('Body state differs: '+str((code, platform, first['seed'], first['step'], left, right)))
            comparisons[platform] = dict(queries=len(queries), hits=hits, contact_error=error,
                                         pose_error=pose_error, steps=2400,
                                         final_states=[r for r in states if r['step'] == 600])
        levels.append(dict(code=code, triangles=len(planes), comparisons=comparisons))
        print(json.dumps(dict(code=code, pass_=True, comparisons={p: {k:v for k,v in c.items() if k != 'final_states'}
                                                                 for p,c in comparisons.items()})), flush=True)
    sources = [p for p in (ROOT/'src').rglob('*') if p.suffix in ('.c','.h')]
    sources += [ROOT/'tests/rewrite/ground_test.c',ROOT/'tests/rewrite/ground_export.c',
                ROOT/'tests/rewrite/surface_fixture.h',ROOT/'CMakeLists.txt',Path(__file__).resolve()]
    binaries = [WORK/'rewrite-native/dd2_ground_export',WORK/'rewrite-wasm/dd2_ground_export.wasm',sanitized,synthetic]
    report = dict(pass_=True, verified_at=datetime.now(timezone.utc).isoformat(),scope=__doc__.strip(),
                  original_sha256=ORIGINAL_SHA256,corners=corners,levels=levels,calls=calls,
                  state_tolerance=dict(absolute_by_component=STATE_TOLERANCES, relative=1e-8,
                                       spinning_contact_count=2, other_contact_count=0),
                  source_sha256={str(p.relative_to(ROOT)):digest(p) for p in sources},
                  binary_sha256={str(p):digest(p) for p in binaries})
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    sanitized.unlink(); synthetic.unlink()
    for call in calls: (output/(call['label']+'.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, report=str(output/'report.json'),
                          queries_per_target=sum(l['comparisons']['native']['queries'] for l in levels),
                          steps_per_target=sum(l['comparisons']['native']['steps'] for l in levels))))


if __name__ == '__main__': main()
