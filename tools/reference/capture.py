#!/usr/bin/env python3
"""Capture unmodified dd2h.exe at Draw_All entry using a private Wine CD device.

Requires 32-bit Wine, gcc multilib, GDB, Xvfb and xdotool. Captures contain
copyrighted original data and belong outside Git. A capture alone does not
establish original/native/WASM parity or audio output equivalence.
"""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import struct
import subprocess
import time
from cdrom import build_cdrom

ROOT = Path(__file__).resolve().parents[2]
WORK = ROOT / "third_party" / "wine-reference"
EXE_SHA256 = "0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2"


def original_pid(prefix):
    for process in Path("/proc").glob("[0-9]*"):
        try:
            if (process / "comm").read_text().strip() == "dd2h.exe" and (
                f"WINEPREFIX={prefix}".encode() + b"\0") in (process / "environ").read_bytes():
                return int(process.name)
        except (OSError, ProcessLookupError):
            pass
    return None


def state(pid):
    with open(f"/proc/{pid}/mem", "rb", buffering=0) as memory:
        def read(address, size):
            memory.seek(address)
            return memory.read(size)
        return {"pid": pid, "level": struct.unpack("<i", read(0x936FF4, 4))[0],
                "cf": struct.unpack("<i", read(0x462FF0, 4))[0],
                "screen": read(0x460005, 1)[0],
                "movie": struct.unpack("<i", read(0x462CD4, 4))[0]}


def capture(pid, output, env, frame, menu, timeout):
    # GDB stops at the same engine function as the port's diagnostic capture.
    # The condition is evaluated in the original address space, without writes.
    condition = "*(int*)0x936ff4 == 0" if menu else f"*(int*)0x936ff4 == 9 && *(int*)0x462ff0 >= {frame}"
    commands = ["set pagination off", "set auto-solib-add off", f"attach {pid}",
                "hbreak *0x420c9c", f"condition 1 {condition}", "continue"]
    regions = [("image.bin", 0x400000, 0x980400),
               ("framebuf.bin", 0x700450, 0x74B450),
               ("palette.bin", 0x700050, 0x700450)]
    for name, start, end in regions:
        commands.append(f'dump binary memory {name} 0x{start:x} 0x{end:x}')
    commands += ["printf \"REFERENCE_CF=%d\\n\", *(int*)0x462ff0", "detach", "quit"]
    script = output / "capture.gdb"
    script.write_text("\n".join(commands) + "\n")
    with (output / "gdb.log").open("wb") as log:
        subprocess.run(["gdb", "--nx", "-q", "-batch", "-x", str(script)],
                       env=env, cwd=output, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=timeout)
    image = (output / "image.bin").read_bytes()
    if len(image) != 0x580400:
        raise RuntimeError("Incomplete original image capture")
    saved_cf = struct.unpack_from("<i", image, 0x462FF0 - 0x400000)[0]
    saved_level = struct.unpack_from("<i", image, 0x936FF4 - 0x400000)[0]
    log = (output / "gdb.log").read_text()
    if "Breakpoint 1," not in log or "0x00420c9c" not in log:
        raise RuntimeError("Reference did not stop at Draw_All entry")
    if (menu and saved_level != 0) or (not menu and (saved_level != 9 or saved_cf < frame)):
        raise RuntimeError("Wrong reference checkpoint")
    fb, palette = (output / "framebuf.bin").read_bytes(), (output / "palette.bin").read_bytes()
    if len(fb) != 307200 or len(palette) != 1024:
        raise RuntimeError("Incomplete reference framebuffer or palette")
    (output / "frame.ppm").write_bytes(b"P6\n640 480\n255\n" + b"".join(
        palette[index * 4:index * 4 + 3] for index in fb))
    result = {"level": saved_level, "cf": saved_cf, "phase": "Draw_All entry @0x420c9c",
              "mode": "menu" if menu else "FE attract", "exe_modified": False,
              "exe_sha256": EXE_SHA256,
              "audio_comparison": "pending"}
    (output / "checkpoint.json").write_text(json.dumps(result, indent=2) + "\n")
    print(f"Original captured: level={saved_level}, cf={saved_cf}, Draw_All entry -> {output}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-dir", type=Path, default=ROOT / "DestructionDerby2")
    parser.add_argument("--output", type=Path, default=Path("/tmp/dd2-reference"))
    parser.add_argument("--mode", choices=("menu", "attract"), default="attract")
    parser.add_argument("--frame", type=int, default=150)
    parser.add_argument("--timeout", type=float, default=90)
    parser.add_argument("--trace-cd", action="store_true")
    args = parser.parse_args()
    if args.frame < 1 or args.timeout <= 0:
        parser.error("frame and timeout must be positive")
    game, output = args.game_dir.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if any((output / name).exists() for name in ("image.bin", "checkpoint.json")):
        parser.error("output already contains a capture; use a fresh directory")
    WORK.mkdir(parents=True, exist_ok=True)
    with (WORK / "capture.lock").open("w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        run(game, output, args)


def run(game, output, args):
    with (game / "dd2h.exe").open("rb") as executable:
        if hashlib.file_digest(executable, "sha256").hexdigest() != EXE_SHA256:
            raise ValueError("Reference EXE differs from the supported unmodified dd2h.exe")
    _, libraries = build_cdrom(game, WORK / "cdrom")
    prefix = WORK / "prefix"
    if original_pid(prefix):
        raise RuntimeError("A reference is already running in this private Wine prefix")
    rundir = WORK / "game"
    rundir.mkdir(exist_ok=True)
    for file in game.iterdir():
        destination = rundir / file.name
        if file.name == "SaveGames":
            shutil.copyfile(file, destination)  # Each run starts from the user's current save copy.
        else:
            if destination.is_symlink():
                destination.unlink()
            if destination.exists():
                raise RuntimeError(f"Unexpected reference asset at {destination}")
            destination.symlink_to(file, target_is_directory=file.is_dir())
    device = WORK / "cdrom-device"
    device.touch()
    alsa = WORK / "asound.conf"
    alsa.write_text("pcm.!default { type null }\n")
    env = {key: value for key, value in os.environ.items() if not key.startswith("DD2_")}
    env.update(WINEPREFIX=str(prefix), WINEARCH="win32", WINEDEBUG="-all",
               ALSA_CONFIG_PATH=str(alsa), DD2_CD_ROOT=str(game / "Redbook"),
               DD2_CD_DEVICE=str(device), LD_PRELOAD="dd2_cdrom.so",
               LD_LIBRARY_PATH=":".join(map(str, libraries)) +
                   (":" + env["LD_LIBRARY_PATH"] if env.get("LD_LIBRARY_PATH") else ""))
    if args.trace_cd:
        env["DD2_CD_TRACE"] = "1"
    wine = None
    xserver = None
    with (output / "wine.log").open("wb") as wine_log, (output / "xvfb.log").open("wb") as xlog:
        try:
            xserver = subprocess.Popen(["Xvfb", "-displayfd", "1", "-screen", "0", "640x480x16"],
                                      stdout=subprocess.PIPE, stderr=xlog, start_new_session=True)
            display = xserver.stdout.readline().decode().strip()
            if not display:
                raise RuntimeError("Xvfb did not start")
            env["DISPLAY"] = ":" + display
            if not (prefix / "drive_c/windows/system32/kernel32.dll").exists():
                print("Initializing private 32-bit Wine prefix...", flush=True)
                subprocess.run(["wineboot", "--init"], env=env, stdout=wine_log, stderr=wine_log,
                               check=True, timeout=60)
            dosdevices = prefix / "dosdevices"
            for name, target in (("d:", rundir), ("d::", device)):
                link = dosdevices / name
                if link.is_symlink():
                    link.unlink()
                if link.exists():
                    raise RuntimeError(f"Unexpected directory at {link}")
                link.symlink_to(target)
            for key, value, data in [(r"HKLM\Software\Wine\Drives", "d:", "cdrom"),
                                     (r"HKCU\Software\Wine\Drivers", "Audio", "alsa")]:
                subprocess.run(["wine", "reg", "add", key, "/v", value, "/t", "REG_SZ", "/d", data, "/f"],
                               env=env, stdout=wine_log, stderr=wine_log, check=True, timeout=30)
            # Drive types are read at Wine startup; registry writes need a restart.
            subprocess.run(["wineserver", "-k"], env=env, check=True, timeout=10)
            subprocess.run(["wineserver", "-w"], env=env, check=True, timeout=10)
            wine = subprocess.Popen(["wine", "dd2h.exe"], cwd=rundir, env=env,
                                    stdout=wine_log, stderr=wine_log, start_new_session=True)
            deadline, last_report, last_escape = time.monotonic() + args.timeout, 0, 0
            while time.monotonic() < deadline:
                pid = original_pid(prefix)
                if pid:
                    try:
                        current = state(pid)
                    except (FileNotFoundError, ProcessLookupError, OSError, struct.error):
                        time.sleep(0.05)
                        continue
                    now = time.monotonic()
                    if now - last_report >= 5:
                        print(json.dumps(current), flush=True)
                        last_report = now
                    if current["movie"] and now - last_escape >= 2:
                        subprocess.run(["xdotool", "search", "--name", "PC-DD2", "windowfocus", "key", "Escape"],
                                       env=env, stdout=subprocess.DEVNULL, stderr=wine_log, timeout=5)
                        last_escape = now
                    ready = (current["screen"] == 201 and current["level"] == 0) if args.mode == "menu" else current["level"] == 9 and current["cf"] > 0
                    if ready:
                        capture(pid, output, env, args.frame, args.mode == "menu",
                                max(1, deadline - time.monotonic()))
                        return
                if wine.poll() is not None:
                    raise RuntimeError(f"Original exited before checkpoint ({wine.returncode}); see {output / 'wine.log'}")
                time.sleep(0.05)
            raise TimeoutError(f"Original did not reach {args.mode} checkpoint in {args.timeout}s")
        finally:
            # Every process belongs to the dedicated prefix or to this launcher.
            subprocess.run(["wineserver", "-k"], env=env, stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL, timeout=10)
            if wine and wine.poll() is None:
                os.killpg(wine.pid, signal.SIGTERM)
                wine.wait(timeout=10)
            if xserver:
                xserver.terminate()
                xserver.wait(timeout=5)


if __name__ == "__main__":
    main()
