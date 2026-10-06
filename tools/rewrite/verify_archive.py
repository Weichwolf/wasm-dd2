#!/usr/bin/env python3
"""Check the C archive decoder against every provisioned original asset."""
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

ORIGINAL_SHA256 = '03c6ca7adc5e616a4784a82f3b1b7a1f85489e97318b7e01f867e65504d5f22b'


def fingerprint(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return f'{value:016x}'


def reference(data):
    entries = []
    for packed_name, sector, size in struct.iter_unpack('<18sHI', data[:0x2808]):
        if packed_name[0] == 0:
            break
        name = packed_name.split(b'\0', 1)[0].decode('ascii')
        offset = sector * 2048
        payload = data[offset:offset + size]
        if len(payload) != size:
            raise ValueError('Original payload truncated')
        entries.append(dict(name=name, offset=offset, size=size,
                            fnv1a64=fingerprint(payload)))
    if len(entries) != 114:
        raise ValueError('Expected all 114 original entries')
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path, default=ROOT / 'DestructionDerby2')
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-archive-verification')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (args.game_dir / 'Dirinfo').resolve()
    data = archive.read_bytes()
    if hashlib.sha256(data).hexdigest() != ORIGINAL_SHA256:
        raise ValueError('Provision the supported unmodified original Dirinfo')
    expected = reference(data)
    calls = []

    def run(command, label, success=True):
        log = output / (label + '.log')
        with log.open('w') as stream:
            result = run_bounded(command, directory=output, timeout=60,
                                 stdout=stream, stderr=subprocess.STDOUT, cwd=ROOT)
        calls.append(dict(label=label, command=list(map(str, command)),
                          returncode=result.returncode,
                          output_sha256=hashlib.sha256(log.read_bytes()).hexdigest()))
        content = log.read_text()
        if result.returncode != (0 if success else 1) or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(f'{label} unexpected exit {result.returncode}: {log.read_text()}')
        if not success and content:
            raise RuntimeError(label + ' produced unexpected diagnostics during rejection')
        return log

    sanitized = output / 'dd2_archive_sanitized'
    run([tool('clang'), '-std=c11', '-O1', '-g', '-I', str(ROOT / 'src'),
         '-Wall', '-Wextra', '-Wpedantic', '-Wno-unused-parameter', '-Wno-unused-function',
         '-fno-strict-aliasing', '-ffast-math', '-Werror', '-Wshadow', '-Wconversion',
         '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wformat=2',
         '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
         str(ROOT / 'src/assets/archive.c'), str(ROOT / 'tests/rewrite/archive_test.c'),
         '-o', str(sanitized)], 'sanitizer-build')
    commands = {
        'native': [str(WORK / 'rewrite-native/dd2_archive_test')],
        'wasm': ['node', str(WORK / 'rewrite-wasm/dd2_archive_test.js')],
        'sanitized': [str(sanitized)],
    }
    for platform, command in commands.items():
        run(command, platform + '-bounds')
        inventory = run([*command, str(archive)], platform + '-original')
        if json.loads(inventory.read_text()) != expected:
            raise ValueError(platform + ' differs from independent original inventory')

    mutated = output / 'mutated-archive.bin'
    variants = []
    for label, index, field, replacement in (
        ('name-without-terminator', 0, 0, b'A' * 18),
        ('invalid-name', 113, 0, b'\xff'),
        ('directory-payload', 0, 18, struct.pack('<H', 1)),
        ('offset-outside-file', 113, 18, struct.pack('<H', 65535)),
        ('length-overflow', 113, 20, struct.pack('<I', 0xffffffff)),
        ('duplicate-name', 1, 0, data[:18]),
    ):
        changed = bytearray(data)
        at = index * 24 + field
        changed[at:at + len(replacement)] = replacement
        variants.append((label, changed))
    last = expected[-1]
    variants.append(('truncated-last-payload', data[:last['offset'] + last['size'] - 1]))
    variants.append(('truncated-directory', data[:0x2807]))
    for label, changed in variants:
        mutated.write_bytes(changed)
        for platform, command in commands.items():
            run([*command, str(mutated)], platform + '-' + label, success=False)
    # Sector padding is not asset data. A loader must accept an otherwise complete
    # archive ending exactly at the final declared byte.
    mutated.write_bytes(data[:last['offset'] + last['size']])
    for platform, command in commands.items():
        inventory = run([*command, str(mutated)], platform + '-without-tail-padding')
        if json.loads(inventory.read_text()) != expected:
            raise ValueError(platform + ' depends on unnecessary trailing sector padding')

    sources = ['src/assets/archive.c', 'src/assets/archive.h',
               'tests/rewrite/archive_test.c', 'tools/rewrite/verify_archive.py', 'CMakeLists.txt',
               'Makefile', '.clang-tidy', '.clang-format']
    binaries = [WORK / 'rewrite-native/dd2_archive_test',
                WORK / 'rewrite-wasm/dd2_archive_test.js',
                WORK / 'rewrite-wasm/dd2_archive_test.wasm', sanitized]
    report = dict(pass_=True, scope='original archive access only; asset formats/gameplay pending',
                  verified_at=datetime.now(timezone.utc).isoformat(), original_sha256=ORIGINAL_SHA256,
                  comparison='exact names, offsets, lengths and full-payload FNV-1a64',
                  entries=expected, payload_bytes=sum(e['size'] for e in expected),
                  platforms=list(commands), rejection_variants=len(variants), calls=calls,
                  binary_sha256={str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in binaries},
                  source_sha256={p: hashlib.sha256((ROOT / p).read_bytes()).hexdigest()
                                 for p in sources})
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    # Every producer above has terminated. Keep compact evidence, remove all raw
    # original/mutation inventories, test copies and completed logs.
    mutated.unlink()
    sanitized.unlink()
    for call in calls:
        (output / (call['label'] + '.log')).unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, entries=len(expected), platforms=list(commands),
                          report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
