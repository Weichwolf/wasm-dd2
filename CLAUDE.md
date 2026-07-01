# Destruction Derby 2 → WASM

Port **`dd2h.exe`** (1996 game, 640x480 hi-res build) to a reproducible C build (native-primary, WASM
target). **Code is the truth** — verify every claim against `DestructionDerby2/dd2h.exe` or a run.

## Pipeline (fixed): `dd2h.exe → decompile → transpile/patch → compile (native/wasm)`
- **Never hand-edit decompiled engine source** (`re_out/dd2.c`, `re_out/dd2_*.c` = pristine Ghidra output).
  All engine fixes go in **`tools/transpile.py`** as anchor-asserted `sub()` patches (`re_out/*.c → build/*.c`);
  each patch documents its own rationale inline (that's where detailed notes belong, NOT here).
- Only hand-written code = the platform/runtime shim: `re_out/dd2_com.c` (DirectDraw→g_pixels), `dd2_win32.c`,
  `dd2_filio.c`, `dd2_stubs.c`, `dd2_input.c`, `dd2_runtime.c`, `tools/native_main.c`. Editing those is fine.
- `tools/decompile.sh` runs Ghidra (`/home/cosmo/tools/ghidra_12.1.2_PUBLIC`, proj `dd2_ghidra_proj`,
  PROGRAM=**dd2h.exe**) → `re_out/dd2_decomp.c` + `functions.txt`. GTE/jumptable register-arg fns are recovered
  by `tools/recover_regargs.py` / hand overlay.

## Commands
- `make pipeline` = decompile → check anchors → native build+verify → wasm build+verify (10 demo levels, both targets).
- `make verify` (native) / `make verify-wasm` (node) — crash-free N/10. `python3 tools/transpile.py --check` = verify anchors.
- Native debug: `ASAN=' ' bash tools/build_native.sh /tmp/dd2_native_na` (no-ASan for real crashes); run from
  `DestructionDerby2/` as `DD2_LEVEL=N /tmp/dd2_native_na`. WASM: `bash tools/build.sh` → `/tmp/lvltest/dd2run.js`.
- Browser build: `bash tools/build_web.sh web/dd2`; serve + open, or `node tools/browser/shot.js web/dd2 out.png`.
- Reference capture: `WINEPREFIX=<owned-dir>/wp32 tools/refcapture.sh` (needs `xvfb-run -s "-screen 0 640x480x16"`
  + WINEARCH=win32; reads state via /proc/PID/mem, NOT gdb breakpoints).

## Status (3 sequential stages, each gates the next)
- **Stage 1 — crash-free: DONE.** Both targets 10/10 on all demo levels via a family of `transpile.py` guards
  (GEOM-GUARD, GUARD AE/AG/AH/AK/AM/AO, DRAWPRIM-GUARD, FIX BK/PADTYPE…) that convert corruption symptoms into
  safe skips. Interactive (demo_mode=0, `DD2_LIVE=1`) renders the live race; keyboard input wired
  (`re_out/dd2_input.c`, FIX KEYMAP/PADTYPE).
- **Stage 2 — bit-identical: ACTIVE, re-base underway.** ROOT CAUSE FOUND: the reconstruction was built from the
  WRONG binary — **dd2.exe (320x240)** instead of **dd2h.exe (640x480)**. Framebuffer `@0x700450`: 640x480 8bpp =
  0x4b000 vs 320x240 = 0x12c00 → diff **0x38400** = exactly the .bss/layout shift that made every geometry buffer
  ≥0x713050 land 0x38400 too low → decompress-window corruption (black-blob cars) + the residual crashes.
  **Fix = re-base on dd2h.exe** (dd2.exe deleted; decompile.sh → dd2h.exe; dd2h analyzed in Ghidra, 838 fns / 518
  named at correct shifted addrs). REMAINING: regen `dd2_symbols.h` for dd2h BSS, swap `re_out/dd2.c` to dd2h,
  redo GTE overlay, re-anchor transpile patches, regen `dd2_image.bin` (dd2h snapshot), rebuild, then bit-verify
  vs dd2h via `refcapture.sh` (expect +0x38400 to vanish, cars to render). dd2h decomp staged in
  `re_out/dd2h_staging/` (gitignored).
- **Stage 3 — playable: after Stage 2.** Menus/track-select/race on keyboard+pad. Input path proven
  (steering responds); browser renders. Blocked on Stage-2 correctness (corrupt geometry).

## Conventions
- Faithful reconstruction — no approximations/band-aids. Commit progress; verify BOTH targets after every change.
- `dd2_image.bin` (memory snapshot, loaded at 0x400000) must be present in `DestructionDerby2/` at run time.
- Wine gotchas: NEVER `pkill -f dd2h.exe` (kills own shell) → use `pkill -x`; win32 prefix must be in an owned dir.
- Full pre-2026-07-01 session history (Stage-2 investigation, per-guard postmortems) is in git history
  (commit before this rewrite) and `/tmp/CLAUDE_full_backup.md` if needed.
