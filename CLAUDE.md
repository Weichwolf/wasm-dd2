# Destruction Derby 2 → WASM

Port `dd2h.exe` (1996 PC game) to a reproducible C build (native-primary, WASM as a build target).
**Code is the truth.** Verify every claim against the binary (`DestructionDerby2/dd2h.exe`) or a run — do not trust prose.

## Goal / acceptance
The deterministic attract-demo must be **crash-free on all 10 levels** and **bit-identical** to reference
`dd2h.exe` (compare the 8-bit indexed framebuffer `@0x700450`, 320x240) plus audio sample-for-sample.
Then: fully playable (menus, track-select, race; keyboard + pad). Engine C is mechanically derived from
Ghidra; only the platform/runtime shim is hand-written (DirectDraw→g_pixels/WebGL, DirectSound→WebAudio, Win32/CRT).

## Pipeline (RULE: never edit decompiled code)
`dd2h.exe → decompile → patch → compile → run` — and it must run identically, in Wine too.
- **NEVER hand-edit the decompiled engine source (`re_out/dd2.c`, `re_out/dd2_*.c`).** It must stay the
  pristine Ghidra output so re-decompiling reproduces it. Re-running `decompile.sh` would wipe hand edits.
- **All engine fixes go in `tools/transpile.py` as anchor-asserted `sub()` patches** (`re_out/*.c → build/*.c`).
  Each patch asserts its anchor still exists, so decompile drift is caught loudly. `--check` verifies anchors.
- The ONLY hand-written code is the platform/runtime compat layer (`re_out/dd2_com.c` DirectDraw shim,
  `dd2_win32.c`, `dd2_filio.c`, `tools/native_main.c`) — editing those is fine.
- `tools/decompile.sh` → `re_out/dd2_decomp.c` (+ `re_out/dd2.c`, pristine). Ghidra can't decompile the
  register-convention GTE fns (drops reg args as `unaff_E**` / bare `GTERPT()`); `tools/recover_regargs.py`
  recovers those from the binary disasm (feed into transpile patches, don't hand-write).
- NOTE: `re_out/dd2.c` currently still holds legacy hand-edits (pre-rule). Those should be migrated into
  transpile patches over time so `re_out/` is fully regenerable.

## Build & run (native debug)
- `ASAN=' ' bash tools/build_native.sh /tmp/dd2_native_na`  — **use no-ASan for real crashes**: ASan
  false-positives on the engine's hand-computed global-pointer arithmetic (faults at low addrs like 0x20).
- Demo: from `DestructionDerby2/`, `DD2_LEVEL=N /tmp/dd2_native_na` (prints "demo returned (no crash!)").
- Front-end: `DD2_FE=1 /tmp/dd2_native_na`.
- Reference (works here — no install needed): `WINEPREFIX=… xvfb-run -a wine dd2h.exe`.
  Extract its 8-bit framebuffer for bit-compare: ptrace-read `0x700450` (320x240) from the live
  process (ptrace_scope=1 → the reader must launch wine as its own descendant; see /tmp/refcap.c).
  Our build dumps the same surface via the `ids_flip` hook (`DD2_FRAMEDIR=…`). Compare byte-for-byte.

## Current state (verify, don't trust)
- Demo **7/10** crash-free (no-ASan); L2/L3/L6 crash. L3 ROOT-CAUSED (via `tools/eipcatch.c`, a ptrace
  SIGSEGV→EIP catcher, since no gdb): faulting insn is the walk-loop `while(*(char*)(_gpoly+3))` with
  `_gpoly=0x666666ff` — `0x66`=`'f'`, so **`_gpoly` walked off the poly stream into ASCII string data**.
  = the `_gpoly` command-stream **desync**: a draw handler in `FUN_0041fb7c`'s dispatch advances `_gpoly`
  by the wrong stride for some command type (FIX(L15) already fixed `FUN_0041a2f4`'s stride). Layout-
  sensitive (probes shift it → heisenbug). FIX = audit each handler's `_gpoly` advance/branch vs x86.
- Renders the 3D demo scene AND the front-end **title screen** ("DESTRUCTION DERBY 2" logo) — both verified
  by capturing `0x700450`. The menu uses the SAME 3D engine as the race but is static/deterministic + far
  fewer objects (no desync) → the cleanest bit-exact target. Title compare still low (palette/align), WIP.
- **Reference capture works** (`tools/refcap.c` ptrace-reads `0x700450`; `tools/eipcatch.c` catches faults).
  Our build dumps the same surface (`DD2_FRAMEDIR=…`, `DD2_CFDUMP=1` keys on `current_frame`@0x462ff0).
  Bit-compare blocked by alignment: our `DemoModeLevel` skips the intro the reference shows; counters differ.

## Conventions
- **Never edit decompiled code; fixes are transpile patches (see Pipeline).** Compat layer is editable.
- Commit/push only when asked. Faithful reconstruction from the binary — no approximations/band-aids.
