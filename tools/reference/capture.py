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
import sys
import time
from cdrom import build_cdrom
from audio import build_audio, summarize_audio

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from artifacts import WORK as ARTIFACTS, prepare_output, run_bounded
WORK = ARTIFACTS / 'wine-reference'
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
                "screen_diagnostic": "byte of cached CLUT pointer; not a screen identifier",
                "menu": {name: struct.unpack("<I", read(address, 4))[0] for name,address in
                         (("poly_list",0x940010),("restart_cd_audio",0x467420))},
                "cd": {name: struct.unpack("<I", read(address, 4))[0] for name,address in
                       (("enabled",0x462d74),("playing",0x462d70),("from",0x74f174),("to",0x74f178))},
                "movie": struct.unpack("<i", read(0x462CD4, 4))[0]}


def menu_ready(current):
    # Original FUN_00450fe0 @0x450fec stores the active polygon-list pointer
    # at 0x940010. Front_End @0x4502f6 selects Main Menu (0x4696b0), then
    # rotates the slab, starts CD and clears restart_cd_audio at 0x450347.
    # The cached CLUT pointer byte previously tested here changes while drawing;
    # it can look ready during loading and identifies neither a menu nor a phase.
    return (current['level']==0 and current['movie']==0 and
            current['menu']==dict(poly_list=0x4696b0,restart_cd_audio=0))


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
    parser.add_argument("--output", type=Path, default=ARTIFACTS / 'reference')
    parser.add_argument("--mode", choices=("menu", "attract", "audio", "startup"), default="attract")
    parser.add_argument('--startup-frames',type=int,default=128,
                        help='record this many first actual frontend presentations in startup mode (64..512)')
    frames = parser.add_mutually_exclusive_group()
    frames.add_argument("--frame", type=int, default=150)
    frames.add_argument("--frames", type=int, nargs="+",
                        help="capture increasing race checkpoints in one unmodified attract run")
    parser.add_argument("--timeout", type=float, default=90)
    parser.add_argument("--race-level",type=int,choices=range(1,11),default=9,help="wait for this naturally selected attract level; requires --race-stream")
    parser.add_argument("--race-images",type=int,nargs="+",default=[],help="also dump original engine memory at these racing presentation indices for diagnostics")
    parser.add_argument("--race-image-counters",type=int,nargs="+",default=[],help="also dump original engine memory at every presentation of these cf counters (0..700)")
    parser.add_argument("--race-stream",action="store_true",help="record every selected demo racing-loop frame and actual game clock/random return using read-only hardware breakpoints")
    parser.add_argument('--champ-history',action='store_true',help='record actual five-retirement championship clock/RNG/input history; real X11 keys, read-only hardware breakpoints')
    parser.add_argument('--champ-history-steps',type=int,choices=(17,95),default=95,help='17 stops after first result for diagnosis; 95 completes the retirement path')
    parser.add_argument('--champ-history-frame-delay-ms',type=int,default=0,help='diagnostic elapsed-time delay at real Play_Game draw entries; 0..250, no clock/frame-skip/state writes')
    parser.add_argument('--normal-arena-history',action='store_true',help='actual Total Destruction player arena, Up+Right inputs until natural finish; requires --champ-history')
    parser.add_argument('--natural-champ-history',action='store_true',help='drive the first Wrecking championship race without Retire, show all four leagues, start race two; read-only road follower and recorded real X11 keys')
    parser.add_argument('--natural-season-history',action='store_true',help='extend natural championship to all five races, all league pages and the actual end-of-season transition; requires --natural-champ-history')
    parser.add_argument('--steady-driver',action='store_true',help='use the slower road follower with yaw-rate feedback; requires --natural-champ-history, does not guarantee completed laps')
    parser.add_argument('--normal-arena-full-video',action='store_true',help='also capture every menu/loading/fade PutDispEnv presentation including Swap_Buffers; requires --normal-arena-history')
    parser.add_argument("--race-full-history",action="store_true",help="also record every preceding race clock and all random calls from the frontend; requires --race-stream")
    parser.add_argument("--race-physics",action="store_true",help="also record all preceding/target car-state checkpoints and global random callers; requires --race-full-history")
    parser.add_argument('--race-step-window',type=int,nargs=2,metavar=('START_CF','END_CF'),default=[],
                        help='observe every Car_Movement entry/return in this small target counter window; requires --race-physics')
    parser.add_argument('--race-step-levels',type=int,nargs='+',choices=range(1,11),default=[],
                        help='also observe the step window in preceding demos of these levels; requires --race-step-window')
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
    parser.add_argument("--menu-cycle", type=int, choices=(0,64,256), default=0,
                        help="capture 64 highlight or 256 car-rotation presentations at each menu checkpoint")
    parser.add_argument("--audio", action="store_true",
                        help="observe original Wine ALSA accepted PCM, with committed format and transport events")
    parser.add_argument("--audio-rate", type=int, choices=(22050,44100,48000), default=44100,
                        help="virtual playback device rate when --audio is enabled")
    parser.add_argument("--audio-device", choices=("clock","null"), default="clock",
                        help="clock uses a real-time sample clock; null is diagnostic and consumes too fast")
    parser.add_argument("--audio-tail", type=float, default=0,
                        help="seconds to keep running after video/navigation capture (requires --audio)")
    args = parser.parse_args()
    if args.champ_history and (args.mode!='menu' or args.audio or args.keys is not None or args.frames or args.race_stream or args.menu_cycle):
        parser.error('--champ-history requires menu mode, no audio/other capture sequence')
    if args.champ_history_steps!=95 and not args.champ_history:
        parser.error('--champ-history-steps requires --champ-history')
    if not 0<=args.champ_history_frame_delay_ms<=250 or (args.champ_history_frame_delay_ms and not args.champ_history):
        parser.error('--champ-history-frame-delay-ms requires --champ-history and 0..250')
    if args.normal_arena_history and (not args.champ_history or args.champ_history_steps!=95):
        parser.error('--normal-arena-history requires --champ-history with default steps')
    if args.normal_arena_full_video and not args.normal_arena_history:
        parser.error('--normal-arena-full-video requires --normal-arena-history')
    if args.natural_champ_history and (not args.champ_history or args.normal_arena_history or args.champ_history_steps!=95):
        parser.error('--natural-champ-history requires --champ-history and no other history sequence')
    if args.steady_driver and not args.natural_champ_history:
        parser.error('--steady-driver requires --natural-champ-history')
    if args.natural_season_history and not args.natural_champ_history:
        parser.error('--natural-season-history requires --natural-champ-history')
    if args.mode=='startup' and (args.audio or args.keys is not None or args.frames or args.race_stream or
                                args.menu_cycle or not 64<=args.startup_frames<=512):
        parser.error('startup mode requires 64..512 first presentations, no audio/navigation/race capture')
    if args.mode!='startup' and args.startup_frames!=128:
        parser.error('--startup-frames requires --mode startup')
    if args.race_full_history and not args.race_stream:parser.error('--race-full-history requires --race-stream')
    if args.race_physics and not args.race_full_history:parser.error('--race-physics requires --race-full-history')
    if args.race_step_window and (not args.race_physics or not 4 <= args.race_step_window[0] <= args.race_step_window[1] <= 690):
        parser.error('--race-step-window requires --race-physics and 4 <= START_CF <= END_CF <= 690')
    if args.race_step_levels and not args.race_step_window:parser.error('--race-step-levels requires --race-step-window')
    if args.race_level!=9 and not args.race_stream:parser.error('--race-level requires --race-stream')
    if args.race_images and (not args.race_stream or min(args.race_images)<0):parser.error('--race-images requires --race-stream and nonnegative indices')
    if args.race_image_counters and (not args.race_stream or any(i not in range(701) for i in args.race_image_counters)):
        parser.error('--race-image-counters requires --race-stream and counters in 0..700')
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
    game, output = args.game_dir.resolve(), prepare_output(args.output)
    output.mkdir(parents=True, exist_ok=True)
    if (any((output / name).exists() for name in ("image.bin", "checkpoint.json", "navigation.json", "audio", "video-checkpoints.json", "race", "startup", "history"))
            or any(output.glob("step*/checkpoint.json")) or any(output.glob("frame*/checkpoint.json"))):
        parser.error("output already contains a capture; use a fresh directory")
    WORK.mkdir(parents=True, exist_ok=True)
    with (WORK / "capture.lock").open("w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        run(game, output, args)
        if args.audio:
            # Successful capture and scoped process cleanup completed.
            report=summarize_audio(output/"audio",require_played=args.audio_device=="clock")
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
    initial_save_sha256=hashlib.sha256((rundir/"SaveGames").read_bytes()).hexdigest()
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
                    if args.mode=='startup' and current['movie']:
                        capture_startup_video(pid,output,env,args.startup_frames,not args.keep_movie,
                                              max(1,deadline-time.monotonic()),initial_save_sha256)
                        return
                    if current["movie"] and not args.keep_movie and now - last_escape >= 2:
                        subprocess.run(["xdotool", "search", "--name", "PC-DD2", "windowfocus", "key", "Escape"],
                                       env=env, stdout=subprocess.DEVNULL, stderr=wine_log, timeout=5)
                        last_escape = now
                    ready = menu_ready(current) if args.mode in ("menu","audio") or args.race_stream else current["level"] == 9 and current["cf"] > 0
                    if ready:
                        if args.champ_history:
                            capture_champ_history(pid,output,env,max(1,deadline-time.monotonic()),args.champ_history_steps,initial_save_sha256,args.champ_history_frame_delay_ms,args.normal_arena_history,args.normal_arena_full_video,args.natural_champ_history,args.steady_driver,args.natural_season_history)
                            return
                        if args.race_stream:
                            capture_race_stream(pid,output,env,max(1,deadline-time.monotonic()),args.race_level,args.race_images,args.race_image_counters,args.race_full_history,args.race_physics,args.race_step_window,args.race_step_levels)
                            return
                        if args.mode=="audio":
                            end = time.monotonic() + args.audio_tail
                            while time.monotonic() < end:
                                if wine.poll() is not None:
                                    raise RuntimeError("Original exited during audio recording")
                                observe_state(pid)
                                time.sleep(min(0.02, max(0, end-time.monotonic())))
                            result={"mode":"audio","phase":"live engine; no debugger", "start_state":current,
                                    "readiness":"Main Menu polygon list with frontend startup/CD restart completed; bounded non-atomic observations",
                                    "end_state":observe_state(pid),"exe_modified":False,"exe_sha256":EXE_SHA256,
                                    "state_observations":"audio/engine.jsonl; bounded non-atomic reads, may include attract",
                                    "audio_comparison":"pending"}
                            (output/"checkpoint.json").write_text(json.dumps(result,indent=2)+"\n")
                            return
                        if args.keys is not None:
                            navigate(pid, output, env, args.keys, args.key_hold, args.acknowledged_key, deadline, wine_log, args.menu_cycle, initial_save_sha256)
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


def capture_startup_video(pid,output,env,frames,skip_movie,timeout,initial_save_sha256):
    script=output/'startup.gdb'
    script.write_text('\n'.join(['set pagination off','set auto-solib-add off',f'attach {pid}',
        'python',f'import sys;sys.path.insert(0,{str(ROOT/"tools")!r})',
        'from startup_video_gdb import record_startup',
        f'record_startup({str(output)!r},{frames},{skip_movie!r})','end','detach','quit'])+'\n')
    with (output/'startup-gdb.log').open('wb') as log:
        run_bounded(['gdb','--nx','-q','-batch','-x',str(script)],env=env,directory=output,
                    stdout=log,stderr=subprocess.STDOUT,check=True,timeout=timeout)
    path=output/'startup/startup.json'
    result=json.loads(path.read_text())
    result.update(exe_modified=False,exe_sha256=EXE_SHA256,
                  initial_save_sha256=initial_save_sha256)
    path.write_text(json.dumps(result,indent=2)+'\n')


def capture_race_stream(pid,output,env,timeout,level=9,image_frames=(),image_counters=(),full_history=False,physics_trace=False,step_window=(),step_levels=()):
    script=output/"race.gdb"
    module,function=('race_history_gdb','record_history') if full_history else ('race_stream_gdb','record_race')
    options='' if full_history else 'random_trace=True,'
    tail=f',physics_trace={physics_trace!r},step_window={step_window!r},step_levels={step_levels!r}' if full_history else ''
    script.write_text("\n".join(["set pagination off","set auto-solib-add off",f"attach {pid}",
        "python",f"import sys; sys.path.insert(0,{str(ROOT/'tools')!r})",
        f"from {module} import {function}",f"{function}({str(output)!r},{level},{options}image_frames={image_frames!r},image_counters={image_counters!r}{tail})",
        "end","detach","quit"])+"\n")
    with (output/"race-gdb.log").open("wb") as log:
        run_bounded(["gdb","--nx","-q","-batch","-x",str(script)],env=env,
            directory=output,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=timeout)
    result=json.loads((output/"race/race.json").read_text())
    result.update(exe_modified=False,exe_sha256=EXE_SHA256)
    (output/"race/race.json").write_text(json.dumps(result,indent=2)+"\n")
    print(f"Original racing loop captured: {len(result['frames'])} frames, {result['clock_calls']} actual tick returns",flush=True)


def capture_champ_history(pid,output,env,timeout,steps,initial_save_sha256,frame_delay_ms=0,normal_arena=False,full_video=False,natural_champ=False,steady_driver=False,natural_season=False):
    script=output/'history.gdb'
    script.write_text('set pagination off\nset confirm off\nset auto-solib-add off\n'+f'attach {pid}\npython\nimport sys\nsys.path.insert(0,{str(ROOT / "tools")!r})\nfrom champ_history_gdb import record_champ_history\n'+f'record_champ_history({str(output)!r},{steps},game_frame_delay_ms={frame_delay_ms},normal_arena={normal_arena!r},full_video={full_video!r},natural_champ={natural_champ!r},steady_driver={steady_driver!r},natural_season={natural_season!r})\nend\ndetach\nquit\n')
    with (output/'history.log').open('w') as log:
        run_bounded(['gdb','--nx','-q','-batch','-x',str(script)],directory=output,env=env,
                    stdout=log,stderr=subprocess.STDOUT,check=True,timeout=timeout)
    history=output/'history/history.json'
    meta=json.loads(history.read_text())
    if len(meta['keys'])!=(60 if natural_season else 20 if natural_champ else 9 if normal_arena else steps) or len(meta['checkpoints'])!=(66 if natural_season else 22 if natural_champ else 11 if normal_arena else steps+1):
        raise RuntimeError('Incomplete original championship history')
    meta['initial_save_sha256']=initial_save_sha256
    history.write_text(json.dumps(meta,indent=2)+'\n')


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


def navigate(pid, output, env, keys, key_hold, acknowledged, deadline, wine_log, cycle_frames=0, initial_save_sha256=None):
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
        "initial_save_sha256": initial_save_sha256,
        "checkpoints": [f"step{i:02d}-{key or 'boot'}" for i, key in enumerate(sequence)],
        "scope": "menu checkpoints; timing alignment and full parity pending"}, indent=2)+"\n")


if __name__ == "__main__":
    main()
