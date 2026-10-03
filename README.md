# wasm-dd2 — Destruction Derby 2 (dd2h.exe) → WebAssembly

A faithful port of the 1996 game **Destruction Derby 2** (`dd2h.exe`, 640x480 build) to reproducible C
compiling to native and WebAssembly. The engine C is **mechanically derived from the binary via Ghidra**
(decompile → transpile/patch → compile); only the platform/runtime shim (DirectDraw→SDL/canvas,
DirectSound→SDL/WebAudio, Win32/CRT) is hand-written.

Pipeline: `dd2h.exe → tools/decompile.sh → tools/transpile.py → tools/build*.sh`.
See **CLAUDE.md** for build commands, current status, and conventions.

Place the original game files (`dd2h.exe` and `Dirinfo`) in `DestructionDerby2/`, then
run `make image`, `make native`, `make wasm`, and `make web`. `make shot` captures the
browser build. Game files and generated builds are ignored by Git.
The ISO release packages the game in InstallShield 3's `DATA.Z`; it can be unpacked
with [unshieldv3](https://github.com/wfr/unshieldv3).

Alternatively, run `make provision`. It downloads the provided Windows BIN/CUE ZIP,
verifies its SHA-256, builds a pinned unshieldv3 extractor with Git/CMake/GCC, and
installs the game and memory image in `DestructionDerby2/`. It also extracts all 18
CD audio tracks byte-for-byte into `DestructionDerby2/Redbook/` with a disc manifest.
Existing `SaveGames` is preserved. Downloads, original files and audio are ignored
by Git. An existing ZIP can be used with
`python3 tools/provision_game.py --archive /path/to/Destruction-Derby-2_Win_EN_ISO-Version.zip`;
`--url` accepts a refreshed download link to the same verified archive.

On Debian, `emscripten`, `gcc-multilib`, `libc6-dev-i386`, `node-playwright`, and
`chromium` provide the build and browser-test tools. The build uses `emcc` from PATH,
or falls back to `~/Git/emsdk` (override with `EMSDK`). Browser tools use Playwright's
downloaded Chromium when available, otherwise `/usr/bin/chromium`; override with
`PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH`. They also adapt Debian Playwright 1.38's
callback-based `rimraf` dependency for clean browser teardown.
The browser shell creates an empty save-file target before linking it into IDBFS,
so Emscripten 3.1 can initialize the first memory card without a dangling symlink.

Native play uses the i386 SDL2 runtime (`libsdl2-2.0-0:i386`). Install
`libsdl2-dev:i386`, or run `make provision-native` to extract Debian's i386
development headers under ignored `third_party/` without installing them.
`make play-native` builds and opens the 640×480 game window, forwards keyboard
and a controller connected at boot, and queues the shared Float32 audio mix.
For native controller play, select Configuration → Control Method → Joystick.
Arrow keys navigate, Enter confirms, A accelerates, Z brakes, and Escape pauses.
The engine stays at debug optimization; the handwritten audio shim uses `-O2`
without fast-math so its exact FIR can keep up with real time. For a build
without SDL, use `DD2_BUILD_HEADLESS=1 make native`.
`make verify-native-sdl` checks actual renderer and X11 pixels, accepted SDL
audio bytes and keyboard/virtual-controller transport. `make verify-native-window`
uses real X11 keys on the game to check menus, CD controls, populated racing,
acceleration and pause/resume with exact renderer/audio comparisons. Tests need
Xvfb, xdotool and Python Pillow; output goes to fresh directories under `/tmp`.
These checks cover the SDL boundary, not physical audio/controller hardware or
complete original game-stream timing.
`make verify-native-window NATIVE_WINDOW_ARGS='--controller'` additionally
attaches a virtual SDL device before boot, selects Joystick using real menu
input and verifies actual player steering, movement, gas, brake and release.

`make verify-browser-pad` launches a live race with a synthetic Gamepad API
device present at boot. It checks steering endpoints, accelerator/brake/release
values in car physics, actual player movement and browser errors. It does not
exercise physical HID hardware.

The WASM builds disable LLVM FastISel. With Debian Emscripten 3.1.69 / LLVM 19, its
folded unsigned memory offsets trap on valid wrapping 32-bit engine addresses in
`AI_Com_Server` on level 10. SelectionDAG emits the required 32-bit addition.
`make verify` and `make verify-wasm` check every demo and fail on nonzero exits,
engine errors, or timeouts; detailed logs go to `/tmp/dd2-verify-native` and
`/tmp/dd2-verify-wasm`. These are crash checks, not proof of complete game fidelity.

The Node build transfers `DD2_*` options from `process.env` into Emscripten's libc
environment before boot. `make verify-parity` enables sound and compares every
presented indexed framebuffer, its palette, the flip/RNG log, the effects PCM,
combined Float32 device output, music part and raw 44100Hz CD PCM
byte-for-byte between native and WASM on all ten demos. Logs and results go to
`/tmp/dd2-parity`; identical captures are removed, failed captures are retained.
This target does not compare with `dd2h.exe` or validate menu actions.

Current Debian validation: all ten sound-enabled demos return on both ports.
Every presented frame and palette, flip/RNG log and generated PCM byte matches
across native and WASM (1525-1527 frames, 10569888 bytes in each effects,
combined and music-part Float32 stream, and 4939200 raw CD PCM bytes per demo). Patch
835 restores the original contiguous angle vector for the animated L1 objects;
its split stack locals caused the previous 137-frame discrepancy. Menu behavior,
full championships, other menu actions and complete comparisons with the running original
remain open acceptance work.

Redbook playback now uses the original engine's MCI track selection, Play, Stop,
resume and repeat calls. Patches 836/837 restore the original contiguous MCI
parameter blocks and mandatory CD check. The backend reads the original stereo
s16le CDDA at 44100Hz; browser builds serve the tracks separately and load one
track at a time. The C mixer combines CD and effects in source creation order
on one 44100Hz Float32 stereo device, delivered through one SDL/WebAudio sink.
Effects retain their source rates; the calibrated C FIR converts them once.
CD source capture uses `DD2_CDPCM=<file>`; effects use `DD2_SNDPCM=<file>`.
`DD2_MIXPCM` captures the final mix and `DD2_MUSICPCM` its music part, all
Float32 little-endian stereo at the device rate with `.json` format sidecars.
`DD2_SND_RATE` can select 22050/44100/48000Hz for calibration.
Deterministic runs share a 25Hz audio clock. Interactive
CD playback and the 400ms multimedia timer use elapsed real time, including menus.

`make verify-redbook` compares native/WASM playback directly with all 18 track
prefixes and one complete track: 29848052 exact PCM bytes, including stop/resume,
pause, cross-track boundaries, end-of-track, replay and error checks.
`make verify-redbook-controls` checks drained/live Pause/Resume and repeated
controls against 11 transport-state records and 50568 exact source PCM bytes.
Both complete mixed/music streams also match 515088 exact Float32 bytes,
including silence after completion and throughout repeated pause commands.
With `REDBOOK_CONTROLS_ARGS='--wine --mingw <32-bit compiler> --output <new dir>'`,
the same fixture runs against actual Wine MCI. Pause/Resume leave a completed
track stopped; explicit Play starts a new transport. The one-second drain wait
excludes asynchronous CD ring-end timing from this state comparison.
`make verify-redbook-restart` checks the original's Stop/TO-only Play restart:
the public CD position has sector resolution, so restarting repeats the current
fractional sector. MCI Pause/Resume retain the same buffer's sample cursor.
The fixture checks 70736 exact source and 317872 complete mixed PCM bytes.
`REDBOOK_RESTART_ARGS='--wine --mingw <32-bit compiler> --output <new dir>'`
also captures Wine's actual mixer with an explicit hardware Q snapshot. Both
literal source starts must match byte for byte; altered PCM and the old
within-sector restart are rejected. Queued control edges and the live game's
transport clock remain outside this fixture's proof.
Native window QA also pauses/resumes a real race through X11, checks the
public sector and actual resumed source bytes, and verifies the accepted SDL
mix. `make verify-browser-redbook-restart` checks the live browser keyboard
flow, stopped race/CD state, resumed CDDA bytes and every shared WebAudio
buffer bit. `BROWSER_RESTART_OUTPUT=<empty dir>` retains its report/source/PNG.
Both checks exercise a fractional stopped sector, so the preceding incorrect
sample-cursor restart cannot pass merely by stopping at an aligned sector.
`node tools/browser/qa_redbook.js web/dd2`
navigates the CD-player menu with real keyboard input, checks Play/Stop/Next/Prev,
and compares the submitted WebAudio buffers from tracks 2/3 with their CDDA files.
These checks establish exact source PCM and exercised controls. They do not yet
establish mixed hardware-output parity or original pause/resume timing.

Menu effects now keep playing with the race counter fixed: interactive DirectSound
mixing uses elapsed real time and flushes samples before buffer controls/queries.
`make verify-menu-audio` checks looping, Stop, one-shot exhaustion and frequency
changes on both backends against 9696 known Float32 PCM bytes with `cf=0`.
`node tools/browser/qa_menu_audio.js web/dd2` verifies real menu navigation submits
nonzero effect buffers without advancing the race counter, and compares every
submitted WebAudio sample bit with its C mixer source. The complete original
mixed stream and its timing remain unverified.

DirectSound buffer controls now preserve the source cursor: `SetCurrentPosition`
seeks by byte offset, repeated `Play` continues playback, and `Stop`/`Play`
resumes instead of restarting. Automatic one-shot completion resets the cursor
to zero, and high-frequency steps wrap correctly across multiple short loops.
`GetCurrentPosition` now returns cursor values; status follows consumed samples
rather than an estimated race-frame duration. The original engine explicitly
seeks zero when starting a new effect (`dd2h.exe` at 0x415e19-0x415e21).

```sh
make verify-sound-cursor
# Optional real DirectSound API reference; requires 32-bit MinGW, Wine and Xvfb:
make verify-sound-cursor SOUND_CURSOR_ARGS='--wine --mingw i686-w64-mingw32-gcc'
```

Both ports match all 14 stopped-buffer cursor/error records from real Wine
DirectSound, including stereo frame alignment and invalid offsets. Controlled
seek/repeated Play/Stop/resume/end/short-loop sequences match 2992 known Float32 PCM bytes
exactly on both ports. The Wine check uses a separate API fixture, not the game
EXE, and does not establish original mixed PCM or timing equality.

Effects now retain Float32 precision through mixing and WebAudio delivery. The
old Q15 gain and per-buffer 16-bit clipping differed from actual Wine output:
at volume -600 a constant 0.5 source produced 0.2505798340 instead of
0.2499961853. A reproducible integer table now supplies the observed backend's
quantized gain; products and sums round to Float32 consistently on x87 and WASM.
Volume, pan and frequency queries, invalid parameters, control capabilities,
original-frequency restoration and configured duplicate-buffer settings are
also exercised against real DirectSound.

```sh
make verify-sound-gain
# Actual Wine PCM calibration, captured under a clocked virtual device:
make verify-sound-gain SOUND_GAIN_ARGS='--wine --mingw i686-w64-mingw32-gcc --output /tmp/fresh-gain-calibration'
```

The port test checks all 10001 volume values, six pan steps and a three-source
mix above 1.0 against exact Float32 bytes. The Wine calibration checks 13 gain/pan
cases, unsigned mono8 normalization and the same mixed amplitude with source
and device both at 22050Hz.
Every reference sample must be an exact expected value, silence, or an exact
source combination during multi-source start/stop; no amplitude tolerance is
allowed. Tone extents and startup/trailing silence are reported separately.
One-bit corruption, a lost tone and a clipped mix are rejected. These isolated
fixtures establish gain/mix amplitudes and exercised API controls; resampling,
full original stream timing, combined CD/effects output and Windows hardware
equivalence remain open. The observer uses `DD2_AUDIO_PROCESS=gain.exe` for the
fixture; original game captures still default to `dd2h.exe`.

Patch 838 fixes corrupted championship names: the computer-name table began at
an 8-byte offset per human instead of the original 16-byte offset. A real menu
run through Championship, name entry, Go, Pause/Retire/Yes and View League now
matches the original in both native and browser rendering on all four divisions:
all 20 names/points, all 307200
framebuffer pixels per page, and all 1024 palette bytes per page.
`node tools/browser/qa_champ_scores.js web/dd2 /tmp/fresh-champ-browser` exercises
that flow and saves the score data, raw pixels/palettes and screenshots. It releases
keys after a presentation confirms that the engine polled their control bits.
It waits for 16 face-on slab presentations before menu input, because the
transition also polls controls but ignores navigation. `--slow-menus` repeats
the selection/name-entry flow with 3x CPU throttling, restoring normal speed
before the realtime race. Both modes cover Retire/Yes and all four divisions;
the test has a five-minute deadline for an unresponsive browser.

`make verify-champ-names` tests the real engine's naming code on native/WASM with
1/2/5/10 human drivers. For the original reference and browser comparison:

```sh
python3 tools/reference/capture.py --mode menu --acknowledged-key --timeout 150 \
  --output /tmp/fresh-champ-original --keys Return Return Return Return Up Left \
  Return Down Down Return Escape Down Down Down Return Up Return Right Return Right Right Right
make verify-champ-names CHAMPREF=/tmp/fresh-champ-original \
  CHAMPBROWSER=/tmp/fresh-champ-browser/scores.json
```

For the native full frontend (requires a debug build and GDB):

```sh
ASAN=' ' bash tools/build_native.sh /tmp/dd2_native
python3 tools/capture_native_menu.py --output /tmp/fresh-champ-native --keys \
  Return Return Return Return Up Left Return Down Down Return Escape Down Down Down \
  Return Up Return Right Return Right Right Right
make verify-champ-names CHAMPREF=/tmp/fresh-champ-original \
  CHAMPBROWSER=/tmp/fresh-champ-browser/scores.json CHAMPNATIVE=/tmp/fresh-champ-native
```

The reference sends real X11 key events and uses hardware breakpoints to let each
key reach the pad reader and acknowledge its release, without changing engine
memory. It records all intervening held polls; Wine may queue key-up late.
Ordinary `--keys` can also use `--key-hold=<seconds>`. Navigation captures include races/results.
The native/WASM score-builder comparison uses captured original standings as an
explicit fixture; browser and native captures exercise live championships. The
native helper calls the normal `dd2_key_event` bridge for one pad poll per key
and checks the real renderer output; native window-system input and hardware audio
are not exercised. Full seasons, promotion/relegation and complete streams still
need acceptance checks. Use fresh capture directories; stale output is rejected.

Patch 847 restores the original replay script's WORD packets and two-byte cursor
steps. DWORD cursors skipped a packet whenever the control changed, leaving
zero-repeat gaps; playback then read the gaps. The termination helper also
overwrote the next packet with a DWORD store.

```sh
make verify-replay
node tools/browser/qa_replay.js web/dd2 /tmp/wasm-dd2/fresh-browser-replay
```

The component check executes four actual original x86 functions from the verified
unmodified executable with explicit inputs and compares all 384 script/car-state
checkpoints with native/WASM. Reversing only patch 847 rejects both targets at the
first skipped WORD; changed first bytes and truncated checkpoints also fail.
It requires 32-bit GCC, Emscripten and Node, writes bounded output under
`/tmp/wasm-dd2/`, and deletes successful raw comparisons after writing the report.
The browser check drives a real practice race, records changing acceleration and
steering, chooses Retire/Yes/View Replay, and waits for natural playback completion.
The old browser build is rejected for zero-length packets. Shared browser race
launch detection uses advancing physics ticks, level and quit state: byte
0x460005 belongs to a cached texture CLUT pointer and is not a screen identifier.
Recorded/replayed positions at common ticks are diagnostic observations, not
complete original replay video/audio acceptance.

Patch 848 restores packed replay metadata WORD stores and signed WORD loads.
The final magic store previously erased the selected car; loading read adjoining
fields together, and the file dispatcher compared a combined magic/car DWORD.
`make verify-replay-metadata` executes the actual original pack/load functions
in 16 isolated runs (eight cases each), stopping with read-only hardware
breakpoints before their external file browser/player calls. Both ports match
every packed byte, script, car order, guard and setup field (14462 bytes per
checkpoint). Cases include real values, negative WORDs and discarded upper
halves. Removing only patch 848 rejects the overwritten car and widened loads.
GDB is required in addition to the replay component dependencies. These API
boundary comparisons do not run the original file browser or complete player;
full original save/load behavior, replay trajectories and original audio still
require acceptance.

`make verify-browser-replay-save` exercises actual Select Car/practice/Retire/
Yes/Save Replay/name entry, normal page navigation, File Manager loading and
natural playback completion. It checks every script/order byte, the complete
128-KiB card hash before/after restart, RAM/file equality and restoration of
the previous frontend configuration. It also cancels then confirms deletion
and checks the deleted card after another restart. The test performs no engine
or file writes and never calls `syncfs` itself. Its report uses a fresh directory
under `/tmp/wasm-dd2/`; `BROWSER_REPLAY_SAVE_OUTPUT` can supply that path.

The browser now persists actual card writes while the original card FILE remains
open. Writes are batched at the next event turn; asynchronous IndexedDB snapshots
are serialized and a write during an active sync queues another snapshot.
The previous unload-only shell loses the named replay on ordinary navigation
and is rejected by the same test.

`make verify-native-replay` exercises the same save/restart/load/playback and
cancelled/confirmed deletion flow in the real native SDL window, using X11 keys.
It boots the normal intro/frontend three times, checks the entire 128-KiB card
against engine RAM, compares complete saved/loaded script/order bytes, requires
natural playback and restores the previous frontend configuration. A private
game directory prevents changing provisioned/user saves and is removed after
successful checks. `NATIVE_REPLAY_ARGS='--output /tmp/wasm-dd2/fresh-native-replay'`
selects the report path; Xvfb and xdotool are required. This is functional native
window acceptance. Whole original menu/player video/audio and overwrite/full-card
behavior still need checks.

`make verify-card` compares seven actual card functions with unmodified original
x86 over 26 explicit cases: empty/full/sparse cards, first free blocks 0/7/14,
reserved flags, missing/dormant names, duplicate-name exemptions and first
configuration selection. Each native/WASM comparison covers all 131072 card
bytes, every directory row, guards, complete 8192-byte source/loaded blocks and
the return value or external seek boundary (148040 bytes total). Read-only
hardware breakpoints stop original execution before file seeking or at the
fixture return marker; original code/registers are never changed. Mutated guard,
directory, first/last payload and outcome bytes, and truncated checkpoints fail.
This establishes component conformance, including refusal to save a full card;
it does not run the complete original memory-card menu or establish host file
persistence. Reports use `/tmp/wasm-dd2/`, and successful raw checkpoints are
discarded. GDB, 32-bit GCC, Emscripten and Node are required.

Menu graphics can be compared over complete 64-frame highlight cycles. This
checks every indexed pixel and palette byte at presentation, including the
browser's canvas conversion; it uses no pixel mask or tolerance. For the path
Main Menu -> Configuration -> Audio Volume:

```sh
python3 tools/reference/capture.py --mode menu --acknowledged-key --menu-cycle 64 \
  --timeout 150 --output /tmp/fresh-original-cycle --keys Down Right Right Return Right Return
python3 tools/capture_native_menu.py --menu-cycle 64 --timeout 150 \
  --output /tmp/fresh-native-cycle --keys Down Right Right Return Right Return
node tools/browser/capture_menu_cycle.js web/dd2 /tmp/fresh-browser-cycle \
  Down Right Right Return Right Return
make verify-menu-cycles MCREF=/tmp/fresh-original-cycle MCNATIVE=/tmp/fresh-native-cycle \
  MCBROWSER=/tmp/fresh-browser-cycle MCREPORT=/tmp/fresh-cycle-report.json
```

All seven checkpoints on this path match the original on both ports: 448 frames
per target. `Draw_All` presents the existing framebuffer before rasterizing the
next image, so reference/native captures stop at entry. Pairing by the recorded
highlight counter aligns this specific animation. It does not establish wall-clock
timing, other animations, input-repeat timing or audio-stream equality.

Comparing with the Windows original also needs 32-bit Wine, GDB and Xvfb. On Debian:
```sh
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install wine wine32:i386 gdb xvfb xauth
```

`make refcapture` runs the unmodified Windows EXE in a private 32-bit Wine prefix
under `/tmp/wasm-dd2/wine-reference/`, with an isolated copy of `SaveGames`. A Linux
CD device adapter exposes the provisioned disc TOC and exact CDDA sector reads
through Wine's normal MCI/DirectSound driver. It needs no physical CD drive and
does not patch the EXE or engine memory. `make verify-cdrom` checks all TOC entries,
108 audio reads including track boundaries, invalid requests and descriptor
isolation. These checks verify the CD data source, not mixed playback PCM parity.
The default adapter reports NO_STATUS with a track2 Q address. Wine clears its
Q cache in that state and ignores the supplied address; a live TO-only restart
can fall back to track2. The optional `DD2_CD_Q_POSITION` file supplies an explicit
absolute LBA and Linux audio status for controlled API fixtures. The restart
fixture first observes a PLAY snapshot, then reports COMPLETED to retain Wine's
cached sector through Stop. These are declared device inputs, not an actual
transport clock. See [Wine's Q-channel conversion](https://github.com/wine-mirror/wine/blob/wine-10.0/dlls/ntdll/unix/cdrom.c).
Live pause/resume timing still needs a valid transport reference before complete
original audio acceptance.

Verification captures and logs belong under `/tmp/wasm-dd2/`. Never store
them in this repository. On this machine `/tmp` is a 16 GiB tmpfs. Racing
capture/comparison subprocesses stop above 2 GiB of output or below 1 GiB
free space. Successful race comparisons discard raw port frames after writing
their JSON report; failed captures remain for the current diagnosis. Delete
completed original captures when that diagnosis is finished. Old logs (over
one hour) are cleaned before/after the main capture and verification commands;
open logs are preserved. Run `make clean-logs` for the same cleanup manually.

Complete original racing-loop captures also record every actual game clock
return and Watcom random call using read-only hardware breakpoints. For a later
naturally selected attract race (the rotation starts 9, 6, 5, 2, 3, 4):

```sh
make refcapture-race-stream RACE_CAPTURE_ARGS='--race-level 6 --timeout 360 --output /tmp/wasm-dd2/fresh-original-l6'
make verify-reference-race-stream RACE_REFERENCE=/tmp/wasm-dd2/fresh-original-l6 RACE_COMPARISON=/tmp/wasm-dd2/fresh-comparison-l6
make verify-random-reference RANDOM_REFERENCE_ARGS='--output /tmp/wasm-dd2/fresh-random-check'
```

Add `RACE_COMPARISON_ARGS='--attract-history'` to run the real frontend and
preceding demos. That mode requires the naturally calculated initial RNG seed
and blink counter to match; neither is overwritten from the reference. Only
the selected race receives the recorded clock. The complete L6 capture also
passes on native/WASM after the real L9 demo in this mode.
New captures also record each preceding demo at its original first game-clock
return, using a read-only hardware breakpoint. The completed manifest contains
`preceding_demos`; `preceding-demos.jsonl` retains these entry observations even
if the capture times out. Attract-history comparisons require the recorded
level sequence to match and reject an extra preceding demo. These entry records
do not capture the preceding races' complete clocks or establish full-prefix
timing equivalence.

Add `--race-full-history` to `RACE_CAPTURE_ARGS` to record every preceding
race's clock inputs and all Watcom random calls from the frontend, including
demo selection. Compare this capture with `--attract-history`. Both ports
calculate the global random stream from boot seed 1, replay all preceding
clock inputs and check the observed demo entry states. No inherited engine
state is copied. Only the selected racing loop's video is captured and checked;
this mode still excludes original audio and physical output clocks. A fresh
L6 capture passes both ports after the actual sequence [9,7,3,1,1,7], with
1403 target frames, 154599 total clock inputs and 25198 global random calls.
A fresh L7 capture also passes both ports after its actual L9 prefix: all
1115 target frames, 39063 total clock inputs and 11733 global random calls.
The comparator records partial results for both targets after an engine exit
or deadline and rejects incomplete clock/RNG consumption and missing diagnostic
images. A failure in the first target does not skip the second target.

For a read-only native physics diagnosis, add `--race-physics` to a full-history
capture. It saves all 20 cars' primitive/dynamics, render, handling and wheel
records at every preceding and selected race presentation and records every original random
caller's return address and engine phase. Run the native observer with:

```sh
python3 tools/reference/trace_native_physics.py --capture /tmp/wasm-dd2/original-physics-capture --output /tmp/wasm-dd2/fresh-native-physics
```

The observer uses native hardware breakpoints and compares measured states;
it copies no engine state. Add `--race-step-window START_CF END_CF` to the original
capture to observe every Car_Movement entry and return in a small counter window.
Optional `--race-step-levels 8 10` includes those windows in preceding demos too.
Entry/return observations reuse one hardware slot, and debugger or breakpoint
insertion errors reject the run. A fresh L9 control matches all 401 car checkpoints
(10345800 bytes) and all 6948 random callers/phases. Changed first car bytes
and caller names are rejected. This diagnoses native physics; use the separate
video comparator for native/WASM framebuffer acceptance.

For the older L3 capture, a controlled native experiment isolates the effect
of retained debris vertices:

```sh
python3 tools/reference/diagnose_debris_history.py --capture /tmp/wasm-dd2/dd2-original-race-l3-stream-reviewed --output /tmp/wasm-dd2/fresh-l3-debris-controls --slot 30
```

It first runs the unchanged port, then uses GDB to copy only the inactive
slot's 24 vertex bytes before the first race clock call. The unchanged run
differs by 25 pixels in one frame; the controlled run matches all 1243 frames.
This diagnoses missing inherited initial state in the direct harness. It does
not establish naturally calculated history or WASM acceptance. The original
executable and reference captures remain untouched.

By default, the ports match the observed initial RNG seed and inherited DEMO MODE blink
counter once, then calculate every subsequent random result/state and blink
phase themselves. Every presented framebuffer/palette byte and clock/random
call phase is compared literally, including the countdown and final rendered
frame. Missing, partial, leftover or changed random records fail; a deliberately
changed multiplier is also rejected on native ASan/UBSan and WASM. A changed
first pixel or recorded state phase must fail the game comparison. The actual
supported CRT is Watcom (`rand` at 0x456cbc), with multiplier 0x41c64e6d and
increment 0x3039; earlier MSVC/address comments were incorrect.

`--race-images INDEX...` additionally saves original engine memory at selected
presentation indices for diagnosis. `--race-image-counters CF...` saves every
presentation of selected counters, including both occurrences of counter zero
across the green-light reset. Port diagnostic filenames use presentation
indices so these occurrences cannot overwrite one another. Captures and original assets remain ignored
by Git. Debugger stops affect elapsed time, so replaying the observed API clock
is an explicit comparison input. These checks cover the captured racing render
loop; initialization/fades, original racing audio, other modes and physical or
undebugged output timing still require acceptance.

Several L9 video checkpoints can be captured from one unmodified attract run
and compared with one run of each port:

```sh
python3 tools/reference/capture.py --mode attract --frames 50 150 300 450 600 650 \
  --timeout 150 --output /tmp/fresh-original-video
python3 tools/reference/compare_video.py --capture /tmp/fresh-original-video
```

The manifest records each actual counter and snapshot directory. Captures wait
for the green light because the countdown resets the counter. Attract requests
are limited to cf1..700: Play_Game's 1500-step demo budget ends before larger
race counters. These are exact framebuffer/palette checkpoints; complete
original video/audio streams and other tracks still need acceptance checks.

The original Wine mixer's accepted PCM can now be captured directly, without a
debugger, through a private ALSA device with a monotonic sample clock. On Debian
13 this additionally needs `libasound2-dev`, `libasound2t64:i386` and the existing
GCC multilib toolchain. The observer and device build for both process widths:

```sh
make verify-audio-observer
python3 tools/reference/capture.py --mode audio --audio --audio-rate 44100 \
  --audio-tail 3 --output /tmp/fresh-original-audio
```

`audio/summary.json` records the actual committed format, frame counts and PCM
hashes; the tested original selects stereo Float32 at 44100Hz. Each stream's
`.jsonl` journal records accepted/failed writes, monotonic call bounds and
transport events; its `.pcm` contains only accepted bytes. `audio/engine.jsonl`
records bounded, non-atomic observations of the live engine. The capture starts
at process launch and can include the intro and automatic transition from the
menu to the attract demo. The EXE and engine memory remain unmodified; no GDB
stops occur in audio mode. Use fresh output directories and keep these original
PCM files outside Git.

The ALSA device's clock, polling, manual start, pause/resume, rewind, prepare
reset and underrun pass real 32/64-bit ALSA tests with exact timestamp-derived
sample bounds. Observer tests check exact accepted bytes, failed-write exclusion,
process isolation and rejection of damaged captures. A live reference accepted
3.100 seconds of PCM over 3.060 seconds of wall time. The old ALSA null device
accepted 9.263 seconds over 2.573 seconds; `--audio-device null` remains a diagnostic
option and is unsuitable for timing acceptance. Accepted PCM includes queued
data; drops/rewinds must be applied before constructing a playback timeline.
`--audio` also works with video/navigation captures, whose debugger stops can
cause audio underruns. Full original/native/WASM mixed PCM comparison, playback
alignment and Windows hardware equivalence remain open acceptance work.

The capture stops at `Draw_All` entry using a hardware breakpoint and saves the
original image, framebuffer, palette and checkpoint metadata to a fresh output
directory. For example:

```sh
OUT=/tmp/dd2-ref-150 make refcapture
REFMODE=menu OUT=/tmp/dd2-ref-menu make refcapture
```

The launcher skips the intro with Escape, waits for the selected checkpoint,
fails on errors/timeouts and terminates only its own Wine prefix and Xvfb.
Original menu and L9/cf150 captures now work on Debian. The new L9/cf150 reference
matches both native and WASM exactly: all 307200 indexed pixels and 1024 palette
bytes at the first `Draw_All` entry. Reproduce the comparison of an existing
capture with `make verify-reference-video REFCAP=/tmp/dd2-ref-150`. This checks
one checkpoint; systematic full-run comparisons and original audio output
validation remain pending.
