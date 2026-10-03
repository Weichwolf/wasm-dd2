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

Track Select has a reproducible original/native/browser comparison for all seven
road previews and four bowl previews with the default four-road/one-bowl unlock
state. Each case uses 28 real navigation actions and records 64 consecutive
presentations at each of its 29 checkpoints. All 3,712 complete framebuffer and
palette pairs match the unmodified original on each port; the browser also
checks every actual canvas pixel. The gate checks track/mode/type, screen list,
unlock limits, lock flag and the track saved for cancellation. It covers locked
confirmation, wraps, confirmation/cancellation/reopening and F1/F2 shortcuts.
It pairs by the observed highlight counter and does not prove chronological
transitions, race launch, earned unlocks, live timing or audio.

The original retains its lock flag after cancelling a locked preview: opening
the restored unlocked track and pressing Enter still leaves the menu open.
Changing the track clears/recomputes the flag. Both ports reproduce this
observed original behavior; this test does not repair that original defect.

Capture each case (`road` and `bowl`) on all three targets using fresh directories:

```sh
make clean-logs
make verify-track-selection TRACK_SELECTION_ARGS='capture --case road --target original --output /tmp/wasm-dd2/track-road-original'
make verify-track-selection TRACK_SELECTION_ARGS='capture --case road --target native --output /tmp/wasm-dd2/track-road-native'
make verify-track-selection TRACK_SELECTION_ARGS='capture --case road --target browser --output /tmp/wasm-dd2/track-road-browser'
make verify-track-selection TRACK_SELECTION_ARGS='compare --case road --original /tmp/wasm-dd2/track-road-original --native /tmp/wasm-dd2/track-road-native --browser /tmp/wasm-dd2/track-road-browser --report /tmp/wasm-dd2/track-road-report.json --negative --clean'
make clean-logs
```

Original input uses acknowledged X11 keys; native uses the normal headless
`dd2_key_event` bridge; browser uses DOM keyboard events. All start from the same
supplied SaveGames bytes, with no engine-state writes. Seven damaged image,
palette, selection-state and phase-sequence checks reject on each target.
Successful raw images are removed after the report retains their hashes and
navigation states. Current reports: `/tmp/wasm-dd2/track-{road,bowl}-verified-851.json`.

Patch 851 corrects championship outcome classification: the original reads the
league and rank as WORDs, while the port read overlapping DWORDs. Last place in
division 4 therefore returned relegation instead of elimination and restarted
the bottom season. Both ports now finish the five-race Retire/Yes path by returning
to the frontend as the original does. This also restores the other classification
branches when neighboring temporary points are nonzero.

`make verify-league-standing LEAGUE_STANDING_ARGS='--output /tmp/wasm-dd2/fresh-standing-check'`
compares 210 explicit packed-field cases with actual unmodified original x86 on
native/WASM and rejects the old DWORD aliases. The original full five-race run
also matches native's cumulative standings, all 100 league names/points and
20 complete league framebuffer/palette pairs. The live browser completes the
same retirement/elimination flow, but its strict original comparison remains
open: the first 12 league images match; races 4/5 have different computer points.
The browser's live simulation batches and pause times differ. A complete original
input/clock/RNG recording is required to diagnose this; matching a visible counter
alone does not establish matching physics. Chronological video/audio, ordinary
race finishes, promotion and other seasons are not accepted by this test.

```sh
make clean-logs
make verify-champ-season CHAMP_SEASON_ARGS='capture --target original --output /tmp/wasm-dd2/season-original'
make verify-champ-season CHAMP_SEASON_ARGS='capture --target native --output /tmp/wasm-dd2/season-native'
make verify-champ-season CHAMP_SEASON_ARGS='capture --target browser --output /tmp/wasm-dd2/season-browser'
make verify-champ-season CHAMP_SEASON_ARGS='compare --original /tmp/wasm-dd2/season-original --native /tmp/wasm-dd2/season-native --browser /tmp/wasm-dd2/season-browser --report /tmp/wasm-dd2/season-report.json --negative --clean'
make clean-logs
```

The full comparator stays strict and currently fails for the live browser run;
`--clean` removes raw captures only if both targets pass. Browser `capture` also
supports `--reference <original-capture> --stop-after-pause` as a focused input
diagnosis, with observations saved before teardown; this is not a full-season
acceptance run. The historical patch-834 original-crash diagnosis was wrong:
the unmodified original eliminates this player and never decrements the season.
Its user-sanctioned clamp remains for an inconsistent season/league state.

Patch 852 repairs championship promotion/relegation: the port confused the
driver loop counter with the stored last-place driver of the top division.
It consequently wrote league/rank into car index 20, outside the 20-driver
table, and left duplicate/missing ranks for the next season. The patch restores
the original independent counter and all six stored transfer indices. It also
removes patch 833's artificial sorting-array initialization: the original reset
zeros points, preserving league membership, and valid transfers fill every slot.

`make verify-season-transition SEASON_TRANSITION_ARGS='--output /tmp/wasm-dd2/fresh-season-transition'`
compares 6,240 explicit component cases with actual original x86: all human
league/rank positions, eight driver permutations per position, point ties/order,
five statistics-history slots and existing/new unlock limits. Native and WASM
match complete targeted standings, statistics, scalar fields and surrounding
guards. The old code fails all 480 transfers and 4,320 continuing-season cases;
its write beyond the driver table is explicitly rejected. This proves these
functions on the supplied inputs; ordinary race finishes, earned promotions,
live subsequent seasons and their full chronological A/V still need acceptance.
All ten rebuilt native/Node demos remain exact (15,255 frames and generated PCM).
A fresh native retirement run again matches all 20 original league images;
the fresh browser completes elimination but fails the strict points/image gate
across all five races with different independently observed pause times. Reports
are `/tmp/wasm-dd2/season-transition-fixed-852-final/report.json`,
`/tmp/wasm-dd2/parity-season-transition-852.json` and
`/tmp/wasm-dd2/champ-season-standing-fixed-852.json` (last report remains failed).

Patch 853 restores the six contiguous Total Destruction timer digits. Ghidra
split the minute digits from a four-element array, while the sprite loop still
read six elements; native and WASM displayed stack garbage in the first pair.
The patch restores the original contiguous stores and reverse digit traversal.

The natural arena comparison drives the actual original with X11 keys, selects
Total Destruction on arena 10 and holds Up/Right until the normal finish, without
Retire or engine-state injection. Native uses the normal keyboard bridge;
the production browser uses DOM keyboard events and checks actual canvas pixels.
All calculated RNG triples are checked against the original from boot seed 1;
recorded original clock returns are explicit inputs. Both targets match all
516 indexed racing Draw_All-entry/platform pictures and palettes, 11 navigation
states, race-ready/result pictures and the natural completion state, consuming
3,624 clock values and 15,018 independently calculated random values. The
pre-fix native and browser recordings each fail 503 racing pictures and the
race-ready picture. Changed pixels/palettes, Retire, unfinished races and changed
or truncated random evidence are rejected. This accepts this arena input trace;
loading/fades, physical timing, chronological PCM and other races remain open.

```sh
make clean-logs
python3 tools/reference/capture.py --mode menu --champ-history --normal-arena-history --timeout 420 --output /tmp/wasm-dd2/arena-original
python3 tools/capture_native_champ_history.py --reference /tmp/wasm-dd2/arena-original/history --output /tmp/wasm-dd2/arena-native
python3 tools/wasm_rng_layout.py --wasm web/dd2/index.wasm --output /tmp/wasm-dd2/arena-wasm-layout
node tools/browser/capture_champ_season.js web/dd2 /tmp/wasm-dd2/arena-browser --api-reference=/tmp/wasm-dd2/arena-original/history --rng-layout=/tmp/wasm-dd2/arena-wasm-layout/layout.json
make verify-normal-arena-history NORMAL_ARENA_HISTORY_ARGS='--original /tmp/wasm-dd2/arena-original/history --native /tmp/wasm-dd2/arena-native/history --browser /tmp/wasm-dd2/arena-browser --report /tmp/wasm-dd2/arena-report.json --clean'
make clean-logs
```

Use fresh directories and build native, Node and browser sequentially first.
The report retains exact image hashes and deletes successful raw pictures with
`--clean`, after checking that no process has them open. `--before-report` also
validates a retained failing baseline against identical original image/API
provenance. Current evidence is
`/tmp/wasm-dd2/normal-arena-timer-digits-853-verified.json`.
The rebuilt browser also passes the controlled five-retirement original API
comparison (44 clock returns, 2,610 random calculations, 96 checkpoints, five
complete standings and 20 retained original league-image hashes), recorded in
`/tmp/wasm-dd2/champ-browser-api-timer-853-verified.json`. The earlier live-clock
browser comparison remains a separate failed case; its post-Retire Steam calls
were traced to the original's timing-dependent rest loop rather than removed.
All ten rebuilt native/Node demos also match exactly: 15,255 presented pictures,
palettes, flip/RNG logs and effects/CD/mixed/music PCM. The report is
`/tmp/wasm-dd2/parity-timer-digits-853.json`; this regression compares the ports,
without establishing complete original racing audio parity.

The extended arena capture adds `--normal-arena-full-video` to the original
command and `--full-video` to the comparator. Native and browser detect this
recording and wait through the normal 64-frame main-menu blink cycle to reach
the original's initial phase; they never write its counter. Full capture stops
at actual original/native `PutDispEnv` entries, including the 24 loading-progress
presentations from `Swap_Buffers` that a `Draw_All`-only recorder misses.
The production browser records and checks every corresponding canvas update.

The fresh extended trace matches all 1,323 indexed pictures and palettes on both
ports, including menu transitions, loading progress, 957 racing presentations,
the final racing picture, fade and 16 steady result pictures. All observed game
states, blink phases and per-picture clock/RNG extents match at 8,046 recorded
clock returns and 14,912 independently calculated random values. Changed menu
phases and API extents are rejected in addition to the other corruptions above.
The debugger's initial pad-observation count is zero for an attach after that
frame's poll and one for an attach before it; every subsequent poll is compared
relative to this explicitly reported starting boundary. Input events and engine
counters are unchanged. Reports are
`/tmp/wasm-dd2/normal-arena-full-present-853-verified.json` and the earlier failed
boundary diagnosis `/tmp/wasm-dd2/normal-arena-full-draw-anchor-853-diagnosis.json`.
The check starts in the main menu: earlier startup/intro, chronological racing
PCM, physical timing and all other input traces still need separate acceptance.

The first Wrecking championship race now also has an original/native/browser
comparison without Retire. The original receives real X11 keys from a read-only
road follower; both ports replay its recorded transitions at the same racing
presentations. The accepted trace ends with a destroyed player in lap two, shows
all four league pages and starts the next actual race on level 2. All 834 racing
pictures/palettes, result and league images, cumulative standings and game/API
states match both ports at 6,187 original clock returns and 6,542 calculated RNG
calls. The report is `/tmp/wasm-dd2/natural-champ-854-release-verified.json`.
This proves that one destruction/result/continuation path; completed laps,
wins, a complete natural season, other menu/loading/fade video and racing PCM
still need separate acceptance.

```sh
make clean-logs
python3 tools/reference/capture.py --mode menu --champ-history \
  --natural-champ-history --timeout 1500 --output /tmp/wasm-dd2/natural-champ-original
python3 tools/capture_native_champ_history.py \
  --reference /tmp/wasm-dd2/natural-champ-original/history \
  --output /tmp/wasm-dd2/natural-champ-native
node tools/browser/capture_champ_season.js web/dd2 /tmp/wasm-dd2/natural-champ-browser \
  --api-reference=/tmp/wasm-dd2/natural-champ-original/history \
  --rng-layout=/tmp/wasm-dd2/browser-timer-853-layout/layout.json
python3 tools/verify_natural_champ_history.py \
  --original /tmp/wasm-dd2/natural-champ-original/history \
  --native /tmp/wasm-dd2/natural-champ-native/history \
  --browser /tmp/wasm-dd2/natural-champ-browser \
  --report /tmp/wasm-dd2/natural-champ-verified.json --clean
make clean-logs
```

Use fresh output directories and a discovered RNG layout matching the browser
binary. Racing images are stored with lossless zlib and decoded for literal byte
comparison; successful raw/compressed pictures are deleted after the report.
The browser drains a bounded picture queue at normal Asyncify presentation
yields and holds the final yield callback while the observer writes its report.
It never sets engine counters or changes recorded API returns. The driver
releases held gameplay keys in the last racing picture, before `Setup_Pad(0)`
remaps A's flag to menu Return: releasing A after the remap leaves that flag held
in the original and loses the first Enter edge. Driving-transition timing,
corrupt pixels, incomplete/trailing zlib and non-natural exits are rejected.

The browser recorder also observes the actual player position, speed, lap,
destruction and completed-lap flag at the first presentation after gameplay
ends. The verifier compares this observation with the original's final player
state. Use `--require-completed-laps --require-player-points` on
`verify_natural_champ_history.py` to require a surviving regular finisher and
positive points earned by the player, including their cumulative league score.
These gates reject the accepted destruction trace above. The new browser finish
observer was regressed against that trace: all 834 uncompressed racing picture
and palette hashes, seven checkpoint pictures and all 22 checkpoint states
match the retained original evidence. Its report is
`/tmp/wasm-dd2/natural-champ-browser-855-finish-observer-regression-verified.json`.
Regular ten-lap finishes and positive player result/continuation parity remain
unaccepted until a complete original/native/browser capture passes those gates.

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
`node tools/browser/qa_champ_scores.js web/dd2 /tmp/wasm-dd2/fresh-champ-browser` exercises
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
  --output /tmp/wasm-dd2/fresh-champ-original --keys Return Return Return Return Up Left \
  Return Down Down Return Escape Down Down Down Return Up Return Right Return Right Right Right
make verify-champ-names CHAMPREF=/tmp/wasm-dd2/fresh-champ-original \
  CHAMPBROWSER=/tmp/wasm-dd2/fresh-champ-browser/scores.json
```

For the native full frontend (requires a debug build and GDB):

```sh
ASAN=' ' bash tools/build_native.sh /tmp/dd2_native
python3 tools/capture_native_menu.py --output /tmp/wasm-dd2/fresh-champ-native --keys \
  Return Return Return Return Up Left Return Down Down Return Escape Down Down Down \
  Return Up Return Right Return Right Right Right
make verify-champ-names CHAMPREF=/tmp/wasm-dd2/fresh-champ-original \
  CHAMPBROWSER=/tmp/wasm-dd2/fresh-champ-browser/scores.json CHAMPNATIVE=/tmp/wasm-dd2/fresh-champ-native
```

The reference sends real X11 key events and uses hardware breakpoints to let each
key reach the pad reader and acknowledge its release, without changing engine
memory. It records all intervening held polls; Wine may queue key-up late.
Ordinary `--keys` can also use `--key-hold=<seconds>`. Navigation captures include races/results.
The native/WASM score-builder comparison uses captured original standings as an
explicit fixture; browser and native captures exercise live championships. The
native helper calls the normal `dd2_key_event` bridge for one pad poll per key
and checks the real renderer output; native window-system input and hardware audio
are not exercised. Ordinary race finishes, promotion/relegation and complete
chronological streams still need acceptance checks. The five-race retirement
case above covers elimination; its browser clock/RNG comparison remains open.
Use fresh capture directories; stale output is rejected.

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
the previous frontend configuration. It cancels an overwrite, then confirms
renaming the same entry from A to B without changing its replay payload.
It also cancels then confirms deletion
and checks the deleted card after another restart. The test performs no engine
or file writes and never calls `syncfs` itself. Its report uses a fresh directory
under `/tmp/wasm-dd2/`; `BROWSER_REPLAY_SAVE_OUTPUT` can supply that path.

The browser now persists actual card writes while the original card FILE remains
open. Writes are batched at the next event turn; asynchronous IndexedDB snapshots
are serialized and a write during an active sync queues another snapshot.
The previous unload-only shell loses the named replay on ordinary navigation
and is rejected by the same test.

`make verify-native-replay` exercises the same save/overwrite/restart/load/playback and
cancelled/confirmed deletion flow in the real native SDL window, using X11 keys.
It boots the normal intro/frontend three times, checks the entire 128-KiB card
against engine RAM, compares complete saved/loaded script/order bytes, requires
natural playback and restores the previous frontend configuration. A private
game directory prevents changing provisioned/user saves and is removed after
successful checks. `NATIVE_REPLAY_ARGS='--output /tmp/wasm-dd2/fresh-native-replay'`
selects the report path; Xvfb and xdotool are required.

Both real-menu tests accept `--full-card` to fill all 15 slots, compare every
complete replay payload, check navigation at slot 14, cancel then confirm its
overwrite, reload the full card, play a saved replay naturally, delete slot 0
and verify the remaining 14 entries after another restart:

```sh
make verify-native-replay NATIVE_REPLAY_ARGS='--full-card'
make verify-browser-replay-save BROWSER_REPLAY_SAVE_ARGS='--full-card'
```

The tests wait for the actual countdown completion and Practice Over state;
slow rendering cannot make a fixed post-key delay establish those transitions.
These are functional port checks. Whole original memory-card menus and complete
player video/audio equivalence still require acceptance.

The browser build optimizes the handwritten DirectSound mixer with `-O2
-fno-strict-aliasing`, as the native build does, while retaining the reconstructed
engine at `-O0`. This reduces mixer processing cost without fast-math or altered
PCM. `make verify-sound-resample SOUND_RESAMPLE_ARGS='--wasm-optimization O2'`
checks the browser compiler settings against the recorded complete Wine PCM
waveforms and actual CPU x87 arithmetic. The browser CD-menu check also writes
a report under `/tmp/wasm-dd2/` and removes successful raw captures.

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
  --timeout 150 --output /tmp/wasm-dd2/fresh-original-cycle --keys Down Right Right Return Right Return
python3 tools/capture_native_menu.py --menu-cycle 64 --timeout 150 \
  --output /tmp/wasm-dd2/fresh-native-cycle --keys Down Right Right Return Right Return
node tools/browser/capture_menu_cycle.js web/dd2 /tmp/wasm-dd2/fresh-browser-cycle \
  Down Right Right Return Right Return
make verify-menu-cycles MCREF=/tmp/wasm-dd2/fresh-original-cycle MCNATIVE=/tmp/wasm-dd2/fresh-native-cycle \
  MCBROWSER=/tmp/wasm-dd2/fresh-browser-cycle MCREPORT=/tmp/wasm-dd2/fresh-cycle-report.json
```

All seven checkpoints on this path match the original on both ports: 448 frames
per target. `Draw_All` presents the existing framebuffer before rasterizing the
next image, so reference/native captures stop at entry. Pairing by the recorded
highlight counter aligns this specific animation. It does not establish wall-clock
timing, other animations, input-repeat timing or audio-stream equality.

Patch 850 fixes missing body panels and wheels in Car Select. Added address
filters in old face handlers rejected order-table buckets above `0x900000`,
while the actual frontend table begins at `0x935ff0`. At one measured Rookie
pose, the original queued 98 car primitives and the ports only 54; all 171
transformed vertices already matched. Removing the 15 added insertion filters
restores the original visibility tests and unconditional list insertion.

The car preview rotates independently of the 64-step menu colour counter.
Use 256 presentations for a complete model rotation and pair by the observed
car identity and 12-bit angles. This compares every framebuffer/palette byte
without fitting images or masking pixels; it is a renderer component check,
and does not prove chronological video, input timing or audio equality:

```sh
make native web
make clean-logs
python3 tools/reference/capture.py --mode menu --acknowledged-key --menu-cycle 256 --timeout 180 --output /tmp/wasm-dd2/original-car-preview --keys Right Return Right Right Return Return
python3 tools/capture_native_menu.py --menu-cycle 256 --timeout 180 --output /tmp/wasm-dd2/native-car-preview --keys Right Return Right Right Return Return
node tools/browser/capture_menu_cycle.js web/dd2 /tmp/wasm-dd2/browser-car-preview --cycle-frames=256 Right Return Right Right Return Return
make verify-car-preview CAR_PREVIEW_ARGS='--original /tmp/wasm-dd2/original-car-preview --native /tmp/wasm-dd2/native-car-preview --browser /tmp/wasm-dd2/browser-car-preview --report /tmp/wasm-dd2/car-preview-report.json --negative --clean'
make clean-logs
```

The path selects Rookie, Amateur and Pro, confirms Pro and opens the selection
again. All four rotations match the original on both ports: 1,024 complete
frames and palettes per target, with actual browser canvas pixels checked.
The checker also rejects damaged pixels/palettes and swapped neighboring pose
images. `--clean` removes compared raw car frames after saving hashes/report.
Native uses its keyboard bridge under GDB; the original uses acknowledged X11
input, and the browser uses DOM keyboard events. Both ports start with the same
supplied save file. All ten native/Node demos still match each other after the
fix (15,255 frames and all palettes, RNG/Flip logs and generated PCM bytes).

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
make verify-audio-observer AUDIO_OBSERVER_ARGS='--output /tmp/wasm-dd2/fresh-audio-observer'
python3 tools/reference/capture.py --mode audio --audio --audio-rate 44100 \
  --audio-tail 3 --output /tmp/wasm-dd2/fresh-original-audio
```

`audio/summary.json` records the actual committed format, frame counts and PCM
hashes; the tested original selects stereo Float32 at 44100Hz. Each stream's
`.jsonl` journal records accepted/failed writes, monotonic call bounds and
transport events; its `stream-*.pcm` contains only accepted bytes. The clocked
device also writes `played-*.pcm` containing the exact queued samples it consumes,
with `played-*.jsonl` recording their sample-time intervals and transport edges.
Rewound/replaced and dropped samples are excluded from this second stream.
The summary's `played_streams` retain explicit pause/underrun gaps: concatenated
PCM alone is not a continuous playback timeline. `audio/engine.jsonl`
records bounded, non-atomic observations of the live engine. The capture starts
at process launch and can include the intro and automatic transition from the
menu to the attract demo. The EXE and engine memory remain unmodified; no GDB
stops occur in audio mode. Use fresh output directories and keep these original
PCM files outside Git.
The bounded audio tail now starts when the original selects its Main Menu
polygon list and clears the startup/CD restart flag after the slab animation.
The retained legacy `screen` field is a byte of a cached CLUT pointer, not a
screen identifier, and does not determine capture readiness.

The ALSA device's clock, polling, manual start, pause/resume, rewind, prepare
reset and underrun pass real 32/64-bit ALSA tests with exact timestamp-derived
sample bounds. Observer tests check exact accepted bytes, failed-write exclusion,
process isolation and rejection of damaged captures. Consumed-output tests cover
Float32 and S16 stereo at 22050/44100/48000Hz on both process widths, comparing
known sample patterns and independently frozen ALSA delay counters through
rewind, replacement, partial drop, pause/resume, drain, ring wrap and underrun.
Eighteen damaged consumed-stream recordings are rejected. Preparing an actively
running device resets ALSA's application pointer before the plugin callback;
that currently unobservable extent explicitly invalidates the capture. It is
not silently accepted. Successful tests write their report before removing raw
PCM. Two bounded unmodified original runs also produce validated consumed streams;
this verifies capture structure, not original/port audio equality.
A live reference accepted
3.100 seconds of PCM over 3.060 seconds of wall time. The old ALSA null device
accepted 9.263 seconds over 2.573 seconds; `--audio-device null` remains a diagnostic
option and is unsuitable for timing acceptance; it has no consumed-stream recorder.
Accepted PCM includes queued data, while the clocked device's consumed stream
records what remains after drops/rewinds.
`--audio` also works with video/navigation captures, whose debugger stops can
cause audio underruns. Full original/native/WASM mixed PCM comparison, playback
alignment and Windows hardware equivalence remain open acceptance work.

`make verify-original-menu-audio ORIGINAL_MENU_AUDIO_ARGS='--output /tmp/wasm-dd2/fresh-menu-mix'`
records a bounded original audio run with DirectSound tracing and checks the
entire menu-device PCM against independently rendered production port sources.
`--capture /tmp/wasm-dd2/existing-menu-audio` can use an existing capture made
with `--wine-debug=-all,+timestamp,+dsound`. The trace must show the actual
BANK1 index42 mono8 slab effect (11025Hz source, 5512Hz playback, volume -1,
centered pan, one shot) followed by looping CD13. Native, WASM O0 and browser
mixer settings O2 all reproduce the complete recorded window, including initial
silence and effect/CD overlap. Source start positions now come independently
from cumulative original `DSOUND_MixToPrimary` block counts and the device's
separately consumed rate-probe prefix. The checker never searches PCM for an
alignment. It validates every original effect/CD cursor, creation order,
accepted FIFO prefix and remaining software/device queues, then compares all
accepted and consumed bytes. This diagnoses complete sample values at recorded
device positions in the bounded window; identical-input live port scheduling
and intro/hardware output still need acceptance. All bytes remain in
the comparison; damaged prefix, effect, overlap, right-channel tail and truncated
PCM reject. One-sample shifts of either/both sources and five damaged original
mixer timelines also reject. The check rejects a recording that enters the attract race,
instead of fitting the racing sources into this two-source menu check. Tracing
changes execution timing. Reports retain actual mixer blocks, source cursors,
queue extents, independently derived starts and consumed device time
segments; successful generated source PCM is removed after the report.
Automatically recorded original raw PCM is also removed after successful
comparison; recordings supplied with `--capture` belong to the caller. Failed
recordings remain available for diagnosis.

The full application startup can also be observed without writing engine state:

```sh
python3 tools/capture_native_menu_startup.py --output /tmp/wasm-dd2/fresh-native-startup
node tools/browser/capture_menu_startup.js web/dd2 /tmp/wasm-dd2/fresh-browser-startup
make verify-menu-startup-controls MENU_STARTUP_ARGS='--original /tmp/wasm-dd2/original-startup --original-mix-report /tmp/wasm-dd2/original-mix/report.json --native /tmp/wasm-dd2/fresh-native-startup --browser /tmp/wasm-dd2/fresh-browser-startup --report /tmp/wasm-dd2/startup-controls/report.json'
```

The original capture needs `--wine-debug=-all,+timestamp,+dsound,+ddraw`;
its independent sample report comes from `verify-original-menu-audio` above.
Both port captures run normal application initialization and skip the real
intro with keyboard input. The checker verifies the actual loaded sound-bank
source, playback controls, CD13 menu state and presentation count at each
source start. In the recorded startup, the slab effect starts after 38 Flips
and CD after 53 on all three targets. Twenty damaged control sequences reject.
Wine's CD streaming ring loops internally; the port instead consumes the whole
finite CD range, so those storage flags are checked separately. Sample offsets
are reported without declaring them equal: full device timing, PCM and video
equivalence of the complete application remain open. `DD2_SNDLOG=<path>` selects
the port log; `DD2_SNDLOG=1` uses `/tmp/wasm-dd2/sound.log`. Build diagnostics now
also live in `/tmp/wasm-dd2/{native,node,browser}-build/`.

For an actual engine PCM comparison, export the observed device clock and run
both normal application entries with that clock:

```sh
python3 tools/reference/menu_device_clock.py --capture /tmp/wasm-dd2/original-startup --mix-report /tmp/wasm-dd2/original-mix/report.json --output /tmp/wasm-dd2/device-clock/clock.bin
python3 tools/capture_native_menu_startup.py --audio-clock /tmp/wasm-dd2/device-clock/clock.bin --output /tmp/wasm-dd2/native-clock-startup
node tools/browser/capture_menu_startup.js web/dd2 /tmp/wasm-dd2/browser-clock-startup /tmp/wasm-dd2/device-clock/clock.bin
make verify-menu-engine-audio MENU_ENGINE_AUDIO_ARGS='--original /tmp/wasm-dd2/original-startup --mix-report /tmp/wasm-dd2/original-mix/report.json --clock /tmp/wasm-dd2/device-clock/clock.bin --native /tmp/wasm-dd2/native-clock-startup --browser /tmp/wasm-dd2/browser-clock-startup --output /tmp/wasm-dd2/engine-audio-report --negative-runs'
```

`DD2_AUDIO_FRAME_CLOCK` supplies only device progress: the original's primary
mixer block counts plus the separately consumed probe, indexed by actual
presentation calls and bounded by the accepted device extent. It does not
supply sound controls, source data, RNG or engine state. The selected menu
device epoch is recorded independently; preceding device output is excluded.
Repeated clock entries represent presentations without another audio block.
The exporter requires the Flip-derived positions to locate both traced source
starts exactly, and rejects an ambiguous trace instead of adjusting positions.
The production device checks header, rate, extent and monotonicity, then writes
`DD2_AUDIO_CLOCK_REPORT` when the observed window is complete. The capture
controller ends the run at that declared boundary.

The checker compares every actual C mixed byte with the original accepted
stream, and its entire independently consumed prefix, without Python source
recomposition. In the first recording, both real native and browser engine
runs matched 1,192,376 accepted bytes and 1,178,272 consumed bytes. Eight
additional actual native runs reject five damaged clock inputs and three
same-length outputs with either/both sources one sample late. Successful raw
port PCM is deleted after writing the report. A second independently recorded
clock (1,648 presentations rather than 1,072) also matched both actual engines:
1,139,120 accepted bytes and 1,125,016 consumed bytes, with different source
sample start positions. This is a bounded menu-device
audio proof with observed timing; the intro device, full video, live scheduling,
racing audio and physical sinks still need their complete comparisons.

The first frontend video presentations can be compared from normal application
initialization, including loading, the slab transition and a full settled menu
highlight cycle. Build native and browser outputs first, then use fresh capture
directories and the same supplied `DestructionDerby2/SaveGames` input:

```sh
make clean-logs
python3 tools/reference/capture.py --mode startup --output /tmp/wasm-dd2/original-first-video --timeout 90
python3 tools/capture_native_menu_startup.py --video-frames 128 --output /tmp/wasm-dd2/native-first-video
node tools/browser/capture_menu_startup.js web/dd2 /tmp/wasm-dd2/browser-first-video --video-frames=128
make verify-startup-video STARTUP_VIDEO_ARGS='--original /tmp/wasm-dd2/original-first-video/startup --native /tmp/wasm-dd2/native-first-video/startup --browser /tmp/wasm-dd2/browser-first-video/startup --output /tmp/wasm-dd2/first-video-report.json --negative --clean'
make clean-logs
```

The original recorder uses one hardware breakpoint at the successful primary
Flip return (`0x412cc1`); both ports record actual platform presentations, and
the browser also checks every canvas pixel against the indexed image/palette.
Intro skipping uses real keyboard input; observers do not write engine state.
The checker compares every indexed byte and palette byte in chronological
order, checks initial save hashes and presentation state, and rejects damaged
loading/transition/menu frames, palettes and shifted sequences. Each 128-frame
capture uses about 38 MiB; `--clean` removes successful raw frame files after
writing provenance and hashes to the report.

This comparison found a loading-bar error in both ports: frames 10–19 each had
815 incorrect pixels left of the bar. Patch 849 restores the original byte
command and word coordinate stores in the packed primitives; a four-byte
command assignment had overwritten the first X coordinate. With the patch,
all 128 frames and palettes match the original in both native and browser
captures. This proves the recorded frontend video sequence only; intro output,
audio/video synchronization, racing and live display timing remain open.

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
