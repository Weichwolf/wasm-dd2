#!/usr/bin/env python3
"""Check typed profiles against immutable original-x86 packs and physical cards.

All fields are independently decoded, and full-block edits are independently
predicted. This checks the codec, not durable persistence or resumed gameplay.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from verify_season_transition import EXE_SHA

BLOCK_BYTES = 8192
PROFILE_BYTES = 6526
HEADER = ('kind', 'race_mode', 'race_type', 'car', 'track', 'unlocked_circuits',
          'unlocked_arenas', 'view_distance', 'effects_volume', 'controller_type',
          'controller_option', 'quick_race_type', 'statistics_recorded',
          'player_car', 'player', 'simultaneous_players', 'race', 'difficulty',
          'races', 'players', 'cars', 'active_players', 'recording_season', 'season_number')


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def decode(data):
    if len(data) != BLOCK_BYTES:
        raise ValueError('Expected one complete physical block')
    header = dict(zip(HEADER, struct.unpack_from('<24h', data)))
    if header['kind'] not in (0x1010, 0x1020, 0x3030):
        raise ValueError('Unsupported profile kind')
    seasons = []
    for season in range(5):
        base = 48 + season*940
        tracks = [[data[base+i*20:base+i*20+16].hex(),
                   *struct.unpack_from('<2H', data, base+i*20+16)] for i in range(11)]
        drivers = [[data[base+220+i*20:base+220+i*20+16].hex(),
                    *struct.unpack_from('<BBH', data, base+220+i*20+16)] for i in range(20)]
        standings = [data[base+620+i*16:base+620+(i+1)*16].hex() for i in range(20)]
        seasons.append(dict(tracks=tracks, drivers=drivers, standings=standings))
    drivers = [[data[4748+i*54:4764+i*54].hex(),
                *struct.unpack_from('<7h', data, 4764+i*54),
                data[4778+i*54:4802+i*54].hex()] for i in range(20)]
    players = [data[5828+i*12:5828+(i+1)*12].hex() for i in range(10)]
    laps = [[[data[5948+c*80+i*16:5958+c*80+i*16].hex(),
              *struct.unpack_from('<3H', data, 5958+c*80+i*16)] for i in range(5)]
            for c in range(7)]
    return dict(header=header, seasons=seasons, drivers=drivers, players=players,
                laps=laps, bindings=data[6508:PROFILE_BYTES].hex(),
                reserved=data[PROFILE_BYTES:].hex())


def edited(data):
    result = bytearray(data)
    struct.pack_into('<h', result, 16, -1234)
    base = 48+4*940+10*20
    result[base] = ord('Q')
    struct.pack_into('<2H', result, base+16, 65535, 0x8123)
    struct.pack_into('<BBH', result, 48+3*940+220+19*20+16, 255, 254, 0x8abc)
    result[48+2*940+620+7*16] = ord('S')
    struct.pack_into('<7h', result, 4748+19*54+16, -19, 3, 4, 42, 49, 20, 30)
    result[4748+19*54+30] = 0x5a
    result[5828+9*12] = ord('P')
    result[5948+6*80+4*16] = ord('L')
    struct.pack_into('<3H', result, 5958+6*80+4*16, 2, 59, 0x8123)
    result[6525] = 0xf5
    result[-1] = 0xab
    return bytes(result)


def values(value):
    if isinstance(value, dict):
        return sum(values(item) for item in value.values())
    if isinstance(value, list):
        return sum(values(item) for item in value)
    return 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK/'rewrite-save-profile-verification')
    parser.add_argument('--original-cards', type=Path,
                        help='Actual observed saved-A-B.card and its complete report.json')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Use /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    exe = ROOT/'DestructionDerby2/dd2h.exe'
    if digest(exe) != EXE_SHA:
        raise ValueError('Provision the supported unmodified original executable')
    files = [ROOT/name for name in ('src/assets/save_profile.c', 'src/assets/save_profile.h',
             'src/assets/save_card.h', 'src/assets/bytes.h', 'src/platform/file.c',
             'tests/save_profile_test.c', 'tests/save_profile_export.c',
             'tools/reference/save_profile_fixture.c', 'tools/reference/pe_fixture.h',
             'CMakeLists.txt', 'Makefile')]
    files.append(Path(__file__).resolve())
    sources = {str(path.relative_to(ROOT)): digest(path) for path in files}
    calls = []
    temporary = []

    def run(command, label):
        log = output/(label+'.log')
        with log.open('wb') as stream:
            result = run_bounded(command, directory=output, cwd=ROOT, timeout=120,
                                 stdout=stream, stderr=subprocess.STDOUT)
        text = log.read_text(errors='replace')
        calls.append(dict(label=label, command=command, returncode=result.returncode,
                          log_sha256=digest(log)))
        if result.returncode or 'Sanitizer:' in text or 'runtime error:' in text:
            raise RuntimeError(label+' failed: '+text[-4000:])
        temporary.append(log)
        return text

    original = output/'original-x86'
    raw = output/'original-packs.bin'
    run(['gcc', '-m32', '-no-pie', '-O0', '-std=gnu99', '-w',
         str(ROOT/'tools/reference/save_profile_fixture.c'), '-o', str(original)], 'original-build')
    run([str(original), str(exe), str(raw)], 'original-run')
    data = raw.read_bytes()
    if len(data) != 48*BLOCK_BYTES:
        raise ValueError('Incomplete original-x86 fixture')
    fixtures = [(f'original-pack-{case:02d}', data[case*BLOCK_BYTES:(case+1)*BLOCK_BYTES])
                for case in range(48)]
    original_output_sha256 = digest(raw)
    temporary.extend([original, raw])
    observed = None
    observed_sha256 = None
    if args.original_cards is not None:
        receipt = args.original_cards/'report.json'
        report = json.loads(receipt.read_text())
        image = args.original_cards/'saved-A-B.card'
        observed_sha256 = digest(image)
        if not report['complete'] or report['exe_sha256'] != EXE_SHA or \
                report['original_save_before'] != report['original_save_after'] or \
                not any(row['label'] == 'saved-A-B' and row['sha256'] == observed_sha256
                        for row in report['cards']):
            raise ValueError('Actual-original observation identity differs')
        card = image.read_bytes()
        if len(card) != 131072:
            raise ValueError('Incomplete actual original card')
        for slot, name in enumerate(('A', 'B')):
            if struct.unpack_from('<I', card, slot*512)[0] != 1 or \
                    card[slot*512+4:slot*512+6] != name.encode()+b'\0':
                raise ValueError('Actual original physical identity differs')
            fixtures.append(('observed-'+name, card[(slot+1)*BLOCK_BYTES:(slot+2)*BLOCK_BYTES]))
        observed = dict(report_sha256=digest(receipt), card_sha256=observed_sha256,
                        scope=report['scope'], physical_slots=[0, 1])
    flags = ['-std=c11', '-O1', '-g', '-I'+str(ROOT/'src'), '-Wall', '-Wextra', '-Wpedantic',
             '-Wno-unused-parameter', '-Wno-unused-function', '-fno-strict-aliasing', '-ffast-math',
             '-Werror', '-Wshadow', '-Wconversion', '-Wstrict-prototypes', '-Wmissing-prototypes',
             '-Wformat=2', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
    for name in ('test', 'export'):
        units = [str(ROOT/'src/assets/save_profile.c'), str(ROOT/f'tests/save_profile_{name}.c')]
        if name == 'export':
            units.append(str(ROOT/'src/platform/file.c'))
        binary = output/(name+'-sanitized')
        run([tool('clang'), *flags, *units, '-o', str(binary)], 'build-'+name+'-sanitized')
        temporary.append(binary)
    tests = {'native': [str(WORK/'rewrite-native/dd2_save_profile_test')],
             'wasm': ['node', str(WORK/'rewrite-wasm/dd2_save_profile_test.js')],
             'sanitized': [str(output/'test-sanitized')]}
    exporters = {'native': [str(WORK/'rewrite-native/dd2_save_profile_export')],
                 'wasm': ['node', str(WORK/'rewrite-wasm/dd2_save_profile_export.js')],
                 'sanitized': [str(output/'export-sanitized')]}
    binaries = {command[-1]: digest(Path(command[-1])) for command in
                [*tests.values(), *exporters.values()]}
    for path in (WORK/'rewrite-wasm').glob('dd2_save_profile_*.wasm'):
        binaries[str(path)] = digest(path)
    comparisons = []
    for target, command in exporters.items():
        run(tests[target], 'synthetic-'+target)
        for label, block in fixtures:
            fixture = output/(label+'.block')
            fixture.write_bytes(block)
            temporary.append(fixture)
            for operation, expected in (('inspect', block), ('edit', edited(block))):
                result = output/(label+'-'+operation+'-'+target+'.block')
                actual = json.loads(run([*command, str(fixture), str(result), operation],
                                        label+'-'+operation+'-'+target))
                expected_fields = decode(expected)
                if actual != expected_fields or result.read_bytes() != expected:
                    raise ValueError('Independent typed fields or full encoded block differ')
                comparisons.append(dict(target=target, case=label, operation=operation,
                                        checked_values=values(expected_fields), bytes=BLOCK_BYTES,
                                        input_sha256=hashlib.sha256(block).hexdigest(),
                                        output_sha256=digest(result), pass_=True))
                temporary.append(result)
    memcheck = run(['valgrind', '--error-exitcode=99', '--leak-check=full',
                    '--show-leak-kinds=all', '--errors-for-leak-kinds=all',
                    str(WORK/'rewrite-native/dd2_save_profile_test')], 'memcheck')
    if 'ERROR SUMMARY: 0 errors' not in memcheck or 'no leaks are possible' not in memcheck:
        raise ValueError('Memcheck did not prove zero errors/leaks')
    if {str(path.relative_to(ROOT)): digest(path) for path in files} != sources or \
            digest(exe) != EXE_SHA or \
            (args.original_cards is not None and
             digest(args.original_cards/'saved-A-B.card') != observed_sha256):
        raise ValueError('Source/original identity changed during verification')
    report = dict(scope=__doc__, source_sha256=sources, binary_sha256=binaries,
                  original_exe_sha256=EXE_SHA, original_pack_sha256=original_output_sha256,
                  original_observed=observed, fixture_count=len(fixtures), calls=calls,
                  comparisons=comparisons, comparison_count=len(comparisons),
                  checked_values=sum(row['checked_values'] for row in comparisons),
                  memcheck_zero_errors_and_leaks=True, complete=True)
    (output/'verification-report.json').write_text(json.dumps(report, indent=2)+'\n')
    removed = []
    for path in set(temporary):
        if path.exists():
            removed.append(dict(path=str(path), bytes=path.stat().st_size, sha256=digest(path)))
            path.unlink()
    (output/'cleanup-report.json').write_text(json.dumps(dict(removed=removed), indent=2)+'\n')
    print(json.dumps(dict(complete=True, fixtures=len(fixtures), comparisons=len(comparisons),
                         checked_values=report['checked_values'])))


if __name__ == '__main__':
    main()
