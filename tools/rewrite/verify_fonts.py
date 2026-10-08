#!/usr/bin/env python3
"""Verify owned font tables on Native/WASM/ASan and optionally live original Wine.

The original-only observer reads six small glyph tables after normal frontend
startup. No engine writes, debugger, full memory capture or rendering parity.
"""
import argparse
from datetime import datetime, timezone
import fcntl
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, open_files, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.reference import prepare_snapshot, REVISION
from rewrite.verify_archive import ORIGINAL_SHA256


def digest(data):
    return hashlib.sha256(data).hexdigest()


def original_font(archive):
    for offset in range(0, 0x2800, 24):
        name, sector, size = struct.unpack_from('<18sHI', archive, offset)
        name = name.split(b'\0', 1)[0]
        if not name:
            break
        if name == b'LEV0\\FONT.BNK':
            bank = archive[sector * 2048:sector * 2048 + size]
            if len(bank) != size:
                raise ValueError('Original font asset truncated')
            return bank
    raise ValueError('Original font asset missing')


def expected_tables(bank):
    count, = struct.unpack_from('<I', bank, 12)
    if count != 3 or len(bank) != 23964:
        raise ValueError('Expected supported original three-font bank')
    cursor = 16
    fonts, records = [], []
    for index in range(count):
        stride, = struct.unpack_from('<I', bank, cursor)
        fonts.append([list(struct.unpack_from('<4B', bank, cursor + 16 + 4 * glyph))
                      for glyph in range(96)])
        records.append(dict(font=index, offset=cursor, stride=stride))
        cursor += stride
    if cursor != len(bank) + 4:
        raise ValueError('Original final unused stride word differs')
    return dict(fonts=fonts), records


def observe_original(game, output, tables, report):
    # Import only verified frozen original-observation commands from /tmp.
    reference = prepare_snapshot()
    # The script directory also contains rewrite/reference.py. Remove that
    # search entry so it cannot shadow the frozen reference namespace package.
    sys.path[:] = [entry for entry in sys.path
                   if Path(entry).resolve() != Path(__file__).resolve().parent]
    sys.path.insert(0, str(reference / 'tools'))
    sys.path.insert(0, str(reference / 'tools/reference'))
    from reference import capture
    from original_realtime_ui import OriginalRealtimeUI
    from verify_configuration_persistence import original_args

    source_names = ('tools/reference/capture.py', 'tools/original_realtime_ui.py',
                    'tools/verify_configuration_persistence.py', 're_out/dd2.c')
    observation = dict(reference_commit=REVISION, engine_state_writes=False,
                       scope='Six immutable loaded original glyph tables at normal frontend startup',
                       reference_source_sha256={p: digest((reference / p).read_bytes())
                                                for p in source_names},
                       executable_sha256=digest((game / 'dd2h.exe').read_bytes()),
                       original_save_before=digest((game / 'SaveGames').read_bytes()))
    report['original'] = observation
    destination = output / 'original'
    destination.mkdir()
    glyph_tables = [bytes(byte for glyph in font for byte in glyph) for font in tables['fonts']]
    sources = [0, 1, 2, 0, 1, 1]

    def driver(pid, capture_output, env, deadline, rundir):
        ui = OriginalRealtimeUI(pid, rundir, capture_output, env, deadline)
        try:
            ui.settled()
            observation['isolated_save_before'] = digest((rundir / 'SaveGames').read_bytes())
            pointers = ui.read(0x7543d0, len(sources) * 4)
            actual = [ui.read(pointer, 96 * 4)
                      for pointer in struct.unpack('<6I', pointers)]
            if actual != [glyph_tables[index] for index in sources]:
                raise ValueError('Loaded original glyph tables differ from bank/duplicates')
            if pointers != ui.read(0x7543d0, len(sources) * 4):
                raise ValueError('Original font ownership changed during observation')
            observation.update(every_byte_matches=True, fonts=len(actual), glyphs=6 * 96,
                               bytes=sum(map(len, actual)), table_source_indices=sources,
                               table_sha256=[digest(table) for table in actual],
                               menu=hex(ui.integer(0x940010)),
                               input_history=ui.input_history,
                               isolated_save_after=digest((rundir / 'SaveGames').read_bytes()))
        finally:
            ui.stop()

    options = original_args()
    options.timeout = 90
    capture.WORK.mkdir(parents=True, exist_ok=True)
    with (capture.WORK / 'capture.lock').open('w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        capture.run(game, destination, options, on_menu=driver)
    observation['original_save_after'] = digest((game / 'SaveGames').read_bytes())
    if (observation['original_save_before'] != observation['original_save_after'] or
            observation['isolated_save_before'] != observation['isolated_save_after']):
        raise ValueError('Original font observation changed a save file')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path, default=ROOT / 'DestructionDerby2')
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-font-verification')
    parser.add_argument('--original', action='store_true', help='Observe loaded original under Wine')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=False)
    check_space(output)
    archive = (args.game_dir / 'Dirinfo').read_bytes()
    if digest(archive) != ORIGINAL_SHA256:
        raise ValueError('Provision the supported unmodified original Dirinfo')
    bank = original_font(archive)
    tables, records = expected_tables(bank)
    report = dict(pass_=False, scope='Owned original font glyph decoding only; text rendering/menus pending',
                  verified_at=datetime.now(timezone.utc).isoformat(),
                  original_archive_sha256=ORIGINAL_SHA256, original_font_sha256=digest(bank),
                  font_records=records, tables=tables, calls=[],
                  original_final_unused_stride_overrun_bytes=4)
    sources = ['src/assets/font.c', 'src/assets/font.h', 'src/assets/bytes.h',
               'tests/font_test.c', 'tests/asset_fixture.h', 'tools/rewrite/verify_fonts.py',
               'CMakeLists.txt', 'Makefile', '.clang-format', '.clang-tidy']
    report['source_sha256'] = {name: digest((ROOT / name).read_bytes()) for name in sources}

    def run(command, label, accepted=True):
        log = output / (label + '.log')
        started = time.monotonic()
        with log.open('w') as stream:
            result = run_bounded(command, directory=output, timeout=60, cwd=ROOT,
                                 stdout=stream, stderr=subprocess.STDOUT)
        content = log.read_text()
        report['calls'].append(dict(label=label, command=list(map(str, command)),
                                    returncode=result.returncode, timed_out=False,
                                    elapsed_seconds=time.monotonic() - started,
                                    stdout_sha256=digest(log.read_bytes())))
        if result.returncode != (0 if accepted else 1):
            raise RuntimeError(f'{label} exit {result.returncode}: {content}')
        if 'Sanitizer:' in content or 'runtime error:' in content:
            raise RuntimeError(label + ' sanitizer diagnostic')
        if not accepted and content:
            raise RuntimeError(label + ' unexpected output on rejected input')
        return content

    sanitized = output / 'font-sanitized'
    fixture = output / 'font.bin'
    try:
        run([tool('clang'), '-std=c11', '-O1', '-g', '-Isrc',
             '-Wall', '-Wextra', '-Wpedantic', '-Wno-unused-parameter', '-Wno-unused-function',
             '-fno-strict-aliasing', '-ffast-math', '-Werror', '-Wshadow', '-Wconversion',
             '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wformat=2',
             '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
             'src/assets/font.c', 'tests/font_test.c', '-o', str(sanitized)], 'sanitizer-build')
        commands = dict(native=[str(WORK / 'rewrite-native/dd2_font_test')],
                        wasm=['node', str(WORK / 'rewrite-wasm/dd2_font_test.js')],
                        sanitized=[str(sanitized)])
        fixture.write_bytes(bank)
        for target, command in commands.items():
            if not json.loads(run(command, target + '-bounds'))['pass']:
                raise ValueError(target + ' owned/lifetime bounds failed')
            if json.loads(run([*command, str(fixture)], target + '-original')) != tables:
                raise ValueError(target + ' differs from complete original font tables')
        variants = []

        def change(label, offset, value):
            altered = bytearray(bank)
            struct.pack_into('<I', altered, offset, value)
            variants.append((label, altered))

        change('magic', 4, 0)
        change('declared-size', 8, len(bank) + 1)
        change('font-count', 12, 0xffffffff)
        change('empty-with-payload', 12, 0)
        change('first-zero-stride', records[0]['offset'], 0)
        change('middle-short-stride', records[1]['offset'], 399)
        change('first-overflow-stride', records[0]['offset'], 0xffffffff)
        change('last-overflow-stride', records[-1]['offset'], 0xffffffff)
        for name, coordinate in (('glyph-u-overflow', 0), ('glyph-v-overflow', 1)):
            altered = bytearray(bank)
            altered[records[-1]['offset'] + 16 + (ord('A') - 32) * 4 + coordinate] = 255
            variants.append((name, altered))
        for name, length in (('truncated-glyph', records[-1]['offset'] + 16 + 383),
                             ('truncated-last-tail', len(bank) - 1)):
            altered = bytearray(bank[:length])
            struct.pack_into('<I', altered, 8, len(altered))
            variants.append((name, altered))
        for label, altered in variants:
            fixture.write_bytes(altered)
            for target, command in commands.items():
                run([*command, str(fixture)], target + '-' + label, accepted=False)
        # The missing final word is unused. Explicit padding and changes to
        # unrelated bitmap payload must leave all decoded glyphs unchanged.
        padded = bytearray(bank + bytes(4))
        struct.pack_into('<I', padded, 8, len(padded))
        padded[records[0]['offset'] + 400] ^= 0xff
        fixture.write_bytes(padded)
        for target, command in commands.items():
            if json.loads(run([*command, str(fixture)], target + '-explicit-padding')) != tables:
                raise ValueError(target + ' depends on unused payload/padding')
        if args.original:
            observe_original(args.game_dir.resolve(), output, tables, report)
        binaries = [WORK / 'rewrite-native/dd2_font_test',
                    WORK / 'rewrite-wasm/dd2_font_test.js',
                    WORK / 'rewrite-wasm/dd2_font_test.wasm', sanitized]
        report.update(pass_=True, fonts=3, glyphs_per_target=288,
                      every_decoded_byte_matches=True, platforms=list(commands),
                      rejection_variants=len(variants),
                      binary_sha256={str(p): digest(p.read_bytes()) for p in binaries})
    finally:
        raw = [p for p in output.rglob('*.log') if p.is_file() and not p.is_symlink()]
        report['raw_log_sha256'] = {str(p.relative_to(output)): digest(p.read_bytes()) for p in raw}
        (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        if report['pass_']:
            opened = open_files()
            for path in [fixture, sanitized, *raw]:
                stat = path.stat()
                if (stat.st_dev, stat.st_ino) in opened:
                    raise RuntimeError('Completed font producer still owns ' + str(path))
                path.unlink()
        check_space(output)
    print(json.dumps(dict(pass_=True, fonts=3, glyphs_per_target=288,
                          original_observed=args.original, report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
