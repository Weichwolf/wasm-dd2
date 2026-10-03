#!/usr/bin/env python3
"""Capture unmodified dd2h.exe video or ALSA audio with a private Wine CD device.

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
from audio import build_audio, summarize_audio

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


def capture(pid, output, env, frame, menu, timeout, navigation=False):
    # GDB stops at the same engine function as the port's diagnostic capture.
    # The condition is evaluated in the original address space, without writes.
    # The countdown resets cf at the green light. Capture the post-countdown
    # occurrence, matching the port's cf-keyed dumps after that same reset.
    condition = "*(int*)0x936ff4 == 0" if menu else (
        f"*(int*)0x936ff4 == 9 && *(int*)0x784298 < 1 && *(int*)0x462ff0 >= {frame}")
    if navigation:
        condition = "1"
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
    countdown = struct.unpack_from("<i", image, 0x784298 - 0x400000)[0]
    log = (output / "gdb.log").read_text()
    if "Breakpoint 1," not in log or "0x00420c9c" not in log:
        raise RuntimeError("Reference did not stop at Draw_All entry")
    if not navigation and ((menu and saved_level != 0) or
                           (not menu and (saved_level != 9 or saved_cf < frame or countdown >= 1))):
        raise RuntimeError("Wrong reference checkpoint")
    fb, palette = (output / "framebuf.bin").read_bytes(), (output / "palette.bin").read_bytes()
    if len(fb) != 307200 or len(palette) != 1024:
        raise RuntimeError("Incomplete reference framebuffer or palette")
    (output / "frame.ppm").write_bytes(b"P6\n640 480\n255\n" + b"".join(
        palette[index * 4:index * 4 + 3] for index in fb))
    result = {"level": saved_level, "cf": saved_cf, "phase": "Draw_All entry @0x420c9c",
              "mode": "real-key navigation" if navigation else "menu" if menu else "FE attract", "exe_modified": False,
              "exe_sha256": EXE_SHA256,
              "audio_comparison": "pending"}
    if not menu and not navigation:
        result["countdown"] = countdown
    (output / "checkpoint.json").write_text(json.dumps(result, indent=2) + "\n")
    print(f"Original captured: level={saved_level}, cf={saved_cf}, Draw_All entry -> {output}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-dir", type=Path, default=ROOT / "DestructionDerby2")
    parser.add_argument("--output", type=Path, default=Path("/tmp/dd2-reference"))
    parser.add_argument("--mode", choices=("menu", "attract", "audio"), default="attract")
    frames = parser.add_mutually_exclusive_group()
    frames.add_argument("--frame", type=int, default=150)
    frames.add_argument("--frames", type=int, nargs="+",
                        help="capture increasing race checkpoints in one unmodified attract run")
    parser.add_argument("--timeout", type=float, default=90)
    parser.add_argument("--race-stream",action="store_true",help="record every first-L9 demo racing-loop frame and actual game clock return using read-only hardware breakpoints")
    parser.add_argument("--trace-cd", action="store_true")
    parser.add_argument("--wine-debug", default="-all",
                        help="explicit Wine trace channels for API/format diagnostics; tracing alters timing")
    parser.add_argument("--keep-movie", action="store_true",
                        help="play the original intro through completion during reference capture")
    parser.add_argument("--keys", nargs="*", choices=("Left", "Right", "Up", "Down", "Return", "Escape", "F1", "F2"),
                        help="navigate from the initial menu with real X11 keys; capture after each action")
    parser.add_argument("--key-hold", type=float, default=0.14,
                        help="seconds per held key; record this for timing comparisons")
    parser.add_argument("--acknowledged-key", action="store_true",
                        help="wait for real key press/release at pad polls and record held poll counts")
    parser.add_argument("--menu-cycle", type=int, choices=(0,64), default=0,
                        help="capture a complete rendered highlight cycle at every menu checkpoint")
    parser.add_argument("--audio", action="store_true",
                        help="observe original Wine ALSA accepted PCM, with committed format and transport events")
    parser.add_argument("--audio-rate", type=int, choices=(22050,44100,48000), default=44100,
                        help="virtual playback device rate when --audio is enabled")
    parser.add_argument("--audio-device", choices=("clock","null"), default="clock",
                        help="clock uses a real-time sample clock; null is diagnostic and consumes too fast")
    parser.add_argument("--audio-tail", type=float, default=0,
                        help="seconds to keep running after video/navigation capture (requires --audio)")
    args = parser.parse_args()
    if args.race_stream and (args.mode!="attract" or args.frames or args.keys is not None or args.audio):
        parser.error("--race-stream requires attract mode, no frame/key sequence and no audio (debugger changes clock timing)")
    if args.frame < 1 or args.timeout <= 0 or not 0 < args.key_hold <= 1:
        parser.error("frame/timeout must be positive and key hold must be in (0,1]")
    # Play_Game budgets 1500 physics steps; current_frame divides steps by two
    # and resets at the green light. Later counters cannot occur in this demo.
    if args.mode == "attract" and (args.frame > 700 or (args.frames and max(args.frames) > 700)):
        parser.error("attract checkpoints must be within cf1..700")
    if args.frames and (args.mode != "attract" or min(args.frames) < 1 or
                        args.frames != sorted(set(args.frames))):
        parser.error("--frames requires attract mode and strictly increasing frames within cf1..700")
    if args.keys is not None and args.mode != "menu":
        parser.error("--keys requires --mode menu")
    if args.acknowledged_key and args.keys is None:
        parser.error("--acknowledged-key requires --keys")
    if args.menu_cycle and args.mode != "menu":
        parser.error("--menu-cycle requires --mode menu")
    if args.audio_tail < 0 or args.audio_tail > 30 or (args.audio_tail and not args.audio):
        parser.error("--audio-tail must be in [0,30] and requires --audio")
    if args.mode=="audio" and (not args.audio or args.audio_tail<=0):
        parser.error("--mode audio requires --audio and a positive --audio-tail; no debugger is used")
    game, output = args.game_dir.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if (any((output / name).exists() for name in ("image.bin", "checkpoint.json", "navigation.json", "audio", "video-checkpoints.json", "race"))
            or any(output.glob("step*/checkpoint.json")) or any(output.glob("frame*/checkpoint.json"))):
        parser.error("output already contains a capture; use a fresh directory")
    WORK.mkdir(parents=True, exist_ok=True)
    with (WORK / "capture.lock").open("w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        run(game, output, args)
        if args.audio:
            # Successful capture and scoped process cleanup completed.
            report=summarize_audio(output/"audio")
            if any(stream["rate"] != args.audio_rate for stream in report["streams"]):
                raise RuntimeError("Committed reference audio rate differs from requested virtual device")
            report.update(exe_sha256=EXE_SHA256,exe_modified=False,virtual_device_rate=args.audio_rate,
                          virtual_device=args.audio_device,wine_debug=args.wine_debug,
                          movie_autoskip=not args.keep_movie)
            (output/"audio/summary.json").write_text(json.dumps(report,indent=2)+"\n")


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
    env.update(WINEPREFIX=str(prefix), WINEARCH="win32", WINEDEBUG=args.wine_debug,
               ALSA_CONFIG_PATH=str(alsa), DD2_CD_ROOT=str(game / "Redbook"),
               DD2_CD_DEVICE=str(device), LD_PRELOAD="dd2_cdrom.so",
               LD_LIBRARY_PATH=":".join(map(str, libraries)) +
                   (":" + env["LD_LIBRARY_PATH"] if env.get("LD_LIBRARY_PATH") else ""))
    if args.audio:
        audio_libraries=build_audio(WORK/"audio")
        (output/"audio").mkdir()
        env.update(DD2_AUDIO_CAPTURE=str(output/"audio"),DD2_AUDIO_RATE=str(args.audio_rate))
        env["LD_PRELOAD"]+=" dd2_audio.so"
        env["LD_LIBRARY_PATH"]=":".join(map(str,audio_libraries))+":"+env["LD_LIBRARY_PATH"]
        if args.audio_device=="clock":
            library=json.dumps(str(WORK/"audio"/"$LIB"/"dd2_clock.so"))
            alsa.write_text(f'pcm_type.dd2clock {{ lib {library} }}\npcm.!default {{ type dd2clock }}\n')
    if args.trace_cd:
        env["DD2_CD_TRACE"] = "1"
    wine = None
    xserver = None
    engine_log = (output / "audio/engine.jsonl").open("x") if args.audio else None

    def observe_state(pid):
        # These are bounded, read-only observations of a running process, not
        # atomic snapshots or a claim to have sampled every engine frame.
        before = time.monotonic_ns()
        current = state(pid)
        after = time.monotonic_ns()
        if engine_log:
            engine_log.write(json.dumps({"read_begin_ns": before, "read_end_ns": after,
                                         **current}) + "\n")
            engine_log.flush()
        return current

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
                        current = observe_state(pid)
                    except (FileNotFoundError, ProcessLookupError, OSError, struct.error):
                        time.sleep(0.05)
                        continue
                    now = time.monotonic()
                    if now - last_report >= 5:
                        print(json.dumps(current), flush=True)
                        last_report = now
                    if current["movie"] and not args.keep_movie and now - last_escape >= 2:
                        subprocess.run(["xdotool", "search", "--name", "PC-DD2", "windowfocus", "key", "Escape"],
                                       env=env, stdout=subprocess.DEVNULL, stderr=wine_log, timeout=5)
                        last_escape = now
                    ready = (current["screen"] == 201 and current["level"] == 0) if args.mode in ("menu","audio") or args.race_stream else current["level"] == 9 and current["cf"] > 0
                    if ready:
                        if args.race_stream:
                            capture_race_stream(pid,output,env,max(1,deadline-time.monotonic()))
                            return
                        if args.mode=="audio":
                            end = time.monotonic() + args.audio_tail
                            while time.monotonic() < end:
                                if wine.poll() is not None:
                                    raise RuntimeError("Original exited during audio recording")
                                observe_state(pid)
                                time.sleep(min(0.02, max(0, end-time.monotonic())))
                            result={"mode":"audio","phase":"live engine; no debugger", "start_state":current,
                                    "end_state":observe_state(pid),"exe_modified":False,"exe_sha256":EXE_SHA256,
                                    "state_observations":"audio/engine.jsonl; bounded non-atomic reads, may include attract",
                                    "audio_comparison":"pending"}
                            (output/"checkpoint.json").write_text(json.dumps(result,indent=2)+"\n")
                            return
                        if args.keys is not None:
                            navigate(pid, output, env, args.keys, args.key_hold, args.acknowledged_key, deadline, wine_log, args.menu_cycle)
                            if args.audio_tail:
                                time.sleep(args.audio_tail)
                            return
                        if args.frames:
                            checkpoints = []
                            for frame in args.frames:
                                directory = output / f"frame{frame:05d}"
                                directory.mkdir()
                                capture(pid, directory, env, frame, False,
                                        max(1, deadline-time.monotonic()))
                                checkpoints.append({"requested_cf": frame, "directory": directory.name,
                                    "checkpoint": json.loads((directory/"checkpoint.json").read_text())})
                            # Publish a complete manifest only after every
                            # requested checkpoint succeeds. Failed runs retain
                            # their individual snapshots for diagnosis.
                            (output/"video-checkpoints.json").write_text(json.dumps({
                                "scope": "read-only Draw_All video checkpoints; complete streams and audio pending",
                                "requested_frames": args.frames,
                                "checkpoints": checkpoints}, indent=2)+"\n")
                            if args.audio_tail:
                                time.sleep(args.audio_tail)
                            return
                        capture(pid, output, env, args.frame, args.mode == "menu",
                                max(1, deadline - time.monotonic()))
                        if args.menu_cycle:
                            capture_cycle(pid, output, env, args.menu_cycle, max(1, deadline-time.monotonic()))
                        if args.audio_tail:
                            time.sleep(args.audio_tail)
                        return
                if wine.poll() is not None:
                    raise RuntimeError(f"Original exited before checkpoint ({wine.returncode}); see {output / 'wine.log'}")
                time.sleep(0.05)
            raise TimeoutError(f"Original did not reach {args.mode} checkpoint in {args.timeout}s")
        finally:
            if engine_log:
                engine_log.close()
            # Every process belongs to the dedicated prefix or to this launcher.
            subprocess.run(["wineserver", "-k"], env=env, stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL, timeout=10)
            if wine and wine.poll() is None:
                os.killpg(wine.pid, signal.SIGTERM)
                wine.wait(timeout=10)
            if xserver:
                xserver.terminate()
                xserver.wait(timeout=5)


def capture_race_stream(pid,output,env,timeout):
    script=output/"race.gdb"
    script.write_text("\n".join(["set pagination off","set auto-solib-add off",f"attach {pid}",
        "python",f"import sys; sys.path.insert(0,{str(ROOT/'tools')!r})",
        "from race_stream_gdb import record_race",f"record_race({str(output)!r},9)",
        "end","detach","quit"])+"\n")
    with (output/"race-gdb.log").open("wb") as log:
        subprocess.run(["gdb","--nx","-q","-batch","-x",str(script)],env=env,
            stdout=log,stderr=subprocess.STDOUT,check=True,timeout=timeout)
    result=json.loads((output/"race/race.json").read_text())
    result.update(exe_modified=False,exe_sha256=EXE_SHA256)
    (output/"race/race.json").write_text(json.dumps(result,indent=2)+"\n")
    print(f"Original racing loop captured: {len(result['frames'])} frames, {result['clock_calls']} actual tick returns",flush=True)


def key_acknowledged(pid, output, env, key, timeout):
    # Translate_Keypress @0x423050 matches active map bytes @0x46302c and
    # sets these byte flags. ReadPad @0x422da4 consumes them. No memory writes.
    vkeys = {"Left": 0x25, "Right": 0x27, "Up": 0x26, "Down": 0x28,
             "Return": 0x0d, "Escape": 0x1b, "F1": 0x70, "F2": 0x71}
    flags = [0x46303f, 0x463040, 0x463043, 0x463046, 0x463044, 0x463045,
             0x463048, 0x46304a, 0x463047, 0x463049, 0x46304b, 0x46304e, 0x46304c, 0x46304d]
    with open(f"/proc/{pid}/mem", "rb", buffering=0) as memory:
        memory.seek(0x46302c)
        mapping = memory.read(14)
    index = next((i for i in (0, 1, 2, 3, 4, 5, 8, 6, 9, 7, 10, 11, 12, 13)
                  if mapping[i] == vkeys[key]), None)
    if index is None:
        raise RuntimeError(f"{key} is not mapped in the original keyboard map")
    flag = flags[index]
    command = ["xdotool", "search", "--name", "PC-DD2", "windowfocus"]
    commands = ["set pagination off", "set auto-solib-add off", f"attach {pid}",
                "hbreak *0x422da4", f"condition 1 *(unsigned char*)0x{flag:x} != 0",
                "python import subprocess",
                f"python subprocess.run({command + ['keydown', key]!r}, check=True, timeout=5, stdout=subprocess.DEVNULL)",
                "continue", "delete 1",
                f"python subprocess.run({command + ['keyup', key]!r}, check=True, timeout=5, stdout=subprocess.DEVNULL)",
                # Count every intervening held poll instead of hiding them
                # behind a conditional breakpoint. Wine may queue key-up late.
                "python",
                "class ReleasePoll(gdb.Breakpoint):",
                "    held = 1",
                "    def stop(self):",
                f"        if int(gdb.parse_and_eval('*(unsigned char*)0x{flag:x}')):",
                "            self.held += 1",
                "            return False",
                "        return True",
                "release_poll = ReleasePoll('*0x422da4', type=gdb.BP_HARDWARE_BREAKPOINT)",
                "end", "continue",
                "python print('KEY_HELD_POLLS=%d' % release_poll.held)",
                f'printf "KEY_RELEASED=%d\\n", *(unsigned char*)0x{flag:x}', "detach", "quit"]
    script = output / "input.gdb"
    script.write_text("\n".join(commands)+"\n")
    with (output / "input.log").open("wb") as log:
        subprocess.run(["gdb", "--nx", "-q", "-batch", "-x", str(script)], env=env,
                       stdout=log, stderr=subprocess.STDOUT, check=True, timeout=timeout)
    log = (output / "input.log").read_text()
    if "Breakpoint 1," not in log or "Breakpoint 2," not in log or "KEY_RELEASED=0" not in log:
        raise RuntimeError("Original did not consume/release the real key at pad polls")
    held = int(next(line.split("=", 1)[1] for line in log.splitlines() if line.startswith("KEY_HELD_POLLS=")))
    if held < 1:
        raise RuntimeError("Original did not observe a held key")
    return held


def capture_cycle(pid, output, env, frames, timeout):
    commands = ["set pagination off", "set auto-solib-add off", f"attach {pid}",
                "python", "import sys", f"sys.path.insert(0, {str(ROOT / 'tools')!r})",
                "from menu_cycle_gdb import record_cycle",
                f"record_cycle({str(output)!r}, {frames}, 0x420c9c, hardware=True)",
                "end", "detach", "quit"]
    script = output / "cycle.gdb"
    script.write_text("\n".join(commands)+"\n")
    with (output / "cycle.log").open("wb") as log:
        subprocess.run(["gdb", "--nx", "-q", "-batch", "-x", str(script)], env=env,
                       stdout=log, stderr=subprocess.STDOUT, check=True, timeout=timeout)
    cycle = json.loads((output / "cycle/cycle.json").read_text())
    if len(cycle["frames"]) != frames:
        raise RuntimeError("Incomplete original rendered cycle")


def navigate(pid, output, env, keys, key_hold, acknowledged, deadline, wine_log, cycle_frames=0):
    """Read-only checkpoints after real input; no EXE or engine-state writes."""
    sequence = [None, *keys]
    held_polls = []
    for index, key in enumerate(sequence):
        if time.monotonic() >= deadline:
            raise TimeoutError("Menu navigation timed out")
        directory = output / f"step{index:02d}-{key or 'boot'}"
        directory.mkdir()
        if key and acknowledged:
            held_polls.append(key_acknowledged(pid, directory, env, key, max(1, deadline-time.monotonic())))
        elif key:
            command = ["xdotool", "search", "--name", "PC-DD2", "windowfocus"]
            # One xdotool process keeps process-start overhead out of the held
            # interval. It otherwise caused many repeats in the fast Wine FE.
            subprocess.run([*command, "keydown", key, "sleep", str(key_hold), "keyup", key],
                           env=env, stdout=subprocess.DEVNULL,
                           stderr=wine_log, check=True, timeout=5)
        time.sleep(0.7)
        capture(pid, directory, env, 0, True, max(1, deadline-time.monotonic()), navigation=True)
        if cycle_frames:
            capture_cycle(pid, directory, env, cycle_frames, max(1, deadline-time.monotonic()))
    (output / "navigation.json").write_text(json.dumps({"keys": keys,
        "key_hold_seconds": None if acknowledged else key_hold,
        "acknowledged_keys": acknowledged, "held_pad_polls": held_polls, "input": "real X11 keys",
        "checkpoints": [f"step{i:02d}-{key or 'boot'}" for i, key in enumerate(sequence)],
        "scope": "menu checkpoints; timing alignment and full parity pending"}, indent=2)+"\n")


if __name__ == "__main__":
    main()
