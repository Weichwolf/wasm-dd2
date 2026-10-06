#!/usr/bin/env python3
"""Check PAL channel order against a real unmodified original race under Wine.

Reference-only absolute addresses identify small read-only observations. The
rewrite runtime never depends on them. Real X11 input starts a normal race;
no debugger stops, full memory images or framebuffer parity are involved.
"""
import argparse
from datetime import datetime, timezone
import fcntl
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from artifacts import WORK, check_space, open_files, prepare_output
from original_realtime_ui import OriginalRealtimeUI
from reference.capture import EXE_SHA256
from rewrite.verify_archive import ORIGINAL_SHA256
from rewrite.verify_levels import assets
from rewrite.verify_meshes import decompress
from verify_configuration_persistence import WINE_WORK, original_args, run_original


def digest(data):
    return hashlib.sha256(data).hexdigest()


def scene_origins(ui, files, code):
    level = files[f'LEV{code}\\LEVEL.DAT']
    start, end = struct.unpack_from('<2I', level)
    section = level[start:end]
    count = struct.unpack_from('<I', section)[0] // 4
    offsets = struct.unpack_from(f'<{count}I', section)
    blocks = [decompress(section[start:end]) if code in '1234567' else section[start:end]
              for start, end in zip(offsets, (*offsets[1:], len(section)))]
    active = ui.read(0x7892a0, 14 * 4)
    counts = struct.unpack('<14I', ui.read(0x7892d8, 14 * 4))
    actual_origins, expected_origins = bytearray(), bytearray()
    observed = []
    for slot, block in enumerate(struct.unpack('<14i', active)):
        if block < 0:
            continue
        if block >= len(blocks) or counts[slot] > 32:
            raise ValueError('Original active scene block/count outside bounds')
        data = blocks[block]
        source_count, = struct.unpack_from('<I', data)
        if counts[slot] != source_count:
            raise ValueError('Loaded scene count differs from source block')
        origins = ui.read(0x7876a0 + slot * 0x200, source_count * 16)
        for index in range(source_count):
            shape, *position = struct.unpack_from('<I3i', data, 4 + index * 16)
            flags = data[shape + 4]
            expected = position if flags & 128 else [(axis & -32768) + 16384 for axis in position]
            actual = origins[index * 16:index * 16 + 12]
            encoded = struct.pack('<3i', *expected)
            if actual != encoded:
                raise ValueError(f'Loaded original vertex origin differs: block {block}, object {index}; '
                                 f'actual {struct.unpack("<3i", actual)}, expected {expected}')
            actual_origins.extend(actual)
            expected_origins.extend(encoded)
        observed.append(dict(slot=slot, block=block, objects=source_count))
    if active != ui.read(0x7892a0, 14 * 4):
        raise ValueError('Active scene changed during bounded non-atomic observation')
    if not observed or not actual_origins:
        raise ValueError('No loaded original scene objects were observed')
    return dict(scope='Loaded original scene vertex origins, not rendered-frame parity',
                objects=len(actual_origins) // 12, blocks=observed,
                every_origin_matches=True, actual_sha256=digest(actual_origins),
                expected_sha256=digest(expected_origins))


def observe(ui, files):
    for key in ('Down', 'Down', 'Return'):
        ui.key(key)
    ui.wait(lambda: 1 <= ui.integer(0x936ff4) <= 11 and
            ui.integer(0x7746c0) > 0, timeout=35)
    ui.wait(lambda: ui.integer(0x784298) < 1 and
            ui.integer(0x462ff0) >= 2, timeout=35)
    if ui.integer(0x46385c) != 0 or ui.integer(0x467074) != 0:
        raise ValueError('A live player race is required')
    code = '0123456789AB'[ui.integer(0x936ff4)]
    source = files[f'LEV{code}\\LEVEL.PAL']
    expected = bytearray(1024)
    expected[0::4], expected[1::4], expected[2::4] = source[2::4], source[1::4], source[0::4]
    expected[3::4] = bytes([4]) * 256  # Original DirectDraw PALETTEENTRY flags.
    actual = ui.read(0x700050, 1024)
    if actual != expected:
        raise ValueError('Loaded original palette differs from BGR-reserved PAL conversion')
    if source[0::4] == source[2::4]:
        raise ValueError('This palette cannot distinguish red from blue')
    source_cluts = files[f'LEV{code}\\LEVEL.CLT'] + files.get(f'LEV{code}\\LEVEL.ECL', b'')
    pointer = int.from_bytes(ui.read(0x74c4d4, 4), 'little')
    actual_cluts = ui.read(pointer, len(source_cluts))
    if actual_cluts != source_cluts:
        raise ValueError('Loaded original CLT/ECL banks differ from source data')
    return dict(level=code, frame=ui.integer(0x462ff0), palette_entries=256,
                source_order='BGR-reserved', output_order='RGB-flags',
                every_palette_byte_matches=True, every_clut_byte_matches=True,
                palette_sha256=digest(actual), source_palette_sha256=digest(source),
                clut_bytes=len(actual_cluts), clut_sha256=digest(actual_cluts),
                input_history=ui.input_history)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game-dir', type=Path, default=ROOT / 'DestructionDerby2')
    parser.add_argument('--output', type=Path, default=WORK / 'rewrite-original-palette')
    parser.add_argument('--scene-origins', action='store_true',
                        help='Also check loaded original static/local scene vertex origins')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    game = args.game_dir.resolve()
    original = (game / 'Dirinfo').read_bytes()
    if digest(original) != ORIGINAL_SHA256:
        raise ValueError('Provision the supported original Dirinfo')
    files = assets(original)
    report = dict(pass_=False, scope=__doc__.strip(), exe_sha256=EXE_SHA256,
                  original_sha256=ORIGINAL_SHA256, engine_state_writes=False,
                  verified_at=datetime.now(timezone.utc).isoformat(),
                  source_sha256=digest(Path(__file__).read_bytes()))

    def driver(pid, destination, env, deadline, rundir):
        ui = OriginalRealtimeUI(pid, rundir, destination, env, deadline)
        try:
            report['observation'] = observe(ui, files)
            if args.scene_origins:
                report['scene_origins'] = scene_origins(ui, files, report['observation']['level'])
        finally:
            ui.stop()

    settings = original_args()
    settings.timeout = 100
    WINE_WORK.mkdir(parents=True, exist_ok=True)
    try:
        with (WINE_WORK / 'capture.lock').open('w') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            run_original(game, output, settings, on_menu=driver)
        check_space(output)
        report['pass_'] = True
    finally:
        logs = [path for path in output.glob('*.log') if path.is_file() and not path.is_symlink()]
        report['log_sha256'] = {path.name: digest(path.read_bytes()) for path in logs}
        (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        if report['pass_']:
            opened = open_files()
            for path in logs:
                stat = path.stat()
                if (stat.st_dev, stat.st_ino) not in opened:
                    path.unlink()
    print(json.dumps(dict(pass_=True, level=report['observation']['level'],
                          palette_entries=256, report=str(output / 'report.json'))))


if __name__ == '__main__':
    main()
