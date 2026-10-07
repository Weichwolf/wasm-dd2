#!/usr/bin/env python3
"""Check original road graphs, lane geometry and vertical contacts on three targets.

Independent reader walks source links, reads row offsets from the original image,
and uses integer edge tests plus rational plane heights. This checks the rewrite's
data/contact layer, not original physics, suspension, lap rules or driving parity.
"""
import argparse
from datetime import datetime, timezone
from fractions import Fraction
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

NONE = (1 << 32) - 1
CODES = '123456789AB'
CORNERS = ((0, 1, 3), (2, 3, 1))


def digest(data):
    return hashlib.sha256(data).hexdigest()


def triangles(kind, lane, count):
    active = {0, 1}
    # Map_Height's first/last-lane dispatch, independent of a mask lookup table.
    if lane == 0:
        if kind in (2, 4): active.discard(0)
        if kind in (5, 7): active.discard(1)
    if lane == count - 1:
        if kind in (3, 4): active.discard(0)
        if kind in (6, 7): active.discard(1)
    return sum(1 << triangle for triangle in active)


def reference(level, code, rows):
    offsets = struct.unpack_from('<29I', level)
    source = level[offsets[1]:offsets[2]]
    vertices = list(struct.iter_unpack('<3i', level[offsets[2]:offsets[3]]))
    strips, cells, main = {}, {}, []
    if code in '1234567':
        pending = [0]
        while pending:
            offset = pending.pop()
            if offset in strips: continue
            at = offset + 4
            kind, lanes, _, lane_start, _, heading = source[at:at+6]
            number, first, flags = struct.unpack_from('<3H', source, at+14)
            forward, backward, branch = struct.unpack_from('<3I', source, at+20)
            strips[offset] = [offset, forward, backward, branch if kind in (8, 9) else NONE,
                              NONE, kind, lanes, lane_start, heading, number, first, flags]
            pending.extend([forward, backward])
            if kind in (8, 9): pending.append(branch)
            row_a = first + rows[kind][0]
            row_b = first + rows[kind][1] + lanes + 1
            for lane in range(lanes):
                flags, heading = source[at+36+lane*14:at+38+lane*14]
                cells[(offset, lane)] = [offset, lane, flags, heading, triangles(kind, lane, lanes),
                                        row_a+lane, row_a+lane+1, row_b+lane+1, row_b+lane]
        current = 0
        while current not in main:
            strips[current][4] = len(main)
            main.append(current)
            current = strips[current][1]
        if current != 0 or len(strips) != struct.unpack_from('<I', source)[0]:
            raise ValueError('Original road topology did not close or inventory differs')
    else:
        if len(vertices) != 1024 or len(source) != 1024*14:
            raise ValueError('Original arena grid extent differs')
        for row in range(31):
            for column in range(31):
                vertex = row*32+column
                flags, heading = source[vertex*14:vertex*14+2]
                cells[(NONE, vertex)] = [NONE, vertex, flags, heading, 3,
                                         vertex, vertex+1, vertex+33, vertex+32]
    return dict(vertices=vertices, strips=strips, cells=cells, main_count=len(main))


def plane_sample(vertices, cell, triangle):
    # Query is the source triangle centroid; all projected edge predicates stay
    # integral by scaling both coordinates by three. No C barycentric formula.
    query = [sum(vertices[cell[5+corner]][axis] for corner in CORNERS[triangle])
             for axis in (0, 2)]
    for candidate, corners in enumerate(CORNERS):
        if not cell[4] & (1 << candidate): continue
        a, b, c = [vertices[cell[5+corner]] for corner in corners]
        edge = [b[i]-a[i] for i in range(3)]
        other = [c[i]-a[i] for i in range(3)]
        normal = [edge[1]*other[2]-edge[2]*other[1],
                  edge[2]*other[0]-edge[0]*other[2],
                  edge[0]*other[1]-edge[1]*other[0]]
        if normal[1] == 0: continue
        signs = []
        for start, end in ((a, b), (b, c), (c, a)):
            signs.append((end[0]-start[0])*(query[1]-3*start[2]) -
                         (end[2]-start[2])*(query[0]-3*start[0]))
        if min(signs) < 0 < max(signs): continue
        height = Fraction(a[1]) - (normal[0]*(Fraction(query[0], 3)-a[0]) +
                                  normal[2]*(Fraction(query[1], 3)-a[2])) / normal[1]
        length = math.sqrt(sum(value*value for value in normal))
        factor = (1 if normal[1] > 0 else -1)/length
        return [1, candidate, float(height), *(value*factor for value in normal)]
    return [0, 0, 0, 0, 0, 0]


def compare(actual, expected):
    if actual['vertices'] != [list(v) for v in expected['vertices']]:
        raise ValueError('Owned road vertices differ')
    if actual['main_count'] != expected['main_count']:
        raise ValueError('Main loop count differs')
    if {row[0]:row for row in actual['strips']} != expected['strips']:
        raise ValueError('Decoded strip fields or link inventory differ')
    cells = {(row[0], row[1]):row for row in actual['cells']}
    if set(cells) != set(expected['cells']):
        raise ValueError('Lane cell inventory differs')
    samples, contacts, max_height, max_normal = 0, 0, 0, 0
    for key, geometry in expected['cells'].items():
        row = cells[key]
        if row[:9] != geometry:
            raise ValueError('Lane geometry/attributes differ: ' + str(key))
        for triangle in range(2):
            sample = row[9][triangle]
            wanted = plane_sample(expected['vertices'], geometry, triangle)
            samples += 1
            contacts += wanted[0]
            if sample[:2] != wanted[:2]:
                raise ValueError('Triangle contact selection differs: ' + str((key, triangle, sample, wanted)))
            max_height = max(max_height, abs(sample[2]-wanted[2]))
            max_normal = max(max_normal, *(abs(a-b) for a,b in zip(sample[3:],wanted[3:])))
            if max_height > 1e-8 or max_normal > 1e-10:
                raise ValueError('Contact plane differs: ' + str((key, triangle, sample, wanted)))
    return dict(vertices=len(expected['vertices']), strips=len(expected['strips']),
                main_count=expected['main_count'], cells=len(cells), samples=samples,
                valid_contacts=contacts, max_height_error=max_height, max_normal_error=max_normal)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK/'rewrite-road-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents: parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (ROOT/'DestructionDerby2/Dirinfo').resolve()
    data = archive.read_bytes()
    if digest(data) != ORIGINAL_SHA256: raise ValueError('Provision the supported original Dirinfo')
    files = assets(data)
    image_path = ROOT/'DestructionDerby2/dd2_image.bin'
    image = image_path.read_bytes()
    row_table = list(struct.iter_unpack('<2i', image[0x463dcc-0x400000:0x463e2c-0x400000]))
    calls = []
    def run(command, label, expected=0):
        path = output/(label+'.log')
        with path.open('wb') as log:
            result = run_bounded(command, directory=output, timeout=60,
                                 stdout=log, stderr=subprocess.STDOUT, cwd=ROOT)
        content = path.read_text(errors='replace')
        calls.append(dict(label=label, returncode=result.returncode, log_sha256=digest(path.read_bytes())))
        if result.returncode != expected or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label+' failed: '+content[-4000:])
        if expected and content: raise ValueError(label+' emitted output during rejection')
        return path
    flags = ['-std=c11','-O1','-g','-I',str(ROOT/'src'),
             '-Wall','-Wextra','-Wpedantic','-Wno-unused-parameter','-Wno-unused-function',
             '-fno-strict-aliasing','-ffast-math','-Werror','-Wshadow','-Wconversion',
             '-Wstrict-prototypes','-Wmissing-prototypes','-Wformat=2',
             '-fsanitize=address,undefined','-fno-omit-frame-pointer']
    units = [ROOT/f'src/assets/{name}.c' for name in ('archive','level','road')]
    units += [ROOT/'src/physics/road_contact.c', ROOT/'src/platform/file.c']
    sanitized_export = output/'dd2_road_export_sanitized'
    sanitized_test = output/'dd2_road_test_sanitized'
    for name, binary in (('road_export',sanitized_export),('road_test',sanitized_test)):
        run([tool('clang'),*flags,*map(str,units),str(ROOT/f'tests/{name}.c'),
             '-lm','-o',str(binary)],name+'-sanitizer-build')
    run([str(sanitized_test)],'sanitized-synthetic')
    commands = {'native':[str(WORK/'rewrite-native/dd2_road_export')],
                'wasm':['node',str(WORK/'rewrite-wasm/dd2_road_export.js')],
                'sanitized':[str(sanitized_export)]}
    levels = []
    for code in CODES:
        expected = reference(files[f'LEV{code}\\LEVEL.DAT'],code,row_table)
        targets = {}
        for platform, command in commands.items():
            path = run([*command,str(archive),code],code+'-'+platform)
            targets[platform] = compare(json.loads(path.read_text()),expected)
        levels.append(dict(code=code, comparisons=targets))
        print(json.dumps(dict(code=code,pass_=True,cells=len(expected['cells']))),flush=True)
    level = files['LEV1\\LEVEL.DAT']
    offsets = struct.unpack_from('<29I',level)
    level_offset = next(sector*2048 for name,sector,size in struct.iter_unpack('<18sHI',data[:0x2808])
                        if name.split(b'\0',1)[0] == b'LEV1\\LEVEL.DAT')
    first = level_offset+offsets[1]+4
    original = reference(level,'1',row_table)
    split = next(offset for offset,row in original['strips'].items() if row[5]==8)
    branch = original['strips'][split][3]
    variants = [('zero-count',level_offset+offsets[1],struct.pack('<I',0)),
                ('huge-count',level_offset+offsets[1],struct.pack('<I',NONE)),
                ('unknown-kind',first,b'\x0c'), ('empty-lanes',first+1,b'\x00'),
                ('excessive-lanes',first+1,b'\x80'),
                ('unaligned-link',first+20,struct.pack('<I',2)),
                ('overlapping-link',first+20,struct.pack('<I',4)),
                ('outside-link',first+24,struct.pack('<I',NONE)),
                ('outside-vertex',first+16,struct.pack('<H',65535)),
                ('branch-forward-cycle',first+branch+20,struct.pack('<I',branch)),
                ('branch-backward-cycle',first+branch+24,struct.pack('<I',branch))]
    mutated = output/'mutated-archive.bin'
    for label, at, replacement in variants:
        changed = bytearray(data); changed[at:at+len(replacement)] = replacement
        mutated.write_bytes(changed)
        for platform, command in commands.items():
            run([*command,str(mutated),'1'],platform+'-'+label,expected=1)
    sources = ['src/assets/road.c','src/assets/road.h','src/assets/track.c','src/assets/track.h',
               'src/physics/road_contact.c','src/physics/road_contact.h',
               'tests/road_test.c','tests/road_export.c','tools/rewrite/verify_roads.py',
               'CMakeLists.txt']
    binaries = [WORK/'rewrite-native/dd2_road_export',WORK/'rewrite-wasm/dd2_road_export.js',
                WORK/'rewrite-wasm/dd2_road_export.wasm',sanitized_export,sanitized_test]
    report = dict(pass_=True, verified_at=datetime.now(timezone.utc).isoformat(), scope=__doc__.strip(),
                  original_sha256=ORIGINAL_SHA256, original_image_sha256=digest(image),
                  row_table=row_table, levels=levels, rejections=len(variants), calls=calls,
                  sanitizer_scope='All road/contact/export C units instrumented; no SoftGL library linked',
                  source_sha256={p:digest((ROOT/p).read_bytes()) for p in sources},
                  binary_sha256={str(p):digest(p.read_bytes()) for p in binaries})
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    mutated.unlink();sanitized_export.unlink();sanitized_test.unlink()
    for call in calls: (output/(call['label']+'.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True,levels=len(levels),report=str(output/'report.json'))))


if __name__ == '__main__': main()
