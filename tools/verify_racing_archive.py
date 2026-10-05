#!/usr/bin/env python3
"""Check lossless racing storage against closed actual original image pairs.

Exercise the production comparison reader, open-file exclusion and damaged
archive evidence. This validates storage only; no new game run or original/
port parity is claimed. All original metadata and API streams remain intact.
"""
import argparse
import copy
import json
from pathlib import Path
import shutil
import zlib

from artifacts import WORK, prepare_output, open_files
from racing_archive import (FRAME, PALETTE, MANIFEST, compact, compress,
                            decompress, digest, clear_cache)
from verify_normal_arena_history import picture, exact_bytes


def require(value, message):
    if not value: raise ValueError(message)


def check_corpus(root, expected):
    for prefix, image, palette in expected:
        require(exact_bytes(picture(root, prefix, '.bin', FRAME), image, FRAME),
                'Literal original image differs')
        require(exact_bytes(picture(root, prefix, '.pal', PALETTE), palette, PALETTE),
                'Literal original palette differs')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True, help='original history image directory')
    parser.add_argument('--first', type=int, required=True)
    parser.add_argument('--frames', type=int, default=256)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = prepare_output(args.output)
    require(WORK in output.parents and not output.exists(), 'Use a fresh /tmp/wasm-dd2/ path')
    require(64 <= args.frames <= 512, 'Use 64..512 bounded corpus frames')
    output.mkdir(); root = output/'fixture'; root.mkdir()
    expected = []; total = 0
    for i in range(args.first, args.first+args.frames):
        prefix = f'race{i:05d}'
        image = picture(args.reference, prefix, '.bin', FRAME)
        palette = picture(args.reference, prefix, '.pal', PALETTE)
        expected.append((prefix, image, palette))
        # Keep a genuinely independent zlib source fixture: the public archive
        # reader is then compared literally with the untouched reference bytes.
        (root/(prefix+'.bin.z')).write_bytes(zlib.compress(image, 6))
        (root/(prefix+'.pal')).write_bytes(palette)
        total += (root/(prefix+'.bin.z')).stat().st_size+PALETTE
    prefix = expected[0][0]
    with (root/(prefix+'.bin.z')).open('rb'):
        count = compact(root, minimum_age=0, complete_chunks_only=True)
        require(count == (args.frames-1)//64*64, 'Live partial chunk was not deferred')
        compact(root, minimum_age=0)
        require((root/(prefix+'.bin.z')).exists() and (root/(prefix+'.pal')).exists(),
                'Open image pair was removed')
        check_corpus(root, expected)
    require(compact(root, minimum_age=0) == 1, 'Closed excluded pair was not archived')
    check_corpus(root, expected)
    require(not list(root.glob('race*.bin.z')) and not list(root.glob('race*.pal')),
            'Completed raw pairs remain')
    # Model a process interruption between manifest commit and raw cleanup.
    # A restarted packer must preserve open input and remove closed identical
    # leftovers; it must reject changed input before deleting anything.
    _, image, palette = expected[0]
    remaining = root/(prefix+'.bin.z')
    remaining.write_bytes(zlib.compress(image,6)); (root/(prefix+'.pal')).write_bytes(palette)
    with remaining.open('rb'):
        require(compact(root, minimum_age=0) == 0 and remaining.exists(), 'Open restart input removed')
    damaged = bytearray(image); damaged[0] ^= 1; remaining.write_bytes(zlib.compress(damaged,6))
    try: compact(root, minimum_age=0)
    except ValueError: require(remaining.exists(), 'Changed leftover deleted')
    else: raise RuntimeError('Changed restart input accepted')
    remaining.write_bytes(zlib.compress(image,6))
    require(compact(root, minimum_age=0) == 0 and not remaining.exists(), 'Closed restart input retained')
    (root/(prefix+'.pal')).write_bytes(palette)
    require(compact(root, minimum_age=0) == 0 and not (root/(prefix+'.pal')).exists(),
            'Palette-only restart input retained')
    document = json.loads((root/MANIFEST).read_text())
    chunk = document['chunks'][0]
    path = root/'racing-archive'/chunk['file']
    saved = path.read_bytes()
    raw = decompress(saved, len(chunk['frames']))
    controls = ['open-file-exclusion', 'open-restart-input-exclusion', 'changed-restart-input', 'partial-live-chunk-deferred']
    for label in ('changed-image', 'changed-palette', 'truncated-pack', 'trailing-pack',
                  'oversized-decoded-pack', 'changed-prefix', 'duplicate-prefix',
                  'changed-image-hash', 'changed-literal-hash'):
        altered = copy.deepcopy(document); selected = altered['chunks'][0]; packed = saved
        if label in ('changed-image', 'changed-palette'):
            damaged = bytearray(raw); damaged[0 if label == 'changed-image' else FRAME] ^= 1
            packed = compress(bytes(damaged)); selected['literal_sha256'] = digest(damaged)
            key = 'framebuffer_sha256' if label == 'changed-image' else 'palette_sha256'
            selected['frames'][0][key] = digest(damaged[:FRAME] if label == 'changed-image' else damaged[FRAME:FRAME+PALETTE])
        elif label == 'truncated-pack': packed = saved[:-1]
        elif label == 'trailing-pack': packed = saved+b'\0'
        elif label == 'oversized-decoded-pack': packed = compress(raw+bytes(FRAME+PALETTE))
        elif label == 'changed-prefix': selected['frames'][0]['prefix'] = 'race99999'
        elif label == 'duplicate-prefix': selected['frames'][0]['prefix'] = selected['frames'][1]['prefix']
        elif label == 'changed-image-hash': selected['frames'][0]['framebuffer_sha256'] = '0'*64
        elif label == 'changed-literal-hash': selected['literal_sha256'] = '0'*64
        selected['sha256'] = digest(packed)
        path.write_bytes(packed); (root/MANIFEST).write_text(json.dumps(altered))
        clear_cache()
        try: check_corpus(root, expected)
        except ValueError: controls.append(label)
        else: raise RuntimeError('Damaged racing archive accepted: '+label)
        path.write_bytes(saved); (root/MANIFEST).write_text(json.dumps(document, indent=2)+'\n'); clear_cache()
    check_corpus(root, expected)
    report = dict(scope=__doc__, pass_=True, frames=len(expected), indexed_bytes=len(expected)*FRAME,
                  palette_bytes=len(expected)*PALETTE, source_bytes=total,
                  archive_bytes=sum(c['archived_bytes'] for c in document['chunks']),
                  negative_controls=controls, reference=str(args.reference.resolve()),
                  restart_cleanup=['closed-identical-pair', 'closed-identical-palette-only'],
                  first=args.first, actual_pairs=[dict(prefix=p,framebuffer_sha256=digest(i),palette_sha256=digest(c))
                                                for p,i,c in expected],
                  verifier_sha256=digest(Path(__file__).read_bytes()),
                  archive_tool_sha256=digest(Path(__file__).with_name('racing_archive.py').read_bytes()),
                  original_port_parity='unproven')
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    opened = open_files()
    for file in root.rglob('*'):
        if file.is_file():
            stat = file.stat(); require((stat.st_dev,stat.st_ino) not in opened, 'Fixture file still open')
    shutil.rmtree(root)
    print('PASS lossless original image/palette archive:',len(expected),'frames;',len(controls),'controls')


if __name__ == '__main__': main()
