#!/usr/bin/env python3
"""Compare owned C content decoding on Native, Node/WASM and fresh sanitizers."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import zlib

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from assets.verify_assets import HEADER, TEXTURE, MATERIAL, PART, decode_model
from rewrite.quality import ROOT, tool

FLAGS = ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Wpedantic',
         '-Wno-unused-parameter', '-Wno-unused-function', '-fno-strict-aliasing',
         '-ffast-math', '-Werror', '-Wshadow', '-Wconversion', '-Wstrict-prototypes',
         '-Wmissing-prototypes', '-Wformat=2', '-fsanitize=address,undefined',
         '-fno-omit-frame-pointer', '-I', str(ROOT / 'src'), '-I', str(ROOT / 'tests')]
SIGNATURE = b'\x89PNG\r\n\x1a\n'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def chunk(name, payload):
    return struct.pack('>I', len(payload)) + name + payload + struct.pack('>I', zlib.crc32(name + payload))


def png(header, compressed, before=(), after=()):
    return SIGNATURE + chunk(b'IHDR', header) + b''.join(before) + chunk(b'IDAT', compressed) + b''.join(after) + chunk(b'IEND', b'')


def model_corruptions(data):
    _, textures, materials, parts, vertices, indices = HEADER.unpack_from(data)
    material = HEADER.size + textures * TEXTURE.size
    part = material + materials * MATERIAL.size
    vertex = part + parts * PART.size
    elements = vertex + vertices * 32
    cases = {'empty': b'', 'short-header': data[:20], 'short-tail': data[:-1], 'extra-tail': data + b'\0'}
    for label, offset, value in (
        ('version', 4, 0), ('texture-limit', 8, 33), ('material-limit', 12, 65),
        ('part-limit', 16, 4097), ('vertex-limit', 20, 1000001), ('index-limit', 24, 3000001),
        ('vertex-reference', elements, vertices), ('texture-reference', material + 56, textures),
        ('material-reference', part + 64, materials), ('part-range', part + 72, indices + 3),
        ('part-partition', part + 68, 3), ('part-role', part + 76, 7),
        ('material-nan', material + 32, 0x7fc12345), ('pivot-infinity', part + 80, 0x7f800000),
        ('position-nan', vertex, 0x7fc12345), ('position-infinity', vertex, 0xff800000),
        ('position-bound', vertex, 0x47800001), ('normal-infinity', vertex + 12, 0x7f800000),
        ('normal-nonunit', vertex + 12, 0x41200000), ('uv-nan', vertex + 24, 0x7fc12345),
        ('uv-infinity', vertex + 24, 0xff800000), ('degenerate-face', elements + 4, struct.unpack_from('<I', data, elements)[0]),
    ):
        damaged = bytearray(data)
        struct.pack_into('<I', damaged, offset, value)
        cases[label] = bytes(damaged)
    damaged = bytearray(data)
    damaged[elements + 4:elements + 8], damaged[elements + 8:elements + 12] = data[elements + 8:elements + 12], data[elements + 4:elements + 8]
    cases['reversed-winding'] = bytes(damaged)
    for label, path in (('parent-path', '../outside.png'), ('absolute-path', '/outside.png'),
                        ('empty-path-component', 'textures//outside.png'), ('dot-path-component', 'textures/./outside.png')):
        damaged = bytearray(data)
        damaged[HEADER.size:HEADER.size + 64] = path.encode().ljust(64, b'\0')
        cases[label] = bytes(damaged)
    damaged = bytearray(data)
    damaged[material + 31] = 1
    cases['name-padding'] = bytes(damaged)
    return cases


def png_cases():
    header = struct.pack('>IIBBBBB', 2, 2, 8, 6, 0, 0, 0)
    pixels = bytes((10, 20, 30, 255, 90, 120, 180, 64, 11, 32, 55, 0, 100, 140, 190, 128))
    rows = b'\0' + pixels[:8] + b'\0' + pixels[8:]
    compressed = zlib.compress(rows)
    valid = png(header, compressed)
    end = chunk(b'IEND', b'')
    preamble = SIGNATURE + chunk(b'IHDR', header)
    positives = {
        'plain': valid,
        'contiguous-idat': preamble + chunk(b'IDAT', compressed[:5]) + chunk(b'IDAT', b'') + chunk(b'IDAT', compressed[5:]) + end,
        'ancillary-before-after': png(header, compressed, (chunk(b'tEXt', b'Note\0test'),), (chunk(b'tEXt', b'Note\0test'),)),
        'suggested-palette': png(header, compressed, (chunk(b'PLTE', b'\x10\x20\x30'),)),
    }
    negatives = {
        'empty': b'', 'signature': b'\0' + valid[1:], 'crc': valid[:-1] + bytes([valid[-1] ^ 1]),
        'short-tail': valid[:-1], 'extra-tail': valid + b'\0', 'missing-idat': preamble + end,
        'missing-iend': valid[:-len(end)], 'duplicate-header': preamble + valid[8:],
        'header-not-first': SIGNATURE + chunk(b'tEXt', b'Note\0test') + valid[8:],
        'unknown-critical': png(header, compressed, (chunk(b'ABCD', b''),)),
        'invalid-chunk-name': png(header, compressed, (chunk(b'tE0t', b''),)),
        'noncontiguous-idat': preamble + chunk(b'IDAT', compressed[:5]) + chunk(b'tEXt', b'Note\0test') + chunk(b'IDAT', compressed[5:]) + end,
        'iend-payload': valid[:-len(end)] + chunk(b'IEND', b'\0'),
        'compressed-tail': png(header, compressed + b'\0'), 'deflate-short': png(header, compressed[:-1]),
        'decoded-short': png(header, zlib.compress(rows[:-1])), 'decoded-long': png(header, zlib.compress(rows + b'\0')),
        'filter': png(header, zlib.compress(b'\5' + rows[1:])),
        'transparency-key': png(header, compressed, (chunk(b'tRNS', b'\0' * 6),)),
        'empty-palette': png(header, compressed, (chunk(b'PLTE', b''),)),
        'misaligned-palette': png(header, compressed, (chunk(b'PLTE', b'\0' * 4),)),
        'oversized-palette': png(header, compressed, (chunk(b'PLTE', b'\0' * 771),)),
        'duplicate-palette': png(header, compressed, (chunk(b'PLTE', b'\0' * 3), chunk(b'PLTE', b'\0' * 3))),
        'late-palette': png(header, compressed, (), (chunk(b'PLTE', b'\0' * 3),)),
    }
    for label, offset, value in (('zero-width', 0, 0), ('zero-height', 4, 0),
                                  ('large-width', 0, 2049), ('large-height', 4, 2049)):
        damaged = bytearray(header)
        struct.pack_into('>I', damaged, offset, value)
        negatives[label] = png(damaged, compressed)
    for label, offset, value in (('depth', 8, 16), ('palette-indexed', 9, 3), ('color-type', 9, 5),
                                  ('compression', 10, 1), ('filter-method', 11, 1), ('interlace', 12, 1)):
        damaged = bytearray(header)
        damaged[offset] = value
        negatives[label] = png(damaged, compressed)
    negatives['chunk-extent'] = preamble + struct.pack('>I', 0xffffffff) + valid[len(preamble) + 4:]
    return positives, negatives, pixels


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'authored-content-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    calls = []
    environment = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1:exitcode=86',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1:exitcode=87')

    def run(command, label, expected=0):
        log = output / (label + '.log')
        with log.open('w') as stream:
            result = run_bounded(list(map(str, command)), directory=output, timeout=60,
                                 stdout=stream, stderr=subprocess.STDOUT, cwd=ROOT, env=environment)
        content = log.read_bytes()
        calls.append(dict(label=label, returncode=result.returncode, output_sha256=digest(content)))
        if result.returncode != expected or b'Sanitizer:' in content or b'runtime error:' in content:
            raise RuntimeError(label + ': ' + log.read_text())
        if expected == 1 and content:
            raise RuntimeError(label + ' emitted diagnostics on a normal rejection')
        return log

    sanitized = output / 'dd2_content_sanitized'
    sources = ['src/assets/model.c', 'src/assets/image.c', 'src/platform/file.c', 'tests/content_export.c']
    run([tool('clang'), *FLAGS, *[ROOT / name for name in sources], '-lz', '-lm', '-o', sanitized], 'sanitizer-build')
    binaries = [sanitized]
    for kind in ('model', 'image'):
        binary = output / ('dd2_' + kind + '_sanitized')
        run([tool('clang'), *FLAGS, ROOT / ('src/assets/' + kind + '.c'),
             ROOT / ('tests/' + kind + '_test.c'), '-lz', '-lm', '-o', binary], kind + '-sanitizer-build')
        run([binary], kind + '-sanitizer-bounds')
        binaries.append(binary)
    commands = {'native': [WORK / 'rewrite-native/dd2_content_export'],
                'wasm': ['node', WORK / 'rewrite-wasm/dd2_content_export.js'], 'sanitized': [sanitized]}
    inputs = sorted((ROOT / 'assets/runtime/models').glob('*.dd2mesh')) + sorted((ROOT / 'assets/runtime/textures').glob('*.png'))
    original_hashes = {str(path.relative_to(ROOT)): digest(path.read_bytes()) for path in inputs}
    entries = []
    destination = output / 'decoded.bin'
    mutated = output / 'mutated.bin'
    variants = []
    for path in inputs:
        model = path.suffix == '.dd2mesh'
        data = path.read_bytes()
        if model:
            independent = decode_model(data)
            expected = data
        else:
            with Image.open(path) as image:
                expected = image.convert('RGBA').tobytes()
                independent = {'size': list(image.size), 'mode': image.mode}
        for platform, command in commands.items():
            run([*command, 'model' if model else 'image', path, destination], platform + '-' + path.stem)
            decoded = destination.read_bytes()
            if decoded != expected:
                raise ValueError(platform + ' decode differs: ' + path.name)
            entries.append(dict(platform=platform, asset=str(path.relative_to(ROOT)),
                                decoded_bytes=len(decoded), decoded_sha256=digest(decoded), independent=independent))
            destination.unlink()
        if model:
            for label, damaged in model_corruptions(data).items():
                try:
                    decode_model(damaged)
                except ValueError:
                    pass
                else:
                    raise ValueError('Independent reader accepted damaged mesh: ' + label)
                mutated.write_bytes(damaged)
                for platform, command in commands.items():
                    run([*command, 'model', mutated, destination], platform + '-' + path.stem + '-' + label, 1)
                if destination.exists():
                    raise ValueError('Rejected mesh created an output file')
                variants.append(path.stem + '-' + label)
    positives, negatives, expected = png_cases()
    for label, data in positives.items():
        mutated.write_bytes(data)
        for platform, command in commands.items():
            run([*command, 'image', mutated, destination], platform + '-png-' + label)
            if destination.read_bytes() != expected:
                raise ValueError('PNG row/alpha/chunk result differs: ' + label)
            destination.unlink()
    for label, data in negatives.items():
        mutated.write_bytes(data)
        for platform, command in commands.items():
            run([*command, 'image', mutated, destination], platform + '-png-reject-' + label, 1)
        if destination.exists():
            raise ValueError('Rejected PNG created an output file')
        variants.append('png-' + label)
    if original_hashes != {str(path.relative_to(ROOT)): digest(path.read_bytes()) for path in inputs}:
        raise ValueError('Verification modified a runtime asset')
    sources += ['src/assets/model.h', 'src/assets/image.h', 'tests/model_test.c', 'tests/image_test.c',
                'tests/image_fixture.h', 'tools/assets/verify_content.py', 'tools/assets/verify_assets.py',
                'CMakeLists.txt', 'Makefile', '.clang-tidy', '.clang-format']
    binaries += [WORK / 'rewrite-native/dd2_content_export', WORK / 'rewrite-wasm/dd2_content_export.js', WORK / 'rewrite-wasm/dd2_content_export.wasm']
    report = dict(pass_=True, verified_at=datetime.now(timezone.utc).isoformat(),
                  scope='Owned DD2MESH2 fields/indices and PNG samples only; rendering/gameplay/audio/60 FPS remain unproved',
                  comparison='Exact re-encoding of typed owned mesh fields after source release; independent Pillow RGBA samples',
                  sanitizer_scope='Fresh O1 ASan/UBSan model/image/file/export units and focused decoder fixtures',
                  inputs=original_hashes, entries=entries, platforms=list(commands),
                  png_positive_cases=list(positives), rejection_variants=variants, calls=calls,
                  source_sha256={name: digest((ROOT / name).read_bytes()) for name in sources},
                  binary_sha256={str(path): digest(path.read_bytes()) for path in binaries})
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    mutated.unlink()
    for binary in binaries[:3]:
        binary.unlink()
    for call in calls:
        (output / (call['label'] + '.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, assets=len(inputs), platforms=list(commands),
                          png_positive_cases=len(positives), rejection_variants=len(variants), report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
