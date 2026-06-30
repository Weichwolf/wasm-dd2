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
- Demo **7/10** crash-free (no-ASan); L2/L3/L6 crash. L3 ROOT-CAUSED (tools: `eipcatch.c` ptrace
  SIGSEGV→EIP catcher + `DD2_PLOG_PATCH`/`DD2_PLOG` BSS ring buffer of poly commands, both env-gated):
  faulting insn = walk-loop `while(*(char*)(_gpoly+3))` at dd2.c:6600 with `_gpoly=0x666666ff`. The ring
  buffer proved the poly stream up to the crash is VALID (small counts) — so it is NOT a per-handler
  stride bug. The crashing object's `FUN_0041fb7c` loads `_gpoly = *(iVar3+0x28)` already = `0x666666ff`
  and crashes on the FIRST while-check (before any command records). => a SCENE OBJECT reaches the draw
  path with a corrupt data pointer. Ties to the object LEAK/over-walk: in L3 `_free_mem` drops
  monotonically (objects created, never freed via `Remove_Object`; obj+0x19 lifetime countdown gated by
  obj+0x1a/obj+6 doesn't fire) → `num_scene_objects`/array over-walk → garbage object. TRUE heisenbug
  (shifts under any in-loop probe). FULLY TRACED via `tools/wpcatch.c` (ptrace HW watchpoint = debug
  registers, NO memory perturbation → doesn't move the heisenbug): the crashing object is VALID +
  in-bounds (block6/idx3, num=22, geom ptr p1[1]=0x8020c8 valid heap); a HW watchpoint on 0x8020c8+0x28
  caught the WRITER = **`Decompress`** (build/dd2.c:3543, the LZ geometry decompressor) writing decompressed
  `0x66`('f') bytes into that buffer. So the on-demand geometry streamer (`Decompress(&dec_info)` from the
  scene-object paths re_out:16660/16677/16801/16842/16992) DECOMPRESSES INTO A BUFFER A LIVE SCENE OBJECT
  STILL REFERENCES → corrupt +0x28 → wild `_gpoly` → crash. FIX = the streaming buffer lifecycle (dec_info
  dest must not collide with a referenced object; or the object must be removed before its buffer is reused).
  Tools (all env-gated/standalone): `eipcatch.c` (fault EIP), `wpcatch.c` (HW watchpoint), `DD2_PLOG`/`DD2_OBJLOG`.
- Renders the 3D demo scene AND the front-end **title screen** ("DESTRUCTION DERBY 2" logo) — both verified
  by capturing `0x700450`. The menu uses the SAME 3D engine as the race but is static/deterministic + far
  fewer objects (no desync) → the cleanest bit-exact target. Title compare still low (palette/align), WIP.
- **Reference capture works** (`tools/refcap.c` ptrace-reads `0x700450`; `tools/eipcatch.c` catches faults).
  Our build dumps the same surface (`DD2_FRAMEDIR=…`, `DD2_CFDUMP=1` keys on `current_frame`@0x462ff0).
  Bit-compare blocked by alignment: our `DemoModeLevel` skips the intro the reference shows; counters differ.

## Conventions
- **Never edit decompiled code; fixes are transpile patches (see Pipeline).** Compat layer is editable.
- Commit/push only when asked. Faithful reconstruction from the binary — no approximations/band-aids.
