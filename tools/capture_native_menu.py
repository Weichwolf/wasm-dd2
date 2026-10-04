#!/usr/bin/env python3
"""Drive the native headless frontend through its normal keyboard bridge.

GDB calls dd2_key_event for press/release around one engine ReadPad call. This
exercises the full frontend/race/rendering path, not native window-system input
or an audio hardware sink. Assets and saves stay outside Git; saves are copied.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from artifacts import WORK, prepare_output, run_bounded
from menu_keys import KEYS

ROOT = Path(__file__).resolve().parents[1]
GDB_DRIVER = r'''
import gdb, json, struct
from pathlib import Path

def advance_draw(count):
    bp = gdb.Breakpoint('*Draw_All')
    bp.ignore_count = count - 1
    gdb.execute('continue')
    if int(gdb.parse_and_eval('$pc')) != int(gdb.parse_and_eval('&Draw_All')):
        raise RuntimeError('Native failed to reach Draw_All entry')
    bp.delete()

def capture(index, key):
    directory = Path(output) / ('step%02d-%s' % (index, key or 'boot'))
    directory.mkdir()
    inferior = gdb.selected_inferior()
    image = bytes(inferior.read_memory(0x400000, 0x580400))
    fb = bytes(inferior.read_memory(0x700450, 307200))
    palette = bytes(inferior.read_memory(0x700050, 1024))
    for name, data in [('image.bin', image), ('framebuf.bin', fb), ('palette.bin', palette)]:
        (directory / name).write_bytes(data)
    (directory / 'frame.ppm').write_bytes(b'P6\n640 480\n255\n' + b''.join(palette[i*4:i*4+3] for i in fb))
    state = {'level': struct.unpack_from('<i', image, 0x936ff4-0x400000)[0],
             'cf': struct.unpack_from('<i', image, 0x462ff0-0x400000)[0],
             'phase': 'native Draw_All entry', 'input': 'dd2_key_event',
             'held_pad_polls': 1 if key else 0}
    (directory / 'checkpoint.json').write_text(json.dumps(state, indent=2)+'\n')
    print('Native checkpoint %d %s: %s' % (index, key, state), flush=True)
    if cycle_frames:
        from menu_cycle_gdb import record_cycle
        record_cycle(directory, cycle_frames, int(gdb.parse_and_eval('&Draw_All')), already_at_entry=True)

start = gdb.Breakpoint('*Front_End', temporary=True)
gdb.execute('run')
if int(gdb.parse_and_eval('$pc')) != int(gdb.parse_and_eval('&Front_End')):
    raise RuntimeError('Native failed to enter Front_End')
advance_draw(settle_frames)
capture(0, None)
for index, key in enumerate(keys, 1):
    bp = gdb.Breakpoint('*FUN_00422da4')
    gdb.execute('continue')
    if int(gdb.parse_and_eval('$pc')) != int(gdb.parse_and_eval('&FUN_00422da4')):
        raise RuntimeError('Native failed to reach ReadPad before key-down')
    gdb.execute('call (void)dd2_key_event(%d, 1)' % vkeys[key])
    gdb.execute('continue')
    if int(gdb.parse_and_eval('$pc')) != int(gdb.parse_and_eval('&FUN_00422da4')):
        raise RuntimeError('Native failed to reach the next ReadPad')
    gdb.execute('call (void)dd2_key_event(%d, 0)' % vkeys[key])
    bp.delete()
    advance_draw(settle_frames)
    capture(index, key)
gdb.execute('kill')
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=Path("/tmp/dd2_native"))
    parser.add_argument("--game-dir", type=Path, default=ROOT / "DestructionDerby2")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--keys", nargs="+", choices=KEYS, required=True)
    parser.add_argument("--settle-frames", type=int, default=60)
    parser.add_argument("--menu-cycle", type=int, choices=(0,64,256), default=0)
    parser.add_argument("--timeout", type=float, default=90)
    args = parser.parse_args()
    if args.settle_frames < 1 or args.timeout <= 0:
        parser.error("settle frames and timeout must be positive")
    output, binary, game = prepare_output(args.output), args.binary.resolve(), args.game_dir.resolve()
    if WORK not in output.parents:
        parser.error("verification output must be under /tmp/wasm-dd2/")
    if output.exists() and any(output.iterdir()):
        parser.error("output must be empty; use a fresh capture directory")
    if not binary.is_file():
        parser.error("native binary is missing; run tools/build_native.sh")
    output.mkdir(parents=True, exist_ok=True)
    env = {key: value for key, value in os.environ.items() if not key.startswith("DD2_")}
    env.update(DD2_FE="1", DD2_SOUND="1", DD2_NOSEGV="1")
    parameters = (f"import sys\nsys.path.insert(0, {str(ROOT / 'tools')!r})\n"
                  f"output={str(output)!r}\nkeys={args.keys!r}\nvkeys={KEYS!r}\n"
                  f"settle_frames={args.settle_frames!r}\ncycle_frames={args.menu_cycle!r}\n")
    script = output / "capture.gdb"
    script.write_text("set pagination off\nset confirm off\nset auto-solib-add off\npython\nexec(" +
                      repr(parameters + GDB_DRIVER) + ")\nend\nquit\n")
    with tempfile.TemporaryDirectory(prefix="native-menu-assets-",dir=WORK) as tmp:
        rundir = Path(tmp)
        for asset in game.iterdir():
            destination = rundir / asset.name
            if asset.name == "SaveGames":
                shutil.copyfile(asset, destination)
            else:
                destination.symlink_to(asset, target_is_directory=asset.is_dir())
        initial_save_sha256=hashlib.sha256((rundir/"SaveGames").read_bytes()).hexdigest()
        with (output / "gdb.log").open("wb") as log:
            run_bounded(["gdb", "--nx", "-q", "-batch", "-x", str(script), str(binary)],directory=output,
                           cwd=rundir, env=env, stdout=log, stderr=subprocess.STDOUT,
                           timeout=args.timeout, check=True)
    checkpoints = [f"step{i:02d}-{key or 'boot'}" for i, key in enumerate([None, *args.keys])]
    for name in checkpoints:
        if not (output / name / "checkpoint.json").is_file():
            raise RuntimeError(f"Missing native checkpoint: {name}")
    (output / "navigation.json").write_text(json.dumps({
        "keys": args.keys, "checkpoints": checkpoints, "input": "dd2_key_event",
        "initial_save_sha256": initial_save_sha256,
        "settle_frames": args.settle_frames, "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
        "scope": "native headless frontend; window-system input and hardware audio not exercised"
    }, indent=2)+"\n")
    print(f"Native frontend captures: {output}")


if __name__ == "__main__":
    main()
