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
- **Stage 2 — bit-identical: VIDEO for the L9 attract = DONE (fb).** vs the Windows REFERENCE
  (cold-boot L9 attract, PROVEN 100% bit-deterministic across ref boots): **framebuffer 100.000%
  bit-identical (0/307200 px) at all 29 sampled cfs across cf4–cf695** (4,20,40,50,56,60,70,85,
  100,150,200,210,225,235,240,245,248,280,290,310,330,360,400,450,500,550,600,650,695), and
  **native↔wasm bit-identical for ALL 701 cf-frames** (DD2_FRAMEDIR+DD2_CFDUMP on both targets).
  Audio sample staging 0x7fa164-0x816ff0 100% byte-identical (mixer output = remaining audio
  work). The route from 33.6%: ~20 asm-verified roots; each patch header / dd2_symbols.h comment
  has the details. Recurring root classes to check FIRST on any new divergence: int-typed
  byte/word symbols (store/load width vs asm: `66`-prefix, `xor eax,eax; mov al`, `sar $0x10`
  hi-half loads — e.g. sun-y word DAT_0046526e, flare byte tables DAT_00465255/56 w/ 3-byte
  records, dent table DAT_00466a20, corner INT array DAT_00466a96, shade bytes DAT_00466340/
  DAT_00463896, _car_info word, heap rover _DAT_00774690 must be uint*); scattered locals read
  as structs by callees (TextureDentHi dent-uv blocks, patch 655; Init_LensFlare info blocks);
  GTE reg-args/18-byte matrix uploads/rot-matrix WORD stores (5501ccb); signed `sar` semantics
  in span blitters and GTE const hi-half loads (665); deterministic uninitialized stack reads
  (Setup_Sprite +9 ABR, patch 685 — boot-race values only, later attract cycles differ, open);
  wrong-local args (Calc_Object_Angles local_a0 vs local_80, patches 660/690). Falsified
  (measured, do not re-chase): heap +0x60 shift, timing/frame_skip/rand, draw_face_4pt "hoisted
  opz". Whitelist (legit target-dependent): card region 0x774460–0x7748e0 ONLY, sound host-ptr
  table 0x796f60–0x796fa0, particle ctrl fn-ptrs (+0x64, pool 0x78d740 stride 0xad), __lmptr
  @0x74c6d8, PE header 0x400000–0x400400 — re-validate buckets periodically (too-broad card
  bucket hid the debris divergence for days). Divergence-hunt tooling: DD2_IMGDUMP=<cfs>,
  DD2_FLIPLOG, DD2_CFDUMP+DD2_FRAMEDIR (cf-keyed fb dumps, nat↔wasm), DD2_BYTEWATCH,
  DD2_NOPATCH=1 tracer builds, DD2_HIST/DD2_PIXWIN/DD2_DBGPRIM (dd2h_stubs.c); OT-walk-diff of
  two full dumps (root [0x7542ee+buf*0x8e], otsize @0x754260, links tag&0xffffff, term 0xffffff)
  finds diverging prims fast. Ref captures: wine+gdb 3-stage arming (hbreak 0x4431e8 → 0x420c9c
  cf-gate → target fn); sys.setrecursionlimit MANDATORY in every gdb-python on_stop script (ours
  AND wine — dies after ~50 hits otherwise); refmulti.sh rotates old reffb dumps — save first;
  fb-vs-ref compares only at the same Draw_All-entry phase; prim compares only for prims that
  WIN pixels. **AUDIO = DONE for the L9 attract:** DirectSound COM shim (DD2_SOUND=1) + deterministic
  PCM mixdown (dd2h_stubs.c; engine-frame clock, fixed-point centi-dB, Q16 point-sampler;
  DD2_SNDPCM/<path>, DD2_SNDLOG): PCM stream bit-identical across native runs AND native<->wasm;
  game-side DS call stream matches the SOUND-ENABLED wine reference EXACTLY (59/59 race sound-start
  groups incl. every volume/pan/frequency value; Modify_Sound streams exact in every clean
  measurement window — heavier gdb captures distort wine's wallclock channel timing/frame-skip,
  only ~10cf windows with few breakpoints are valid). Audio roots: patches 700-735 (boot order,
  2x scattered DSBUFFERDESC, dropped reg-args, volume-curve log10 reconstruction + CRT log10 body,
  mm-timer callback 0x41345c reconstruction + deterministic 10-cf/phase-5 driver, commentator gate
  = BYTE test of demo counter [0x7746c0] opening every 128 cf). Sound-enabled refs need the ALSA
  null device (~/.asoundrc `pcm.!default { type null }`) — headless wine otherwise has NO audio
  device and DirectSoundCreate fails (all pre-2026-07-03 refs ran no-sound). Remaining Stage-2
  work: verify later attract cycles (patch-685 remnants).
- **Stage 3 — playable: after Stage 2.** Menus/track-select/race on keyboard+pad. Keyboard input wired
  (`re_out/dd2_input.c`); browser renders.

## Conventions
- Faithful reconstruction — no approximations/band-aids. Commit progress; verify BOTH targets after every change.
- `dd2_image.bin` (memory snapshot, loaded at 0x400000) must be present in `DestructionDerby2/` at run time.
- Wine gotchas: NEVER `pkill -f dd2h.exe` (kills own shell) → use `pkill -x`; win32 prefix = repo-local `.wine-dd2/`.
- Full pre-2026-07-01 investigation history is in git history + `/tmp/CLAUDE_full_backup.md`.
