#!/usr/bin/env python3
"""One-time offline road conversion; normal builds load the committed exports."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from rewrite.verify_archive import ORIGINAL_SHA256
from rewrite.verify_levels import assets
from rewrite.verify_roads import reference, CODES, NONE
from assets.prepare_roads import prepare

ROOT = Path(__file__).resolve().parents[2]
ROWS = ((0, 0), (0, 0), (-1, -1), (0, -1), (-1, -2), (0, -1),
        (0, 0), (0, -1), (0, 0), (0, 0), (-1, -1), (0, 0))


def converted(expected, arena):
    # Preserve existing physics cell identities and tie ordering. The reference
    # parser is offset-keyed; translate every link to an owned index once here.
    order, indices = ([] if arena else [0]), ({} if arena else {0: 0})
    for offset in order:
        for link in expected['strips'][offset][1:4]:
            if link != NONE and link not in indices:
                indices[link] = len(order)
                order.append(link)
    strips, cells = [], []
    for offset in order:
        row = expected['strips'][offset]
        strip = dict(next=indices[row[1]], previous=indices[row[2]],
                     branch=NONE if row[3] == NONE else indices[row[3]], first_cell=len(cells),
                     main_order=row[4], kind=row[5], lanes=row[6], heading=row[8], flags=row[11])
        strips.append(strip)
        for lane in range(strip['lanes']):
            cell = expected['cells'][(offset, lane)]
            cells.append(dict(vertices=cell[5:9], strip=indices[offset], lane=lane,
                              surface_flags=cell[2], heading=cell[3], triangle_mask=cell[4]))
    if arena:
        for (_, lane), cell in expected['cells'].items():
            cells.append(dict(vertices=cell[5:9], strip=NONE, lane=lane,
                              surface_flags=cell[2], heading=cell[3], triangle_mask=cell[4]))
    return dict(format='DD2ROAD1', layout='arena' if arena else 'racing', units_per_meter=160,
                up='Y', main_count=expected['main_count'], vertices=expected['vertices'],
                strips=strips, cells=cells)


def convert(archive, runtime):
    data = archive.read_bytes()
    if hashlib.sha256(data).hexdigest() != ORIGINAL_SHA256:
        raise ValueError('Provision the supported original archive for offline conversion')
    files = assets(data)
    destination = runtime / 'roads'
    destination.mkdir(parents=True, exist_ok=True)
    levels = []
    for code in CODES:
        road = converted(reference(files[f'LEV{code}\\LEVEL.DAT'], code, ROWS), code not in '1234567')
        name = f'roads/level-{code.lower()}.json'
        (runtime / name).write_text(json.dumps(road, indent=2) + '\n')
        levels.append(dict(level=code, editable=name, vertices=len(road['vertices']),
                           strips=len(road['strips']), cells=len(road['cells']), main_count=road['main_count']))
    manifest = dict(format='DD2ROADS1', source_sha256=ORIGINAL_SHA256,
                    recipe='tools/assets/convert_roads.py', intermediate=True, units_per_meter=160,
                    limitations=['Intermediate reference-derived gameplay geometry, not final authored tracks.',
                                 'Physics keeps quantized positions; rendering converts them to meters.',
                                 'Original source offsets, raw records and unused source fields are omitted.',
                                 'Default game/provider, UI and audio migration remain open.'], levels=levels)
    prepare(runtime, manifest)
    (destination / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, default=ROOT / 'DestructionDerby2/Dirinfo')
    parser.add_argument('--output', type=Path, default=ROOT / 'assets/runtime')
    args = parser.parse_args()
    manifest = convert(args.archive, args.output)
    print(json.dumps(dict(levels=len(manifest['levels']), bytes=sum(level['bytes'] for level in manifest['levels']))))


if __name__ == '__main__':
    main()
