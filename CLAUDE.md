# Destruction Derby 2 → WASM

## Goal
A fully playable 1:1 rebuild of the original **`dd2h.exe`** (Destruction Derby 2, 640x480) with
**bit-identical audio and video output**: given the same input and RNG state, the rebuild produces the
same framebuffer every frame and the same audio stream as the original on Windows — menus, track
select, race, and demo mode fully usable via keyboard/pad, native and in the browser (WASM). The route
there is a reproducible chain that always starts with `make` (`make decompile → make patch → make
native / make wasm`), so the whole pipeline binary → decompile → patches → build is implicitly
documented by the Make targets; every correction to the decompile is a commented, traceable patch.
No guards, no approximations, no band-aids — any observable deviation from the original is by
definition a bug. **Code is the truth** — verify every claim against `DestructionDerby2/dd2h.exe` or a run.

## Pipeline — everything starts with `make` (the target chain IS the documentation; see `make help`)
`make decompile → assemble → patch → native/wasm → verify/verify-wasm` (+ `symbols`, `image`, `web`,
`shot`, `refcapture`). `make pipeline` runs the full from-binary chain.
- **Never hand-edit decompiled engine source** (`re_out/dd2_decomp.c` = pristine Ghidra output;
  `re_out/dd2.c` = mechanical assembly of it). Every engine correction is an ordered, exact-context
  **`patches/NNN-slug.diff`** with its rationale in the file header (that's where detailed notes belong,
  NOT here). `make patch` applies the series with -F0 --fuzz=0 — decompile drift fails loudly.
- Only hand-written code = the platform/runtime shim: `re_out/dd2_com.c` (DirectDraw→g_pixels), `dd2_win32.c`,
  `dd2_filio.c`, `dd2_stubs.c`, `dd2_input.c`, `dd2_runtime.c`, `tools/native_main.c`. Editing those is fine.
- `make decompile` runs Ghidra from `third_party/` (gitignored: ghidra_12.1.2_PUBLIC, jdk-21.0.11+10,
  proj `third_party/dd2_ghidra_proj`, PROGRAM=**dd2h.exe**); the project-DB setup scripts live in `tools/ghidra/`. Register-arg call sites are
  recovered by `tools/recover_regargs.py` (emits patch-stage material).

## Commands
- Native debug: `make native NATIVE=/tmp/dd2h_na`, run from `DestructionDerby2/` as
  `env -u DD2_NOSEGV DD2_LEVEL=N /tmp/dd2h_na`; symbolize with `addr2line -f -e`.
- Reference capture (Stage-2 bit-verify): `make refcapture` (WINEPREFIX defaults to the repo-local
  gitignored win32 prefix `.wine-dd2/`; needs xvfb; reads state via /proc/PID/mem, NOT gdb breakpoints).

## Status (3 sequential stages, each gates the next)
- Source: pure dd2h.exe decompile (`re_out/dd2.c` = dd2h Ghidra output, 640x480 rasterizers).
  Memory image: `re_out/extract_image.py DestructionDerby2/dd2h.exe` → `dd2_image.bin` (0x580400 bytes).
- **Stage 1 — crash-free: DONE.** `make verify` and `make verify-wasm` = **10/10 on both targets**,
  no guards — every fix is an asm-verified patch. Recurring Ghidra artifact classes (each patch header
  documents its instance): pointer scaling (int-typed byte tables indexed with byte offsets, e.g.
  DAT_00792bae), scattered locals (stack blocks split into separate C locals that gcc reorders —
  vertex arrays for the edge sorter, Setup_Sprite info blocks, camera vectors), dropped registers,
  paired 16-bit loads, unsigned/signed shifts+compares (dth_* span blitters need `sar` semantics),
  mid-function entries, wasm call_indirect signature normalization.
- **Stage 2 — bit-identical: ACTIVE.** Native↔wasm bit-identical (L2 cf182 fb 100%; L9 cf3 full image
  identical modulo relocated fn-ptr tables). vs the REFERENCE (cold-boot L9 attract, PROVEN 100%
  bit-deterministic across ref boots; frozen full image /tmp/ref_full_cf3.bin): **fb 33.6% → 90.4%**
  via eight asm-verified roots: num_cars/screen_w/h/far_z_clip signed (idiv/jg vs unsigned C types),
  _v_norm word-store, car_lookup byte table, FE-boot in both harnesses (LEV0 textures fill pages
  20-25 → texturespace/CLUT/palette/car-CLUT-tables 100% byte-identical; patches 585/600),
  Init_LensFlare contiguous info blocks (595), GTE 18-byte matrix uploads (605 — __globmat short
  truncation left odd matrix elements stale), subdiv dispatch signature (610). Camera EXACT
  (0x3060,-0x103,-0xfa9), frustum normals EXACT, dth span inputs match, sky cull byte-identical.
  Both launchers boot through the REAL
  Init_Main @0x445814 (InitCardSystem/SaveGames, DIRINFO original-case + case-insensitive fopen
  shim, Read_CD_Toc_, Load_Game_Vags) → **audio sample staging 0x7fa164-0x816ff0 is 100%
  byte-identical** to the reference (mixer output = the remaining Stage-3 audio work). Total
  non-whitelisted full-image state diff: ~4.3KB (card region re-capture noise, debris 2-byte
  fields, 0x7959a8 array, invisible sky-dome prim phase). Remaining fb diff (~9.6%): speaker-tower
  face (LED block, x250-390 y95-235), ground trapezoid below camera, ~6 horizon bush billboards.
  Sky band SOLVED (poly_clipx/y signed, ba6dfbd): dth sky spans start ABOVE the screen (y1<0) and
  the unsigned compare no-op'd whole segments incl. their edge-state advance. Verification loop: DD2_HIST=<cf> prim histogram + FT4 dump (with prim addr) vs
  tools/refhist.sh; ref captures via 3-stage gdb arming (hbreak 0x4431e8 → 0x420c9c cf-gate →
  target fn). Pitfall proven twice: prim-level compares must be restricted to prims that WIN
  pixels (hidden sky-dome quads drift legitimately).
- **Stage 3 — playable: after Stage 2.** Menus/track-select/race on keyboard+pad. Keyboard input wired
  (`re_out/dd2_input.c`); browser renders.

## Conventions
- Faithful reconstruction — no approximations/band-aids. Commit progress; verify BOTH targets after every change.
- `dd2_image.bin` (memory snapshot, loaded at 0x400000) must be present in `DestructionDerby2/` at run time.
- Wine gotchas: NEVER `pkill -f dd2h.exe` (kills own shell) → use `pkill -x`; win32 prefix = repo-local `.wine-dd2/`.
- Full pre-2026-07-01 investigation history is in git history + `/tmp/CLAUDE_full_backup.md`.
