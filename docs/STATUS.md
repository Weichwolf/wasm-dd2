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

## CURRENT STATE (2026-06-18, session 1)
- Circuits LEV1,4,5,6 complete 6/6, deterministic (MATCH). LEV2,3,7 (long tracks) partial:
  cars wedge at sharp spots — approximate AI+cross-section centerline ceiling.
- Arenas LEV8–B = 32-rib ~125 m bowls (demolition; 2-lap racing ill-fits → handle separately).
- Non-tracks: LEV0 (menu), LEVF (1 rib), LEVC/D/E (no LEVEL.DAT) → detect & skip gracefully.
- Sim infra solid: seam-free laps (integrated progress), windowed localization, spur+Laplacian
  smoothing, reverse-realign recovery, determinism verified.

## NEXT ACTION (fidelity-first, keep pipeline working & verified)
**Priority: replace the approximate centerline with the game's REAL racing line** (fixes LEV2/3/7
navigation faithfully). LEVEL.DAT section 1 = 86% valid vertex indices — likely the track path/strips.
Cross-ref `Track_Follow`, `Init_Track_Follow_Data`, `Init_Track_Strip_Numbers`, `Search_For_Strip`
in `re_out/dd2_decomp.c`. Then: arenas → time/demolition completion; guard non-tracks; WASM
(SDL3+WebGL2)+DVD look; verify all tracks ×N seeds with screenshots.
Older fidelity items (port real physics/geometry) remain after navigation is robust.

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
