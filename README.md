# wasm-dd2 — Destruction Derby 2 (dd2h.exe) → WebAssembly

A faithful port of the 1996 game **Destruction Derby 2** (`dd2h.exe`, 640x480 build) to reproducible C
compiling to native and WebAssembly. The engine C is **mechanically derived from the binary via Ghidra**
(decompile → transpile/patch → compile); only the platform/runtime shim (DirectDraw→canvas,
DirectSound→WebAudio, Win32/CRT) is hand-written.

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

The WASM builds disable LLVM FastISel. With Debian Emscripten 3.1.69 / LLVM 19, its
folded unsigned memory offsets trap on valid wrapping 32-bit engine addresses in
`AI_Com_Server` on level 10. SelectionDAG emits the required 32-bit addition.
`make verify` and `make verify-wasm` check every demo and fail on nonzero exits,
engine errors, or timeouts; detailed logs go to `/tmp/dd2-verify-native` and
`/tmp/dd2-verify-wasm`. These are crash checks, not proof of complete game fidelity.

The Node build transfers `DD2_*` options from `process.env` into Emscripten's libc
environment before boot. `make verify-parity` enables sound and compares every
presented indexed framebuffer, its palette, the flip/RNG log, the effects PCM
stream and the separate 44100Hz CD PCM stream
byte-for-byte between native and WASM on all ten demos. Logs and results go to
`/tmp/dd2-parity`; identical captures are removed, failed captures are retained.
This target does not compare with `dd2h.exe` or validate menu actions.

Current Debian validation: all ten sound-enabled demos return on both ports.
Every presented frame and palette, flip/RNG log and generated PCM byte matches
across native and WASM (1525-1527 frames, 2638944 effects PCM bytes and 4939200
CD PCM bytes per demo). Patch
835 restores the original contiguous angle vector for the animated L1 objects;
its split stack locals caused the previous 137-frame discrepancy. Menu behavior,
full championships, other menu actions and complete comparisons with the running original
remain open acceptance work.

Redbook playback now uses the original engine's MCI track selection, Play, Stop,
resume and repeat calls. Patches 836/837 restore the original contiguous MCI
parameter blocks and mandatory CD check. The backend reads the original stereo
s16le CDDA at 44100Hz; browser builds serve the tracks separately and load one
track at a time into a shared 44100Hz WebAudio device. Effects retain their
22050Hz source buffers. CD source capture uses `DD2_CDPCM=<file>`; effects
use `DD2_SNDPCM=<file>`. Deterministic runs share a 25Hz audio clock. Interactive
CD playback and the 400ms multimedia timer use elapsed real time, including menus.

`make verify-redbook` compares native/WASM playback directly with all 18 track
prefixes and one complete track: 29821008 exact PCM bytes, including stop/resume,
end-of-track, replay and error checks. `node tools/browser/qa_redbook.js web/dd2`
navigates the CD-player menu with real keyboard input, checks Play/Stop/Next/Prev,
and compares the submitted WebAudio buffers from tracks 2/3 with their CDDA files.
These checks establish exact source PCM and exercised controls. They do not yet
establish mixed hardware-output parity or original pause/resume timing.

Menu effects now keep playing with the race counter fixed: interactive DirectSound
mixing uses elapsed real time and flushes samples before buffer controls/queries.
`make verify-menu-audio` checks looping, Stop, one-shot exhaustion and frequency
changes on both backends against 4848 known PCM bytes with `cf=0`.
`node tools/browser/qa_menu_audio.js web/dd2` verifies real menu navigation submits
nonzero effect buffers without advancing the race counter. The complete original
mixed stream and its timing remain unverified.

Patch 838 fixes corrupted championship names: the computer-name table began at
an 8-byte offset per human instead of the original 16-byte offset. A real menu
run through Championship, name entry, Go, Pause/Retire/Yes and View League now
matches the original in both native and browser rendering on all four divisions:
all 20 names/points, all 307200
framebuffer pixels per page, and all 1024 palette bytes per page.
`node tools/browser/qa_champ_scores.js web/dd2 /tmp/fresh-champ-browser` exercises
that flow and saves the score data, raw pixels/palettes and screenshots. It releases
keys after one presented frame; long held keys skipped pause items in older tests.

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

Comparing with the Windows original also needs 32-bit Wine, GDB and Xvfb. On Debian:
```sh
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install wine wine32:i386 gdb xvfb xauth
```

`make refcapture` runs the unmodified Windows EXE in a private 32-bit Wine prefix
under `third_party/wine-reference/`, with an isolated copy of `SaveGames`. A Linux
CD device adapter exposes the provisioned disc TOC and exact CDDA sector reads
through Wine's normal MCI/DirectSound driver. It needs no physical CD drive and
does not patch the EXE or engine memory. `make verify-cdrom` checks all TOC entries,
108 audio reads including track boundaries, invalid requests and descriptor
isolation. These checks verify the CD data source, not mixed playback PCM parity.
The adapter currently reports a fixed CD position in Q-channel requests. Wine's
digital playback path queries this position for TO-only restart, so this reference
cannot yet prove pause/resume cursor fidelity. That limitation must be resolved
before using it for complete original audio acceptance.

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
