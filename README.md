# wasm-dd2 — Destruction Derby 2, reimplemented in C → WebAssembly

A from-scratch C reimplementation of **Destruction Derby 2** (1996, Reflections/Psygnosis),
built from the shipped binary + `Dirinfo` asset archive (no source code), compiling to
**WebAssembly** (Emscripten, SDL3 + OpenGL ES3 / WebGL2) and carrying the
[`wasm-dvd-gl`](../wasm-dvd-gl) cinematic look (low-res render → H.264/WebCodecs crunch → bilinear upscale).

The engine splits into a **deterministic, renderer-free simulation core** (assets, track, vehicle
physics, AI, race) and a **GL render layer**, so it runs both in a browser and **fully headless**.
A self-play AI drives every car; a headless mode renders offscreen and emits PNG screenshots, so
complete races are verifiable without a display.

## Status

| Area | State |
|------|-------|
| Asset extraction (`Dirinfo`) | ✅ exact — 114 files, format reverse-engineered (`docs/REVERSING.md`) |
| Tracks | ✅ **11 playable** (circuits LEV1–7, demolition arenas LEV8–B) complete **6/6 cars × 3 seeds, deterministic, crash-free** |
| Vehicle physics | ✅ **faithful** — DD2's slip-angle tire-force model reconstructed from `Car_Drive_Motion` (real surface-friction/grip constants); default. Arcade fallback via `DD2_ARCADE=1` |
| WASM build | ✅ runs in-browser (WebGL2), verified via headless Chromium |
| DVD look | ✅ H.264/WebCodecs crunch + bilinear upscale, verified in-browser |
| Verification | ✅ `tests/run_all.sh` + montage + per-track screenshot sequences |

**Not real tracks** (properties of the shipped data, see `out/all_tracks_montage.png`): `LEV0`/`LEVF`
are the menu/special levels (no drivable loop); `LEVC`/`LEVD`/`LEVE` are absent from `Dirinfo`.

**Known issues / remaining fidelity work:** track geometry uses a cross-section reconstruction of the
real vertices (not yet the original strip/polygon connectivity), so **LEV4 has a cosmetic infield-chord
sliver** (races fine). The deeper-fidelity items — real strip geometry (LEVEL.DAT §0/§1) and faithful
collision/AI command lists — are documented in `docs/STATUS.md`/`docs/REVERSING.md`.

## Build & run

```bash
# 1. extract assets from the original Dirinfo archive
python3 -m venv .venv && . .venv/bin/activate && pip install pillow numpy
python tools/unpack_dirinfo.py                 # -> assets/raw/

# 2a. native headless build (EGL + Mesa llvmpipe; for verification/screenshots)
./build_native.sh
./build-native/dd2_view  assets/raw/LEV5/LEVEL.DAT out/view.png      # render a track
./build-native/dd2_race  assets/raw/LEV5/LEVEL.DAT out/race 2 6 1 2  # race: <laps> <cars> <seed> <shot_interval_s>

# 2b. WASM build (needs emsdk at ~/Git/emsdk)
./build_wasm.sh                                # -> web/build/
#   serve web/build/ over http and open index.html  (Chrome/Edge for the DVD look)
```

## Verify

```bash
. .venv/bin/activate
./tests/run_all.sh          # every level × 3 seeds: completion + determinism
python tools/montage.py     # out/all_tracks_montage.png  (all 16 addresses)
python tools/sequences.py   # out/seq_LEV*.png            (race progress per track)
# browser render check (Playwright + Chromium in tools/browser/):
PLAYWRIGHT_BROWSERS_PATH=~/.cache/ms-playwright node tools/browser/shot.js web/build out/wasm.png 9000
```

## Layout

```
src/core/      deterministic sim: dmath, track (DAT→road ribbon), vehicle (tire physics + AI), race
src/render/    GLES3 / WebGL2 renderer (track + cars)
src/platform/  headless EGL context + PNG screenshot (native); SDL3 for browser
src/main_*.c   headless viewer / headless race runner / SDL3+WebGL2 frontend (DVD look)
tools/         asset extraction + RE + verification (Python) ; browser/ (Playwright)
docs/          STATUS.md (resume/working doc) ; REVERSING.md (binary/format notes)
DestructionDerby2/  original game data (input, read-only)
```

Built with Claude Code (Opus 4.8).
