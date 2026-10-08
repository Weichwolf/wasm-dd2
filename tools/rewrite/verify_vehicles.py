#!/usr/bin/env python3
"""Check fixed-step vehicle dynamics on original roads on three targets.

Twenty-four distributed starts per level exercise settling, falling, throttle,
steering, braking and reverse. Compare sampled poses, velocities, orientations
and wheel contacts on native, Node/WASM and ASan/UBSan. Synthetic tests cover
analytic rest height, grip, reverse steering, bridge selection, fast landings,
airborne controls and transactional rejection under fast-math. This is a vehicle
core check, not original driving parity, body collision or playable-race coverage.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256

CODES = '123456789AB'
ABSOLUTE_TOLERANCE = 1e-5
RELATIVE_TOLERANCE = 1e-8


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def compare(expected, actual, context):
    if len(expected) != len(actual): raise ValueError('Sample count differs: '+context)
    maximum = 0.0
    for first, second in zip(expected, actual):
        if first.get('summary'):
            if first != second: raise ValueError('Summary differs: '+str((context,first,second)))
            continue
        if first['seed'] != second['seed'] or first['step'] != second['step']:
            raise ValueError('Sample order differs: '+context)
        if len(first['state']) != 14 or len(second['state']) != 14:
            raise ValueError('Incomplete core state: '+context)
        if len(first['wheels']) != 4 or len(second['wheels']) != 4:
            raise ValueError('Incomplete wheel state: '+context)
        values = [*zip(first['state'],second['state'])]
        for left, right in zip(first['wheels'],second['wheels']):
            if len(left) != 5 or len(right) != 5 or left[:3] != right[:3]:
                raise ValueError('Wheel contact differs: '+str((context,first['seed'],first['step'],left,right)))
            values.extend(zip(left[3:],right[3:]))
        for left, right in values:
            if not math.isfinite(left) or not math.isfinite(right):
                raise ValueError('Nonfinite state: '+context)
            maximum = max(maximum,abs(left-right))
            if not math.isclose(left,right,rel_tol=RELATIVE_TOLERANCE,abs_tol=ABSOLUTE_TOLERANCE):
                raise ValueError('State differs: '+str((context,first['seed'],first['step'],left,right)))
    return maximum


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=WORK/'rewrite-vehicle-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents: parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True,exist_ok=True)
    check_space(output)
    archive = (ROOT/'DestructionDerby2/Dirinfo').resolve()
    if digest(archive) != ORIGINAL_SHA256: raise ValueError('Provision the supported original Dirinfo')
    calls = []
    def run(command,label):
        path = output/(label+'.log')
        start = time.monotonic()
        with path.open('wb') as log:
            result = run_bounded(command,directory=output,timeout=120,cwd=ROOT,
                                 stdout=log,stderr=subprocess.STDOUT)
        content = path.read_text(errors='replace')
        calls.append(dict(label=label,returncode=result.returncode,
                          elapsed_seconds=time.monotonic()-start,log_sha256=digest(path)))
        if result.returncode or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label+' failed: '+content[-6000:])
        return path
    flags = ['-std=c11','-O1','-g','-I',str(ROOT/'src'),
             '-Wall','-Wextra','-Wpedantic','-Wno-unused-parameter','-Wno-unused-function',
             '-fno-strict-aliasing','-ffast-math','-Werror','-Wshadow','-Wconversion',
             '-Wstrict-prototypes','-Wmissing-prototypes','-Wformat=2',
             '-fsanitize=address,undefined','-fno-omit-frame-pointer']
    units = [ROOT/f'src/assets/{name}.c' for name in ('archive','level','road')]
    units += [ROOT/f'src/physics/{name}.c' for name in ('road_contact','road_surface','body_surface','vehicle')]
    units += [ROOT/'src/platform/file.c']
    sanitized_export = output/'dd2_vehicle_export_sanitized'
    sanitized_test = output/'dd2_vehicle_test_sanitized'
    for name,binary in (('vehicle_export',sanitized_export),('vehicle_test',sanitized_test)):
        run([tool('clang'),*flags,*map(str,units),str(ROOT/f'tests/{name}.c'),
             '-lm','-o',str(binary)],name+'-sanitizer-build')
    run([str(sanitized_test)],'sanitized-synthetic')
    commands = {'native':[str(WORK/'rewrite-native/dd2_vehicle_export')],
                'wasm':['node',str(WORK/'rewrite-wasm/dd2_vehicle_export.js')],
                'sanitized':[str(sanitized_export)]}
    levels = []
    for code in CODES:
        targets = {}
        for platform,command in commands.items():
            path = run([*command,str(archive),code],code+'-'+platform)
            targets[platform] = [json.loads(line) for line in path.read_text().splitlines()]
        summary = targets['native'][-1]
        if len(targets['native']) != 1201 or summary.get('seeds') != 24 or summary.get('steps') != 24000:
            raise ValueError('Vehicle query coverage insufficient: '+code)
        if summary['grounded'] == 0 or summary['airborne'] == 0:
            raise ValueError('Missing contact/flight coverage: '+code)
        if summary['surface_types'][2:] != [0,0]:
            raise ValueError('Unspecified original surface class: '+code)
        errors = {platform:compare(targets['native'],actual,code+'-'+platform)
                  for platform,actual in targets.items()}
        levels.append(dict(code=code,summary=summary,max_absolute_error=errors))
        print(json.dumps(dict(code=code,pass_=True,steps=summary['steps'],errors=errors)),flush=True)
    sources = ['src/physics/body_surface.c','src/physics/body_surface.h','src/physics/body_geometry.h','src/physics/collision_math.h','src/physics/vehicle_collision.h','src/physics/vehicle.c','src/physics/vehicle.h','src/physics/numeric.h',
               'src/physics/road_contact.c','src/physics/road_contact.h',
               'src/physics/road_surface.c','src/physics/road_surface.h',
               'src/assets/road.c','src/assets/road.h','tests/vehicle_test.c',
               'tests/vehicle_export.c','tests/surface_fixture.h',
               'tools/rewrite/verify_vehicles.py','CMakeLists.txt']
    binaries = [WORK/'rewrite-native/dd2_vehicle_export',WORK/'rewrite-wasm/dd2_vehicle_export.js',
                WORK/'rewrite-wasm/dd2_vehicle_export.wasm',sanitized_export,sanitized_test]
    report = dict(pass_=True,verified_at=datetime.now(timezone.utc).isoformat(),scope=__doc__.strip(),
                  original_sha256=ORIGINAL_SHA256,levels=levels,calls=calls,
                  tolerance=dict(absolute=ABSOLUTE_TOLERANCE,relative=RELATIVE_TOLERANCE),
                  sanitizer_scope='All vehicle/contact/decoder/probe C units instrumented; no SoftGL library linked',
                  source_sha256={p:digest(ROOT/p) for p in sources},
                  binary_sha256={str(p):digest(p) for p in binaries})
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    sanitized_export.unlink();sanitized_test.unlink()
    for call in calls: (output/(call['label']+'.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True,levels=len(levels),
                          steps_per_target=sum(level['summary']['steps'] for level in levels),
                          report=str(output/'report.json'))))


if __name__ == '__main__': main()
