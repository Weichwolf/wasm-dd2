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
  device and DirectSoundCreate fails (all pre-2026-07-03 refs ran no-sound). FAITHFUL BOOT PATH
  CONFIRMED: the real FE-driven attract (Front_End idle -> View_Frontend_Replay, a fixed recorded
  replay — NOT the DemoModeLevel harness) is bit-identical to the Windows reference (cf200 AND
  cf450 = 0/307200) and to the DemoModeLevel harness (cf200 = 0/307200); since it is a
  deterministic fixed replay, every repeat is identical by construction. The old patch-685
  "later attract cycles differ" concern was a HARNESS artifact (DemoMode's rand()%10+1 level pick
  leaves different Setup_Sprite stack remnants across cycles) — the faithful FE attract has no
  such variance. Stage-2 video = complete for the L9 attract on the real boot path.
- **Stage 3 — playable: ALL FOUR categories DONE on both targets.** Demo (bit-identical),
  Race, Menus, Track-select each bridge to a playable populated race on native AND browser
  (interactive build `make web` -> web/dd2). Live race (DD2_PLAY=9 native / DD2_LIVE=1 wasm):
  keyboard AND gamepad drive the car; scripted runs (DD2_SCRIPT/DD2_PADSCRIPT, cf-keyed — flip
  counts are NOT target-invariant, the engine counter is) are BIT-IDENTICAL native<->wasm (full
  live races, cmp-exact audio streams). Controls matrix (measured, cf600-differential +
  Pause_Mode-break + fb-diff): arrows=steer, A=accel, Z=brake/reverse, ESC/ENTER=pause
  (exact-cf), F1/F2=camera view, W/S/SPACE/UP/DOWN=no race effect; pad axes=analog steer,
  button1=accel, button2=brake. Browser: realtime 25fps via DD2_REALTIME (GetTickCount=real ms;
  engine's own frame limiter), WebAudio sink (mixer ticks scheduled on a time cursor), Gamepad
  API polling -> dd2_pad_update (synthetic-pad E2E validated; pad must be connected at load =
  faithful Windows detect-at-boot). FE MENUS: full main menu renders + navigable; measured E2E
  (headless Chromium, tools/browser/trackselect.js): Select Track -> F2 cycles track (S.C.A.
  Motorplex) -> Go! -> Enter launches a live level-7 race with num_cars=20, opponents populated
  and AI driving. Roots fixed on the FE path: Print x/y/z phantom-locals (755), Set_World_Matrix
  int-stride = the black-menu root (760), LINE-prim wasm signature (765), 9 unexported menu-screen
  /action handlers (770/775), nav-table + track_lookup pointer-scaling (780), value-cycle
  call_indirect signature (785 — F1/F2 wasm-only), FirstSavedGame dropped return = phantom-save
  loaded num_cars=0 = browser-only 0-opponent race (790). Pre-race: pad-state block int-typing
  (perma-pause), JOYCAPS scattered locals (745), _pad_option semantics (750). Attract-cycle
  demo #2 (L6, FE cycle) is bit-identical vs ref from cf4 on (was 253013/307200 px at cf4):
  root = patch-365 fragment call sites passing sentinel (0xe,-1) instead of the live free-slot
  index/block number — Init_Scene_Objects collected ZERO scenery blocks on every L1-7 entry
  (fix 800; companion CLUT-anim scattered-locals fix 795). L9 cf245/450/650 + verify 10/10
  both targets re-confirmed after both. FE-cycle demos #3-#5 (L5/L2/L3) fb-verified vs ref:
  0/307200 at cf4+cf150 each (one attach-capture per side, lv-gated). FE-cycle nat<->wasm
  parity: fliplog (cf+rand_calls per flip) identical across the whole boot->FE->L9->FE->L6
  run (5724 flips) and demo#2 cf0-175 flip-phase fb dumps 176/176 bit-identical (node `fe`
  mode + DD2_CFONLY=1 DD2_CFDUMP; kill both runs INSIDE the same demo or cf-keyed files get
  overwritten by the next demo). Remaining polish
  (non-blocking): physical Xbox pad on real hardware (SW chain validated via synthetic pad).

## Conventions
- Faithful reconstruction — no approximations/band-aids. Commit progress; verify BOTH targets after every change.
- `dd2_image.bin` (memory snapshot, loaded at 0x400000) must be present in `DestructionDerby2/` at run time.
- Wine gotchas: NEVER `pkill -f dd2h.exe` (kills own shell) → use `pkill -x`; win32 prefix = repo-local `.wine-dd2/`.
- Full pre-2026-07-01 investigation history is in git history + `/tmp/CLAUDE_full_backup.md`.
