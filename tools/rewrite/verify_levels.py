#!/usr/bin/env python3
"""Compare every original level/texture field on native, WASM and ASan/UBSan."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256

LEVELS = '0123456789ABF'
PAGE_SIDE = 256
PAGE_BYTES = PAGE_SIDE ** 2
ATLAS_BYTES = PAGE_BYTES * 32
CLUT_BANK_BYTES = 4096


def assets(data):
    result = {}
    for name, sector, size in struct.iter_unpack('<18sHI', data[:0x2808]):
        if name[0] == 0:
            break
        result[name.split(b'\0', 1)[0].decode('ascii')] = data[sector * 2048:sector * 2048 + size]
    return result


def assemble_atlas(files, code):
    first = files[f'LEV{code}\\LEVEL.TX0']
    count = struct.unpack_from('<I', first)[0]
    descriptors = list(struct.iter_unpack('<8H', first[4:4 + count * 16]))
    atlas = bytearray(ATLAS_BYTES)
    image = 0
    part_count = 0
    while f'LEV{code}\\LEVEL.TX{part_count}' in files:
        data = files[f'LEV{code}\\LEVEL.TX{part_count}']
        offset = 4 + count * 16 if part_count == 0 else 0
        while offset < len(data):
            depth, width, height, x, y, *_ = descriptors[image]
            assert depth in (4, 8) and width > 0 and height > 0
            assert x + width <= PAGE_SIDE and y + height <= PAGE_SIDE * 32
            assert data[offset:offset + 4] == b'TEXT'
            offset += 4
            payload = data[offset:offset + width * height]
            assert len(payload) == width * height
            for row in range(height):
                start = (y + row) * PAGE_SIDE + x
                atlas[start:start + width] = payload[row * width:(row + 1) * width]
            offset += width * height
            image += 1
        assert offset == len(data)
        part_count += 1
    assert image == count
    return bytes(atlas), count, part_count


def page_rgba(indices, palette, lookup, cutout):
    # Translate independently using complete channel lookup tables. No runtime
    # decoder or original memory relocation participates in this reference.
    colors = indices.translate(lookup)
    output = bytearray(len(indices) * 4)
    for channel in range(3):
        # LEVEL.PAL stores Windows RGBQUAD entries: B, G, R, reserved.
        output[channel::4] = colors.translate(palette[2 - channel::4])
    if cutout:
        alpha = bytes(0 if index % 16 == 0 else 255 for index in range(256))
        output[3::4] = indices.translate(alpha)
    else:
        output[3::4] = b'\xff' * len(indices)
    return bytes(output)


def compare_export(path, files):
    report = []
    expected_hash = hashlib.sha256()
    with path.open('rb') as stream:
        def expect(data, label):
            expected_hash.update(data)
            actual = stream.read(len(data))
            if actual != data:
                raise ValueError(f'{path.name}: {label} differs')
        for code in LEVELS:
            level = files[f'LEV{code}\\LEVEL.DAT']
            offsets = struct.unpack_from('<29I', level)
            ends = (*offsets[1:], len(level))
            sections = [level[start:end] for start, end in zip(offsets, ends)]
            assert offsets[0] == 116 and all(116 <= a <= b <= len(level)
                                             for a, b in zip(offsets, ends))
            vertices = list(struct.iter_unpack('<3i', sections[2]))
            definitions = list(struct.iter_unpack('<HH8B', sections[4][4:]))
            assert len(definitions) == struct.unpack_from('<I', sections[4])[0]
            atlas, images, parts = assemble_atlas(files, code)
            palette = files[f'LEV{code}\\LEVEL.PAL']
            cluts = files[f'LEV{code}\\LEVEL.CLT'] + files.get(f'LEV{code}\\LEVEL.ECL', b'')
            assert len(palette) == 1024 and len(cluts) % CLUT_BANK_BYTES == 0
            banks = len(cluts) // CLUT_BANK_BYTES
            expect(code.encode('ascii'), code + ' name')
            expect(struct.pack('<4I', len(vertices), len(definitions), images, banks), code + ' counts')
            section_words = [word for pair in zip(offsets, map(len, sections)) for word in pair]
            expect(struct.pack('<58I', *section_words), code + ' every section offset/length')
            for vertex in vertices:
                expect(struct.pack('<3i', *vertex), code + ' signed vertex')
            for definition in definitions:
                expect(struct.pack('<HH8B', *definition), code + ' texture definition')
            expect(atlas, code + ' entire index atlas')
            rgba_hash = hashlib.sha256()
            for page in range(32):
                indices = atlas[page * PAGE_BYTES:(page + 1) * PAGE_BYTES]
                rgba = page_rgba(indices, palette, cluts[8 * 256:9 * 256], True)
                expect(rgba, code + f' page {page} RGBA')
                rgba_hash.update(rgba)
            for shade, cutout in ((0, False), (15, True)):
                start = (banks - 1) * CLUT_BANK_BYTES + shade * 256
                expect(page_rgba(atlas[:PAGE_BYTES], palette, cluts[start:start + 256], cutout),
                       code + f' last palette bank/shade {shade}')
            report.append(dict(level=code, bytes=len(level), vertices=len(vertices),
                               definitions=len(definitions), images=images, parts=parts, banks=banks,
                               atlas_sha256=hashlib.sha256(atlas).hexdigest(),
                               rgba_pages_sha256=rgba_hash.hexdigest(), rgba_pages_checked=32))
        if stream.read(1):
            raise ValueError('Unexpected export tail')
    return dict(levels=report, expected_sha256=expected_hash.hexdigest(), bytes=path.stat().st_size)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path, default=ROOT / 'DestructionDerby2')
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-level-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (args.game_dir / 'Dirinfo').resolve()
    original = archive.read_bytes()
    if hashlib.sha256(original).hexdigest() != ORIGINAL_SHA256:
        raise ValueError('Provision the supported unmodified original Dirinfo')
    files = assets(original)
    calls = []

    def run(command, label, success=True):
        log = output / (label + '.log')
        with log.open('w') as stream:
            result = run_bounded(command, directory=output, timeout=60,
                                 stdout=stream, stderr=subprocess.STDOUT, cwd=ROOT)
        content = log.read_text()
        calls.append(dict(label=label, command=list(map(str, command)), returncode=result.returncode,
                          log_sha256=hashlib.sha256(log.read_bytes()).hexdigest()))
        if result.returncode != (0 if success else 1) or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label + ' failed: ' + content)
        if not success and content:
            raise RuntimeError(label + ' produced unexpected rejection diagnostics')

    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT / 'src'), '-Wall', '-Wextra', '-Wpedantic',
             '-Wno-unused-parameter', '-Wno-unused-function', '-fno-strict-aliasing', '-ffast-math',
             '-Werror', '-Wshadow', '-Wconversion', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wformat=2', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    units = [str(ROOT / p) for p in ('src/assets/archive.c', 'src/assets/level.c', 'src/assets/textures.c')]
    sanitized_test = output / 'dd2_level_sanitized'
    sanitized_export = output / 'dd2_level_export_sanitized'
    for name, binary in (('level_test', sanitized_test), ('level_export', sanitized_export)):
        run([tool('clang'), *flags, *units, str(ROOT / f'tests/{name}.c'), '-o', str(binary)],
            name + '-sanitizer-build')
    run([str(sanitized_test)], 'sanitized-bounds')
    commands = {
        'native': [str(WORK / 'rewrite-native/dd2_level_export')],
        'wasm': ['node', str(WORK / 'rewrite-wasm/dd2_level_export.js')],
        'sanitized': [str(sanitized_export)],
    }
    comparisons = {}
    for platform, command in commands.items():
        raw = output / (platform + '.bin')
        run([*command, str(archive), str(raw)], platform + '-original')
        comparisons[platform] = compare_export(raw, files)
        with raw.open('rb') as stream:
            comparisons[platform]['actual_sha256'] = hashlib.file_digest(stream, 'sha256').hexdigest()
        if comparisons[platform]['actual_sha256'] != comparisons[platform]['expected_sha256']:
            raise ValueError(platform + ' export digest differs')
        # Producer and independent comparison have both terminated; retain the
        # hashes/field counts and immediately remove the successful raw export.
        raw.unlink()

    positions = {}
    for name, sector, _ in struct.iter_unpack('<18sHI', original[:0x2808]):
        if name[0] == 0:
            break
        positions[name.split(b'\0', 1)[0].decode('ascii')] = sector * 2048
    # The embedded definitions are the runtime source; the standalone TDF is
    # not loaded by Init_Graphics. Resolve the level section before mutating it.
    level_start = positions['LEV1\\LEVEL.DAT']
    embedded = level_start + struct.unpack_from('<I', original, level_start + 16)[0]
    variants = (
        ('level-offset-overflow', positions['LEV0\\LEVEL.DAT'] + 4, struct.pack('<I', 0xffffffff)),
        ('definition-count-overflow', embedded, struct.pack('<I', 0xffffffff)),
        ('texture-count-overflow', positions['LEV0\\LEVEL.TX0'], struct.pack('<I', 0xffffffff)),
        ('texture-zero-width', positions['LEV0\\LEVEL.TX0'] + 6, b'\0\0'),
        ('texture-outside-atlas', positions['LEV0\\LEVEL.TX0'] + 12, b'\xff\xff'),
        ('texture-invalid-format', positions['LEV0\\LEVEL.TX0'] + 4, b'\x02\0'),
    )
    mutated = output / 'mutated-archive.bin'
    raw = output / 'rejected-export.bin'
    for name, at, value in variants:
        changed = bytearray(original)
        changed[at:at + len(value)] = value
        mutated.write_bytes(changed)
        for platform, command in commands.items():
            run([*command, str(mutated), str(raw)], platform + '-' + name, success=False)
            raw.unlink(missing_ok=True)

    sources = ['src/assets/bytes.h', 'src/assets/level.c', 'src/assets/level.h',
               'src/assets/textures.c', 'src/assets/textures.h', 'tests/asset_fixture.h',
               'tests/level_test.c', 'tests/level_export.c',
               'tests/texture_render_test.c', 'tools/rewrite/verify_levels.py', 'CMakeLists.txt',
               'Makefile', '.clang-format', '.clang-tidy']
    binaries = [WORK / 'rewrite-native/dd2_level_export', WORK / 'rewrite-wasm/dd2_level_export.js',
                WORK / 'rewrite-wasm/dd2_level_export.wasm', sanitized_test, sanitized_export]
    report = dict(pass_=True, scope='typed level sections/vertices/UVs and textures; meshes/gameplay pending',
                  verified_at=datetime.now(timezone.utc).isoformat(), original_sha256=ORIGINAL_SHA256,
                  comparison='every exported byte independently reconstructed from original archive',
                  comparisons=comparisons, rejection_variants=len(variants), calls=calls,
                  source_sha256={p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest() for p in sources},
                  binary_sha256={str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in binaries})
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    mutated.unlink()
    sanitized_test.unlink()
    sanitized_export.unlink()
    for call in calls:
        (output / (call['label'] + '.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, levels=len(LEVELS), platforms=list(commands),
                          report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
