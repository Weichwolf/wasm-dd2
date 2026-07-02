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
- Source: pure dd2h.exe decompile (`re_out/dd2.c` = dd2h Ghidra output, 640x480 rasterizers).
  Memory image: `re_out/extract_image.py DestructionDerby2/dd2h.exe` → `dd2_image.bin` (0x580400 bytes).
- **Stage 1 — crash-free: ACTIVE, currently 0/10.** Build:
  `DD2H_BESTEFFORT=1 ASAN=' ' bash tools/build_native.sh /tmp/dd2h_na`, run from `DestructionDerby2/` as
  `env -u DD2_NOSEGV DD2_LEVEL=N /tmp/dd2h_na`; symbolize crashes with `addr2line -f -e`. (Gdb HW
  watchpoints do NOT fire in the mmap'd image region — kernel/fread writes bypass them.) Dominant crash
  class: Ghidra pointer-scaling (int*/short* globals indexed with byte offsets). Crash map: **L1-7**
  `Generate_Surface_Normals` @0x4268b8 (unrecovered switch jumptable, calls raw code labels); **L8-10**
  `MulMatrix2` (GTE `extraout_ECX` dropped output register). Big open items: in_EAX/register-arg recovery
  (67 fns), full GTE per-call register wiring via `tools/recover_regargs.py` (static `_g_*` defaults run
  but give WRONG vectors), the jumptable reconstruction, render loop. Always verify ALL 10 levels, both
  targets — crashes are level-data-dependent.
- **Stage 2 — bit-identical: after Stage 1.** Bit-verify vs dd2h reference (`refcapture.sh`, e.g.
  L9/frame151 memcmp of level_data_buffer/car_vertices/rgb_lookup). Known residual: MPE heap BASE not
  consumed (0x816ff0 = InitHeap self-ptr vs ref real data) — allocation-order divergence (MPE allocs:
  prim 0x60000, heap 0x120000).
- **Stage 3 — playable: after Stage 2.** Menus/track-select/race on keyboard+pad. Keyboard input wired
  (`re_out/dd2_input.c`); browser renders.

## Conventions
- Faithful reconstruction — no approximations/band-aids. Commit progress; verify BOTH targets after every change.
- `dd2_image.bin` (memory snapshot, loaded at 0x400000) must be present in `DestructionDerby2/` at run time.
- Wine gotchas: NEVER `pkill -f dd2h.exe` (kills own shell) → use `pkill -x`; win32 prefix must be in an owned dir.
- Full pre-2026-07-01 investigation history is in git history + `/tmp/CLAUDE_full_backup.md`.
