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
- Reference capture (Stage-2 bit-verify): `make refcapture` (private prefix in
  `third_party/wine-reference/`; needs xvfb; polls /proc/PID/mem, then captures at
  Draw_All entry using a GDB hardware breakpoint).

## Current acceptance check (2026-10-02, Debian 13)
- System Emscripten 3.1.69 / LLVM 19 requires `-mllvm -fast-isel=false`: its FastISel
  folded load offsets trap on valid wrapping 32-bit addresses in AI_Com_Server on L10.
  Both build scripts now use SelectionDAG; native and WASM demos return on all 10 levels.
- `tools/verify_demos.py` checks exit status, timeout and error logs. The previous Make
  loops could report success despite failed runs. Passing these checks proves crash-free
  demos only; it does not establish complete original/native/WASM output equivalence.
- Browser track selection -> populated L7 race and IDBFS reload persistence pass with
  Debian Playwright/Chromium. The shell creates the save-file symlink target before boot.
- Original assets and 18 exact CDDA track extracts are provisioned with `make provision`
  and remain gitignored. `mciSendCommandA` now delegates to re_out/dd2_cd.c: the
  original engine controls CD tracks, Play/Stop/resume/repeat, with exact 44100Hz
  stereo s16le source data. Patches 836/837 restore contiguous MCI parameter blocks
  and the mandatory valid-CD boot branch. Browser builds fetch one raw CD track
  at a time and submit it to WebAudio; native/Node expose source PCM capture.
  `make verify-redbook` passes all 18 prefixes, stop/resume positions, full track02,
  end/replay and invalid/missing-disc cases: 29821008 exact bytes against CDDA.
  Browser real-key CD menu Play/Stop/Next/Prev passes; submitted track2/3 buffers
  match source bytes exactly. Logs: /tmp/dd2-browser-redbook-reviewed.log and
  /tmp/dd2-redbook-absolute-time.log. Source equality is not full mixed output parity.
  Interactive menu effects now use elapsed real time, flushing samples before
  buffer controls/queries. The prior mixer produced zero buffers with cf=0;
  real browser navigation now submits nonzero effects while cf remains fixed
  (/tmp/dd2-menu-audio-before.log, /tmp/dd2-menu-audio-after.log). Controlled 55ms
  native/WASM COM tests at fixed cf compare 4848 known PCM bytes exactly, covering
  looping, Stop, one-shot exhaustion and half-frequency playback. Reproduce with
  make verify-menu-audio and tools/browser/qa_menu_audio.js. Full original mixed
  playback and timing are still unverified.
  After this mixer change, all ten native/WASM demos still match frame/palette,
  RNG/flip, effects and CD PCM exactly (/tmp/dd2-parity-menu-audio-clock/results.json).
  The real-key browser CD menu still passes (/tmp/dd2-redbook-menu-clock-regression.log),
  and original L9/cf150 framebuffer/palette remain exact on both ports.
- The Node build now imports `DD2_*` process options into libc ENV before boot. Before
  this fix, Node silently ignored sound, input scripts and frame capture options.
  New `make verify-parity` compares every presented frame AND its palette, flip/RNG
  logs and generated PCM bytes. All ten sound-enabled demos return. L2-L10 match
  exactly across native/WASM (1525-1527 presented frames and 2638944 PCM bytes per
  demo). Patch 835 restores Draw_Other_Objects' contiguous 8-byte angle vector
  (two movsd and a third-WORD replacement at 0x42d028-0x42d0a4). The decompile
  wrote the Z angle to an invented global and passed a standalone DWORD, so the
  rotating Wild Bill mesh read target-dependent adjacent stack bytes. L1 now
  also matches: 1525 frames/palettes and 2638944 PCM bytes. Rebuilt native, Node
  and browser; the other nine complete demos still match. Verification logs:
  /tmp/dd2-parity-L1-fix/results.json and /tmp/dd2-parity-835-regression/results.json.
  After Redbook implementation, all ten demos also match with a shared 25Hz audio
  clock: /tmp/dd2-parity-cd-shared-clock/results.json (4939200 CD PCM bytes per demo,
  same frame/palette/RNG/effects results). Reading the fake GetTickCount polling
  clock for music would advance it over twice as fast; use dd2_audio_ms instead.
  Real-time Node demos pass after replacing the incorrect handcrafted timespec
  with libc's struct timespec (Emscripten time_t is 64-bit).
  Earlier observed differences at 0x874c84/0x874d60/0x874de4 are OT bucket heads,
  not geometry-cache structures; they were downstream consequences.
- User reports menu graphics/actions and championship scores broken. Reproduce by
  navigating the menus and verify action effects and rendering against the original;
  label/alive checks alone are insufficient. Patch 838 fixes the reproduced
  corrupted/empty championship names: Setup_Driver_Names started its computer-name
  table at humans*8 bytes, while original 0x44c5a7-0x44c5b3 uses humans*16 bytes.
  Real Championship -> name "A" -> Go -> Pause/Retire/Yes -> View League on the
  browser now matches all 20 original names/points and all four pages' framebuffer
  and palette bytes exactly. Original capture: /tmp/dd2-original-champ-acknowledged;
  browser: /tmp/dd2-champ-scores-strict-final; logs /tmp/dd2-champ-names-final.log.
  Tools/reference/capture.py --mode menu --acknowledged-key --keys ... drives real X11
  inputs via read-only ReadPad hardware breakpoints and records each held-poll
  count until release. Wine can queue key-up late, so this does not claim one
  poll per key. This avoids long held keys in the fast Wine FE and missed short
  keys during a race. Browser qa_champ_scores
  releases each key after one presented frame and asserts actual Retire activation;
  prior 140ms/down-confirm tests skipped menu items or selected No.
  make verify-champ-names tests 1/2/5/10-human naming on native/WASM. Optional
  CHAMPREF/CHAMPBROWSER checks real score builders with original standings as an
  explicit fixture and the live browser's names/points/pixels/palettes against the
  original. Native live viewer rendering, full seasons/promotion/relegation, other
  menu reports and complete original streams remain open. All ten native/WASM
  demos still match after patch 838: /tmp/dd2-parity-champ-names/results.json.
- Wine 10 and wine32:i386 are installed. The original requires a valid audio CD
  before DirectDraw/FE initialization. `make refcapture` now builds a virtual
  Linux CD adapter from the verified Redbook manifest and serves exact sector
  reads to Wine's own MCI/DirectSound driver. No original EXE or engine memory
  changes. It uses a private prefix under third_party/wine-reference, an isolated
  save copy and scoped cleanup (the old global pkill path is removed). Menu and
  L9/cf150 snapshots at Draw_All entry work. `make verify-cdrom` passes the TOC,
  108 exact LBA/MSF audio reads (including cross-track reads), invalid request and
  unrelated-descriptor checks. `tools/reference/compare_video.py` compares the
  first port Draw_All image with an existing original L9 checkpoint and fails
  on missing captures, process errors or any differing byte. L9/cf150 passes
  on both targets (all 307200 pixel and 1024 palette bytes); logs/capture in
  /tmp/dd2-ref-attract-device. `make verify-reference-video REFCAP=<capture>`
  reproduces it. This is one checkpoint; full original mixed playback PCM
  and video stream parity remain open.
  The adapter Q-channel position is currently fixed at track2 start. Wine mcicda's
  TO-only MCI_PLAY reads that position, even after digital playback; current Wine
  reference pause/restart therefore cannot establish original cursor fidelity.
  Historical claims below are investigation context, not full acceptance evidence.

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
  both targets re-confirmed after both. FE-cycle demos #3-#7 (L5/L2/L3/L4 + the SECOND L9 =
  full rotation wrap) fb-verified vs ref: 0/307200 at cf4+cf150 each (one attach-capture per
  side, lv-gated) — the whole attract rotation 9,6,5,2,3,4,9,... is bit-identical incl. cycle
  wrap (patch-685 "later attract cycles" concern definitively closed). FE-cycle nat<->wasm
  parity: fliplog (cf+rand_calls per flip) identical across the whole boot->FE->L9->FE->L6
  run (5724 flips) and demo#2 cf0-175 flip-phase fb dumps 176/176 bit-identical (node `fe`
  mode + DD2_CFONLY=1 DD2_CFDUMP; kill both runs INSIDE the same demo or cf-keyed files get
  overwritten by the next demo). FE MAIN MENU pixel-exact vs ref: 0/307200 (compare key:
  screen byte @0x460005==201, lv==0, menu age>300 flips [palette fade], slab byte
  @0x4699cc==32 [flip-counter mod 64]); root was patch-480's frame miscount in the
  FE background fill FUN_00411e0c (prim is ARG 1 at [ebp+0x14] after the 4-push
  prologue, not arg 3 — fix 805; the type-0x60 full-screen f0 fill never painted,
  races have no 0x60 prims so only FE edges showed it). ALL deep FE sub-screens now OPEN +
  render + navigate on BOTH targets: View Statistics (821), Sound Volume (822), CD Player
  (823) — reconstructed 1:1 from objdump + registered. The prior sessions' "binaryen/
  emscripten call_indirect table-index-OOB toolchain bug" that blocked these for days is
  DISPROVEN (misdiagnosis): each screen trapped ONLY because its setup/action handlers sat in
  the dispatch tables as UNREGISTERED raw VAs (dd2_relocate never rewrote them to real table
  indices) — once reconstructed+registered they work with zero wasm errors. Isolation proof:
  tools/browser/shifttest.js (shifting the whole wasm table by adding N address-taken
  suspending fns changes nothing). Companion fix: info_screen_directions/_dire_stats int-typed
  byte tables (Info LEFT-nav OOB). An agent-driven test->fix loop then closed 4 more DEEP-feature
  bugs (non-dispatch; found via QA agents + native ASan/interactive repro): 827 Keyboard-config
  renderer crash (string symbol typed int** deref'd as pointer), 828 Name Entry accepted no
  letters (name buffers int** not char** -> chars 4 bytes apart), 829 Keyboard rebind never
  advanced (GetKeyState stubbed 0 + only ~13 keys forwarded), 830-833 CHAMPIONSHIP
  fully working (830 driving crash = Sort_Leagues loop-bound artifact; 831 season-end sibling
  Reset_League_Info; 832 "View League" between-race trap = raw &LAB_00453c00 fn-ptr store; 833
  season-end wild-write = Reset_League_Info uninitialized aiStack_5c -> FUN_0044c9c0 OOB). Full
  5-race season + standings + View League + season end all work. 834 = the retire-ALL-5-races
  edge (relegate out of division 0 -> Do_End_Of_Season_Stuff case 3 `_current_season--` from 0
  to -1 -> OOB level read @&DAT_0046758c[-1] -> crash): the ORIGINAL dd2h.exe has this same
  latent bug (asm `decl 0x93dec0`, NO clamp; only reachable via the abnormal all-retired path).
  Fixed on explicit user request as the ONE SANCTIONED DELIBERATE DEVIATION in the tree (patch
  834 header flags it): clamp `if (0 < _current_season)` so a bottom-division relegation keeps
  season 0. Off the attract, so verify/verify-wasm 10/10 + L9 702/702 nat<->wasm unaffected;
  native repro DD2_SEASONEND (forces division-0 relegation -> season stays 0). [Supersedes the
  prior "leave faithful / do-not-fix" note for this path.]
  Test infra: tools/browser/fepass.js (17/17 label-verified full pass: every FE screen + race
  lifecycle + championship, with page.on('crash') detection) + native DD2_FETEST/DD2_KBTEST/
  DD2_CHAMP/DD2_SEASONEND/DD2_KBREBIND2 repro modes. A 3-agent QA sweep (qa_ttmp/qa_fetree/qa_edge)
  + a 10-pass consecutive-clean loop (tools/browser/passrun.sh + consecutive.sh = fepass + 5min
  adversarial fuzz + rotating deep probe) then closed the last items:
  - KEYBOARD REBIND (real bug, BOTH targets): (a) dd2_symbols.h typed the rebind working-keymap
    sub-bytes DAT_0093fd93/95/98/99/9c/9d as int, but the original does BYTE store/load
    (FUN_0044fd80 `a2 XX fd 93 00`=mov moffs8,AL; FUN_0044f9d4 `a0 XX fd 93 00`; `8a 15` cmp-load)
    -> each 32-bit store zeroed 3 adjacent keymap bytes -> rebind never committed. Retyped to
    unsigned char* (fd90 stays int = &copy-base). (b) BROWSER-only: web/shell_port.html forwarded
    a 13-key allowlist (arrows/Enter/Esc/Space/F1/F2 + KeyA/S/W/Z) -> D/Q/E/etc. never reached
    dd2_key_event so the rebind screen couldn't bind them; now forwards all KeyA-Z + Digit0-9.
    Proof: native DD2_KBREBIND2 (deterministic 5-prompt bind+commit) + browser per-key detection.
  - BROWSER SAVE PERSISTENCE (IDBFS): the interactive build used MEMFS (--preload-file) so saves
    didn't survive a reload (engine card-save path IS faithful + persists on native+node/real disk;
    InitCardSystem recreates a byte-identical 128KB SaveGames when absent). Web-only fix: build_web.sh
    links -lidbfs.js, drops the SaveGames preload; shell_port.html mounts IDBFS at /persist, gates
    boot on FS.syncfs(true), symlinks /SaveGames->/persist/SaveGames, flushes on tab-hide/unload.
    Proof qa_persist_idbfs.js: marker survives reload in file AND engine reads it back @0x754460.
  - TEST-VALIDITY: the shared launch check `screen@0x460005 != 201` false-passes on transient
    sb=41 loading frames -> felib.waitRace() now polls for sb==89 with engine frame @0x462ff0
    advancing (a dialog stall = frozen cf FAILS). Coverage closed: Time-Trial (race_type=1) +
    2-Player (race_type=3, Select_Multi via Enter_Driver_Names EMPTY-commit exit) both browser-
    launch+drive clean; patch 834 retire-all confirmed (native DD2_SEASONEND + browser retire loop).
  Remaining polish (non-blocking): physical Xbox pad on real hardware (SW chain validated via
  synthetic pad).

## Conventions
- Faithful reconstruction — no approximations/band-aids. Commit progress; verify BOTH targets after every change.
- `dd2_image.bin` (memory snapshot, loaded at 0x400000) must be present in `DestructionDerby2/` at run time.
- Wine gotchas: NEVER `pkill -f dd2h.exe` (kills own shell) → use `pkill -x`; win32 prefix = repo-local `.wine-dd2/`.
- Full pre-2026-07-01 investigation history is in git history + `/tmp/CLAUDE_full_backup.md`.
