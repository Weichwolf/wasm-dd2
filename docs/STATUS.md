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
- [ ] **M1 asset decoders** — PAL/CLT palette, TX textures (+dims), SPR sprites, LEVEL.DAT
      geometry, FONT.BNK, VAG audio. Each → PNG/WAV + structural assertions.
- [ ] **M2 renderer** — load a track, GLES3 free-fly camera, offscreen screenshot test.
- [ ] **M3 vehicle + physics + driving**.
- [ ] **M4 self-play AI + headless race sim** (also the main test harness).
- [ ] **M5 DVD-look present, audio, HUD, menus, race modes; verify ALL 16 tracks ×N**.

## Dirinfo archive format (cracked — details in REVERSING.md)
TOC of 114 records at file start. Record = `NAME\0` + field.
Common 9-byte field = `[flag u8][reserved u16=0][sector u16][size u32]`, **offset = sector*2048**.
Special header records: COPYRIGH.BMP `[u16 sec][u32 size]`; LOADING.BMP `[u8 flag][u16 sec][u32 size]`;
FONT.BNK `"RAW\0"[u16 sec][u32 size]`. Run `python tools/unpack_dirinfo.py` to extract.

## NEXT ACTION
Start M1: decode `LEVEL.PAL` (256×RGBA?) and a `LEVEL.TX*` texture, emit PNG proofs;
determine texture dimensions (inspect TX headers). Then geometry (`LEVEL.DAT`).
