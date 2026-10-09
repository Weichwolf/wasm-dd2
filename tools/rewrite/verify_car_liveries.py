#!/usr/bin/env python3
"""Compare original high-body paint/number bindings and actual cross-target rendering."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, open_files, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256
from rewrite.verify_levels import assets
from verify_season_transition import EXE_SHA

LEVELS = '123456789AB'
WIDTH, HEIGHT, FRAMES = 320, 240, 24
FRAME_BYTES = WIDTH * HEIGHT * 4
CASE_WORDS = 18 + 99 * 10
CASES_PER_LEVEL = 60


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def compare_pixels(left, right):
    if len(left) != FRAME_BYTES or len(right) != FRAME_BYTES:
        raise ValueError('Invalid car frame extent')
    coverage = [sum(any(data[i:i + 3]) for i in range(0, len(data), 4)) for data in (left, right)]
    changed = sum(left[i:i + 4] != right[i:i + 4] for i in range(0, len(left), 4))
    error = sum(abs(a - b) for a, b in zip(left, right)) / len(left)
    if min(coverage) < 100 or changed > WIDTH * HEIGHT // 100 or error > 0.5:
        raise ValueError(f'Car render differs: coverage={coverage}, changed={changed}, error={error}')
    return {'visible_pixels': coverage, 'changed_pixels': changed, 'mean_channel_error': error,
            'native_sha256': hashlib.sha256(left).hexdigest(),
            'candidate_sha256': hashlib.sha256(right).hexdigest()}


def corruption_variants(archive):
    data = archive.read_bytes()
    sector = next(sector for name, sector, _ in struct.iter_unpack('<18sHI', data[:0x2808])
                  if name.split(b'\0', 1)[0] == b'LEV1\\LEVEL.DAT')
    level = sector * 2048
    sprites = level + struct.unpack_from('<I', data, level + 3 * 4)[0]
    count = struct.unpack_from('<I', data, sprites)[0]
    record = next(sprites + 4 + index * 24 for index in range(count)
                  if data[sprites + 4 + index * 24 + 14:sprites + 4 + index * 24 + 24]
                  .split(b'\0', 1)[0] == b'DR88A')
    return [('count', sprites, struct.pack('<I', count + 1)),
            ('missing-template', record + 14, b'XX88A\0'),
            ('horizontal-coordinate', record, struct.pack('<H', 256)),
            ('page', record + 2, struct.pack('<H', 8192)),
            ('palette-bank', record + 10, struct.pack('<H', 65535))]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-car-liveries-0005')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be under /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (ROOT / 'DestructionDerby2/Dirinfo').resolve()
    executable = (ROOT / 'DestructionDerby2/dd2h.exe').resolve()
    if digest(archive) != ORIGINAL_SHA256 or digest(executable) != EXE_SHA:
        raise ValueError('Supported unmodified original assets are required')
    files = assets(archive.read_bytes())
    sources = [p for folder in ('src', 'tests') for p in (ROOT / folder).rglob('*')
               if p.suffix in ('.c', '.h')]
    sources += [ROOT / name for name in ('CMakeLists.txt', 'Makefile', '.clang-tidy', '.clang-format',
                'tools/reference/car_livery_fixture.c', 'tools/reference/pe_fixture.h',
                'tools/rewrite/verify_car_liveries.py', 'tools/rewrite/verify_levels.py',
                'tools/artifacts.py')]
    source_hashes = {str(p.relative_to(ROOT)): digest(p) for p in sources}
    calls = []

    def run(command, label, timeout=60):
        log = output / f'{label}.log'
        with log.open('wb') as stream:
            result = run_bounded(list(map(str, command)), directory=output, timeout=timeout,
                                 cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT)
        calls.append({'label': label, 'command': list(map(str, command)),
                      'returncode': result.returncode, 'log_sha256': digest(log)})
        text = log.read_text(errors='replace')
        if result.returncode or 'Sanitizer:' in text or 'runtime error:' in text:
            raise RuntimeError(f'{label} failed: {text[-4000:]}')

    original = output / 'original-car-liveries'
    run(['gcc', '-m32', '-no-pie', '-O0', '-std=gnu99', '-w',
         ROOT / 'tools/reference/car_livery_fixture.c', '-o', original], 'original-build')
    expected = bytearray()
    for code in LEVELS:
        level = files[f'LEV{code}\\LEVEL.DAT']
        offsets = struct.unpack_from('<29I', level)
        sections = [level[a:b] for a, b in zip(offsets, (*offsets[1:], len(level)))]
        for name, index in (('sprites', 3), ('mesh', 17), ('definitions', 4)):
            (output / f'{name}.bin').write_bytes(sections[index])
        raw = output / 'original.bin'
        run([original, executable, output / 'sprites.bin', output / 'mesh.bin',
             output / 'definitions.bin', raw], f'{code}-original')
        data = raw.read_bytes()
        if len(data) != CASES_PER_LEVEL * CASE_WORDS * 4:
            raise ValueError('Incomplete original component inventory')
        expected.extend(data)
    softgl = WORK / 'rewrite-native/softgl/libsoftgl.a'
    sanitized = output / 'dd2_car_livery_sanitized'
    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT / 'src'),
             '-I', str(ROOT / 'deps/softgl/libsoftgl/include'), '-Wall', '-Wextra', '-Wpedantic',
             '-Wno-unused-parameter', '-Wno-unused-function', '-fno-strict-aliasing', '-ffast-math',
             '-Werror', '-Wshadow', '-Wconversion', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wformat=2', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    units = [ROOT / f'src/assets/{name}.c' for name in
             ('car_class', 'archive', 'car', 'level', 'textures', 'lz', 'mesh', 'scene', 'track', 'road')]
    units += [ROOT / 'src/physics/damage.c']
    units += [ROOT / f'src/render/{name}.c' for name in ('renderer', 'mesh_draw', 'camera')]
    run([tool('clang'), *flags, *units, ROOT / 'tests/car_livery_export.c', softgl,
         '-lm', '-o', sanitized], 'sanitizer-build', 180)
    commands = {'native': [WORK / 'rewrite-native/dd2_car_livery_export'],
                'wasm': ['node', WORK / 'rewrite-wasm/dd2_car_livery_export.js'],
                'sanitized': [sanitized]}
    binaries = [original, sanitized, softgl, *[WORK / f'rewrite-{platform}/{name}'
                for platform, names in (('native', ['dd2_car_livery_export']),
                        ('wasm', ['dd2_car_livery_export.js', 'dd2_car_livery_export.wasm']))
                for name in names]]
    binary_hashes = {str(p): digest(p) for p in binaries}
    comparisons = []
    for platform, command in commands.items():
        metadata, images = output / f'{platform}.bin', output / f'{platform}.rgba'
        run([*command, archive, metadata, images], f'{platform}-export', 180)
        actual = metadata.read_bytes()
        if actual != expected:
            offset = next((i for i, pair in enumerate(zip(actual, expected)) if pair[0] != pair[1]),
                          min(len(actual), len(expected)))
            raise ValueError(f'{platform} material word differs at {offset // 4}: '
                             f'level={offset // (CASES_PER_LEVEL * CASE_WORDS * 4)} '
                             f'case={(offset // (CASE_WORDS * 4)) % CASES_PER_LEVEL} '
                             f'field={(offset // 4) % CASE_WORDS}')
        if images.stat().st_size != len(LEVELS) * FRAMES * FRAME_BYTES:
            raise ValueError('Incomplete rendered inventory')
        comparisons.append({'platform': platform, 'material_cases': len(LEVELS) * CASES_PER_LEVEL,
                            'material_sha256': digest(metadata), 'render_sha256': digest(images)})
        print(json.dumps({'platform': platform, 'material_cases': len(LEVELS) * CASES_PER_LEVEL,
                          'pass_': True}), flush=True)
    rejected = []
    for label, offset, replacement in corruption_variants(archive):
        mutated = bytearray(archive.read_bytes())
        mutated[offset:offset + len(replacement)] = replacement
        path = output / 'corrupt-original.bin'
        path.write_bytes(mutated)
        for platform, command in commands.items():
            log = output / f'reject-{label}-{platform}.log'
            metadata, pixels = output / 'rejected.bin', output / 'rejected.rgba'
            with log.open('wb') as stream:
                result = run_bounded(list(map(str, [*command, path, metadata, pixels])),
                                     directory=output, timeout=60, cwd=ROOT,
                                     stdout=stream, stderr=subprocess.STDOUT)
            content = log.read_text(errors='replace')
            if result.returncode != 1 or metadata.stat().st_size or pixels.stat().st_size or \
                    'Sanitizer:' in content or 'runtime error:' in content:
                raise ValueError(f'{platform}: corrupted {label} was not safely rejected')
            rejected.append({'platform': platform, 'variant': label,
                             'mutated_archive_sha256': digest(path), 'returncode': result.returncode,
                             'log_sha256': digest(log)})
    render_checks = []
    with (output / 'native.rgba').open('rb') as native, (output / 'wasm.rgba').open('rb') as wasm, \
            (output / 'sanitized.rgba').open('rb') as san:
        for code in LEVELS:
            frames = []
            for index in range(FRAMES):
                frame = native.read(FRAME_BYTES)
                frames.append(frame)
                render_checks.append({'level': code, 'frame': index,
                                      'wasm': compare_pixels(frame, wasm.read(FRAME_BYTES)),
                                      'sanitized': compare_pixels(frame, san.read(FRAME_BYTES))})
            if len({hashlib.sha256(frame).digest() for frame in frames[:22]}) != 22:
                raise ValueError(f'{code}: driver/class liveries are not individually visible')
            if frames[0] == frames[22] or frames[0] != frames[23]:
                raise ValueError(f'{code}: damage/reset did not preserve owned livery/geometry')
            if code in '18':
                from PIL import Image, ImageDraw
                montage = Image.new('RGB', (WIDTH * 5, (HEIGHT + 24) * 5), '#222222')
                draw = ImageDraw.Draw(montage)
                numbers = [1, 0, 7, 13, 17, 35, 37, 40, 42, 47, 50, 52, 53, 64, 66, 69, 77, 82, 88, 99]
                for index, frame in enumerate(frames):
                    x, y = (index % 5) * WIDTH, (index // 5) * (HEIGHT + 24)
                    image = Image.frombytes('RGBA', (WIDTH, HEIGHT), frame).transpose(Image.Transpose.FLIP_TOP_BOTTOM)
                    montage.paste(image.convert('RGB'), (x, y))
                    label = f'Driver {index}: #{numbers[index]:02d}' if index < 20 else \
                            ('Amateur', 'Pro', 'Rookie damaged', 'Rookie restored')[index - 20]
                    draw.text((x + 6, y + HEIGHT + 4), label, fill='white')
                montage.save(output / f'level-{code}-liveries.png')
    if any(digest(ROOT / name) != value for name, value in source_hashes.items()):
        raise ValueError('Sources changed during verification')
    if digest(archive) != ORIGINAL_SHA256 or digest(executable) != EXE_SHA:
        raise ValueError('Original assets changed during verification')
    report = {'pass_': True, 'verified_at': datetime.now(timezone.utc).isoformat(),
              'scope': __doc__, 'original_executable_sha256': EXE_SHA,
              'original_archive_sha256': ORIGINAL_SHA256,
              'original_component': 'Unmodified Init_Car_Cluts, high-detail paint and door-number '
                                    'functions executed with bounded copied original data; all NPCs '
                                    'use high detail in this fixture, independent of original LOD.',
              'limits': 'No original pixel parity, physics/class selection, detached panels or '
                        'complete vehicle/game acceptance claim.',
              'sanitizer_scope': 'All reached rewrite C units instrumented; pinned release SoftGL uninstrumented.',
              'source_sha256': source_hashes, 'binary_sha256': binary_hashes,
              'softgl_commit': subprocess.check_output(['git', '-C', str(ROOT / 'deps/softgl'),
                                                       'rev-parse', 'HEAD'], text=True).strip(),
              'comparisons': comparisons, 'render_checks': render_checks,
              'rejections': rejected, 'calls': calls}
    (output / 'verification-report.json').write_text(json.dumps(report, indent=2) + '\n')
    opened = open_files()
    removed = 0
    for path in output.iterdir():
        if path.suffix in ('.bin', '.rgba', '.log') or path in (original, sanitized):
            stat = path.stat()
            if (stat.st_dev, stat.st_ino) in opened:
                raise ValueError('Successful raw output is still open')
            removed += stat.st_size
            path.unlink()
    (output / 'cleanup-report.json').write_text(json.dumps({'removed_raw_bytes': removed,
            'visual_review_pending': ['level-1-liveries.png', 'level-8-liveries.png']}, indent=2) + '\n')
    check_space(output)
    print(json.dumps({'pass_': True, 'cases_per_target': len(LEVELS) * CASES_PER_LEVEL,
                      'images_per_target': len(LEVELS) * FRAMES,
                      'report': str(output / 'verification-report.json')}))


if __name__ == '__main__':
    main()
