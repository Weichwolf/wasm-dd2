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

Patch 854 restores the original empty polygon-list marker in `Highlight_Area`,
used by the pit camera. Original instructions at `43b704/43b70c` copy byte
`43a7b0 = ff` into the local list. The decompile omitted that store, so highlight
areas with an empty triangle or quad list traversed stack bytes as polygon
indices. The regular championship reference exposed overwritten model vertices
and polygon data, intermittent native renderer crashes, and a browser WASM
indirect-dispatch trap in `FUN_0041ff98` at game frame 10605.

`make verify-highlight HIGHLIGHT_ARGS='--output /tmp/wasm-dd2/fresh-highlight'`
executes the unmodified original x86 function and the actual native/WASM C
function on all ten valid areas and four RGB values. All 40 complete model and
guard snapshots match; changing the empty list into a finite nonempty list fails
exactly the 20 cases that use it. Damaged guards and truncated output also reject.
The report is `/tmp/wasm-dd2/highlight-854-components-final/report.json`.
This is a component check, not a complete pit UI or audio/video proof.

Patch 855 restores the signed pit-camera section and WORD offset. Moving left
from section zero now wraps to section five and decrements the revolution count,
as in the original. Both live controls and replay read the left-offset WORD at
`0x464a68 + section * 4`. The previous unsigned section underflowed to
`0xffffffff` during the fourth natural championship race on level seven;
the first different picture was racing capture 18,388, before RNG counts diverged.

`make verify-pit-camera PIT_CAMERA_ARGS='--output /tmp/wasm-dd2/fresh-pit-camera'`
compares unchanged original x86 with native/WASM controls in 3,600 explicit
cases, including all sections, both directions, signed angle/offset boundaries,
replay packets and complete model guards. All outputs match. Reintroducing the
unsigned section or offset fails 300 cases each; the incorrect left-table stride
fails 1,500 cases. This component proof does not establish full-season parity;
the complete original/native/browser comparison must pass separately.

Patch 856 restores the numeric text parser's byte character classification and
Watcom's wrapping 32-bit decimal conversion. The old DWORD table lookup treated
`117` as zero and advanced the source cursor incorrectly; glibc's 32-bit `atoi`
also saturated `9876543210` instead of returning the original wrapped DWORD.
`make verify-print-number PRINT_NUMBER_ARGS='--output /tmp/wasm-dd2/fresh-print-number'`
compares 849 cases with unchanged original x86, native, native ASan and WASM.
All first-byte values, three starting positions, signed/overflow boundaries,
delimiters and the original ten-character bound are covered. Values, cursors and
source/destination guards match; the old table stride fails 234 cases on each
port. This component proof does not establish full text-rendering or menu parity.

Patch 857 restores Print's contiguous three-byte RGB stack block. Both the `%C`
parser and glyph initializer consume that block; the separate scalar locals
previously corrupted neighboring stack bytes and supplied incorrect green/blue
channels. Native ASan exposed the read overflow during actual frontend startup.
`make verify-print-rgb PRINT_RGB_ARGS='--output /tmp/wasm-dd2/fresh-print-rgb'`
compares 1,152 complete text-record/glyph-packet cases with unchanged original
x86, native, native ASan and WASM, covering font/shader modes, colors, mid-string
color changes, byte overflow and alignment. All outputs match; the old scalar
layout fails all 1,152 cases on both ports and is rejected by native ASan.
The corrected native ASan application also initializes and renders 64 main-menu
draws successfully. These checks do not establish full frontend-frame or audio parity.

Patch 858 restores the top-three position pointer's contiguous rank/vector
stack block, including their original overlap. Separate scalar locals caused
an actual ASan write overflow when starting the championship's first race.
`make verify-position-pointers POSITION_POINTERS_ARGS='--output /tmp/wasm-dd2/fresh-position-pointers'`
compares 2,800 cases against unchanged x86 on native, native ASan and WASM:
every car, both buffers, ranks zero through four, projection depth boundaries,
visibility gates, signed vector subtraction, rotation and concurrent cars.
Complete packets, ordering tables, GTE state and input guards match; ASan
rejects the old scalar write. The corrected ASan application also reaches
the first 64 racing pictures through normal menus and acknowledged input;
their indexed pixels and palettes match the original. These checks establish
the component and bounded race startup, not complete-season or audio parity.

Patch 859 restores four packed WORD fields in the championship menus. The
main-menu cross's DWORD x store erased its adjacent y coordinate; the season
result's DWORD underline lengths erased neighboring RGB bytes. The unchanged
x86 uses WORD stores for all four fields. The existing five-race comparison
identified precisely these two image failures after matching all 19,474 racing
pictures on both ports. `make verify-champ-menu-fields CHAMP_MENU_FIELDS_ARGS='--output /tmp/wasm-dd2/fresh-champ-menu-fields'`
checks 504 cases against unchanged x86 on native, native ASan and WASM: six race
types, every human league/rank position and four neighboring-byte patterns.
The original result helper and bounded frontend instruction branch match all
packed fields and guards; the old DWORD stores are rejected separately. This
component proof does not establish corrected live menu images or full-season
acceptance; the complete latest-engine comparison must still pass.

Patch 860 reconstructs Duplicate_Font's original zero blend byte. Both original
font setup call chains leave Setup_Font's literal zero argument in the stack
byte later read as Duplicate_Font's descriptor byte 9. Host stack layouts,
including ASan, otherwise change the copied font's transparency mode and the
result heading. `make verify-duplicate-font DUPLICATE_FONT_ARGS='--output /tmp/wasm-dd2/fresh-duplicate-font'`
executes both call orders against unchanged x86, native, native ASan and WASM
in 256 cases covering all 32 page numbers and varied coordinates, CLUTs and glyph
patterns. Duplicate metadata, all 96 glyphs per bank, source fields and guards
match; forced blend bytes 1, 2 and 3 are rejected on both ports. Opaque source
font page stack residue is explicitly excluded. This component comparison does
not establish complete menu rendering, championships or A/V parity.
The actual native ASan first natural result checkpoint also matches the original
framebuffer, palette and state at 33,632 clock/41,616 independently verified RNG
calls; all 2,502 previously different pixels are fixed. Its bounded report is
`/tmp/wasm-dd2/font-blend-860-native-result-verified.json`. This checkpoint does
not accept every racing picture, complete seasons, PCM or physical timing.

Both corrected native and browser engines complete the new regular ten-lap
first-race reference with a surviving player and 75 points. All 4,314 racing pictures and
palettes, 24,632 actual original clock returns, 35,999 calculated RNG calls,
selected result/league images, standings and the next actual race start match.
The report is `/tmp/wasm-dd2/natural-champ-highlight-854-regular-verified.json`;
both `--require-completed-laps` and `--require-player-points` were enforced.
Successful raw pictures were removed after writing the report. Complete seasons,
wins, other menu/loading/fade video, racing PCM and physical timing remain open.

The natural-season recorder extends the acknowledged input sequence to all five
Wrecking championship races on levels 1, 2, 5, 7 and 10. It resets the road
follower at each actual road-race start and uses held acceleration/steering in
the final arena. Both ports replay the original's actual recorded key transitions
and clock returns; the final arena also contributes every racing picture.
`verify-natural-season-history` requires five natural completions, all twenty
drivers' cumulative scores, all twenty league pages and the actual final
elimination or next-season gameplay transition. Natural destruction remains
explicit and does not prove five surviving regular finishes or wins. This check
excludes other menu/loading/fade video, racing PCM and physical timing.

The complete recorded season passed on Native with AddressSanitizer and the
browser build through patch 860 (`f09136c`): all 19,474 racing indexed pictures
and palettes, all 66 checkpoint states and 31 selected checkpoint images were
exact against the unmodified original. Both ports consumed 110,227 original
clock returns and checked 154,548 calculated RNG records. Every driver's points
and all twenty league pages matched, followed by the original's actual
elimination to the frontend. Three road races completed laps; the remaining
road race and final arena ended with a destroyed player. This is evidence for
that input trace and those captured builds; the later audio patch 861 is covered
separately by its mixer comparison.

```sh
make clean-logs
mkdir -p /tmp/wasm-dd2
make native NATIVE=/tmp/wasm-dd2/natural-season-dd2-native
make web
python3 tools/reference/capture.py --mode menu --champ-history \
  --natural-champ-history --natural-season-history --steady-driver \
  --timeout 7200 --output /tmp/wasm-dd2/natural-season-original
python3 tools/capture_native_champ_history.py \
  --binary /tmp/wasm-dd2/natural-season-dd2-native \
  --reference /tmp/wasm-dd2/natural-season-original/history \
  --output /tmp/wasm-dd2/natural-season-native
python3 tools/wasm_rng_layout.py --wasm web/dd2/index.wasm \
  --output /tmp/wasm-dd2/natural-season-layout
node tools/browser/capture_champ_season.js web/dd2 /tmp/wasm-dd2/natural-season-browser \
  --api-reference=/tmp/wasm-dd2/natural-season-original/history \
  --rng-layout=/tmp/wasm-dd2/natural-season-layout/layout.json
make verify-natural-season-history NATURAL_SEASON_HISTORY_ARGS='--original /tmp/wasm-dd2/natural-season-original/history --native /tmp/wasm-dd2/natural-season-native/history --browser /tmp/wasm-dd2/natural-season-browser --report /tmp/wasm-dd2/natural-season-verified.json --clean'
make clean-logs
```

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

Racing acceptance also checks the complete original/native drawing timeline:
every observed gameplay draw must have exactly one racing-manifest entry in the
same order and at the same observed game/API state. At native `PutDispEnv`, the
recorder's observed owner stack distinguishes gameplay from frontend/loading
draws. The check accepts the retained 4,314-picture championship histories and
the 957/917-picture arena histories, including their 1,323/1,259 full
presentation streams. Missing, duplicate or reordered racing entries, changed
API counts and altered platform observations reject. These are retained-history
verifier regressions, not new complete-season or audio acceptance. Their reports
are `/tmp/wasm-dd2/racing-timeline-855-verified.json` and
`/tmp/wasm-dd2/racing-timeline-855-arena-regression.json`.

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

For a completed-lap attempt, add `--steady-driver` to the original command.
This external keyboard driver uses averaged road centers, a lower target speed
and yaw feedback. It reverses its steering feedback when the car actually backs
up, including while the accelerator stops that backwards motion. Stall detection
uses the physical FD strip at `0x7926ac` and handles the track's normal strip wrap.
The confirmed lap checkpoint at `0x795c4a` is unsuitable: the original only advances
it in sequence, so it stays unchanged while a car returns after rolling backwards.
Treating that value as current travel caused unnecessary repeated reversals in
the incomplete lap-eight recording. The source report includes the driver hash;
the ports replay its actual keys rather than running that driver again. This
option is a capture aid and does not establish a completed race.

For a browser crash that depends on ordinary timing, `diagnose_normal_finish.js`
can record actual `GetTickCount` import returns and DOM key delivery boundaries
with `--api-layout=<matching-layout.json>`. It retains the original clock return
values and checks observed seed/counter checkpoints against the literal LCG.
`capture_native_api_diagnosis.py` replays those inputs at native presentations
with strict clock/RNG files and compares the observed states, including player
position, speed, heading, damage, lap and race points. This is a diagnostic
comparison between the ports; it does not accept original video or PCM parity.
The recorder bounds its metadata and saves input/state/error details on failure;
`--trap-trace` adds a read-only uncaught-exception debugger trace after green.
Menu taps wait for 16 actual presentations with a completed menu transition,
so slow rendering does not silently lose the next Enter. Diagnostic reports
retain the controller source hash used at launch.

```sh
make clean-logs
node tools/browser/diagnose_normal_finish.js web/dd2 /tmp/wasm-dd2/api-drive \
  --championship --steady-track --duration=180 \
  --api-layout=/tmp/wasm-dd2/browser-timer-853-layout/layout.json
python3 tools/capture_native_api_diagnosis.py \
  --reference /tmp/wasm-dd2/api-drive --output /tmp/wasm-dd2/api-drive-native
make clean-logs
```

Two actual-clock transport/state regressions passed: 6,311 presentations at
4,910,657 clock reads and 6,993 RNG calls, and 2,939 presentations including
player motion/damage at 949,760 clock reads and 1,508 RNG calls. Shifted driving
inputs and changed expected player positions/points are rejected. Reports are
`/tmp/wasm-dd2/natural-champ-native-856-api-drive/report.json` and
`/tmp/wasm-dd2/natural-champ-native-856-api-player-drive/report.json`.

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

The software DirectSound backend rejects secondary `DSBCAPS_LOCHARDWARE`
requests with `DSERR_UNSUPPORTED` and clears the output pointer. This preserves
the original engine's retry from flags `0xe6` to `0xe2`, observed in the original
sound-bank loading trace. Primary buffers remain exempt, matching
[Wine 10's buffer creation implementation](https://github.com/wine-mirror/wine/blob/wine-10.0/dlls/dsound/dsound.c).
`make verify-sound-hardware SOUND_HARDWARE_ARGS='--output /tmp/wasm-dd2/sound-hardware'`
checks the actual COM results against Wine, native, native ASan and WASM,
including the software retry and primary exemption. A mutation restoring the
old hardware acceptance must fail. The check covers this API branch; full
engine audio timing remains a separate comparison.

Patch 868 accelerates the exact native FIR arithmetic with the i386 x87. It
selects nearest/even and 64-significand-bit precision only while mixing, keeps
the calibrated Float32 spills, then restores the engine's original control
word. WASM retains the checked integer implementation. The previous software
path's extra ASan cost let elapsed audio work grow faster than it was processed,
preventing a loaded championship from reaching eight physics ticks in 60 seconds.
The actual ASan load now reaches and pauses the next race, preserving all saved
fields and matching 128 original card-menu images. The optimization does not
drop audio blocks, samples, source advancement or control calls.

The production resampler check now compiles the patched `build/` mixer, rather
than the pristine shim. Native, native ASan and WASM pass 24 complete format/rate
waveforms and six short-loop cases against calibrated Wine hashes. In addition
to the existing 1,084,900 CPU-x87 oracle results, 500,012 production arithmetic
results match the software model. Twelve caller precision/rounding combinations
check control-word restoration; mutations using 53 bits or failing to restore
the caller are rejected. Run `make verify-sound-resample
SOUND_RESAMPLE_ARGS='--wasm-optimization O2 --output /tmp/wasm-dd2/resample --clean'`
to retain the report and remove successful raw comparisons. These checks cover
the exercised arithmetic and transport routes; full interactive scheduling and
every race's chronological video/PCM remain separate requirements.

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

The Configuration -> Control Method -> Keyboard path now has a separate
original/native/browser comparison. It assigns five distinct keys, rejects four
duplicate attempts, confirms the complete map, cancels a partial replacement,
and reopens the screen to verify the committed map. All 27 checkpoint states
and 1,728 complete framebuffer/palette pairs match the original on each port;
browser captures also check the actual canvas pixels. Fourteen changed-map,
cancel/reopen, active-map, pixel and palette cases are rejected.

Unmapped original keys are acknowledged at the actual binding function and
its return, followed by the real key-up window message and return. This avoids
depending on a pad flag or the final binding's retained GetKeyState latch.
Native input uses the normal `dd2_key_event` bridge, and browser input uses the
production DOM keyboard handler. This check covers settled menu states and
highlight cycles; transition video, audio, physical input timing, subsequent
racing with the new map and persistence remain separate requirements.

```sh
make clean-logs
make verify-keyboard-binding-menu KEYBOARD_BINDING_MENU_ARGS='capture --target original --output /tmp/wasm-dd2/keyboard-original'
make verify-keyboard-binding-menu KEYBOARD_BINDING_MENU_ARGS='capture --target native --binary /tmp/dd2_native --output /tmp/wasm-dd2/keyboard-native'
make verify-keyboard-binding-menu KEYBOARD_BINDING_MENU_ARGS='capture --target browser --build web/dd2 --output /tmp/wasm-dd2/keyboard-browser'
make verify-keyboard-binding-menu KEYBOARD_BINDING_MENU_ARGS='compare --original /tmp/wasm-dd2/keyboard-original --native /tmp/wasm-dd2/keyboard-native --browser /tmp/wasm-dd2/keyboard-browser --report /tmp/wasm-dd2/keyboard-report.json --clean'
make clean-logs
```

Configuration persistence and subsequent racing controls have a separate live
original/native/browser check. It binds Left, Right, Fast Steer, Accelerate and
Brake to B, C, D, E and F, saves configuration A through the normal memory-card
menus, and restarts each process. The browser closes Chromium entirely and
reopens its isolated persistent profile; its production IndexedDB loading and
save hooks handle persistence. Every original/native input is an actual X11
key; browser events are trusted Playwright keyboard events. Observers read
state and files without writing engine state or invoking save/load functions.

The initial, saved and reloaded complete 128 KiB cards match the unmodified
original byte for byte on both ports. The packed 6,526-byte configuration,
restored settings and all eighteen saved map bytes also match. A normal player
race activates the saved map: throttle/brake reach +/-32768, normal steering
reaches +/-256 and keyboard Fast Steer reaches the original +/-511 limit.
Both steering directions, released controls, actual acceleration movement and
the removed A/Z/Space bindings are checked. Fourteen corrupted restored-map,
card-map, active-map, throttle, fast-steer, release and old-key cases are
rejected. This checks the exercised configuration/card/input functionality;
race trajectories, video/audio streams and physical scheduling are separate
requirements. It does not accept all settings, card edge cases or controllers.

```sh
make clean-logs
make verify-configuration-persistence CONFIGURATION_PERSISTENCE_ARGS='capture --target original --output /tmp/wasm-dd2/config-original'
make verify-configuration-persistence CONFIGURATION_PERSISTENCE_ARGS='capture --target native --binary /tmp/dd2_native --output /tmp/wasm-dd2/config-native'
make capture-browser-configuration-persistence BROWSER_CONFIGURATION_ARGS='web/dd2 /tmp/wasm-dd2/config-browser'
make verify-configuration-persistence CONFIGURATION_PERSISTENCE_ARGS='compare --original /tmp/wasm-dd2/config-original --native /tmp/wasm-dd2/config-native --browser /tmp/wasm-dd2/config-browser --report /tmp/wasm-dd2/config-verified.json --clean'
make clean-logs
```

Patch 866 restores packed WORD coordinates and underline lengths in the card
menus. The incorrect DWORD assignments erased adjacent coordinates or colour
bytes; the Save underline alone differed at 488 pixels. It also reconstructs
the original contiguous 24-byte tick/cross descriptors in confirmations and
the pair of 40-byte, double-buffered sprites in the Please Wait routine. The
latter overflowed a separated local array while saving under AddressSanitizer.
Each layout is derived from the original x86 stores, copies and stack offsets.

The configuration/card UI scenario uses genuine X11 or trusted Playwright keys
and four complete process startups with an isolated card. It exercises slider
minimum/maximum saturation, preview, commit and cancellation, saving two named
configurations, overwrite/name cancellation, overwrite/rename, manual loading,
automatic first-configuration loading and cancelled/confirmed deletion. Every
checkpoint compares all 128 KiB of card data with the original and checks the
stored, working and actual master SFX volume separately. The original loads a
muted configuration without immediately updating the initialized master volume;
the observer records that behavior rather than assuming those fields are equal.

Selected settled menu images cover all 64 slab-highlight phases. Confirmation
dialogs also have an independent selected-card counter (+20 modulo 255, period
51). The target observers select the original's observed counter pairs, reading
only small counters at intervening presentations and retaining just 64 complete
framebuffer/palette pairs per checkpoint. Every byte is compared; browser
captures additionally compare every actual canvas pixel. No engine state is
written, no image fitting or pixel masking is used. This is a selected-renderer
and functionality comparison; chronological video/PCM, physical scheduling,
every intermediate slider level and full-card/duplicate-name errors remain
separate requirements.

The verified native, native ASan and browser runs each match 22 checkpoints and
704 complete framebuffer/palette pairs against the original. All four native
ASan sessions finish without a sanitizer diagnostic. Eighty-seven changed-state,
pixel, palette and independent-card-phase cases are rejected by the comparator.

```sh
make clean-logs
make verify-configuration-card-ui CONFIGURATION_CARD_UI_ARGS='capture --target original --output /tmp/wasm-dd2/card-original'
make verify-configuration-card-ui CONFIGURATION_CARD_UI_ARGS='capture --target native --binary /tmp/dd2_native --reference /tmp/wasm-dd2/card-original --output /tmp/wasm-dd2/card-native'
make capture-browser-configuration-card-ui BROWSER_CONFIGURATION_CARD_UI_ARGS='web/dd2 /tmp/wasm-dd2/card-browser /tmp/wasm-dd2/card-original'
# Optionally capture the same scenario with an ASan native binary, then add --asan to compare.
make verify-configuration-card-ui CONFIGURATION_CARD_UI_ARGS='compare --original /tmp/wasm-dd2/card-original --native /tmp/wasm-dd2/card-native --browser /tmp/wasm-dd2/card-browser --report /tmp/wasm-dd2/card-verified.json --clean'
make clean-logs
```

Patch 867 fixes the filename alphabet's colours. The original clears three
individual RGB bytes at `0x93a3c8..0x93a3ca`; DWORD aliases also erased the
primitive opcode and coordinates. The resulting alphabet displayed coloured
texture pixels instead of the original black letters, differing at 2,409 pixels
in the empty filename dialog. The patch restores the original BYTE stores.

The additional card-edge scenario uses the same live observers and real keys.
It checks each of the 26 alphabet entries, cursor and backspace limits, the
eight-character cap, empty and long filenames, duplicate refusal/cancellation
and the same-name overwrite exemption. It fills all fifteen slots through the
normal Save Configuration UI, checks full-card selection/overwrite/rename,
deletes and reuses the final slot, restarts with the full card and loads its
last configuration. The original accepts an empty filename and allows
overwriting any occupied slot on a full card; these are observed behaviors.
After duplicate refusal, a new entry retains the attempted name while an
occupied entry resets to the previously saved name. The scenario records the
actual displayed text, cursor, error detail, settings and complete card bytes.
Four process startups check normal persistence, including full Chromium
restarts. Selected rendered cycles use the independent-counter alignment above;
chronological video/PCM, other save types, corrupted cards and disconnected
storage remain separate requirements.

The verified native, native ASan and browser runs each match all 67 states and
complete 128 KiB cards, plus 1,024 framebuffer/palette pairs against the original.
All four ASan sessions finish without a diagnostic. The report records actual
card write/refusal transitions and rejects 129 changed-text, cursor, card,
settings, pixel, palette and independent-card-phase cases.

```sh
make clean-logs
make verify-configuration-card-ui CONFIGURATION_CARD_UI_ARGS='capture --target original --scenario tools/configuration_card_edges.json --output /tmp/wasm-dd2/edges-original'
make verify-configuration-card-ui CONFIGURATION_CARD_UI_ARGS='capture --target native --scenario tools/configuration_card_edges.json --binary /tmp/dd2_native --reference /tmp/wasm-dd2/edges-original --output /tmp/wasm-dd2/edges-native'
make capture-browser-configuration-card-ui BROWSER_CONFIGURATION_CARD_UI_ARGS='web/dd2 /tmp/wasm-dd2/edges-browser /tmp/wasm-dd2/edges-original tools/configuration_card_edges.json'
make verify-configuration-card-ui CONFIGURATION_CARD_UI_ARGS='compare --scenario tools/configuration_card_edges.json --original /tmp/wasm-dd2/edges-original --native /tmp/wasm-dd2/edges-native --browser /tmp/wasm-dd2/edges-browser --report /tmp/wasm-dd2/edges-verified.json --clean'
make clean-logs
```

The championship-save check generates a real `0x3030` card with the running
original: select Stock Car and car 1, name the player, race, Retire/Yes, then
File Options / Save Game. All 6,526 payload bytes must match the original's
actual saved fields, statistics, standings, names, fastest laps and bindings.
Each application starts with that same complete card as a file input. The
browser seeds IndexedDB before engine startup, then uses normal IDBFS loading.
The game must remain at the default frontend until File Manager loads the save;
the next race must restore every field and region, the car order and actual
racing bindings, and permit pause. Loading must preserve all 128 KiB of the card.

The verified Stock Car case resumes race index 1 on track 2. Native and browser
each match the original's complete loaded state and 128 card-menu framebuffers
and palettes; actual browser canvas pixels are checked too. The comparator
rejects 76 changed-field, region, pixel and palette cases. This covers the
exercised save/load route and selected menu cycles. Chronological race video/PCM,
subsequent finishes, every season, multiplayer and replay saves remain separate.

```sh
make clean-logs
make verify-championship-save CHAMPIONSHIP_SAVE_ARGS='generate --mode 1 --output /tmp/wasm-dd2/champ-save-fixture'
make verify-championship-save CHAMPIONSHIP_SAVE_ARGS='capture --target original --fixture /tmp/wasm-dd2/champ-save-fixture --output /tmp/wasm-dd2/champ-save-original'
make verify-championship-save CHAMPIONSHIP_SAVE_ARGS='capture --target native --binary /tmp/dd2_native --fixture /tmp/wasm-dd2/champ-save-fixture --reference /tmp/wasm-dd2/champ-save-original --output /tmp/wasm-dd2/champ-save-native'
make capture-browser-championship-save BROWSER_CHAMPIONSHIP_SAVE_ARGS='web/dd2 /tmp/wasm-dd2/champ-save-browser /tmp/wasm-dd2/champ-save-fixture /tmp/wasm-dd2/champ-save-original'
make verify-championship-save CHAMPIONSHIP_SAVE_ARGS='compare --fixture /tmp/wasm-dd2/champ-save-fixture --original /tmp/wasm-dd2/champ-save-original --native /tmp/wasm-dd2/champ-save-native --browser /tmp/wasm-dd2/champ-save-browser --report /tmp/wasm-dd2/champ-save-verified.json --clean'
make clean-logs
```

Patch 870 restores the signed WORD Y/width fields of the Driver, Track and
Championship statistics table rectangles. The previous BYTE aliases placed
their dark translucent backgrounds outside the visible slab and truncated the
Championship width. The corrected types follow the unchanged original callers'
DWORD/SAR-16 loads and the actual packed rectangle descriptors.

The persisted-statistics check loads the original-generated Stock Car save,
retires its remaining three races in the original, returns after elimination
and saves configuration B in slot 1. The complete `0x1010` configuration packs
the real completed-season statistics; normal startup restores all saved fields
and regions while leaving the earlier championship slot unchanged. Actual X11
or trusted Playwright keys exercise all 20 driver pages, all 11 track pages,
Championship standings, driver/track wrapping, category/season limits and
reopening. Displayed values are checked against the original saved payload.
The gate requires 53 checkpoint states and 2,304 complete indexed/palette pairs
on each of Native, Native ASan and WASM, with actual browser canvas checks.
It rejects altered pixels, palettes and statistics, plus all three real missing
backgrounds captured from a pre-870 native executable. This covers one eliminated
Stock Car season and settled 64-phase menu cycles; chronological racing A/V,
multiple seasons, victories/promotion and physical output/timing remain open.
The verified production builds pass this gate on all three targets. The report
is `/tmp/wasm-dd2/statistics-876-final-verified.json`; all 228 negative cases are
rejected, and successful raw frame comparisons are removed after the report.

```sh
make clean-logs
make verify-statistics-ui STATISTICS_UI_ARGS='generate --championship /tmp/wasm-dd2/champ-save-fixture --output /tmp/wasm-dd2/stats-fixture'
make verify-statistics-ui STATISTICS_UI_ARGS='capture --target original --fixture /tmp/wasm-dd2/stats-fixture --output /tmp/wasm-dd2/stats-original'
make verify-statistics-ui STATISTICS_UI_ARGS='capture --target native --binary /tmp/dd2_native --fixture /tmp/wasm-dd2/stats-fixture --output /tmp/wasm-dd2/stats-native'
make verify-statistics-ui STATISTICS_UI_ARGS='capture --target native --binary /tmp/dd2_asan --fixture /tmp/wasm-dd2/stats-fixture --output /tmp/wasm-dd2/stats-asan'
make capture-browser-statistics-ui BROWSER_STATISTICS_UI_ARGS='web/dd2 /tmp/wasm-dd2/stats-browser /tmp/wasm-dd2/stats-fixture'
make verify-statistics-ui STATISTICS_UI_ARGS='baseline --fixture /tmp/wasm-dd2/stats-fixture --original /tmp/wasm-dd2/stats-original --binary /tmp/dd2_native_before_870 --output /tmp/wasm-dd2/stats-before'
make verify-statistics-ui STATISTICS_UI_ARGS='compare --fixture /tmp/wasm-dd2/stats-fixture --original /tmp/wasm-dd2/stats-original --native /tmp/wasm-dd2/stats-native --asan /tmp/wasm-dd2/stats-asan --browser /tmp/wasm-dd2/stats-browser --before /tmp/wasm-dd2/stats-before/report.json --report /tmp/wasm-dd2/stats-verified.json --clean'
make clean-logs
```

Patch 871 maps the lap-record table height to the original signed WORD at
`0x46956a`. Its prior host-only global stayed zero, so the dark table background
vanished after the correctly rendered entry rotation. The original steady loop
loads this field with DWORD/SAR-16; the packed rectangle is (-120,-38,232,95).

The same actual-menu tools accept `--scenario tools/lap_records_ui.json`
(the optional fourth browser argument). This scenario checks all five displayed
records on all seven road tracks against the original configuration's saved
fastest laps, both wrap directions, preserved selection on reopening and the
unchanged complete configuration/card. Nineteen states and nine full menu cycles
require 576 exact indexed/palette pairs per target, including actual canvas
conversion. A real pre-871 executable supplies the missing-background regression.
The verified run is recorded in
`/tmp/wasm-dd2/lap-records-877-final-verified.json`: Native, Native with ASan and
WASM pass all 19 states and 576 frame/palette comparisons each; 64 negative
checks reject altered output and the actual pre-fix background.
The check covers the displayed saved records and settled menus; earning a new
record, other result tables, chronological racing A/V and physical timing remain
separate requirements.

```sh
make clean-logs
make verify-statistics-ui STATISTICS_UI_ARGS='capture --scenario tools/lap_records_ui.json --target original --fixture /tmp/wasm-dd2/stats-fixture --output /tmp/wasm-dd2/laps-original'
make verify-statistics-ui STATISTICS_UI_ARGS='capture --scenario tools/lap_records_ui.json --target native --binary /tmp/dd2_native --fixture /tmp/wasm-dd2/stats-fixture --output /tmp/wasm-dd2/laps-native'
make verify-statistics-ui STATISTICS_UI_ARGS='capture --scenario tools/lap_records_ui.json --target native --binary /tmp/dd2_asan --fixture /tmp/wasm-dd2/stats-fixture --output /tmp/wasm-dd2/laps-asan'
make capture-browser-statistics-ui BROWSER_STATISTICS_UI_ARGS='web/dd2 /tmp/wasm-dd2/laps-browser /tmp/wasm-dd2/stats-fixture tools/lap_records_ui.json'
make verify-statistics-ui STATISTICS_UI_ARGS='baseline --scenario tools/lap_records_ui.json --fixture /tmp/wasm-dd2/stats-fixture --original /tmp/wasm-dd2/laps-original --binary /tmp/dd2_native_before_871 --output /tmp/wasm-dd2/laps-before'
make verify-statistics-ui STATISTICS_UI_ARGS='compare --scenario tools/lap_records_ui.json --fixture /tmp/wasm-dd2/stats-fixture --original /tmp/wasm-dd2/laps-original --native /tmp/wasm-dd2/laps-native --asan /tmp/wasm-dd2/laps-asan --browser /tmp/wasm-dd2/laps-browser --before /tmp/wasm-dd2/laps-before/report.json --report /tmp/wasm-dd2/laps-verified.json --clean'
make clean-logs
```

The original-replay check records an actual Stock Car practice run with car 1
on track 1 in the unmodified original, holds acceleration after the countdown,
then uses Retire / Yes and Save Replay to create a real `0x2020` card. All
7,206 payload bytes must match the original's live metadata, recorded input
tape and car order. Original, native, AddressSanitizer and browser captures
start with that same complete card as a file input. The browser seeds IndexedDB
before engine startup and uses trusted Playwright keys and normal IDBFS loading.

Loading through File Manager must restore every replay metadata field, the
complete input tape and car order, apply recorded acceleration after the
countdown, reach the tape terminal naturally, and restore the prior frontend
choices. The complete card must remain unchanged. Two selected card-menu cycles
compare all 128 framebuffers and palettes per target; the browser also checks
the actual canvas pixels. Changed metadata, missing acceleration, premature
completion, changed tape/order/settings/card state and changed pixels/palettes
are rejected. These checks cover this actual replay save/load route; complete
chronological racing video/PCM, every tape and replay controls remain open.

```sh
make clean-logs
make verify-original-replay ORIGINAL_REPLAY_ARGS='generate --output /tmp/wasm-dd2/replay-fixture'
make verify-original-replay ORIGINAL_REPLAY_ARGS='capture --target original --fixture /tmp/wasm-dd2/replay-fixture --output /tmp/wasm-dd2/replay-original'
make verify-original-replay ORIGINAL_REPLAY_ARGS='capture --target native --binary /tmp/dd2_native --fixture /tmp/wasm-dd2/replay-fixture --reference /tmp/wasm-dd2/replay-original --output /tmp/wasm-dd2/replay-native'
make capture-browser-original-replay BROWSER_ORIGINAL_REPLAY_ARGS='web/dd2 /tmp/wasm-dd2/replay-browser /tmp/wasm-dd2/replay-fixture /tmp/wasm-dd2/replay-original'
make verify-original-replay ORIGINAL_REPLAY_ARGS='compare --fixture /tmp/wasm-dd2/replay-fixture --original /tmp/wasm-dd2/replay-original --native /tmp/wasm-dd2/replay-native --browser /tmp/wasm-dd2/replay-browser --report /tmp/wasm-dd2/replay-verified.json --clean'
make clean-logs
```

The replay-history check extends that same original-produced card to the entire
natural racing playback. Four read-only hardware breakpoint slots observe all
engine `GetTickCount` returns, all Watcom random triples, real X11 input and each
`Play_Game` / `Draw_All` pending presentation. Native uses the recorded clock
file and checks every independently calculated random triple from seed 1.
Browser uses the same explicit API files, production RNG assertions, read-only
API counters/seed observations and trusted keyboard input. Its copied input
files are labelled `input-*`; they are not presented as observed API outputs.

Every racing indexed framebuffer and palette is compared literally, along
with physics counters, tape position, recorded controls and hashes of all 20
vehicle positions/matrices/angles, state blocks, render FD and wheel FD. Browser
captures additionally check every canvas RGBA pixel. Physical primitive-buffer
storage is retained as a diagnostic: prior loading flips can exchange its two
packet slots without changing any physics or output pixel. It is not a pose
comparison. Captures use compressed indexed pictures and optional selected
vehicle checkpoints, never full memory images per frame.

The verified car-1 Stock Car practice replay has 109 racing presentations,
550 actual clock returns and 484 computed random triples in one recorded
original timing history. Native, AddressSanitizer and browser match all 109
images/palettes and vehicle-state hashes; 57 changed-state, pixel and palette
cases are rejected. An earlier original timing history produced 273 images,
also literally matched in the completed video/buffer-storage diagnosis.
Debugger stops affect elapsed time, so image counts can differ between original
runs. The ports must match the exact recorded history used for their comparison.
Chronological audio, loading/menu video, undebugged physical timing and other
replays remain open; this is not whole-game A/V acceptance.

```sh
make clean-logs
make verify-replay-history REPLAY_HISTORY_ARGS='capture --target original --fixture /tmp/wasm-dd2/replay-fixture --output /tmp/wasm-dd2/replay-history-original'
make verify-replay-history REPLAY_HISTORY_ARGS='capture --target native --binary /tmp/dd2_native --fixture /tmp/wasm-dd2/replay-fixture --reference /tmp/wasm-dd2/replay-history-original/history --output /tmp/wasm-dd2/replay-history-native'
python3 tools/wasm_rng_layout.py --wasm web/dd2/index.wasm --output /tmp/wasm-dd2/replay-history-layout
make capture-browser-replay-history BROWSER_REPLAY_HISTORY_ARGS='web/dd2 /tmp/wasm-dd2/replay-history-browser /tmp/wasm-dd2/replay-fixture /tmp/wasm-dd2/replay-history-original /tmp/wasm-dd2/replay-history-layout/layout.json'
make verify-replay-history REPLAY_HISTORY_ARGS='compare --original /tmp/wasm-dd2/replay-history-original --native /tmp/wasm-dd2/replay-history-native --browser /tmp/wasm-dd2/replay-history-browser --report /tmp/wasm-dd2/replay-history-verified.json --clean'
make clean-logs
```

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

Patch 861 preserves the fractional source cursor when duplicating a sound.
The actual Wine original copies freqAccNum and resets only the integral cursor
and playback state. Resetting the fraction made a naturally duplicated racing
collision sound advance to byte 158 instead of 159 under the same controls.
`verify-original-race-audio` replays actual source lifetimes, duplication,
seeks, changing pan/volume/frequency, CD sector/ring writes and primary block
extents through the production source mixer. Original cursor and gain values
are assertions, with no waveform alignment or cursor/phase injection.
Changed controls are ordered by their actual locked Recalc completion, rather
than the earlier API-entry log. The virtual CD observer records each successful
read's sector range and monotonic interval when `--trace-cd` is enabled.

The bounded original menu-to-level-9 run, including CD13 to CD10 and a played
duplicate, matches all 10,864,208 accepted PCM bytes on native, native ASan and
WASM. All 1,356,263 independently consumed stereo frames also match; empty-FIFO
underrun/reset boundaries and queued samples stay explicit. Both ports reject
the old zero duplicate phase. The report is
`/tmp/wasm-dd2/race-mixer-861-components-final/report.json`. This is a mixer
component comparison using original controls and sectors, not identical-input
engine scheduling, browser sink timing or full game A/V parity. The capture
must reach the race and exercise a naturally played duplicate.

```sh
make clean-logs
python3 tools/reference/capture.py --mode audio --audio --trace-cd \
  --audio-tail 30 --timeout 120 --wine-debug=-all,+timestamp,+dsound \
  --output /tmp/wasm-dd2/fresh-original-race-audio
make verify-original-race-audio ORIGINAL_RACE_AUDIO_ARGS='--capture /tmp/wasm-dd2/fresh-original-race-audio --output /tmp/wasm-dd2/fresh-race-mixer'
make clean-logs
```

The same component checker accepts a complete, naturally terminated original
replay with `--replay-fixture`. `capture_original_replay_audio.py` loads an
original-produced replay card through six genuine X11 keys, without debugger
stops or engine writes. It checks the loaded tape/order, recorded acceleration,
natural tape completion, restored frontend settings and unchanged card. The
capture includes DirectSound, DirectDraw, clock relay and inline timer markers;
missing presentations or callback markers still reject. The component checker
also validates the producer, replay metadata and completion before mixing.

The checked practice replay compared all 3,206,768 accepted PCM bytes and the
independently consumed prefix in native, ASan and WASM, including 93 sources,
the naturally played duplicate and menu/track 2 CD sectors. Zero duplicate
phase and damaged/truncated PCM reject. The comparison report is
`/tmp/wasm-dd2/replay-audio-871-components/report.json`. Original sound controls
are supplied to this isolated mixer test; complete replay audio from the live
port engines, browser delivery and physical timing remain unproved.

```sh
make clean-logs
python3 tools/capture_original_replay_audio.py \
  --fixture /tmp/wasm-dd2/replay-fixture \
  --output /tmp/wasm-dd2/original-replay-audio
make verify-original-race-audio ORIGINAL_RACE_AUDIO_ARGS='--capture /tmp/wasm-dd2/original-replay-audio --replay-fixture /tmp/wasm-dd2/replay-fixture --output /tmp/wasm-dd2/replay-mixer'
make clean-logs
```

For racing-engine timing inputs without debugger stops, `--trace-game-clock`
enables filtered Wine relay in the private reference prefix. It exports only
completed GetTickCount calls whose return addresses match the five import-call
sites verified in the unmodified executable. `game-clock/ticks.bin` contains
their literal DWORD returns; `observations.bin` records entry/return trace
timestamps, trace line numbers, original Flip counts and caller addresses.
Wine trace timestamps have limited precision and tracing affects the observed
timing. These are explicit replay inputs, not physical-clock acceptance.
DLL-internal calls are excluded by their callers. After the original process
has terminated, a single unreturned entry on the final trace line may be
reported as an incomplete tail; its return is never invented. Interior gaps,
reordered calls and unknown engine callers reject and remove partial exports.
The audio checkpoint also records the initial save hash. Exporting these inputs
does not yet compare the ports' complete racing-engine audio scheduling.

Patch 862 lets a real native SDL application use `DD2_TICK_REPLAY` without
overriding it with the default live clock. Ordinary interactive startup retains
its live clock; explicitly requesting both clock modes still rejects. The
`verify-native-sdl-clock` fixture creates an actual SDL window and checks literal
wrapped/repeated clock returns, exhausted/partial/leftover inputs, the explicit
mode conflict and the old initializer's failure. A native application diagnosis
also reaches level 9/cf404 with identical original clock-call positions at all
906 racing presentations. Its full PCM still differs beginning at sample
89,161; per-presentation device input and multimedia timer scheduling need
further comparison. This is not racing audio/video acceptance.

Patch 863 restores the original game-window activation and its independent
multimedia timers. An undebugged original relay records `ShowWindow` calling
`timeSetEvent` at return site `0x4133d8` (ID 16), before `Init_Application`
registers another at `0x412a9d` (ID 33). Cancelling the stored second ID leaves
the first running. The previous USER32 shim emitted no activation, collapsed
all timer IDs to 1 and cancelled all timers together. Recorded-device mode
also skipped the timer, leaving completed one-shot channels occupied.

Timers now follow elapsed real or audio-device time, including menus with a
fixed engine frame counter, and preserve fractional sample time. The old
fitted frame-counter phase is removed. The registration trace and
[Wine 10 timer implementation](https://github.com/wine-mirror/wine/blob/wine-10.0/dlls/winmm/time.c)
establish separate IDs, cancellation and WORD-sized generation behavior.
The transport fixture tests production USER32/timer/mixer code on native,
native ASan and WASM: staggered deadlines across clock wrap, independent
cancellation/reactivation, WORD ID wrap, reentrant polling and fixed-menu-frame
observed sample progress. It also rejects the previous timer implementation,
missing USER32 activation, skipped replay polling and six damaged original
registration traces.

The actual native application diagnosis now matches the first 1,156 mono
sound control calls at their original presentation positions, compared with
five before this correction. Later source-control timing and full PCM still
differ; the observed device input currently has only presentation resolution.
This is not full engine audio/video acceptance. Reproduce timer evidence and
transport checks with a fresh capture:

```sh
make clean-logs
python3 tools/reference/capture.py --mode audio --audio --trace-cd \
  --trace-multimedia-timer --audio-tail 20 --timeout 120 \
  --wine-debug=-all,+dsound,+ddraw --output /tmp/wasm-dd2/original-timers
make verify-multimedia-timers MULTIMEDIA_TIMER_ARGS='--original /tmp/wasm-dd2/original-timers --output /tmp/wasm-dd2/timer-transport'
make clean-logs
```

`--trace-multimedia-timer` can also be combined with `--trace-game-clock`.
Relay timestamps and tracing overhead remain explicit input limitations;
registration evidence does not observe every timer callback.

For actual callback ordering, `--trace-timer-callbacks` builds a private
forwarding `winmm.dll` with clang, lld-link and llvm-dlltool. All 189 installed
Wine exports retain their names/ordinals; only `timeSetEvent` and
`timeKillEvent` are wrapped. The installed Wine implementation executes the
real timers and each original callback retains its arguments. Every backend
section byte is unchanged. The private backend copy only neutralizes the
16-byte Wine builtin marker in its non-executable DOS stub so it can load
under an alias. The game executable is unchanged. The observer is scoped to
the private reference Wine process.

Binary records and inline trace markers identify callback begin/end among
sound API and mixer calls. Read-only observations of the original callback
counter are bracketed with `CLOCK_MONOTONIC_RAW`, the Linux Wine QPC domain
([Wine source](https://github.com/wine-mirror/wine/blob/wine-10.0/dlls/ntdll/unix/sync.c)).
The existing audio observations retain their separate `CLOCK_MONOTONIC`
intervals. No offset is fitted and no engine state is written. The verifier
checks each counter against completed/started callbacks, direct Wine and
observer timer probes, callback arguments, independent cancellation, all
forwarder targets and deliberately damaged journals, markers and counters.
Logging changes timing; these are original service observations, not complete
port audio/video acceptance. Controls can change between individual sources
inside one original mix block, so a per-presentation device clock alone is
insufficient for the full audio comparison.

```sh
make clean-logs
python3 tools/reference/capture.py --mode audio --audio --trace-cd \
  --trace-game-clock --trace-timer-callbacks --audio-tail 20 --timeout 120 \
  --wine-debug=-all,+dsound,+ddraw --output /tmp/wasm-dd2/original-callbacks
make verify-original-timer-callbacks ORIGINAL_TIMER_CALLBACK_ARGS='--capture /tmp/wasm-dd2/original-callbacks --output /tmp/wasm-dd2/callback-verification'
make clean-logs
```

Use a fresh output directory for each run. Callback observation can be combined
with game-clock tracing; it is separate from the registration relay mode.

One callback-capable reference run validates 100 actual callbacks against 987
read-only original counter observations. Its chronological mixer comparison
matches all 7,108,480 accepted PCM bytes on native, native ASan and WASM. That
comparison still supplies recorded original controls to the mixer component.
The actual native application baseline matches original clock-call positions
at all 892 captured racing presentations, but its PCM differs at frame
100,626: the first racing ambient source starts at frame 101,067 instead of
100,626. Later it stops a source that the original timer has already cleared.
The same reference contains 137 changed controls affecting a source mixed
later within an already started primary block. These observations require
service ordering within mixer blocks; the per-presentation device input does
not establish full engine audio equivalence. Capture timings and the first
remaining difference vary between runs.

The optional `DD2_AUDIO_SERVICES` comparison input preserves the observed
ordering of individual source mixing steps, control commits and actual timer
callbacks within primary blocks. The real application executes its own engine
controls and callback body; recorded API values, statuses, cursors and gains
are assertions. Main-thread API calls must also occur at the original clock
and presentation positions. Wine's looping CD ring and the port's finite CD
track are checked as separate storage representations. Interactive playback
uses its existing clock path; this input does not establish interactive timing
or other scenarios' audio/video equivalence.

Patch 869 extends that comparison input to main-thread sound APIs inside a
timer callback's observed lifetime. The real original replay contains SetPan,
SetVolume and SetFrequency between a callback's begin and its GetStatus call.
The exporter retains all three operations and both callback markers in their
literal order, with the same trace/clock provenance. Overlapping callbacks,
unknown contexts and unmatched begin/end records reject.

The port preserves the executing callback's own C continuation while the main
engine supplies intervening calls. Native uses `ucontext` with ASan stack-switch
annotations; browser builds use
[Emscripten fibers and Asyncify](https://emscripten.org/docs/api_reference/fiber.h.html).
Inputs without interleaving retain the existing inline callback path. The
synchronous Node build rejects interleaved service inputs explicitly; the
focused WASM test enables fibers with `DD2_AUDIO_FIBERS` and Asyncify.

`verify-audio-callback-interleave` checks 512 callback executions per native,
ASan and WASM target. Its four controlled sequences exercise suspension before
and between callback APIs and before callback completion. Main and nested
callback locals must survive, every callback must execute exactly once, and
playback statuses must follow the port's own stop/play operations. Fifteen
damaged API/status/context cases reject; the prior inline scheduler also fails
on native and WASM. These are scheduling tests, not a complete replay-engine
PCM comparison. The actual startup/attract PCM regression remains a separate
check. The bounded replay-engine comparison below exercises these continuations
with the real application and explicitly scheduled keyboard input.

```sh
make clean-logs
make verify-audio-callback-interleave AUDIO_CALLBACK_INTERLEAVE_ARGS='--output /tmp/wasm-dd2/callback-interleave-check'
make clean-logs
```

One actual startup/frontend/level-9 attract window now passes on native,
native ASan and the browser: 16,889 observed services, 98 source lifetimes and
100 callbacks produce all 7,108,480 accepted PCM bytes and the 7,094,376-byte
played prefix exactly like the unmodified original. All 2,009 WebAudio buffers
match the C mix without missing or dropped buffers. Eleven damaged input cases
are rejected, including cursor/gain changes, moved API calls and missing/wrong
callbacks; gain and callback mutations also run in the actual browser. This
proves that bounded trace with observed service timing. Other audio scenarios,
interactive scheduling and video remain separate requirements.

The complete captured replay audio window also passes on native, native ASan
and the browser: all 3,206,768 accepted PCM bytes and the independently played
prefix match the running original. The real engines load an original-produced
card, process six genuine keys, execute all 4,961 observed services and 45 timer
callbacks, decode the tape terminal naturally, and restore frontend settings.
All 902 WebAudio buffers match the C output without missing or dropped buffers.
The comparator independently exports the original service trace again and
checks its provenance, the whole output, card/tape identity and frontend/CD
endpoint. Thirteen negative cases reject, including two actual engine runs with the
first Enter release shifted by one presentation in either direction.

This uses diagnostic keyboard scheduling: key-down positions come from the
original's frontend sound triggers, rather than recorded OS input timestamps.
In this recording, the first confirmation at presentation 1740 reaches the next
slab sound at 1770. The code's two button stages and two eight-frame rotations
suggest twelve first-stage frames instead of its seven-frame minimum. Releasing
Enter for the pad poll at 1752 reproduces that sequence; immediate release starts
the sound five frames early. This is an explicit input-duration hypothesis, not
a verified original key-release event or an engine correction. Full original
OS input identity, chronological video and physical timing remain open.

Native observers use hardware execution breakpoints and one hardware write
watchpoint on the port's actual audio-completion flag. They capture the selected
card/tape/settings observations while stopped at that endpoint, before detach
allows further frontend activity. The browser pauses ordinary Asyncify yield
callbacks for trusted Playwright keys and ends at the audio completion yield;
no observer writes engine state. Successful raw port PCM is removed after the
comparison report is written.

For a completed original replay capture and its independently verified mixer
and service exports, run these captures. `RELEASE_FLIP` must be diagnosed for
that original recording; 1752 belongs to the recording described above.

```sh
make clean-logs
python3 tools/capture_native_replay_audio.py --binary /tmp/wasm-dd2/audio-native \
  --fixture /tmp/wasm-dd2/replay-fixture --original /tmp/wasm-dd2/original-replay-audio \
  --services /tmp/wasm-dd2/replay-services --output /tmp/wasm-dd2/native-replay-audio \
  --first-return-release-flip "$RELEASE_FLIP"
python3 tools/capture_native_replay_audio.py --binary /tmp/wasm-dd2/audio-asan \
  --fixture /tmp/wasm-dd2/replay-fixture --original /tmp/wasm-dd2/original-replay-audio \
  --services /tmp/wasm-dd2/replay-services --output /tmp/wasm-dd2/asan-replay-audio \
  --first-return-release-flip "$RELEASE_FLIP"
node tools/browser/capture_engine_audio.js /tmp/wasm-dd2/audio-browser \
  /tmp/wasm-dd2/browser-replay-audio /tmp/wasm-dd2/replay-services/services.bin \
  /tmp/wasm-dd2/original-replay-audio/game-clock/ticks.bin \
  /tmp/wasm-dd2/replay-fixture /tmp/wasm-dd2/native-replay-audio/checkpoint.json
# Both of these native captures must fail with the real API-position assertion:
for offset in -1 1; do
  python3 tools/capture_native_replay_audio.py --binary /tmp/wasm-dd2/audio-native \
    --fixture /tmp/wasm-dd2/replay-fixture --original /tmp/wasm-dd2/original-replay-audio \
    --services /tmp/wasm-dd2/replay-services --output "/tmp/wasm-dd2/replay-release-$offset" \
    --first-return-release-flip "$((RELEASE_FLIP + offset))"
done
make verify-replay-engine-audio REPLAY_ENGINE_AUDIO_ARGS='--fixture /tmp/wasm-dd2/replay-fixture --original /tmp/wasm-dd2/original-replay-audio --services /tmp/wasm-dd2/replay-services --mixer /tmp/wasm-dd2/replay-mixer --native /tmp/wasm-dd2/native-replay-audio --native-asan /tmp/wasm-dd2/asan-replay-audio --browser /tmp/wasm-dd2/browser-replay-audio --negative-early /tmp/wasm-dd2/replay-release--1 --negative-late /tmp/wasm-dd2/replay-release-1 --output /tmp/wasm-dd2/replay-engine-comparison'
make clean-logs
```

An additional replay recording verifies all twelve key-down/up edges directly
from Wine `+msg` entry/return pairs in the unchanged original window procedure.
Each edge retains its VK, message, lParam, thread, window, trace timestamp and
presentation/game-clock position. The exporter independently rereads and hashes
the original trace; it rejects edited input JSON. Native and browser captures
cross-check these positions against the original sound triggers and observe the
actual clock counter at each edge. This recording passes all 3,164,408 accepted
PCM bytes and the 3,150,304-byte independently played prefix on native, native
ASan and WASM, with 5,608 services, 386 callbacks and 891 WebAudio buffers.
Seventeen negative cases reject, including actual one-presentation early/late
Enter releases and damaged keyboard provenance, release and browser clock
observations.

This covers the shared menu/replay sound device. The original intro completes
naturally to avoid its Wine startup Escape race; ports skip their intro with a
real Escape key. Intro audio equivalence, physical OS timing, chronological video
and other scenarios remain open. Full source lParams are retained as evidence;
the port input bridge consumes their key-up bit rather than reproducing every
OS message field. The bounded audio endpoint is the last observed service's
presentation/clock position, which can precede the trace's final presentation.

Prefer observed edges for a new capture. After independently checking its mixer
and exporting audio services, use the following route; the legacy diagnostic
release route above remains available for recordings without `+msg` evidence.

```sh
make clean-logs
python3 tools/capture_original_replay_audio.py --fixture /tmp/wasm-dd2/replay-fixture \
  --output /tmp/wasm-dd2/original-replay-audio --trace-keyboard --keep-movie
# The capture writes keyboard-input.json. For an existing +msg capture:
make export-original-keyboard ORIGINAL_KEYBOARD_ARGS='--capture /tmp/wasm-dd2/original-replay-audio --fixture /tmp/wasm-dd2/replay-fixture --output /tmp/wasm-dd2/replay-keyboard.json'
for target in native asan; do
  python3 tools/capture_native_replay_audio.py --binary "/tmp/wasm-dd2/audio-$target" \
    --fixture /tmp/wasm-dd2/replay-fixture --original /tmp/wasm-dd2/original-replay-audio \
    --services /tmp/wasm-dd2/replay-services --keyboard-input /tmp/wasm-dd2/replay-keyboard.json \
    --output "/tmp/wasm-dd2/$target-replay-audio"
done
python3 tools/wasm_rng_layout.py --wasm /tmp/wasm-dd2/audio-browser/index.wasm \
  --output /tmp/wasm-dd2/replay-browser-layout
node tools/browser/capture_engine_audio.js /tmp/wasm-dd2/audio-browser \
  /tmp/wasm-dd2/browser-replay-audio /tmp/wasm-dd2/replay-services/services.bin \
  /tmp/wasm-dd2/original-replay-audio/game-clock/ticks.bin /tmp/wasm-dd2/replay-fixture \
  /tmp/wasm-dd2/native-replay-audio/checkpoint.json /tmp/wasm-dd2/replay-browser-layout/layout.json
# Both deliberately perturbed captures must fail the real API-position assertion:
for offset in -1 1; do
  python3 tools/capture_native_replay_audio.py --binary /tmp/wasm-dd2/audio-native \
    --fixture /tmp/wasm-dd2/replay-fixture --original /tmp/wasm-dd2/original-replay-audio \
    --services /tmp/wasm-dd2/replay-services --keyboard-input /tmp/wasm-dd2/replay-keyboard.json \
    --diagnostic-release-offset "$offset" --output "/tmp/wasm-dd2/replay-release-$offset"
done
make verify-replay-engine-audio REPLAY_ENGINE_AUDIO_ARGS='--fixture /tmp/wasm-dd2/replay-fixture --original /tmp/wasm-dd2/original-replay-audio --services /tmp/wasm-dd2/replay-services --mixer /tmp/wasm-dd2/replay-mixer --keyboard-input /tmp/wasm-dd2/replay-keyboard.json --native /tmp/wasm-dd2/native-replay-audio --native-asan /tmp/wasm-dd2/asan-replay-audio --browser /tmp/wasm-dd2/browser-replay-audio --negative-early /tmp/wasm-dd2/replay-release--1 --negative-late /tmp/wasm-dd2/replay-release-1 --output /tmp/wasm-dd2/replay-engine-comparison'
make clean-logs
```

The same-run video/audio route now passes a new original replay recording:
native, native ASan and WASM match every indexed/palette byte in all 2,340
presentations through the audio endpoint, including menu/loading, 287 racing
presentations and return to File Manager. All 3,217,320 accepted PCM bytes and
the independently played 3,203,216-byte prefix match from these same runs, with
5,654 observed services and 387 callbacks. Four main sound APIs actually
interleave with timer callbacks in this source recording. The browser directly
compares each full indexed image/palette against the original binary record,
checks actual `getImageData` canvas pixels, and observes the real clock counter
at every presentation. The joint verifier independently re-exports original
video and audio inputs and rejects 25 damaged input/output cases in total.

The reference-only DirectDraw1 observer forwards the game's calls to the
installed Wine backend, preserving all 28 exports and all 17 backend PE
sections. Its private backend alias changes only the non-executable DOS-stub
builtin marker, as with the timer observer. Each successful Flip records the
actual uploaded indexed pixels, confirms equality with the engine buffer, and
independently reads any attached device palette. The executed observer rebuilds
byte for byte from its sources. There are no debugger stops or writes to game
memory. Observer I/O changes measured timing; recorded clocks/services are
explicit comparison inputs.

This proof covers one bounded menu/replay sequence. Original intro output and
physical display/OS timing remain open. The first six successful original
uploads have no attached palette; their indexed data and engine palette are
compared, while original device RGB output remains unproved. All 2,334 attached
device palettes within the window match the engine palette exactly. Two original
presentations after the final audio service are outside the joint window.
Capture raw video is bounded to 4,096 frames (under 1.2 GiB); successful raw port
images and PCM are deleted after writing the joint report.

To capture this evidence, add `--trace-video` to the original command together
with `--trace-keyboard --keep-movie`, then export its video after the original
capture completes. Use a fresh directory for each run:

```sh
make clean-logs
python3 tools/capture_original_replay_audio.py --fixture /tmp/wasm-dd2/replay-fixture \
  --output /tmp/wasm-dd2/original-replay-av --trace-keyboard --keep-movie --trace-video
make export-original-video ORIGINAL_VIDEO_ARGS='--capture /tmp/wasm-dd2/original-replay-av --output /tmp/wasm-dd2/original-replay-video.json'
# Verify this recording's chronological mixer and export its audio services,
# as above. Use --trace-video for both native and ASan replay audio captures.
node tools/browser/capture_engine_audio.js /tmp/wasm-dd2/audio-browser \
  /tmp/wasm-dd2/browser-replay-av /tmp/wasm-dd2/replay-av-services/services.bin \
  /tmp/wasm-dd2/original-replay-av/game-clock/ticks.bin /tmp/wasm-dd2/replay-fixture \
  /tmp/wasm-dd2/native-replay-av/checkpoint.json /tmp/wasm-dd2/replay-browser-layout/layout.json \
  /tmp/wasm-dd2/original-replay-video.json
# Add --video-reference /tmp/wasm-dd2/original-replay-video.json to the full
# verify-replay-engine-audio command, using the same three video/audio captures
# and actual early/late input rejection runs from this new original recording.
make clean-logs
```

Export inputs from one original capture that has passed the clock, callback
observer and chronological mixer checks, then run both actual applications:

```sh
make clean-logs
python3 tools/reference/engine_audio_services.py --capture /tmp/wasm-dd2/original \
  --mixer /tmp/wasm-dd2/mixer-comparison --output /tmp/wasm-dd2/audio-services
make native NATIVE=/tmp/wasm-dd2/audio-native
python3 tools/capture_native_engine_audio.py --binary /tmp/wasm-dd2/audio-native \
  --services /tmp/wasm-dd2/audio-services/services.bin \
  --clock /tmp/wasm-dd2/original/game-clock/ticks.bin --output /tmp/wasm-dd2/audio-native-capture
bash tools/build_web.sh /tmp/wasm-dd2/audio-browser
node tools/browser/capture_engine_audio.js /tmp/wasm-dd2/audio-browser \
  /tmp/wasm-dd2/audio-browser-capture /tmp/wasm-dd2/audio-services/services.bin \
  /tmp/wasm-dd2/original/game-clock/ticks.bin
make verify-engine-audio ENGINE_AUDIO_ARGS='--original /tmp/wasm-dd2/original --mixer /tmp/wasm-dd2/mixer-comparison --services /tmp/wasm-dd2/audio-services/services.bin --native /tmp/wasm-dd2/audio-native-capture --browser /tmp/wasm-dd2/audio-browser-capture --binary /tmp/wasm-dd2/audio-native --browser-build /tmp/wasm-dd2/audio-browser --negative-runs --output /tmp/wasm-dd2/engine-audio-comparison'
make clean-logs
```

The verifier independently rebuilds the service input from the original trace,
compares every accepted PCM byte and the played prefix, and requires exact
WebAudio buffer delivery without dropped buffers. Negative runs exercise the
actual native application with damaged input, source cursors/gains, control
values, API positions and timer callbacks. `--native-asan` can additionally
check an actual application capture made with AddressSanitizer. Successful raw
PCM is removed after its comparison report is written. Browser observation
stops at a normal Asyncify yield after the bounded input completes.

```sh
make clean-logs
make verify-native-sdl-clock NATIVE_SDL_CLOCK_ARGS='--output /tmp/wasm-dd2/native-sdl-clock'
make clean-logs
```

```sh
make clean-logs
python3 tools/reference/capture.py --mode audio --audio --trace-cd \
  --trace-game-clock --audio-tail 20 --timeout 120 \
  --wine-debug=-all,+dsound,+ddraw --output /tmp/wasm-dd2/racing-clock-original
make verify-original-game-clock ORIGINAL_GAME_CLOCK_ARGS='--capture /tmp/wasm-dd2/racing-clock-original --executable DestructionDerby2/dd2h.exe --output /tmp/wasm-dd2/racing-clock-negative-tests'
make clean-logs
```

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
