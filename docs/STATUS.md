# DD2-WASM — Project Status & Resume Doc

> **READ THIS FIRST after any context reset.** It is the single source of truth for
> where the project is and what to do next. Update it at the end of every work session.

## Mission (see also the user's /goal)
From-scratch C reimplementation of **Destruction Derby 2** (1996, Reflections/Psygnosis),
compiled to **WASM via Emscripten** (SDL3 + OpenGL ES3 / WebGL2), with the **`wasm-dvd-gl`
look** (low-res FBO → H.264/WebCodecs crunch → bilinear upscale). Engine split into a
deterministic renderer-free **core** + a **GL render layer**, so it runs in-browser and
**fully headless**. A **self-play AI** drives all cars; **headless mode** renders offscreen
and dumps a PNG every N seconds.

## Definition of DONE (hard requirement from the user)
All **16 tracks LEV0–LEVF** simulated to completion **headless**, **multiple times each**,
**deterministically + crash-free**, with **screenshot sequences** confirming correct
rendering and plausible racing. Do not stop until this holds.

## Autonomy
User is away **until 2026-06-22**. Work autonomously, make all decisions. Unlimited
time/budget. A 30-min wakeup heartbeat keeps the loop alive (autonomous /loop dynamic).

## Environment (verified 2026-06-18)
- Reference project to copy build/style from: `~/Git/wasm-dvd-gl` (SDL3+GLES3+emscripten).
- emsdk: `~/Git/emsdk/emsdk_env.sh` → `emcc` 5.0.7, bundled `node` v22. SDL3 via `-sUSE_SDL=3`.
- Native toolchain: `cmake` 3.31, `ninja`, `gcc` 14, `python3` 3.13. **No clang, no passwordless sudo** (cannot apt-install).
- **Headless GL works**: EGL surfaceless + Mesa **llvmpipe** → **GLES 3.2** offscreen, verified
  by `tools/egl_probe.c` (`cc tools/egl_probe.c -lEGL -lGLESv2`). This is the headless screenshot path.
- Python venv at `.venv` (pillow, numpy). Activate: `. .venv/bin/activate`.
- No system browser / playwright (and no sudo to install). Rely on native EGL path for render tests;
  WASM build smoke-tested under emsdk `node`. WebCodecs DVD-look only runs in a real browser (test later/optional).

## Layout
```
DestructionDerby2/   original game (dd2.exe, dd2h.exe, Dirinfo, SaveGames)  [read-only input]
assets/raw/          extracted Dirinfo files (regenerable; gitignored)
assets/dirinfo_manifest.json   TOC dump
tools/               python tooling (unpack_dirinfo.py, egl_probe.c, decoders…)
docs/                STATUS.md (this), REVERSING.md (format notes)
src/core/            (planned) renderer-free sim: assets, track, physics, AI
src/render/          (planned) GLES3 renderer + DVD-look present
src/platform/        (planned) SDL3 (browser/desktop) + headless EGL
web/                 (planned) shell.html etc.
tests/               (planned) headless test harness
```

## Milestones
- [x] **M0 tooling+extraction** — venv, `Dirinfo` unpacker (114 files, 0 problems, 99.5%),
      headless EGL GLES3 proven.
- [x] **M2 renderer** — C engine renders real tracks headless (EGL→GLES3→PNG). `build_native.sh`.
- [x] **M3+M4 (approx) sim** — vehicle physics + AI + deterministic race + screenshots
      (`build-native/dd2_race`). LEV5 fully completes 2 laps, deterministic, verified.
      NOTE: this physics/AI is MY approximation, not from the binary (see RE track below).
- [x] **RE decompiler** — Ghidra (JDK21) decompiled dd2.exe (debug symbols) → 837 named funcs.
- [ ] **FIDELITY (user requirement): reconstruct logic from dd2.exe**, not approximate. In progress.
- [ ] **all 16 tracks reliably complete** — currently only LEV5 finishes; most TIME OUT (driving too weak).
- [ ] **WASM build (SDL3+WebGL2) + DVD look**.
- [~] **M1 asset decoders** — IN PROGRESS.
      - [x] `LEVEL.PAL` = 256 × `[R][G][B][pad]` (RGB order confirmed: readable white text, natural colors).
      - [x] `LEVEL.TX*` = `[u32 count]` + count×16B dir records `[tag=4][w u16][h u16][x u16][y u16][...]`,
            then 8-bit indexed pixels packed sequentially. VISUALLY CONFIRMED (CAPRIO COUNTY billboard, driver art).
      - [ ] proper per-tile extraction (cut each tile at its true w×h; verify Σ w*h == pixel-bytes).
      - [ ] `LEVEL.DAT` geometry — header is a u32 section-offset table (first ptr 0x74), then data. Decode next.
      - [ ] `.CLT` collision, `.SPR` sprites, `.ECL` (fixed 86016 B), `.TDF`, `FONT.BNK`, `VAGS\BANK1.SBK` (VAG ADPCM).
- [ ] **M2 renderer** — load a track, GLES3 free-fly camera, offscreen screenshot test.
- [ ] **M3 vehicle + physics + driving**.
- [ ] **M4 self-play AI + headless race sim** (also the main test harness).
- [ ] **M5 DVD-look present, audio, HUD, menus, race modes; verify ALL 16 tracks ×N**.

## Dirinfo archive format (cracked — details in REVERSING.md)
TOC of 114 records at file start. Record = `NAME\0` + field.
Common 9-byte field = `[flag u8][reserved u16=0][sector u16][size u32]`, **offset = sector*2048**.
Special header records: COPYRIGH.BMP `[u16 sec][u32 size]`; LOADING.BMP `[u8 flag][u16 sec][u32 size]`;
FONT.BNK `"RAW\0"[u16 sec][u32 size]`. Run `python tools/unpack_dirinfo.py` to extract.

## Verification trick
PNGs can be inspected with the Read tool (renders images). So the headless loop = decode/render → PNG →
Read → judge correctness. Use this for every visual check.

## CURRENT STATE (2026-06-18, session 1 — very late)
- **ALL 11 playable tracks PASS** (`./tests/run_all.sh`): circuits LEV1,2,3,4,5,6,7 + arenas LEV8,9,A,B,
  each 6/6 across 3 seeds, deterministic. Loop self-intersection cleanup fixed LEV2/LEV7 (clean loops).
  LEV0/F = NON-TRACK; LEVC/D/E = ABSENT (no data) — these 5 of the 16 addresses are not real tracks.
- **WASM build runs in-browser** (`./build_wasm.sh`, headless Chromium verified).
- **DVD look works in-browser**: low-res 512x384 FBO → H.264/WebCodecs crunch → bilinear upscale
  (WebCodecs confirmed active in headless Chromium). `out/wasm_dvd.png`.
- Verification: native EGL headless screenshots + self-contained Playwright browser shots
  (`node tools/browser/shot.js web/build out/x.png 12000`).
- REMAINING: faithful physics/AI/geometry from decompiled `dd2.exe` (still approximate); broader
  per-track screenshot sequences for the record.

### (earlier) CURRENT STATE (session 1 — mid)
Verification harness: `./tests/run_all.sh` (3 seeds each, completion + determinism).
- **9/11 playable levels PASS all seeds deterministically**: circuits LEV1,3,4,5,6 (6/6) + arenas
  LEV8,9,A,B (demolition mode, 6/6). LEV0/F classified NON-TRACK; LEVC/D/E ABSENT (no data).
- **LEV2, LEV7 FAIL (0/6)**: contiguous mis-ordered-vertex branch (a spurious thin sliver in the
  reconstructed loop, visible in renders) — needs the real track topology, not heuristics.
- **WASM build works**: `./build_wasm.sh` → `web/build/` (SDL3+WebGL2). Verified rendering IN A REAL
  BROWSER via headless Chromium (`tools/browser/shot.js`, Playwright) — LEV5 + AI cars on canvas.
- Sim infra: seam-free laps (integrated progress), windowed localization, spur+Laplacian+spike
  cleanup, reverse-realign + respawn recovery, arena/demolition mode, hits ranking. Deterministic.
- Verification paths: native EGL headless screenshots (primary) + headless-browser screenshots (WASM/DVD).
- STILL APPROXIMATE (not yet from binary): physics/AI/geometry. DVD look not yet added.

## NEXT ACTION (priority order)
1. **Fix LEV2/LEV7** — the only failing playable tracks. Their reconstructed loop has a spurious
   branch from mis-ordered section-2 verts. Best fix = real track topology: decode LEVEL.DAT
   section 1 (track path/strips, 86% vertex indices) cross-ref `Init_Track_Strip_Numbers`/`Track_Follow`
   in `re_out/dd2_decomp.c`. (Fallback: loop-simplification to cut the spurious detour.)
2. **DVD look** (user signature ask): add the wasm-dvd-gl present pass to `main_sdl.c` — render to a
   low-res FBO, H.264 encode/decode via WebCodecs, bilinear upscale. Verify in browser via Playwright.
3. **Faithful physics/AI** from decompiled C (`Car_Drive_Motion`, suspension, `Track_Follow`).
4. **Full verification**: native `run_all.sh` (all pass) + browser screenshots per track; multiple seeds.

## Browser verification
`tools/browser/` has Playwright + Chromium (cache at ~/.cache/ms-playwright). To shoot the WASM build:
serve `web/build` (`python3 -m http.server 8131`) then
`PLAYWRIGHT_BROWSERS_PATH=~/.cache/ms-playwright PORT=8131 node tools/browser/shot.js out/x.png 9000`.

## (orig) NEXT ACTION (fidelity-first, keep pipeline working & verified)
Work from `re_out/dd2_decomp.c` (use `tools/refunc.sh NAME`). Reconstruct faithfully, verify via screenshots.
1. **Physics fidelity**: map the 0x1b2 car struct fields by reading `Car_Drive_Motion`(+_3D),
   `Calc_Suspension_*`, `Car_Friction`. Port the real handling (fixed-point ok) into `vehicle.c`,
   replacing the arcade model. Keep deterministic. Verify a car laps LEV5 plausibly.
2. **Track geometry fidelity**: read the strip/poly loader (LEVEL.DAT section 0 = 28 strips;
   `Init_Track_Strip_Numbers`, `Search_For_Strip`, `Draw_Screen_Polys`) and render the REAL polygons
   + textures, replacing the cross-section approximation.
3. **AI fidelity**: port `Track_Follow` + `AI_CommandList*`.
4. Make ALL 16 tracks complete; then WASM (SDL3+WebGL2) build + DVD look; verify every track ×N seeds.

## Build/run cheatsheet
- extract assets: `. .venv/bin/activate && python tools/unpack_dirinfo.py`
- native build: `./build_native.sh` → `build-native/dd2_view`, `build-native/dd2_race`
- view a track: `./build-native/dd2_view assets/raw/LEV5/LEVEL.DAT out/x.png`
- race: `./build-native/dd2_race assets/raw/LEVn/LEVEL.DAT out/dir <laps> <ncars> <seed> <shot_interval_s>`
- decompiled C: `re_out/dd2_decomp.c`; extract fn: `tools/refunc.sh 'Name @'`
