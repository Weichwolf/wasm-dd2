#!/usr/bin/env python3
"""Compare indexed surface selection with exhaustive original-cell search.

Check every playable level on native, Node/WASM and ASan/UBSan. Queries cover
both triangle centroids with three height windows and every cell corner. The
linear oracle enumerates all cells and uses the independently verified contact
primitive; it does not traverse the bounds hierarchy. No driving parity claim.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256

CODES = '123456789AB'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK/'rewrite-surface-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents: parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (ROOT/'DestructionDerby2/Dirinfo').resolve()
    if digest(archive) != ORIGINAL_SHA256: raise ValueError('Provision the supported original Dirinfo')
    calls = []
    def run(command, label):
        path = output/(label+'.log')
        started = time.monotonic()
        with path.open('wb') as log:
            result = run_bounded(command, directory=output, timeout=120,
                                 stdout=log, stderr=subprocess.STDOUT, cwd=ROOT)
        content = path.read_text(errors='replace')
        calls.append(dict(label=label, returncode=result.returncode,
                          elapsed_seconds=time.monotonic()-started, log_sha256=digest(path)))
        if result.returncode or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label+' failed: '+content[-6000:])
        return path
    flags = ['-std=c11','-O1','-g','-I',str(ROOT/'src'),
             '-Wall','-Wextra','-Wpedantic','-Wno-unused-parameter','-Wno-unused-function',
             '-fno-strict-aliasing','-ffast-math','-Werror','-Wshadow','-Wconversion',
             '-Wstrict-prototypes','-Wmissing-prototypes','-Wformat=2',
             '-fsanitize=address,undefined','-fno-omit-frame-pointer']
    units = [ROOT/f'src/assets/{name}.c' for name in ('archive','level','road')]
    units += [ROOT/f'src/physics/{name}.c' for name in ('road_contact','road_surface')]
    units += [ROOT/'src/platform/file.c']
    sanitized_export = output/'dd2_surface_export_sanitized'
    sanitized_test = output/'dd2_surface_test_sanitized'
    for name, binary in (('surface_export',sanitized_export),('surface_test',sanitized_test)):
        run([tool('clang'),*flags,*map(str,units),str(ROOT/f'tests/{name}.c'),
             '-lm','-o',str(binary)],name+'-sanitizer-build')
    run([str(sanitized_test)],'sanitized-synthetic')
    commands = {'native':[str(WORK/'rewrite-native/dd2_surface_export')],
                'wasm':['node',str(WORK/'rewrite-wasm/dd2_surface_export.js')],
                'sanitized':[str(sanitized_export)]}
    levels = []
    for code in CODES:
        targets = {}
        for platform, command in commands.items():
            targets[platform] = json.loads(run([*command,str(archive),code],code+'-'+platform).read_text())
        baseline = targets['native']
        if baseline['queries'] != baseline['cells']*10 or baseline['found'] < baseline['cells']:
            raise ValueError('Surface query coverage insufficient: '+code)
        for platform, comparison in targets.items():
            if comparison != baseline:
                raise ValueError('Cross-target contact selection differs: '+str((code,platform,comparison,baseline)))
            # Count actual contact calls, including both selection passes. A
            # hidden full scan cannot pass this deterministic pruning gate.
            if comparison['fraction_of_full_scan'] >= .05:
                raise ValueError('Surface query did not prune at least 95% of cells: '+code)
        levels.append(dict(code=code, comparisons=targets))
        print(json.dumps(dict(code=code,pass_=True,queries=baseline['queries'],
                              mean_cell_tests=baseline['mean_cell_tests'])),flush=True)
    sources = ['src/physics/road_contact.c','src/physics/road_contact.h',
               'src/physics/road_surface.c','src/physics/road_surface.h','src/physics/numeric.h',
               'src/assets/road.c','src/assets/road.h','tests/surface_export.c',
               'tests/surface_test.c','tests/surface_fixture.h',
               'tools/rewrite/verify_surfaces.py','CMakeLists.txt']
    binaries = [WORK/'rewrite-native/dd2_surface_export',WORK/'rewrite-wasm/dd2_surface_export.js',
                WORK/'rewrite-wasm/dd2_surface_export.wasm',sanitized_export,sanitized_test]
    report = dict(pass_=True, verified_at=datetime.now(timezone.utc).isoformat(),
                  scope=__doc__.strip(), original_sha256=ORIGINAL_SHA256, levels=levels,
                  calls=calls, sanitizer_scope='All surface/contact/decoder/probe C units instrumented; no SoftGL library linked',
                  source_sha256={p:digest(ROOT/p) for p in sources},
                  binary_sha256={str(p):digest(p) for p in binaries})
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    sanitized_export.unlink();sanitized_test.unlink()
    for call in calls: (output/(call['label']+'.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, levels=len(levels),
                          queries_per_target=sum(level['comparisons']['native']['queries'] for level in levels),
                          report=str(output/'report.json'))))


if __name__ == '__main__': main()
