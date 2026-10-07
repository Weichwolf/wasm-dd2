#!/usr/bin/env python3
"""Check all 45 provisioned effects and 18 Redbook PCM tracks on three targets.

Python's WAVE reader independently supplies effect metadata and signed sample
conversion. Provisioned Redbook bytes supply complete stereo 16-bit PCM outputs.
These are asset-reader checks, not mixing, audible playback or transport parity.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import sys
import wave

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.verify_archive import ORIGINAL_SHA256
from rewrite.verify_levels import assets


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def equal_files(actual, expected):
    with actual.open('rb') as left, expected.open('rb') as right:
        while True:
            left_bytes, right_bytes = left.read(1024 * 1024), right.read(1024 * 1024)
            if left_bytes != right_bytes:
                raise ValueError('Decoded PCM bytes differ: ' + str(actual))
            if not left_bytes:
                return


def effect_reference(bank):
    count = struct.unpack_from('<I', bank, 12)[0]
    if count != 45:
        raise ValueError('Unsupported original effect count')
    metadata, pcm = [], bytearray()
    for index in range(count):
        offset, length, loop, frequency, flags, _, _ = struct.unpack_from('<7I', bank, 16 + index * 28)
        with wave.open(io.BytesIO(bank[offset:offset + length]), 'rb') as stream:
            if stream.getsampwidth() != 1:
                raise ValueError('Unsupported original WAVE sample width')
            samples = stream.readframes(stream.getnframes())
            pcm.extend(struct.pack('<' + str(len(samples)) + 'h', *[(value - 128) * 256 for value in samples]))
            metadata.append(dict(index=index, frames=stream.getnframes(), rate=stream.getframerate(),
                                 frequency=frequency, channels=stream.getnchannels(), bits=8,
                                 loop=int(bool(loop)), flags=flags))
    return metadata, pcm


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-audio-assets-verification')
    parser.add_argument('--native-build', type=Path, default=WORK / 'rewrite-native')
    parser.add_argument('--wasm-build', type=Path, default=WORK / 'rewrite-wasm')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    archive = (ROOT / 'DestructionDerby2/Dirinfo').resolve()
    if digest(archive) != ORIGINAL_SHA256:
        raise ValueError('Provision the supported unmodified original archive')
    bank = assets(archive.read_bytes())['VAGS\\BANK1.SBK']
    expected_metadata, expected_pcm = effect_reference(bank)
    bank_path = output / 'bank.sbk'
    expected_path = output / 'effects-expected.pcm'
    bank_path.write_bytes(bank)
    expected_path.write_bytes(expected_pcm)
    redbook = (ROOT / 'DestructionDerby2/Redbook').resolve()
    disc_path = redbook / 'disc.json'
    disc = json.loads(disc_path.read_text())
    tracks = [track for track in disc['tracks'] if track['type'] == 'AUDIO']
    if [track['number'] for track in tracks] != list(range(2, 20)) or \
            (disc['sample_rate'], disc['channels'], disc['bits_per_sample']) != (44100, 2, 16):
        raise ValueError('Unsupported provisioned Redbook inventory')
    calls = []

    def run(command, label):
        log = output / (label + '.log')
        with log.open('wb') as stream:
            result = run_bounded(command, directory=output, cwd=ROOT, timeout=180,
                                 stdout=stream, stderr=subprocess.STDOUT)
        content = log.read_text(errors='replace')
        calls.append(dict(label=label, returncode=result.returncode, sha256=digest(log)))
        if result.returncode or 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label + ' failed: ' + content[-6000:])
        rows = [json.loads(line) for line in content.splitlines() if line.startswith('{')]
        log.unlink()
        return rows

    flags = ['-std=c11', '-O1', '-g', '-I', str(ROOT / 'src'), '-Wall', '-Wextra', '-Wpedantic',
             '-Wno-unused-parameter', '-Wno-unused-function', '-fno-strict-aliasing', '-ffast-math',
             '-Werror', '-Wshadow', '-Wconversion', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wformat=2', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    sanitized = output / 'export-sanitized'
    sanitized_test = output / 'test-sanitized'
    for fixture, binary in (('audio_export', sanitized), ('audio_test', sanitized_test)):
        units = [ROOT / 'src/assets/audio.c', ROOT / ('tests/rewrite/' + fixture + '.c')]
        if fixture == 'audio_export':
            units.append(ROOT / 'src/platform/file.c')
        run([tool('clang'), *flags, *map(str, units), '-o', str(binary)], fixture + '-build')
    run([str(sanitized_test)], 'synthetic-sanitized')
    commands = {'native': [str(args.native_build / 'dd2_audio_export')],
                'wasm': ['node', str(args.wasm_build / 'dd2_audio_export.js')],
                'sanitized': [str(sanitized)]}
    effects = {}
    for target, command in commands.items():
        actual = output / (target + '.pcm')
        rows = run([*command, 'bank', str(bank_path), str(actual)], 'effects-' + target)
        if rows != expected_metadata:
            raise ValueError('Original effect metadata differs: ' + target)
        equal_files(actual, expected_path)
        effects[target] = dict(pass_=True, effects=45, pcm_bytes=actual.stat().st_size,
                               pcm_sha256=digest(actual))
        actual.unlink()
    bank_path.unlink()
    expected_path.unlink()
    results = []
    for track in tracks:
        path = redbook / f"track{track['number']:02d}.cdda"
        if path.name != track['file'] or digest(path) != track['sha256'] or \
                path.stat().st_size != track['sample_frames'] * 4 or \
                track['sample_frames'] * 4 != (track['end_sector'] - track['start_sector']) * 2352:
            raise ValueError('Provisioned Redbook track differs: ' + str(path))
        expected = dict(frames=track['sample_frames'], rate=44100, channels=2, bits=16)
        targets = {}
        for target, command in commands.items():
            actual = output / (target + '.pcm')
            rows = run([*command, 'cdda', str(path), str(actual)], f"track{track['number']:02d}-" + target)
            if rows != [expected]:
                raise ValueError('Redbook PCM metadata differs: ' + target)
            equal_files(actual, path)
            targets[target] = dict(pass_=True, pcm_bytes=actual.stat().st_size, pcm_sha256=digest(actual))
            actual.unlink()
        results.append(dict(number=track['number'], metadata=expected, targets=targets))
        print(json.dumps(dict(track=track['number'], pass_=True)), flush=True)
    sources = [ROOT / name for name in ('src/assets/audio.c', 'src/assets/audio.h', 'src/assets/bytes.h',
               'src/platform/file.c', 'tests/rewrite/audio_test.c', 'tests/rewrite/audio_export.c', 'CMakeLists.txt')]
    sources.append(Path(__file__).resolve())
    binaries = [args.native_build / 'dd2_audio_export', args.wasm_build / 'dd2_audio_export.wasm', sanitized, sanitized_test]
    report = dict(pass_=True, scope=__doc__.strip(), verified_at=datetime.now(timezone.utc).isoformat(),
                  original_sha256=ORIGINAL_SHA256, bank_sha256=hashlib.sha256(bank).hexdigest(),
                  disc_manifest_sha256=digest(disc_path), effects=effects, effect_metadata=expected_metadata,
                  redbook=results, calls=calls, source_sha256={str(p.relative_to(ROOT)):digest(p) for p in sources},
                  binary_sha256={str(p):digest(p) for p in binaries})
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    sanitized.unlink()
    sanitized_test.unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, effects=45, redbook_tracks=18, report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
