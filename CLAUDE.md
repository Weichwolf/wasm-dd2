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
- **Stage 2 — bit-identical: ACTIVE, re-base VALIDATED.** ROOT CAUSE: built from the WRONG binary — **dd2.exe
  (320x240)** not **dd2h.exe (640x480)**. Framebuffer `@0x700450`: 640x480 8bpp = 0x4b000 vs 320x240 = 0x12c00 →
  diff **0x38400** = the .bss shift making every DATA addr ≥0x713050 land 0x38400 too low → decompress-window
  corruption (black-blob cars) + crashes. FIX (works, env `DD2_DD2H`): transpile post-pass shifts every literal
  in [0x713050,0x940000) by +0x38400 (data shift measured clean; code shifts non-uniformly but C calls by name)
  + `tools/make_dd2h_image.py` inserts a 0x38400 gap in `dd2_image.bin`. RESULT vs dd2h ref (refcapture.sh,
  L9/frame151): **level_data_buffer 8.6%→95.7%, car_vertices 97.3%, rgb_lookup 100%, full 50.6%→75.4%.** Working
  dd2.exe build still 10/10 (env-gated). REMAINING: dd2h build 3/10 crash-free (guards tuned for old layout +
  residual divergence); MPE heap BASE still not consumed (0x816ff0 = InitHeap self-ptr vs ref real data) —
  the original allocation-order residual (MPE allocs are IDENTICAL in dd2h: prim 0x60000, heap 0x120000).
  The dd2h build's crashes are now the RESOLUTION mismatch: dd2.exe code has 320x240 rasterizers (pitch 0x140)
  that write wild addrs on dd2h's 640x480 data (verified: crash in FUN_0041080d/Draw_All; dr_modes[0] is
  640x480 in dd2h vs 320x240 in dd2.exe). Patching pitch constants is fragile (640x480 is different CODE, not a
  const swap). CLEAN PATH (also removes most transpile guards — they were band-aids for the wrong-binary
  corruption, unneeded with correct dd2h code): swap `re_out/dd2.c` to the **dd2h DECOMPILE** itself (correct
  640x480 rasterizers + right code; makes the DD2_DD2H +0x38400 shift unnecessary since dd2h code already uses
  dd2h addrs). Remaining for that: re-recover the GTE register-arg overlay for dd2h (tools/recover_regargs.py),
  migrate the few legit hand-fixes, delete the now-obsolete guards, rebuild + bit-verify vs dd2h. Image:
  `re_out/extract_image.py DestructionDerby2/dd2h.exe` gives the correct dd2h memory image (640x480 dr_modes).
- **Stage 3 — playable: after Stage 2.** Menus/track-select/race on keyboard+pad. Input path proven
  (steering responds); browser renders. Blocked on Stage-2 correctness (corrupt geometry).

## Conventions
- Faithful reconstruction — no approximations/band-aids. Commit progress; verify BOTH targets after every change.
- `dd2_image.bin` (memory snapshot, loaded at 0x400000) must be present in `DestructionDerby2/` at run time.
- Wine gotchas: NEVER `pkill -f dd2h.exe` (kills own shell) → use `pkill -x`; win32 prefix must be in an owned dir.
- Full pre-2026-07-01 investigation history is in git history + `/tmp/CLAUDE_full_backup.md`.
