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
  `/tmp/wasm-dd2/wine-reference/`; needs xvfb; polls /proc/PID/mem, then captures at
  Draw_All entry using a GDB hardware breakpoint).
- Original accepted/consumed PCM: `python3 tools/reference/capture.py --mode audio --audio
  --audio-tail 3 --output /tmp/wasm-dd2/fresh-original-audio` (no debugger). Observer/clock
  calibration: `make verify-audio-observer`; needs ALSA headers and 32/64-bit runtimes.

## Current acceptance check (2026-10-03, Debian 13)
- Patch 851 restores Check_League_Standing's two WORD loads (league @0x93def2,
  rank @0x93def4; literal 66-prefix instructions @0x44c92c..0x44c96f). DWORD
  league included rank=4, misclassifying bottom-division elimination as relegation;
  patch 834 then masked this by restarting season 0. The actual original retires
  all five races and eliminates the player without underflow/crash. The original
  bug attribution in the historical patch-834 diagnosis is disproved and annotated;
  its explicitly sanctioned inconsistent-state clamp remains.
  Actual original-x86/Native/WASM component runs match all 210 classifications,
  including nonzero adjacent points and WORD boundaries; old DWORD variants fail
  (league-standing-words-851-verified/report.json). Native real keyboard-bridge
  navigation now matches all five race standings, 100 names/points, 20 full league
  images/palettes and final elimination. The live browser also completes all five
  Retire/Yes races and eliminates correctly, but its strict original comparison
  remains FAILED at races 4/5 (different computer points; first 12 league images
  match). Keep champ-season-standing-fixed-851.json as a failed full gate; do not
  turn native/component proof into full browser parity. A focused input observer
  records browser frame_skip=6/8 versus 1 in reference/native snapshots; attempting
  to schedule at original counters can skip the input point and a matching counter
  still has different physics ticks. Full input/clock/RNG scheduling needs diagnosis.
  All ten rebuilt native/Node demos remain byte-exact (15,255 frames, all palettes,
  RNG/Flip logs and generated PCM; parity-league-standing-fixed-851.json).
- Track Select road/bowl navigation now matches the original over all seven/four
  previews at default unlock limits 4/1: 29 checkpoints x 64 presentations per
  case, 3,712 full framebuffer/palette pairs per target, actual browser canvas
  readback, same initial SaveGames. Track/mode/type/poly list/lock/limits/saved
  track validate alongside images; wraps, locked confirmations, cancel/reopen
  and F1/F2 shortcuts are included. Seven damaged capture cases reject per
  target. `make verify-track-selection` captures and compares both cases;
  reports track-{road,bowl}-verified-851.json retain hashes/states, raw images
  removed. No engine correction in this change. Pairing uses observed highlight
  phases: transitions, live input/audio timing, earned unlocks and racing remain
  open. Real original capture confirms its stale lock flag after cancel: opening
  the restored unlocked track then Enter stays in Select_Track until a track
  change recomputes the flag. Both ports reproduce this original defect.
- Patch 850 removes 15 added face-handler address filters which rejected the
  actual frontend OT at 0x935ff0. Car Select lost 44 quads despite all 171
  transformed vertices being byte-identical. Targeted pose {640,1920,592}
  captures now show all 98 original car primitives linked literally in native.
  Native/browser match 1,024 complete framebuffer/palette pairs with the
  original over Rookie, Amateur, Pro and confirmed/reopened Pro. Pairing uses
  independently observed periodic model angles, not image fitting or the menu
  colour counter; this proves renderer output for those poses, not full
  chronological video/audio or input timing. Three damaged pixel/palette/pose
  cases reject on each target. See all-car-preview-fixed-850.json and
  car-preview-primitive-diagnosis-850.json under /tmp/wasm-dd2/.
  All ten native/Node demos still match (15,255 frames, palettes, RNG/Flip logs,
  effects/CD/mixed/music bytes; parity-car-ot-fixed-850.json). Captures use the
  same supplied initial SaveGames file; successful raw comparisons are removed.
- Patch 849 restores the original packed loading-bar primitive widths: command
  bytes at 0x93de77/0x93de9b and sixteen X/Y words. The former DWORD command
  store cleared X0=64, painting 815 incorrect pixels in each startup frame
  10..19 on both ports. Two independently recorded original first-128-Flip
  sequences are identical, and fixed native/browser captures match every
  indexed byte and palette byte, with all browser canvas pixels checked.
  Real Escape input skips the actual movie; no engine memory/register writes
  or frame alignment. Six damaged capture cases reject per target.
  `make verify-startup-video` checks loading, slab transitions and a complete
  settled highlight cycle; reports are startup-video-loading-{before,fixed,
  fixed-fresh}.json under /tmp/wasm-dd2/. Completed raw captures were removed.
  This is frontend video acceptance; intro, racing, live timing and complete
  audio/video synchronization remain unproved.
  Ten-level native/Node regression passes all 15,255 frames/palettes, RNG/Flip
  logs and effects/CD/mixed/music PCM (parity-loading-progress-fixed/results.json).
  Native/browser replay of the retained original menu device clock also keeps
  all source anchors and the previously verified 1,192,376-byte PCM hashes
  (startup-clock-loading-regression.json; original raw PCM already deleted).
- Original menu mixer starts are now derived from actual primary block counts
  and the independently consumed ALSA probe; the checker no longer searches
  any PCM pattern for alignment. Literal event-derived positions match all
  three earlier reports (FX/CD: 14199/17346, 20776/27827, 15651/19920).
  Two new unmodified original runs pass Native/WASM O0/O2 at those independently
  observed device positions. original-menu-mix-event-timeline-final/report.json
  compares all 1199184 consumed bytes and the complete accepted FIFO prefix/
  queued tail; every original source cursor and creation order validates.
  Three one-sample source shifts, five damaged mixer timelines and all five
  damaged full PCM cases reject. Trace records retain every primary block,
  source cursor, played sample-time start and both software/device queue sizes.
  Successful raw comparisons are removed after the report. This is full
  bounded source/mixer output at recorded original device positions, not a
  live port game run with identical original input/API scheduling or A/V timing.
  Original movie/racing audio, full live scheduling and game acceptance remain open.
- Original capture readiness now reads actual Main Menu poly_list @0x940010
  and restart_cd_audio @0x467420 after startup, instead of comparing a cached
  CLUT pointer byte with 201. Literal EXE stores/call order are confirmed at
  0x450fec, 0x4502f6 and 0x450347. An unmodified live run observes three old
  false positives during slab startup (restart1/CD stopped), then the new
  ready state with CD13 playing. Start/end and all 148 ready observations are
  recorded under original-menu-mix-real-phase-first/readiness-report.json.
  Its complete 1277280 menu-device PCM bytes match Native, WASM O0 and O2,
  with all five damaged full-stream cases rejected. This validates capture
  phase and complete aligned sample values, not live original/port clocks.
  Successful auto-recorded original raw PCM now also cleans up after its
  comparison report; failed/supplied captures remain for the caller's diagnosis.
  A second fresh run (original-menu-mix-real-phase-final/report.json) also passes
  all three mixers and five damaged-stream checks: 1199480 exact bytes per target.
  It starts/ends in the actual menu with CD13 playing and removes all its original
  raw PCM after the report. Completed earlier slab/CD diagnostics' raws removed.
- Complete bounded original menu-device PCM now matches independent Native,
  WASM O0 and browser-mixer O2 source renders: all 1157048 bytes in
  original-menu-mix-verified-final/report.json, including initial silence,
  resampled slab effect and CD13 overlap/tail. The actual Wine API trace identifies
  BANK1 index42, mono8/11025 source, frequency5512, volume-1, pan0 and one-shot
  play, then the original looping CD13 buffer. Original controls are validated,
  not fitted from samples. Native FIR/gain source bytes and literal CDDA form
  the expected Float32 mix; offsets are inferred from unique source patterns.
  Five damaged full-stream cases reject through the actual recomposition check.
  A fresh three-second run reaches the attract race and correctly rejects the
  two-source menu check (original-menu-mix-verified-fresh). Its evidence exposed
  the then-old cached-CLUT readiness proxy; the phase correction is recorded above.
  make verify-original-menu-audio ORIGINAL_MENU_AUDIO_ARGS='--output /tmp/wasm-dd2/fresh-menu-mix'
  captures/checks a new run; --capture can use an existing traced audio capture.
  This proves the recorded window's sample values after inferred alignment,
  not identical-input live original/port timing, intro, races or hardware output.
- Clocked original-audio capture now records device-consumed PCM separately
  from accepted writes, preserving sample-time intervals and pause/XRUN gaps.
  Real ALSA tests pass both process widths, Float32/S16 and all three supported
  device rates: 12 cases, 2416664 exact bytes against independent known patterns
  and frozen ALSA delay counters, covering replacement/rewind, partial drop,
  pause/resume, drain, ring wrap and underrun. All 18 damaged playback recordings
  reject, as do active prepare resets whose queued extent ALSA makes unobservable.
  make verify-audio-observer AUDIO_OBSERVER_ARGS='--output /tmp/wasm-dd2/fresh-audio-observer'
  reproduces this check; audio-played-verified-final/report.json records the run.
  Successful raw test PCM removed after report. Two unmodified dd2h.exe runs
  produce valid consumed journals; the original-played-audio-cd-state
  capture initially exposed the startup mixed prefix. Later menu PCM includes
  exact track13 source spans; the complete slab/CD diagnosis is recorded above.
  Neither recorder checks nor those source spans establish
  complete original/native/WASM mixed audio, timing or hardware equivalence.
- Browser mixer now uses -O2 -fno-strict-aliasing, matching the native shim's
  compiler settings; reconstructed engine units remain -O0 and no fast-math
  is enabled. The unoptimized browser had slow camera-intro runs that failed
  15s/60s countdown limits while CD loading finished in 0.12s. A separately
  profiled unoptimized run also passed, so these failures are not a deterministic
  semantic negative. Controlled Node mixer runs have identical complete PCM
  hashes at O0/O2 and median times 0.204s/0.157s including process startup
  (mixer-optimization-benchmark/report.json); this is a synthetic cost check,
  not complete original/live browser timing acceptance.
  Browser-setting FIR tests pass all 24 full waveforms and six short loops at
  22050/44100/48000 device rates against captured Wine hashes/cycles, plus
  1084900 software-wide arithmetic results against actual x87 CPU operations
  (sound-resample-wasm-o2/report.json). Raw successful comparisons removed.
- Browser optional full-card QA now passes all 15 actual menu saves, clipped
  last-slot navigation, cancelled/confirmed last-slot overwrite, full-card
  restart, natural slot0 replay playback/return and cancelled/confirmed deletion
  retaining the other 14 entries across another restart. All replay payloads
  and RAM/file bytes match; other slots stay unchanged when overwriting slot14.
  browser-card-full-mixer-o2/report.json: pass, no runtime errors, 140 observed
  playback phases; full/cancelled SHA256
  6b32a72986db6ab6415b950094c001a7da420770ed2e3e16b405f0476fd4814a,
  overwritten/reloaded b7658b15b760e51b55b8c5c3c2357d898037bced0208e5e2d346bc001ba9a2ab,
  deleted/reloaded 336fb3a99d96373776d6a16d0763f4c1523ef718bf2e1094c674dcba3a1edc57.
  Menu audio delivers all 1512 checked buffers exactly from C to WebAudio with
  no missing/mismatched buffer (browser-menu-audio-mixer-o2.log). CD menu
  Play/Stop/Next/Prev passes 643860 exact track2/3 source bytes and 4311 exact
  shared mixer buffers with no additional sink during the menu exercise
  (browser-redbook-mixer-o2-reviewed/report.json). The CD verifier now accounts
  for the separate intro sink before the menu and removes successful captures
  after writing its report under /tmp/wasm-dd2. Candidate and rebuilt canonical
  browser WASM SHA256 match:
  609596b66b373daee18dd5ec0de2f9f4e828b3fc4e777c81ccab9fceafac7ad9.
  HTML remains 9356a7e03df6a0b9c00bf4d156635e18e31d8e957cd888dd202c75bc7a49f22a.
  These are functional and component checks; full original menu/player A/V
  equality, original real-time audio and complete game acceptance remain open.
- Native optional full-card replay QA now passes with real X11 inputs: all 15
  entries carry the exact complete replay payload, last-slot navigation clips at
  14, cancelled overwrite preserves the complete card, confirmed overwrite
  renames O to P without altering the other 14 entries or replay bytes. The
  full card survives process restart, slot0 B loads/plays naturally and restores
  frontend configuration; cancelled deletion preserves all entries, confirmed
  deletion leaves the other 14 unchanged across another process restart.
  native-card-full-state-waits/report.json: pass, 22 observed playback phases;
  full/cancelled SHA256 fd8918fe891ca34eeb0b15dd2bfa9355051201f18ddc618eedae6d014164de09,
  overwritten/reloaded d109fc1602643aee4c142f3cd1923b91772585eef20ac6ccf1f13693b1517e6a,
  deleted/reloaded 8659708be40cabd1062b6a6150a811a6ebdf93ea1449f87ce724c496676d88ab.
  Countdown and Practice Over readiness are actual state predicates, not a
  fixed post-key delay. Browser full-card acceptance is recorded above.
  No full original card-menu/player A/V parity claim.
- Real native and browser replay QA also passes cancelled/confirmed overwrite.
  Cancelling preserves the whole card; confirming renames entry A to B in the
  same slot with the complete replay payload unchanged, then preserves it after
  restart and loads/plays it naturally. Native observes 32 playback counters,
  browser 33; both restore frontend configuration and retain cancelled/confirmed
  deletion behavior (native-replay-overwrite-reviewed/report.json;
  browser-replay-overwrite-first/report.json). Native first/cancelled card SHA256
  829dfbfd2a449e01b767fe0d0b6a00e9a4c7791e71d4242a3f392e16d435b1dd;
  overwritten/reloaded 784fdd7fef4987662a7d05137a68c72ac24fd7b69bfcfc95c7e3b2e81e194c77.
  Browser first/cancelled 01bb36eb9d0b83b51813a547c618253cbc266de9b3fd586611cb54c5b7f42975;
  overwritten/reloaded 85cf4dfe4a54c377e772bf03b80466ae163238cc99fa6c90956d6506f7a25949.
  These are independently timed functional runs, not cross-target/original
  trajectory or video/audio acceptance. Native key acknowledgement handles
  Retire/Yes restoring the previous pad before presentation: the actual level
  transition acknowledges that action even when its old control bits disappear.
  No engine state or input is substituted; keys remain actual X11 events.
- Card component conformance now passes on both ports against actual unmodified
  x86 SaveCardFile/DeleteFileMC/LoadCardFiles/LoadCardFile/DupFileCheck/
  FirstSavedGame/FUN_0042366c. All 26 cases compare 148040 bytes each: complete
  128-KiB card, every directory row, source/loaded 8-KiB blocks, neighbors and
  outcome. Empty/full/sparse cards, first free slots0/7/14, reserved flags,
  absent/dormant names, duplicate exemptions and config selection are covered.
  Literal original execution stops at read-only hardware breakpoints before
  external seek (arguments checked) or at the fixture return marker; no code/
  register writes. Full cards return zero unchanged on all three implementations.
  Changed guard/directory/first+last payload/outcome bytes and truncated input
  reject. make verify-card; card-original-reviewed/report.json; original aggregate
  SHA256 e85635e562795268739341c36a279f6dbe6a8cf5768cc720c3b9badf0600e902.
  This is component acceptance, not original full card-menu or file I/O parity.
- Native replay window QA now passes through actual X11 keyboard input and
  three normal intro/frontend startups: Select Car/practice/Retire/Yes/Save
  Replay/name A/process restart/File Manager/load/natural playback, then
  cancelled/confirmed deletion and another process restart. Every byte of
  the 128-KiB card equals engine RAM and survives restart; complete saved and
  loaded script/order bytes match. Playback advances through 60 observed
  presentation counters, applies acceleration, ends naturally and restores
  the prior car/mode/type/car count. No engine/file writes by the tester; only
  the game's real save path changes the isolated card. Successful private
  game/card removed; report native-replay-make/report.json. make
  verify-native-replay reproduces this check, using Native SHA256
  23ac6a9bfe9cb26284aece1405cc7dbbf326cb2860f5f117ac41734e3c1d25ac.
  This establishes this native functional flow, not original complete player
  video/audio equivalence. Overwrite/full-card cases still need acceptance.
- Browser card persistence now observes actual writes to the session's open
  SaveGames FILE, batches them at the next event turn and serializes IndexedDB
  snapshots. Writes during an active sync queue another snapshot. The former
  unload-only shell lost completed saves when page navigation destroyed the
  asynchronous operation; SDK autoPersist-on-close is insufficient because the
  original keeps this FILE open. No engine save behavior or file bytes change.
  make verify-browser-replay-save exercises real Select Car/practice/Retire/Yes/
  Save Replay/name "A"/normal navigation/File Manager/load/natural completion.
  Saved/reloaded/cancelled-delete cards match across all 131072 bytes by SHA256
  857e8b07df0def4f6add2e3fe8a4485c7d9ec5cf7ab2aa66b1161842861f0ba7;
  confirmed-delete/reloaded cards match SHA256
  3ba7da877411fbde5c0b12561fb3d0e14008e118d6213d04778994bf0246e115.
  Every card file byte matches engine RAM; complete script/order and stored
  nonzero car1 match loaded playback. It advances through 34 observed rendered
  phases, finishes naturally and restores the prior frontend car/mode/type/
  car count. No test state/file writes or forced syncfs. Runtime errors: none
  (browser-replay-save-delete-persist/report.json). The identical final test
  rejects the saved-card loss in an immutable old-shell build with the same
  patch-848 engine (browser-card-persist-full-old-negative/report.json).
  Current browser HTML SHA256
  9356a7e03df6a0b9c00bf4d156635e18e31d8e957cd888dd202c75bc7a49f22a;
  WASM remains 9a7453c5f1c1be7e18c06c4f5231e7b3375876fb18edb0761ae03b679143f6cd.
  This establishes this browser save/load/delete flow, not full original menu/
  player A/V acceptance. Native window save/load and overwrite/full-card cases
  still need checks. Earlier marker-based persistence QA is narrower evidence.
- Patch 848 restores replay metadata WORD stores, signed WORD loads and the
  unsigned WORD file discriminator. Original packing's final magic store must
  preserve the selected car; Load_Card_File must ignore the adjacent car WORD.
  All 16 actual unmodified x86 runs (eight save/eight load cases) match Native
  and WASM at the LoadSave/View_Frontend_Replay boundaries: 14462 bytes each,
  including the complete packed payload/script/order, neighbor guards and
  engine setup. Negative signed WORDs and discarded upper halves match.
  Original executions use read-only hardware breakpoints; neither code nor
  registers are changed. Native/WASM also restore frontend configuration after
  their fixture player returns. Removing only patch 848 overwrites car7 with
  zero at first packed offset2 and rejects widened load fields; changed guard
  bytes/truncated checkpoints are rejected. make verify-replay-metadata;
  replay-metadata-848-reviewed/report.json; original aggregate SHA256
  5ae73bc7ebdc71d347aa15289f80663cd59f2d6c6d65e554f2ce5669ba746986.
  This is component acceptance, not original full-menu save/load or A/V parity.
  The pre-persistence-fix browser Select Car/practice/Retire/Yes/Save Replay/name entry test
  saves car1, every script/order byte and matching card RAM/disk successfully,
  then rejects because the named replay disappears on ordinary navigation
  (browser-replay-save-848-first/report.json). The old marker persistence test
  forced syncfs itself and did not cover this real save/navigation failure.
  The PE loader is shared with the existing replay fixture, which retains all
  384 matching original/native/WASM checkpoints and old-cursor negatives
  (replay-components-848/report.json). All 185 patches apply with zero fuzz.
  All ten default Native/WASM demo regressions still pass with patch 848:
  15255 frames/palettes and every effects/CD/mixed/music PCM and RNG/flip byte
  (parity-848/results.json). Verified Native SHA256
  23ac6a9bfe9cb26284aece1405cc7dbbf326cb2860f5f117ac41734e3c1d25ac;
  Node WASM 48063723cf805c781adc1381e65fd6f3fd113e1656f63dce07e7d5eeafca9086;
  browser WASM 9a7453c5f1c1be7e18c06c4f5231e7b3375876fb18edb0761ae03b679143f6cd.
  These are internal regressions, not complete original audio/video acceptance.
- Patch 847 restores WORD replay packets, two-byte cursor steps and the WORD
  termination store. All 384 component checkpoints (15896 bytes each) match
  literal unmodified original x86 Record_Event/Terminate_Replay/
  Terminate_Replay_Bodge/Control_Car_Replay on Native and WASM. Reversing only
  this patch rejects both targets at checkpoint 2, offset 2 (359 differences);
  changed first bytes and incomplete checkpoints are rejected too. Original
  checkpoint SHA256: 2f6dde8c6f18d8fe541a114478e2a1624e1930eab363fa7b81a634edca78feae.
  Reproduce with make verify-replay; report replay-847-reviewed/report.json.
  Browser practice recording/Retire/Yes/View Replay naturally finishes without
  runtime errors (27 live and 23 playback observations); seven common physics
  ticks have identical positions for all 20 cars. This is a limited diagnostic,
  not complete original replay video/audio parity. The old patch-846 browser
  is rejected for zero-length gap packets (browser-replay-old-cursor-negative).
  Shared browser race detection now uses advancing physical ticks and actual
  level/quit state: byte 0x460005 is a cached CLUT pointer, not a screen ID.
  Championship selection/name entry/race/Retire/Yes/View League still passes
  all four populated score divisions (browser-champ-847.log/scores.json).
  Replay save/load remains unverified; original packing at 0x44ab98 contains
  WORD metadata stores that are still widened in the reconstructed symbols.
- Patch 847 retains complete original L10 video acceptance: all 502 target
  frames/palettes, 116131 global clocks and 39538 global RNG calls, with all
  comparison negatives passing (l10-original-video-847/report.json). All ten
  default Native/WASM demos also pass: 15255 frames/palettes and every RNG/flip
  and effects/CD/mixed/music PCM byte (parity-847/results.json). These internal
  audio regressions do not establish complete original racing audio parity.
  All 184 patches apply with zero fuzz. Verified binaries: Native
  bcfb4f18e23f7f1dfe2ab9030e45ba262d36b0df56d053696642c8aa7e9dafa6,
  Node WASM 06fe1e399e3b878a70804f5daab1a7ddacb283e97be4f88732e381eb86ef7b09,
  browser 1a0fc17a1b5b2dafdba2d7a32c508fc3e0f1e4a11fa797719282e9f98291e986.
- Fresh complete original L10 full-history capture now PASSES BOTH patch-846
  ports: all 502 racing pictures/palettes, 116131 actual global clock returns
  (106831 in preceding demos [9,7,8,3,1,3]) and all 39538 calculated global
  RNG calls. Every observed entry field, prefix sequence and pixel/RNG/blink
  negative passes (/tmp/wasm-dd2/l10-original-video-846/report.json).
  Native separately matches all 7709 preceding/target car checkpoints and
  every RNG caller/phase, with changed car-byte/caller negatives rejected
  (/tmp/wasm-dd2/native-l10-physics-846/report.json). No copied engine state.
  This proves this entire captured racing render loop, not original racing
  audio, loading/fades or physical output clocks. The earlier 1800s dense L10
  attempt timed out after 40 preceding demos and 202 target frames; it was
  never accepted and its raw output was deleted before this successful run.
- Patch 846 restores the eight original BYTE stores in Get_Corner_Positions
  (0x43ca98..0x43cabb). The last wrongly widened store at 0x79263e overwrote
  grounded_count[0] at 0x792640, preventing its 100-step recovery. The old
  native first differs intrinsically in a preceding L10 at cf138/t278, car0
  mode5 versus original mode0, before the RNG caller mismatch at cf151/t302.
  Native/WASM store fixtures preserve every neighbor and the recovery counter;
  reversing only patch 846 makes both fixtures reject with grounded_count=0.
  Reproduce with make verify-corner-lanes (Native ASan/UBSan + WASM).
- Fresh sealed original L8 full-history capture now PASSES BOTH corrected ports:
  all 625 racing pictures/palettes, 191480 actual global clocks and 48418 RNG
  calls; all ten preceding demos [9,7,3,7,10,1,3,6,4,1] and observed entry
  states are calculated naturally. Pixel/history/RNG/blink mutation negatives
  pass (/tmp/wasm-dd2/l8-original-video-846/report.json). Native separately
  matches all 12710 car checkpoints and all 5280 Car_Movement entry/return
  checkpoints (cf145..210), plus every measured RNG caller/phase; changed car
  byte/caller negatives pass (native-l8-car-movement-846-complete/report.json).
  Original/car states are never copied into the engine. Hardware entry/return
  observations share one slot; observer Python/hardware insertion failures
  reject the run. This establishes this captured racing render loop and native
  car-state diagnosis, not full original racing audio/physical-clock parity.
- With patch 846 all ten default demos also pass Native/WASM: 15255 frames,
  palettes, all RNG/flip logs and effects/CD/mixed/music PCM bytes; successful
  raw captures deleted (/tmp/wasm-dd2/parity-846/results.json). All 183 patches
  apply with zero fuzz. Current binaries: Native
  ae3b4d7c8e522203e59d23fc4d7080ab2db56ea9629a9376403afb4e872fca5b,
  Node WASM a833f5d262f1cbc9db2be86cedb6e803658cc1e23c17566123937f3a74883202,
  rebuilt browser a8ba88a3b08898e04267ffd6a6008829e2861387c4833d59977031f8ea6f0002.
  Browser real Championship/name entry/race/Retire/Yes/View League passes all
  four populated divisions and returns to results without runtime errors
  (/tmp/wasm-dd2/browser-champ-846.log). Successful raw captures removed.
  Older L8/L10 rejection notes below describe pre-846 runs, not current acceptance.
- The full-history car observer also records every preceding demo presentation
  and localizes the first literal byte, car index, field offset and phase across
  the entire history. Truncated checkpoint/caller traces are rejected. A fresh
  original L8 capture under /tmp/wasm-dd2/original-l8-history-physics records
  actual prefix [9,7], 3190 total car checkpoints, 1038 target frames, 54725
  clocks and 21774 global RNG calls. Native matches the first 2399 complete
  checkpoints (61894200 literal car bytes), then differs at L8 cf162/ticks326:
  body-vertex Y 0x78a522 and physical position 0x78a744, render FD 0x792690,
  handling yaw velocity 0x792a04 and wheel FD 0x794be8. Previous cf158/ticks318
  is exact. The first RNG caller mismatch is later at global index16932:
  Original Sparks cf200/t401 versus Native FUN_00425444 cf201/t403. Both video
  comparisons reject with 897 target frames and 873 failure records each;
  this pre-846 run had no L8 acceptance. Target-only observations could
  not exclude inherited prefix errors; this full-history trace now excludes
  that explanation for the measured initial L8 physical divergence.
- User-requested storage cleanup: all 668 GiB of ignored historical verification
  artifacts were deleted. Earlier artifact names below describe past runs; those
  raw captures/reports are no longer present and must be recaptured for new checks.
  Keep new captures/logs under /tmp/wasm-dd2, never in the repository. The required budget
  assumes a 16 GiB /tmp tmpfs. Main racing capture/comparison subprocesses stop above
  2 GiB output or below 1 GiB free, without accepting truncated data. Successful
  race comparisons delete raw port dumps after writing reports. Old logs over
  one hour are cleaned before/after main commands; open logs are preserved.
  make clean-logs performs the same cleanup explicitly. Original assets and
  build dependencies remain provisioned and ignored. See AGENTS.md.
- --race-physics now records all 20 cars at every target presentation under
  full original API history: primitive/dynamics 0x78a520 (20*0x27c), render FD
  0x792690 (20*44), handling 0x792a00 (20*0x1b2), wheels 0x794be8 (20*0xb0).
  Global RNG caller addresses are read from original ESP at 0x456cc6; actual
  Watcom rand has no outstanding pushes there (verified 0x456cbc..0x456cde).
  tools/reference/trace_native_physics.py observes Native using read-only
  hardware breakpoints after starti; no engine memory/register writes.
  Fresh L9 control matches all 401 checkpoints/10345800 bytes and all 6948
  caller functions/level/cf/tick phases from real boot seed 1, with changed
  first car byte/caller-name negatives rejected and accepted native artifacts
  restored (dd2-original-l9-physics-callers-reviewed;
  dd2-native-l9-physics-callers-controls-reviewed/report.json).
  Separate full video comparison also passes BOTH ports: all 401 pictures/
  palettes, 5131 clocks, 6948 RNG records and every phase/prefix/pixel negative
  (dd2-original-l9-physics-video-reviewed). This native physics diagnosis is
  not full original physical/audio-clock or WASM physics acceptance.
  The first full original L10 capture is complete: 378 target frames,
  476890 clocks (472790 in 33 prior demos), 130536 global RNG calls, natural
  target blink42/entry RNG count118657. Both ports REJECT: they first diverge
  in the preceding L8 and select L10 after 18 prior demos instead of 33,
  with 1339 target frames and unconsumed clock/RNG inputs (763 rejection
  records each; dd2-original-l10-full-history-inputs-reviewed;
  dd2-original-l10-full-history-comparison-reviewed). This does not isolate
  an intrinsic L10 bug: its target initial state/history already differs.
- The fresh FULL-input original L8 capture remains rejected on BOTH ports,
  not just in direct initial-state matching: 1028 original frames, 58168
  clocks, actual prefix [9,7], all observed initial entry states calculated.
  Both ports stop after the same 925 presentations with RNG exhaustion and
  unconsumed clocks. First presented RNG-phase mismatch is index 264,
  cf200/ticks402 (original 16952 calls, both ports 16943); the first wrong
  framebuffer is index 690, cf424/ticks850. These are genuine unresolved
  engine differences under full prefix API inputs
  (dd2-original-l8-full-history-inputs-reviewed;
  dd2-original-l8-full-history-both-rejection-reviewed/report.json).
  compare_race_stream.py now preserves partial evidence and compares BOTH
  targets after exit/deadline/clock/RNG/loop failures. Missing port diagnostic
  images become rejection records instead of masking the second target.
  Actual L8 fails identically with 894 records per target; a native /usr/bin/
  false control still allows real WASM to pass all 1005 L9 frames and all
  input/state/pixel negatives (dd2-original-l9-native-failure-wasm-positive-reviewed).
  A 1ms deadline independently rejects and records both targets, with no
  complete loop or accepted input extent claimed
  (dd2-original-l9-both-timeout-negative-reviewed). A fresh normal L9 rerun
  also passes both targets with the changed comparator
  (dd2-original-l9-engine-failure-collection-positive-reviewed).
- All ten default demo regressions also pass with the full-history runtime
  instrumentation: 15255 complete native/WASM frames/palettes and all
  effects/mixed/music/CD/RNG/flip bytes
  (dd2-parity-history-api-reviewed/results.json). Canonical /tmp/dd2_native
  and /tmp/lvltest/dd2run.* have been atomically promoted to the verified
  full-history builds listed below. Native normal original-intro/menu/CD/
  boot-time SDL controller/live race/pause/resume/CD sector restart also
  passes 3072000 actual renderer pixels and 7480768 accepted mixed bytes
  (dd2-native-history-api-startup-reviewed/report.json). This verifies native
  sink submissions, not complete original racing/audio-clock equivalence.
- A fresh original L7 loop now passes BOTH ports with all actual prior API
  inputs: 1115 complete frames/palettes, 39063 clocks (20269 in original L9),
  all 11733 global RNG calls calculated from seed 1, observed prefix [9],
  naturally calculated target blink=77 and target entry RNG phase=7491.
  All 16 scenery image checkpoints and pixel/RNG/blink/prefix/entry mutation
  negatives pass (dd2-original-l7-full-history-inputs-reviewed;
  dd2-original-l7-full-history-comparison-reviewed).
  This proves this full-input L7 capture; the six rejected images in the old
  direct-state L7 capture remain a separate unresolved history diagnostic.
  The rebuilt browser also passes actual Championship/name entry/live race/
  Retire/Yes/all four populated score divisions/return to results
  (dd2-browser-champ-history-api-reviewed).
  Full-history instrumentation builds: native
  ba31d796011f7629c1d3f4c01fb51a0d31639959ccbd20ac644edbca60af34a3;
  Node WASM 68fa2ce46a031f2e63800a475ef68cba2aa36ef1c43529e72e944612fe11d94b;
  browser WASM 82593f6f86392b39ceb1791ad902df8fe15d1cddc4c942228e20942465bdbbbb.
- Full preceding API inputs are now supported by --race-full-history plus
  --attract-history: tools/race_history_gdb.py uses at most four read-only
  hardware breakpoints to observe global Watcom rand from frontend seed 1,
  all preceding racing clocks and the selected racing video. Runtime "all"
  RNG mode always REQUIRES the calculated initial seed; it never initializes
  from a reference. An optional output-only level filter avoids dumping prior
  videos and stops only after a target racing frame has actually been logged.
  The fresh original L6 capture passes native AND WASM: 1403 frames/palettes,
  154599 clocks (135578 in six preceding demos), all 25198 global RNG calls,
  actual preceding sequence [9,7,3,1,1,7], all observed prefix entry fields,
  and first-pixel/RNG/blink/prefix/entry mutation negatives
  (dd2-original-l6-full-history-inputs-reviewed;
  dd2-original-l6-full-history-comparison-v1-reviewed).
  This early capture omitted target pre-physics RNG entry metadata; the
  comparator checks its observed first_state and clock, not an inferred RNG
  count. Every global RNG record and presented RNG phase is still exact.
  Subsequent captures also record target_entry with its measured RNG count.
  The fresh L9 full-input capture also passes both ports: 1005 target frames,
  18475 clocks, empty preceding prefix, measured target entry and all global
  seed-1 RNG calls (dd2-original-l9-full-history-inputs-reviewed;
  dd2-original-l9-full-history-comparison-reviewed).
  Only target video is checked; prior video, physical clocks and original
  racing audio remain outside this proof. The old 1371-frame L6 natural-
  history capture also still passes both targets with the new shims
  (dd2-original-l6-history-api-legacy-images-reviewed).
  Strict random transport fixtures pass all 28 native-ASan/UBSan/WASM cases,
  including all-level seed-1 records across levels 0/9/7/6 and wrong-initial,
  truncated/partial/leftover/multiplier negatives
  (dd2-random-full-history-transport-reviewed). These four boot records were
  also independently observed in the fresh original global trace.
  Strict clock wrap/repeat/extent fixtures pass all eight cases on both
  targets (dd2-clock-full-history-transport-reviewed); all 182 exact patches
  still apply (dd2-check-history-api.log).
- The first L3 stream's 25 wrong pixels are fully explained by inherited
  debris vertices, with an unchanged control on the actual patch-845 native
  binary. Original Update_Debris 0x42469a..0x4246a9 masks animation to 7 bits
  and skips the vertex copy when it becomes zero. Init_Debris/Setup_Debris
  retain the slot's old vertices. Slot 30 first spawns with animation zero
  at cf234/ticks469; the direct cold-start harness has zero vertices while
  the original retains [(49,22,-17),(-11,110,-7),(-77,-48,-28)].
  tools/reference/diagnose_debris_history.py runs both native controls:
  unchanged = exactly one wrong frame, index 566, 25 pixels; copying ONLY
  the inactive slot's 24 vertex bytes before the first GetTickCount = all
  1243 frames/palettes, 13181 clocks, 1948 calculated RNG records and all
  diagnostic scenery checkpoints exact
  (dd2-l3-retained-slot30-controls-reviewed/report.json).
  This is a controlled initial-state diagnosis, NOT naturally calculated
  history acceptance, a WASM diagnostic, or an engine correction. No vertex
  reset/copy patch was added. Original files/process were never modified.
  Complete preceding clock inputs remain necessary to calculate this state.
  Fresh original L8 capture contains 846 frames/10983 clocks/9585 RNG calls
  and the actual prior sequence [9,6,5,2,3,4,9,9,2,1,2,1,9,5,6]
  (dd2-original-race-l8-prefix-stream-reviewed). Both natural-history and
  direct-state native runs reject RNG exhaustion, with unconsumed clocks;
  direct-state RNG phase first differs at frame 235, cf115/ticks232
  (4839 port calls versus 4837 original). No complete L8 acceptance claimed.
- Patch 845 restores the signed BYTE camera lane at 0x77cf3a. Original
  0x429294..0x42929f tests the signed byte; 0x42946f..0x42947a sign-extends
  the same byte from an overlapping DWORD for the right-wall clamp.
  At actual L1 cf596/ticks1193, bytes 03 ff 00 00 mean lane 3; the port's
  old int alias read 65283, skipped the clamp and left camera bump=0 instead
  of 32. Car 4's physical state and calculated RNG already matched.
  The detailed independent L1 original capture now matches BOTH targets:
  all 1403 frames/palettes, 23640 clocks, 2458 calculated RNG records,
  naturally calculated seed=872332370/blink=58 and the actually observed
  preceding sequence [9,6,5,2,3,4,9,9,2]. All 31 scenery checkpoints,
  negative first-pixel/state/prefix mutations and 31 full snapshots of six
  named camera/car regions match literally
  (dd2-original-race-l1-camera-byte-history-reviewed;
  dd2-l1-camera-byte-state-reviewed). The actual old WASM fails on the same
  inputs with exactly eight wrong images while fixed native passes
  (dd2-original-race-l1-camera-old-wasm-negative-reviewed).
  Full original L5/L6 regressions pass both ports: 1235/1371 frames,
  13907/15980 clocks and 2990/1860 RNG records
  (dd2-original-race-l{5,6}-camera-byte-regression-reviewed).
  The newly captured original L9 loop also passes fixed native and pre-845
  WASM: 972 frames/17256 clocks, actually recorded empty prefix and negative
  prefix mutations (dd2-original-race-l9-prefix-history-reviewed).
  Native normal intro/menu/CD/controller/live-race/pause/restart passes
  3072000 renderer pixels and 9332264 accepted mixed bytes
  (dd2-native-camera-byte-startup-reviewed). Browser gesture/full/skip starts
  pass 1071513600 canvas pixels, 6039616 exact Wine source PCM bytes per case
  and all 900/1048/1012 submitted shared buffers, reaching a real menu/race
  (dd2-browser-camera-byte-startup-reviewed). These are submitted-byte
  checks, not complete original physical/audio-clock proof.
  Native 7102b62a5cb1b6b96109f0e9678ad66c93f59373cb05d41bea06f5f24d15bae0;
  Node WASM 2c3ae23fcd2d243c09a17c2099107f1bd4bfe840807f0e13f0c75e816e583325;
  browser WASM 84c84d51c7280d0f84b3fbc7699b575855e0e385542f7a5eef918a0444c4e3e8.
  The first independent L1 capture also now passes both ports literally:
  1371 frames/22653 clocks/2458 RNG records after the real frontend history
  (dd2-original-race-l1-camera-byte-first-history-reviewed).
  Full original L2/L3/L4 regressions pass both ports: 1433/1497/812 frames,
  10116/22021/9425 clocks and 938/1948/2040 RNG records with naturally
  calculated initial states (dd2-original-race-l{2,3,4}-camera-byte-regression-reviewed).
  The new L9 capture passes both actual 845 builds as well, including its
  recorded empty prefix and negative prefix checks
  (dd2-original-race-l9-camera-byte-regression-reviewed).
  All ten default demos pass 15255 complete frames/palettes and every
  effects/mixed/music/CD/RNG/flip byte on native/WASM
  (dd2-parity-camera-byte-reviewed/results.json). Canonical /tmp/dd2_native
  and /tmp/lvltest/dd2run.* now contain these verified artifacts.
  Browser Championship/name entry/live-race/Retire/Yes/all four populated
  divisions/return-to-results also passes on the new build
  (dd2-browser-champ-camera-byte-reviewed). The L7 narrow direct-state test
  still rejects the same six images after 845
  (dd2-original-race-l7-camera-byte-direct-reviewed); no full L7 acceptance
  or resolution of its differing prior clock/history is claimed.
- New original race captures record actual preceding demo entries at the
  original first GetTickCount return using read-only hardware breakpoints.
  The comparator checks the recorded level sequence when --attract-history
  is used. The actual L7 capture contains 17 prior entries
  [9,6,5,2,3,4,9,9,2,1,2,1,9,5,6,8,2]; the default-clock native history adds
  [4,1] before reaching L7 and correctly fails its initial RNG check.
  The sequence checker separately rejects this actual 19-entry port history
  and an extra-demo mutation (dd2-original-prefix-metadata-check-reviewed).
  Entry observations are not complete prefix clocks or full-prefix proof.
- Before patch 845, L1 was rejected: the full 1371-frame/22653-clock/2458-RNG capture
  has 9 differing images in direct initial-state matching. Real native/WASM
  history [9,6,5,2,3,4,9,9,2] naturally calculates seed=872332370/blink=58,
  matches all state/clock/RNG phases and 9 scenery checkpoints, and removes
  the one-pixel cf438 difference, but 8 images at cf596..602 still differ
  (dd2-original-race-l1-tunnel-{direct,history}-reviewed). A detailed original
  camera/command capture around those counters enabled the signed-byte fix
  documented above.
  L7 direct initial-state matching also remains rejected: both targets have
  6 differing images despite exact clock/RNG/blink phases over 1147 frames,
  21711 clocks and 4726 RNG calls (dd2-original-race-l7-tunnel-direct-reviewed).
  It excludes the recorded real prefix and is not full original acceptance.
- Championship browser navigation now waits for the actual slab transition
  and engine input consumption rather than releasing at an arbitrary next
  presentation. Both normal speed and 3x-throttled menu selection/name entry
  reach a live championship race, Retire/Yes, View League, all four populated
  divisions and return to results without browser errors
  (dd2-browser-champ-steady-slab-{normal,slow-menus}-reviewed).
  Racing runs at normal speed in both accepted tests; an earlier whole-run
  3x-throttle diagnostic hung at loading and is not accepted. This change is
  test synchronization only, not an engine fix or new original score proof.
- Patch 844 maps all six L2/L4 tunnel color-vector sRam aliases to the
  actual signed WORDs in scene_colour_vectors, instead of zeroed host stubs.
  The original reads these overlapping fields at current_level*8 plus
  0x465922/24/26; 0x42b9c4..0x42ba4c and 0x42ba89..0x42bab2 prove the
  WORD/sign-extended accesses. At original L2 cf454/ticks910, each constant
  is 4096 and car 4's interpolated color matrix is 2626, producing shadow
  opacity 48. The old stub reads produced matrix=0 and opacity=24.
  Both independent complete original L2 captures now match literally on
  native/WASM, using naturally calculated initial states after real demos
  [9,6,5]: 1433 frames/10116 clocks/938 RNG calls and 813 frames/5553
  clocks/940 RNG calls. Every palette, RNG/blink/clock phase and all 9/28
  scenery snapshots match; first-pixel and state mutations are rejected
  (dd2-original-race-l2-tunnel-{stream,images}-reviewed/report.json).
  The actual pre-844 WASM is rejected on the first capture's same inputs:
  exactly 16 wrong frames, despite exact clock/RNG consumption and phases
  (dd2-original-race-l2-old-wasm-negative-reviewed/report.json).
  L4 also matches every original byte on both ports: 812 frames, 9425 clock
  returns and 2040 RNG calls, naturally following [9,6,5,2,3], initial
  seed=3574421218/blink=68 (dd2-original-race-l4-tunnel-history-reviewed).
  Full L5 regression remains exact (1235 frames/13907 clocks/2990 RNG calls;
  dd2-original-race-l5-tunnel-regression-reviewed). All ten default demos
  pass native/WASM parity for 15255 frames/palettes and all audio, RNG/flip
  and CD bytes (dd2-parity-tunnel-colour-reviewed/results.json).
  Native 381826d08cb426aac95471d09cdb7894dc993ba245bbcb27f2c392bfb24f92b8;
  Node WASM 926bc8c59c52e6757b7a0c1e128ba0bb822a434c433042346f417a1af719d832;
  browser WASM 81ed0a8fc0555c90e7579f280aa8bf2f4fe40c9461c0865d2feb949bf0080144.
  Normal browser gesture/full/skip -> menu -> live race passes 954777600
  exact canvas pixels, 6039616 original source PCM bytes per case and all
  913/925/13059 submitted shared buffers (dd2-browser-tunnel-colour-startup-reviewed).
  Native intro/menu/CD/controller/race/pause/restart passes 3072000 renderer
  pixels and 7781704 accepted mixed bytes (dd2-native-tunnel-colour-startup-reviewed).
  These are submitted-byte checks, not complete original output-clock proof.
- The new actual L3 capture (1243 frames/13181 clocks) is still under
  investigation. Its initial RNG does not match the default-clock preceding
  port demos; strict natural-history comparison correctly rejects that
  mismatch (dd2-original-race-l3-tunnel-history-reviewed/native/run.log).
  Explicitly matching the observed initial seed/blink gives exact clock/RNG
  phases but rejects frame 566 (cf234/ticks470): 25 pixels along a narrow
  edge differ on BOTH targets
  (dd2-original-race-l3-tunnel-both-direct-reviewed/report.json).
  The comparator collects both framebuffer failure reports before rejecting
  the run; a native pixel failure no longer hides WASM evidence. Its passing
  L5 regression retains all 1235 frames/13907 clocks/2990 RNG records and
  negative pixel/state checks (dd2-original-race-l5-collect-failures-regression-reviewed).
  A second independently observed L3 loop matches every byte on both ports:
  1497 frames/22021 clocks/1948 calculated RNG calls, all 17 scenery-x checks
  and negative mutations (dd2-original-race-l3-edge-direct-reviewed).
  The real frontend history [9,6,5,2] also passes this complete second loop
  on BOTH ports, with naturally calculated seed=2300983881/blink=30 checked
  without injection (dd2-original-race-l3-edge-history-reviewed).
  The first capture starts at RNG sequence step 59712 from seed 1, whereas
  the accepted second starts at step 12920. Its different prior history is
  not reconstructed yet; that capture's seed=738116801/blink=14 edge remains
  rejected. A controlled native renderer replay of its port OT reproduces
  all port pixels exactly; the narrow discrepancy is behind actual debris
  primitives rather than a native/WASM renderer disagreement
  (dd2-l3-edge-painter-trace-reviewed). No original command image was captured
  at that first loop's cf234, so its engine-versus-renderer cause remains open.
  Complete original racing audio, remaining races/modes and physical output
  clocks remain open; whole-game acceptance is still unproved.
- Patch 843 restores Update_Debris' signed-WORD translation after camera
  rotation, as proved by original 0x42485c..0x424882. At L5 cf546/ticks1094
  slot 116's rotated z=-32775 must wrap to 32761. Retaining the full int
  incorrectly culled primitive 0x778110 and changed the last global __otz
  from 32239 to 23730, which also shaded visible smoke 101 instead of 126.
  Both independently recorded complete L5 loops now match every native/WASM
  framebuffer/palette byte and clock/RNG/blink phase: 1235 frames/13907
  clocks/2990 RNG calls in dd2-original-race-l5-debris-direct-reviewed, and
  1435 frames/21128 clocks/3005 RNG calls in
  dd2-original-race-l5-debris-history-reviewed. The latter naturally derives
  initial seed/blink through real demos [9,6], compares all 19 scenery
  snapshots and rejects first-pixel/RNG/blink corruption. The original
  debris xy, __otz and all three visible smoke shades now also match in
  both targets' frame1158 engine images. No renderer or state injection fix.
  L6 natural-history (1371 frames/15980 clocks/1860 RNG calls) and L9
  (1499 frames/18931 clocks) regressions pass on both final artifacts:
  dd2-original-race-l{6,9}-debris-regression-reviewed/report.json.
  Native artifact 69974c5e4266e61921bbb1bc2cd4827410f14a7e0e2617098a43713750cd47a8;
  Node WASM 9be2f3a62605b68cff68ba7f11f500270c7e35986035dc05db7ff56e50c63193;
  browser WASM 43f3f904adf73d2b9e48ad8c0f31ec2dba1772040f398e3be753446eacefa309.
  All ten default native/WASM demos also pass: 15255 complete frames/palettes,
  every RNG/flip record and all effects/mixed/music/CD bytes, without
  reference files (dd2-parity-debris-word-reviewed/results.json). Canonical
  /tmp/dd2_native and /tmp/lvltest/dd2run.* use these verified artifacts.
  Normal browser gesture/full/skip intro -> menu -> live race also passes:
  1022668800 exact canvas pixels, 6039616 exact original source PCM bytes
  per case and all 918/951/977 submitted shared buffers
  (dd2-browser-debris-word-startup-reviewed/report.json). Native normal
  intro/menu/CD/controller/race/pause/restart passes 3072000 renderer pixels
  and 7519928 accepted mixed bytes (dd2-native-debris-word-startup-reviewed).
  Championship selection/name/Retire/Yes -> results -> View League shows all
  four populated divisions on this browser build
  (dd2-browser-debris-word-champ-trace-reviewed/scores.json). The original
  fixed-delay navigation test first missed a key during an animation;
  navigation synchronization needs repair, not another engine correction.
- Before patch 844, the first actual-original L2 loop was NOT accepted. The unmodified capture
  dd2-original-race-l2-stream-reviewed has 1433 frames, 10116 actual clock
  returns, 938 RNG calls and initial seed=473895114/blink=73. The real
  native history [9,6,5] naturally produces that seed/blink and matches all
  state phases and all nine scenery checkpoints, but 16 framebuffer images
  differ at cf419..479, including a visible car-shadow difference. Preserve
  dd2-original-race-l2-debris-history-reviewed/report.json. A second L2
  capture records detailed engine images around those counters for diagnosis.
  Original complete racing audio, other races/modes and physical output
  clocks remain open. Full whole-game acceptance is still unproved.
- The reference comparator now supports --attract-history: real frontend and
  preceding demos calculate initial RNG/blink states, which are required to
  match rather than initialized from the recording. L6 passes both targets
  after the real L9 demo: all 1371 frames/palettes, 15980 clock returns, 1860
  calculated RNG calls and eleven scenery checkpoints
  (dd2-original-race-l6-history-comparison-reviewed/report.json). The new
  clock/RNG fixture runs pass on native ASan/UBSan and WASM
  (dd2-{clock,random}-attract-history-reviewed/report.json). All ten default
  native/WASM runs still pass: 15255 full frames/palettes and all audio,
  RNG/flip and CD bytes (dd2-parity-attract-history-reviewed/results.json).
  --race-image-counters records every occurrence of selected cf values;
  per-presentation port image names retain the countdown-reset occurrences.
  Existing L9 full-loop regression remains exact on both targets (1499 frames;
  dd2-original-race-l9-history-instrument-regression-reviewed).
- Before patch 843, a second L5 capture had 1435 frames, 21128 clock returns and 3005 RNG calls.
  Its real preceding demos [9,6] naturally produce initial seed=365994542 and
  blink=35. Native matches all clock/RNG/blink phases and all 19 scenery-x
  snapshots, but six presented images at cf547..549 differ
  (dd2-original-race-l5-history-comparison-reviewed/report.json). Positions
  and animation fields of the smoke particles match; their depth-cued shades
  differ because the prior global __otz differs. The original OT also contains
  debris primitive 0x778110, missing in the port. Lens-flare commands and
  active color/texture tables match with the real attract history. Feeding
  the original frame1158 OT/image into the native renderer reproduces every
  byte of original frame1159 (dd2-l5-original-raster-input-reviewed). This is
  a controlled renderer-input test, not complete original game acceptance.
- After patch 842, all ten default native/WASM demo runs again match exactly:
  15255 complete presented frames/palettes, every RNG/flip record and every
  effects/final-mix/music/CD byte, without reference clock/RNG files
  (dd2-parity-scene-word-reviewed/results.json). Canonical /tmp/dd2_native
  and /tmp/lvltest/dd2run.* now use these verified artifacts:
  native 44543af8fb7fe94dd5c4c8f9af663d5056467bca8fe794628a62b817e2922444;
  Node WASM 300dbefd91e28268fa41f2ca7de3bdc973ceb79bf8f0cbd18382443781f34430.
  Browser normal full/skip/gesture startup also passes with WASM
  963e9bc82bc4ea43dbe7bfd60269087daf6fa3d6fd5bc701b8dd70a471524c1a:
  1057382400 exact canvas pixels, 6039616 exact original Wine source PCM
  bytes per case and all 980/1024/906 submitted shared buffers, then real
  menu input and a populated 20-car race
  (dd2-browser-scene-word-startup-reviewed/report.json). Native normal
  intro/menu/CD/controller/race/pause/restart passes 3072000 renderer
  pixels and 7540032 accepted mixed bytes
  (dd2-native-scene-word-startup-reviewed/report.json). Existing final L9
  actual-original comparison remains exact for all 1499 frames on both
  corrected artifacts (dd2-original-race-l9-scene-word-regression-reviewed).
  The real pre-842 WASM artifact is rejected on the same L6 original input:
  22 wrong frames and all 11 wrong first-scene x snapshots; exact clock/RNG
  consumption still passes (dd2-original-race-l6-old-wasm-negative-reviewed).
- The first complete actual-original L5 capture failed before patch 843.
  dd2-original-race-l5-rng-reviewed has 1235 frames, 13907 actual clock
  returns, 2990 actual random calls and initial seed=365994542/blink=35.
  Native computes every random state/return and matches the clock/blink
  phases, but seven presented images differ: cf547..549 (smoke shading)
  and the final cf700 frame. Preserve the failed report
  dd2-original-race-l5-word-comparison-reviewed for investigation.
  Complete original racing audio, all races/modes and physical output
  clocks remain open; the active whole-game goal is still unproved.
- Patch 842 corrects a second scenery-streaming WORD store missed by patch
  405. The supported original resets its stream flag at 0x789356 with
  `mov WORD [0x789356],cx` (0x430ab8); the translated DWORD store also zeroed
  object 0/0's local x at 0x789358. At cf260 the original x is 15133, while
  the old port has 0, despite identical shape/matrix/active-block state.
  The resulting culling removed visible scenery around cf252..270 and later
  streaming boundaries. The fix preserves the original WORD write and
  injects no coordinate. The old L6 runs fail on 16/22 complete frames;
  after the fix both independently recorded L6 render loops match literally
  on native/WASM: 1371 frames/palettes each, all 16316/15980 actual clock
  returns and all 1860 actual random calls per run. Every frame's clock/RNG
  call phase, tick/countdown/skip and DEMO MODE blink state also match.
  dd2-original-race-l6-word-{second-comparison,comparison}-reviewed/report.json
  retains the proofs. The latter also compares original object x at all
  eleven observed engine-image checkpoints, including the defective region.
  Both comparisons reject first-pixel and first RNG/blink-phase corruption.
  Matching initial state explicitly includes original seed=1038462957 and
  inherited blink counter=78, once; subsequent values/states are calculated.
  Default/interactive runs never use those verification-only initializers.
  This proves captured L6 racing rendering at its debugger-observed API
  times, not the original race audio, initialization/fades, every other
  race/mode or physical/undebugged display clocks.
- Original race capture now selects a naturally occurring attract level
  with --race-level and observes actual Watcom rand() with a fourth moving
  hardware breakpoint: 0x456cc6 reads the pre-seed, 0x456cde the stored
  post-seed and actual returned EAX. All original memory/registers remain
  unmodified. The old MSVC/address comments were wrong; the implemented
  seed*0x41c64e6d+0x3039 algorithm already matched this original Watcom CRT.
  DD2_RANDOM_REFERENCE/LEVEL checks every computed pre/post-seed and return;
  it seeds the private CRT RNG once rather than replaying random outputs.
  Native ASan/UBSan and WASM fixtures verify four actual L6 records, level
  gating, partial/exhausted/leftover rejection, changed pre/post/return
  rejection and an actual multiplier mutation (16 cases total;
  dd2-random-state-reference-reviewed/report.json). Strict clock fixtures
  still pass (dd2-clock-random-state-reviewed/report.json). Full capture can
  add --race-images INDEX... for narrowly compared diagnostic engine fields.
- After the original race-stream/clock-replay instrumentation, all ten
  ordinary demo runs still match between native/WASM: 15255 complete
  presented frames/palettes, every RNG/flip record and every effects/final-
  mix/music/CD byte (dd2-parity-clock-stream-reviewed/results.json).
  The ordinary default and interactive paths are checked without a clock
  replay file. Canonical /tmp/dd2_native and /tmp/lvltest/dd2run.* now use
  the verified builds: native f8e3cf2d365ff9e1d22372f771d7b622b36c1621e127aaf6c615e91f513c604b;
  Node WASM 17f0bcd4e2f644c0427da99682f0716774facb78e4dc1d3e20fb694de6c19eca.
  Browser normal full/skip/gesture startup proof uses WASM
  b4f28c8f4527a640f9b1848579167e3cedd8ce915e0e8f9331761962981da296. Prior reports below retain
  their own artifact hashes and narrower scopes. Complete original audio,
  initialization/fades, all other original races/modes and physical output
  timing still require acceptance; the active whole-game goal is unproved.
- The complete first original L9 racing render loop is now compared directly,
  not only at sparse cf checkpoints. The unmodified supported dd2h.exe runs
  normally under Wine; three read-only hardware breakpoints record every
  racing-loop Draw_All pending presentation and every actual GetTickCount
  return, including the busy wait. Ports replay those exact external API
  inputs. dd2-original-race-stream-reviewed/race/race.json has 1499 frames
  from countdown start through the final rendered cf700 endpoint, followed
  by the original budget-exhaustion exit (quit=1, no user quit). The clock
  contains 18931 actual returned DWORDs. Both ports match every frame and
  palette byte and every render's cf/ticks/countdown/frame_skip/clock-call
  phase: 460492800 framebuffer bytes and 1534976 palette bytes per port.
  dd2-original-race-stream-comparison-reviewed/report.json also requires
  exact clock consumption and rejects a deliberately changed first pixel.
  This proves the entire captured racing render loop at matched observed
  API timing; Init_Game/loading/fades, original race audio, other levels/
  modes and physical/undebugged output clocks still require acceptance.
  Native artifact f8e3cf2d365ff9e1d22372f771d7b622b36c1621e127aaf6c615e91f513c604b;
  Node WASM 17f0bcd4e2f644c0427da99682f0716774facb78e4dc1d3e20fb694de6c19eca.
  Reproduce with make refcapture-race-stream
  RACE_CAPTURE_ARGS='--timeout 240 --output <fresh-original-dir>', then
  make verify-reference-race-stream RACE_REFERENCE=<original-dir>
  RACE_COMPARISON=<fresh-comparison-dir>. Captures stay under ignored
  third_party/verification-artifacts; no original pixels/assets enter Git.
- DD2_TICK_REPLAY is a strict, optional headless verification input: one
  little-endian DWORD per game GetTickCount call, no engine-state replacement
  or pixel injection. Default and interactive clocks keep their existing
  behavior. Missing/partial/leftover clock data fails rather than falling
  back to a synthetic clock; DD2_REALTIME cannot be combined with replay.
  make verify-clock-replay CLOCK_REPLAY_ARGS='--output <fresh-dir>' checks
  uint32 wrap/repeated values and all three extent failures on native
  ASan/UBSan and WASM (dd2-clock-replay-reviewed/report.json). Normal real-
  X11 startup/menu/CD/controller/race/pause/restart passes 3072000 renderer
  pixels and 7489584 accepted mixed bytes. Browser normal gesture/full/skip
  startup passes 1080115200 canvas pixels, 6039616 exact source PCM bytes
  per case and all 997/960/1040 submitted shared buffers, then real menu/race
  controls (dd2-{native,browser}-clock-replay-startup-reviewed/report.json).
- Function/editing/navigation keys now also reach the original window
  procedure: browser F1-F24, Backspace/Tab/PageUp/PageDown/Home/End/Insert/
  Delete, and native SDL F13-F24 (F1-F12/editing/navigation were already
  mapped there). The original 39-key rebind scan list is unchanged.
  dd2-keyboard-named-user32-virtual-reviewed/report.json extends the genuine
  USER32 comparison to 104 transitions on all five transports, with native
  direct fixtures under ASan/UBSan. Malformed/unmapped function-key codes
  must dispatch no message. F13-F24 use declared SendInput virtual keys:
  this Wine layout maps their scans to zero, so scan-mode input would deliver
  VK_VOLUME_MUTE instead. The rejected scan-mode run is retained separately
  (dd2-keyboard-named-user32-reviewed). No physical F13-F24 claim is made.
  Numeric VKs follow Winuser.h/the Microsoft virtual-key table:
  https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes.
- dd2-browser-named-keys-reviewed/report.json passes 14 normal-startup intro
  release/press/context-close/main-menu cases, including F3/F12/F24/Home/Tab/
  Delete/Backspace. All 104 shell transitions have DOM isTrusted=true;
  Debian Playwright1.38 lacks F13-F24 names, so those events use Chromium's
  real Input.dispatchKeyEvent protocol. The actual 66cbff7 shell filter,
  with current unchanged WASM, drops 58 transitions and is rejected by the
  same bridge assertion (dd2-browser-named-negative-final-reviewed/report.json).
  make verify-browser-keyboard-negative
  BROWSER_KEYBOARD_NEGATIVE_OUTPUT=<fresh-dir> reproduces this negative.
  L9/L10 full-frame/palette/RNG/effects/final-mix/music/CD comparisons still
  pass (dd2-parity-named-keys-reviewed/results.json). Node WASM is byte-
  identical to the prior 325e3bd artifact; its headless linker removes the
  unused browser-key mapper. The current canonical native is
  6bbb360ad8434bf9f481dcc45a4c5ee54be0372cb1006a300f653314a0cdfea6; current browser WASM
  3d95102fa4214f81614fcbaba591ea22e09ce8646dacb8730e7e1c86253e2ca3. Earlier complete-ten-level
  reports below identify their own native/browser artifact hashes.
- The committed modifier fix (325e3bd) also passes all ten complete
  native/WASM demos: 15255 frames/palettes, every RNG/flip log, effects/final
  mix/music byte and CD source byte match (dd2-parity-keyboard-reviewed/
  results.json). Each level has 10569888 effects/final-mix/music bytes and
  4939200 raw CD bytes. Canonical /tmp/dd2_native and /tmp/lvltest/dd2run.*
  now point to these tested builds: native
  c37fc4c0d40791a425f25a54cbe5fb510fa9afc710fc8c10749313134410efa4,
  Node WASM 8d170dfaee580a3270acdd071b562054c9939144ad88059f78a0675d192b5b04;
  the seven-key browser startup proof uses WASM
  a5cca12c1e6befa98de5abaa7c6152f3d0908b2239110ee051bc89617e79469a.
  This is full cross-port demo evidence; full original clocks and hardware
  comparisons remain open. The SDL fixture additionally verifies that focus
  cleanup of both Shift sides plus A emits exactly three physical releases
  and leaves all corresponding states clear (dd2-native-sdl-keyboard-focus-
  reviewed/report.json). Existing older original-startup reports below
  retain their historical binary hashes and remain separate proof runs.
- Modifier input now dispatches exactly one generic window message per
  physical transition while retaining left/right GetKeyState pressed states.
  Releasing left Shift with right Shift held remains KEYUP, with aggregate
  Shift still pressed; the old native adapter emitted an extra KEYDOWN and
  could incorrectly cancel a movie. Alt/Ctrl combinations, single/double Alt,
  F10 and repeat message classes are checked against genuine Wine10 USER32
  SendInput/GetKeyState, not a reconstructed expected-value function.
  dd2-keyboard-user32-final-reviewed/report.json has 40 exact records on
  native SDL/direct-VK/browser-code and WASM direct-VK/browser-code transports;
  direct native fixtures also pass ASan/UBSan. Rebuilding the historical
  941d609 bridge is a runtime negative control: its first modifier emits two
  messages and fails the same boundary assertion.
  Reproduce: make verify-keyboard KEYBOARD_ARGS='--mingw <32-bit-compiler>
  --output <fresh-dir>'. The right-Shift SendInput flag is an explicitly
  declared Wine10 transport quirk (see keyboard_win32_probe.c). Evidence
  covers pressed-state high bits, message class/count, generic VK and release
  direction; physical layouts, scan-code/lparam metadata, character messages
  and AltGr synthesis are still unproved. Primary message-class reference:
  https://raw.githubusercontent.com/wine-mirror/wine/wine-10.0/server/queue.c.
- The browser shell now forwards six side-specific modifiers and F10.
  dd2-browser-keyboard-reviewed/report.json uses real Chromium key input,
  normal original startup and all seven keys: keyup retains the intro;
  keydown closes its real movie AudioContext and reaches the original main
  menu. All 40 fixture transitions also traverse the actual generated shell.
  make verify-browser-keyboard BROWSER_KEYBOARD_OUTPUT=<fresh-dir> reproduces
  it. Native real-X11 Shift checks cover both intro/outro, every presented
  SDL renderer pixel and all 6039616/6395840 accepted PCM bytes (the complete
  source is queued even when skipped); keyup retains and keydown closes.
  tools/verify_native_movie.py --skip-only --release-key Shift_R
  --skip-key Shift_L selects these cases. The SDL transport fixture now
  links production dd2_input.c rather than mocking key delivery; its 614400
  renderer/X11 pixels and 7976 Float32 bytes remain exact.
  The original rebind poller scans precisely 39 VKs: A-Z, 0-9, Space, Left,
  Right (original image table @0x4692ec). Earlier historical "ANY key"
  descriptions below are superseded; modifiers are not newly rebindable.
- The original-startup builds pass all ten complete cross-port demos:
  15255 frames/palettes, RNG/flip logs and every effects/final-mix/music-
  summand/CD source byte match (dd2-parity-original-boot-reviewed/results.json).
  Canonical /tmp/dd2_native and /tmp/lvltest/dd2run.js/.wasm now use these
  verified builds. Native SHA
  46baacc318f0f2be3018a3c0edbef710b271678cff4fb768b8e55dc74dc98751;
  Node WASM de3d52c174668291b3d50f8fe01304da38345ee6d0a2a1b47f0c195b9bfbe2eb;
  browser WASM e99d55ea0f82f613340d5d269cba9ce5e0f79d420442070d7204e5f2b35682e3.
  Normal native Quit also has real-X11 coverage: skip the actual intro,
  cancel Quit back to the running menu, then confirm Quit. The original
  user WinMain return1 is preserved, both actual SDL streams close and all
  116121600 renderer pixels match the submitted texture. No engine writes
  or shared-save changes are used; the test copies SaveGames first.
  make verify-native-quit NATIVE_QUIT_ARGS='--output <fresh-dir>' reproduces
  dd2-native-original-quit-reviewed/report.json. This checks the real cleanup
  path; complete original output-clock/hardware acceptance remains open.
- Normal SDL window and default browser startup now use the original user
  WinMain sequence @0x423ae0: Init_Application, ddmain's pal_flag=0/Play_Intro/
  Init_Main/Init_Front_End/Front_End, then Close_Application and return1.
  dd2_boot.c calls the reconstructed engine sequence; patch841 selects it
  before the browser diagnostic launcher's synthetic post-frontend state.
  Explicit movie/race/demo entries and headless fixtures remain diagnostics.
  Native real-X11 tests complete or skip the normal intro, then navigate CD/
  Configuration, drive a populated race with a virtual SDL controller, pause
  and restart CD at its exact public sector. Both pass 3072000 renderer pixels
  and every accepted mixed byte (8396640 skip / 7617304 full), in
  dd2-native-original-startup-{skip,full}-reviewed/report.json.
  Browser normal startup passes full, skip and actual required-user-gesture
  cases: 1712 complete intro frames or 31 skipped frames, all 6039616 submitted
  source PCM bytes per case and all 1091481600 actual canvas pixels across
  movie/menu/race. Every 1049/1023/1010 shared Float32 buffer matches C exactly;
  none is submitted during the movie. The movie context closes, DirectSound
  restarts at44100 and actual menu keys launch a race with20 cars.
  Strict autoplay is observed through read-only CDP userGesture:false before
  the first real click: the document is unactivated, the22050 context suspended
  and video remains on frame0. Playwright evaluate/waitForFunction otherwise
  grants user activation; the earlier failed policy fixture is retained in
  dd2-browser-original-startup-reviewed, while the corrected complete rerun
  passes in dd2-browser-original-startup-cdp-reviewed/report.json.
  make verify-browser-startup BROWSER_STARTUP_ARGS='<AVI-proof-dir> <fresh-dir>'
  and make verify-native-window NATIVE_WINDOW_ARGS='--controller --full-intro
  --output <fresh-dir>' reproduce these actual startup paths.
  Seven normal browser menu/Configuration/Audio checkpoints compare all64
  rendered highlight phases against actual original Draw_All presentations:
  448 complete framebuffer/palette pairs, zero differences or pixel masks
  (dd2-browser-config-original-startup-presented-comparison.json). Native SDL
  boundary checks also pass614400 actual renderer/X11 pixels and7976 accepted
  Float32 bytes after teaching its read-only observer separate movie versus
  shared Float32 device lifetimes. Existing eight originalL9 framebuffer/
  palette checkpoints still match the new native and Node binaries.
  Source/sink/API checks do not establish complete original movie presentation,
  driver tails, queued-control timing or physical display/DAC fidelity; those
  remain open, as do the other whole-game original acceptance requirements.
- DirectSound now has actual per-interface AddRef/Release counts, a shared
  default-device lifetime and final cleanup of remaining primary/master
  buffers. The same Win32 probe passes actual Wine; native ASan/UBSan and
  WASM compare 4224 complete mixed PCM bytes over two device epochs and an
  independent MCI CD source. A 69991ms closed-device movie interval contributes
  no audio or stale fractional samples after reopen. Releasing DirectSound
  while CD remains active preserves the CD source and shared output clock.
  SDL clears/closes an idle Float32 sink; the browser stops pending sources,
  removes resume listeners and closes the old AudioContext. A deliberately
  retained old clock fails the intended actual PCM/sink comparison.
  make verify-sound-device SOUND_DEVICE_ARGS='--mingw <compiler>
  --output <fresh-dir>' reproduces
  third_party/verification-artifacts/dd2-sound-device-specific-negative-reviewed.
  The existing Wine/native/WASM cursor and duplicate-lifetime probes pass
  (dd2-sound-cursor-device-reviewed.json and dd2-sound-lifetime-device-reviewed.json).
  Eight original L9 framebuffer/palette checkpoints still match both new
  builds. Actual native menus/CD/controller/race/pause pass 3072000 pixels/
  7580256 mixed bytes; browser CD restart passes 1404 shared buffers/70736
  resumed source bytes (dd2-native-window-sound-device-reviewed and
  dd2-browser-redbook-sound-device-reviewed). These are API/controlled-clock
  and sink-boundary checks, not original movie-to-menu output-clock parity.
  These device-lifetime builds also pass the complete ten-level cross-port
  regression: 15255 frames/palettes, RNG/flip logs and all effects, shared-mix,
  music-summand and CD source bytes match
  (dd2-parity-sound-device-reviewed/results.json). Both full and skipped
  diagnostic films pass again on SDL: 1712/1813 complete frames and every
  source PCM byte (dd2-native-movie-sound-device-lifetimes-reviewed/report.json).
  The first post-cleanup film observer incorrectly joined a retired Float32
  stream to a movie with a reused SDL device ID. Its failed evidence remains
  in dd2-native-movie-sound-device-reviewed; the corrected verifier selects
  the actual open/close lifetime and the fresh four-case rerun passes.
- The completed movie-MCI builds also pass all ten complete sound-enabled
  demos: 15255 presented frames, every framebuffer/palette, RNG/flip log,
  effects/final-mix/music-summand byte and raw CD source byte match between
  native and WASM. Full results are retained in
  third_party/verification-artifacts/dd2-parity-mci-movie-reviewed/results.json.
  Canonical /tmp/dd2_native and /tmp/lvltest/dd2run.js/.wasm now use these
  verified builds; native SHA
  93abecf8d7de24e8bede1c56235f540d94eeea95c8916fecc77718110d0728c9.
  This is cross-port regression evidence, not complete original-game parity.
- Actual engine Play_Movie now has an AVI MCI device, SDL movie texture/PCM16
  device and browser canvas/source-rate AudioContext. DD2_MOVIE=INTRO.AVI or
  OUTRO.AVI runs the real Init_Application/Sound_Remove/Set_Draw_Mode/Play_Movie
  diagnostic entry; browser URLs ?movie=INTRO.AVI / ?movie=OUTRO.AVI use it too.
  Decoder/container and GDI conversions feed this path, without FFmpeg RGB
  substitution or a 44100Hz WebAudio movie resampler. Open/Window/Put/Notify
  Play/Close parameters remain the original patch839 contract; patch840 adds
  the WASM diagnostic entry only. Movie-aware PeekMessage pumps/yields and
  keyboard events use the actual window procedure, which clears Movie_Playing
  on key-down/notify while preserving it on key-up.
  Both complete films present 1712/1813 frames on actual SDL and canvas; every
  SDL renderer/canvas pixel compares directly with its submitted ARGB surface.
  Source-rate device queues/AudioBuffers contain all 6039616/6395840 PCM bytes
  exactly as Wine ACM, with no resampling at the browser source boundary.
  Actual Esc key-up retains playback and D key-down closes/stops each film.
  make verify-native-movie NATIVE_MOVIE_ARGS='--source <AVI-proof-dir>
  --output <fresh-dir>' and make verify-browser-movie
  BROWSER_MOVIE_ARGS='<AVI-proof-dir> <fresh-dir>' reproduce full/skip cases.
  Proofs: third_party/verification-artifacts/dd2-native-movie-final-reviewed
  and dd2-browser-movie-reviewed/report.json. Native uses the declared SDL
  dummy audio device; accepted queue data is distinct from consumed/DAC data.
  Complete source submission on skip does not establish original queued
  control latency or the original accepted ALSA prefix/tail. Those remain open.
  The extracted engine against the production MCI backend also passes both
  full films, early Close, 32-bit clock wrap, every controlled frame deadline,
  delayed drain-before-notify and unavailable audio devices; the latter retain
  video like Wine MCIAVI_player. Native ASan/UBSan/WASM compare every generated
  ARGB byte and source PCM; changed display bits fail. make verify-movie-playback
  MOVIE_PLAYBACK_ARGS='--source <AVI-proof-dir> --output <fresh-dir>' reproduces
  third_party/verification-artifacts/dd2-movie-playback-audio-unavailable-reviewed.
  Eight original L9 framebuffer/palette checkpoints still match the new
  binaries. Actual native menu/CD/controller/race/pause tests pass 3072000
  pixels/8550104 mixed bytes; browser CD restart passes 1590 shared buffers/
  65268 resumed source bytes (dd2-native-window-mci-movie-reviewed and
  dd2-browser-redbook-mci-movie-reviewed under verification-artifacts).
  Normal startup still bypasses Play_Intro; original whole movie presentation/
  output clocks and physical hardware are not established by these checks.
  In particular Wine omits the last source paint and has a variable silent
  audio tail; no claim of reproducing Windows or those driver details is made.
- Portable movie surface conversion now matches actual Win32 GDI RGB32
  StretchDIBits into RGB565 and GetDIBits display expansion. One component
  gradient and sixteen verified original codec checkpoints exercise original
  2x/1x, offset and clipped rectangles: all 83558400 ARGB bytes match on both
  native ASan/UBSan and WASM. Changed display bits fail the same complete-byte
  comparison. make verify-movie-surface MOVIE_SURFACE_ARGS='--mingw <compiler>
  --source <movie-codec-proof-dir> --output <fresh-dir>' reproduces this;
  third_party/verification-artifacts/dd2-movie-surface-negative-reviewed/report.json
  retains actual Win32 fixture/source hashes and each case. This establishes
  the controlled GDI surface conversion, not original dd2h.exe window output,
  full movie sink/timing or physical display fidelity. MCI integration is next.
- The production AVI source interface now owns/parses complete original files,
  preserves separate video/compressed-audio/PCM clocks, decodes all audio and
  supports sequential and forward/backward/repeated frame requests. Empty
  Outro packets hold the prior image; caller buffers can be freed immediately.
  Native ASan/UBSan and WASM compare every 649728000 RGB byte and 12435456
  PCM16 byte with actual Wine ICCVID/ACM, including RGB32 DIB output as requested
  by unmodified dd2h.exe. All seven nonlinear frame requests per film match;
  ten malformed file/header/block cases per film fail without output or
  sanitizer errors. make verify-movie-avi MOVIE_AVI_ARGS='--mingw <compiler>
  --output <fresh-dir>' reproduces this. Complete RGB32 proof:
  third_party/verification-artifacts/dd2-movie-avi-rgb32-final-reviewed/report.json.
  Source components are linked into native, Node and browser builds, all of
  which pass all 176 exact patches. Eight original L9 framebuffer/palette
  checkpoints still match both new binaries. Actual native menus/CD/controller/
  race/pause pass 3072000 rendered pixels and 9957072 accepted mixed bytes
  (/tmp/dd2-native-avi-reviewed/report.json); browser live Pause/Resume passes
  1937 shared WebAudio buffers and 64564 resumed source bytes
  (/tmp/dd2-browser-avi-reviewed/report.json).
  A parallel full RGB32/parity run exhausted /tmp: its failures are retained
  as failed evidence under third_party/verification-artifacts, with old /tmp
  paths symlinked there. The fresh complete RGB32 rerun passes on the larger
  filesystem; the complete ten-level parity rerun passes 10/10: 15255 frames,
  every framebuffer/palette, RNG/flip log, effects/final-mix/music-summand
  byte and raw CD source byte matches. Full results:
  third_party/verification-artifacts/dd2-parity-avi-final-reviewed/results.json.
  Canonical /tmp/dd2_native and /tmp/lvltest/dd2run.js/.wasm now use these
  verified AVI-component builds; native SHA
  87f03b1baac86a1d577402865bfa0f3b2518456c8419ca85e2429656bbfa47f9.
  dd2_avi.c/h is ready for MCI playback integration. Normal startup still
  bypasses movies; original presentation/scaling, clock and skip/completion
  behavior remain open. Decoder/container proof is not full movie acceptance.
- A complete, unmodified dd2h.exe intro now has an actual accepted-audio
  comparison, beyond the codec API fixtures. At the declared 22050Hz reference
  device it emits all 6039616 source PCM16 bytes exactly, followed by 660
  actual silent stereo frames. No alignment, trimming or tolerance is used
  (/tmp/dd2-original-full-intro-audio-reviewed/movie-negative-proof.json).
  Changed source bits and nonzero tails fail even after updating their valid
  capture hashes; accepted-write journals are checked without overwriting
  the original capture's summary metadata. make verify-movie-reference
  MOVIE_REFERENCE_ARGS='--capture <dir> --source <movie-audio-proof-dir>
  --negative-controls --report <fresh-file>' reproduces this.
  Reference capture.py now accepts --keep-movie and explicit --wine-debug,
  recording those choices in its audio summary. The full run used --mode
  audio --audio --audio-rate 22050 --audio-tail 3 --keep-movie
  --wine-debug=-all,+iccvid,+mciavi --timeout 120. The 44100-only virtual
  reference device could not open this film's ADPCM WaveOut stream (error32);
  its later frontend PCM is not valid movie evidence. This is the configured
  device limitation, not evidence about original Windows hardware.
  Original command/codec traces confirm output RGB32/320x192 and destination
  (0,48)-(640,432); they record paints0..1710, not source frame1711. These are
  observations of Wine's MCI driver, not complete original pixel/clock parity.
  MCI/AVI integration, actual presentation and output/control timing still
  remain to be implemented/compared. 32/64-bit ALSA clock/observer regressions
  pass after adding read-only summary validation.
- Portable Microsoft ADPCM source decoding now matches actual Wine Win32 ACM
  for both complete original film audio streams. Intro: 1509904 stereo frames/
  6039616 PCM16LE bytes; Outro: 1598960 frames/6395840 bytes, both at 22050Hz.
  Native ASan/UBSan and WASM directly compare every source byte; malformed
  partial blocks, incomplete formats and invalid predictor indices fail
  before creating output, with no sanitizer error. Changed decoded samples
  are rejected (/tmp/dd2-movie-audio-negative-reviewed/report.json).
  make verify-movie-audio MOVIE_AUDIO_ARGS='--mingw <compiler> --ffmpeg ffmpeg
  --output <fresh-dir>' reproduces this. FFmpeg's default audio matches the
  actual ACM source in these films (its video conversion differs).
  Both movie verifiers share one strict RIFF iterator, and native sanitizer
  failures are fatal. The refactored video verifier passes all 3525 frames/
  649728000 RGB bytes again (/tmp/dd2-movie-codec-riff-reviewed/report.json).
  dd2_msadpcm.c/h and dd2_cinepak.c/h remain decoder components pending AVI
  transport/startup/presentation integration; source codec parity does not
  establish original movie A/V delivery, scaling, clocks or output resampling.
- A portable Cinepak source decoder now matches actual Wine Win32 ICCVID for
  every Intro/Outro frame. The same original compressed packet sequence feeds
  ICDecompress and native ASan/UBSan/WASM; every normalized RGB24 byte is
  compared directly, before saving hashes/checkpoints and deleting large
  matching streams. Intro: 1712 frames/315555840 bytes; Outro: 1813 frames/
  334172160 bytes, including all 12 empty hold packets. All 649728000 bytes
  match both targets (/tmp/dd2-movie-codec-reviewed/report.json).
  Cinepak codebooks/interframes, both-strip inheritance, flag-word crossings,
  vector-chunk padding and signed/clipped chroma conversion are exercised.
  Wine green uses ceil(U/2); FFmpeg's default conversion differs already at
  intro RGB byte8017 (Wine13 vs FFmpeg14), so an unverified FFmpeg RGB path
  would not preserve this decoder's source values. Changed decoded pixels
  are rejected. make verify-movie-codec MOVIE_CODEC_ARGS='--mingw <compiler>
  --ffmpeg ffmpeg --output <fresh-dir>' reproduces the reference run.
  dd2_cinepak.c/h is a component for the pending AVI backend; normal game
  startup/transport still does not play movies. This verifies the real Wine
  codec's source frames, not original Windows hardware, original dd2h.exe's
  movie presentation/scaling/timing or ADPCM/audio output. Those remain open.
- Rebuilt movie-parameter targets pass all ten complete sound-enabled demos:
  15255 presented frames/palettes, RNG/flip logs and every effects, final mix,
  music-summand and raw CD byte (/tmp/dd2-parity-movie-params/results.json).
  All eight existing original L9 video/palette checkpoints still match both
  targets. Actual native SDL/X11 menu/CD/Configuration/Joystick/live-driving
  and pause/resume pass ten rendering checkpoints (3072000 pixels) and
  8376176 exact accepted mixed bytes; Stop/TO-only restarts source at sector
  45582/sample26802216 instead of the fractional stop sample26802569, and
  the next 2352 CD bytes match (/tmp/dd2-native-movie-params-reviewed/report.json).
  Browser live pause/resume restarts relative frame552617 at frame552132:
  68620 exact resumed source bytes and 1672 exact shared WebAudio buffers,
  with no missing/extra/differing buffers or runtime errors
  (/tmp/dd2-browser-movie-params-reviewed/report.json).
  Canonical /tmp/dd2_native and /tmp/lvltest/dd2run.js/.wasm now contain these
  validated builds; native SHA
  d7a8c076d2df56dabbe13a194a2a07708da4a81befd276546ffbc1c3e651c52d.
  Exact patch context includes upstream trailing spaces, so .gitattributes
  exempts only patch files' end-of-line whitespace from Git's warning; all
  176 patches still apply with -F0, and the repeated movie fixture retains
  the same function hash/records (/tmp/dd2-movie-params-patch-format-reviewed).
  These regressions do not establish full original A/V or movie playback.
- Play_Movie's MCI blocks are restored by exact patch839 against original
  414e80..414f75: Open type/filename/returned ID, destination window and
  rectangle, playback callback/zero fields and Close callback now occupy
  their original contiguous offsets. The old extracted function fails with
  an ASan stack-buffer-overflow when MCI reads Open's type at +8 from its
  four-byte local (/tmp/dd2-movie-params-before-actual/native-asan-run.log).
  make verify-movie-params MOVIE_PARAMS_ARGS='--mingw <compiler> --output
  <fresh-dir>' validates real 32-bit WinMM header offsets and nine independent
  mocked call paths on native ASan/WASM: 640/non-640 success, completion pump,
  each early-error cleanup and ignored Close error
  (/tmp/dd2-movie-params-reviewed/report.json). All 176 exact patches and native,
  Node/browser builds pass; eight L9 original framebuffer/palette checkpoints
  still match both rebuilt ports. This restores the engine call contract;
  the platform's AVI decoder and full intro/outro A/V are still absent, and
  normal native/browser startup currently bypasses Play_Intro. This mock
  does not establish actual movie API, decoder or notification delivery.
- Finite CD ranges now have a reproducible Wine-reference limitation probe:
  make verify-redbook-end REDBOOK_END_ARGS='--wine --mingw <compiler> --output
  <fresh-dir>'. Eight ranges at track2+450 sectors exercise 8/39/40/50/52/53/
  65/66 sectors, using the same MCI fixture and one retained DirectSound device.
  Actual Wine accepted audio is a literal source prefix in every case, but
  four ranges lose their tail: the reviewed 50-sector run delivers 23370 of
  29400 stereo frames (6030 missing). Exact cutoff counts vary with its worker
  scheduling; no waveform phase or tolerance is fitted. The eight-sector
  range still reports PLAY at 200ms after its 4704 source frames have ended.
  Wine MCICDA_playLoop's three 13-sector fragments explain these observations;
  they are Wine driver behavior, not evidence about original Windows hardware.
  Both ports preserve every requested sample: 877296 complete raw CD bytes and
  all 4304160 controlled mixed bytes, including stopped silence and expected
  modes. An altered accepted Wine bit is rejected
  (/tmp/dd2-redbook-end-reviewed/report.json). This diagnostic deliberately
  reports the missing reference tail instead of claiming full Wine/port parity
  or reproducing that driver loss in the port. Full original A/V acceptance
  still requires an adequate original CD transport/clock reference.
- Native/browser real race pause/resume now verify the CD sector restart end
  to end. The new native read-only check fails the preceding binary after real
  X11 Escape/Return (stopped/origin=26779857, still fractional;
  /tmp/dd2-native-cd-restart-before.log). The fixed binary stops at 26767025,
  resumes at sector45522/sample26766936, repeats the 89 fractional frames and
  reads the exact next 2352 bytes of the original CD source. Menus, live SDL
  controller/physics and ten renderer checkpoints (3072000 pixels) pass, with
  7549560 exact accepted C-mix bytes
  (/tmp/dd2-native-cd-restart-reviewed/report.json). It retries ordinary
  pause/resume if a stop lands exactly on a sector, without engine writes.
  Browser live keys stop at relative source frame206300 and restart205800:
  67208 exact resumed CDDA bytes, all 1401 shared WebAudio buffers match C,
  no missing/extra/differing buffers or runtime errors, race/CD frozen during
  pause (/tmp/dd2-browser-cd-restart-reviewed/report.json).
  make verify-browser-redbook-restart supports BROWSER_RESTART_OUTPUT=<dir>.
  The rebuilt native/Node pair passes all ten full demos again: 15255 presented
  frames/palettes, RNG/flip logs and every effects/mix/music/raw-CD byte
  (/tmp/dd2-parity-redbook-sector/results.json). Native SHA
  6534927544c283e64c8771036d263d8497511eb74b504d9abe9cf92c29aeba44.
  Canonical /tmp/dd2_native and /tmp/lvltest/dd2run.js/.wasm now contain these
  validated sector-restart builds. All eight existing original L9 checkpoints
  at cf1/25/50/150/300/450/600/650 still match framebuffer and palette exactly
  on both rebuilt ports (/tmp/dd2-redbook-sector-original-video.log).
  These establish exercised input/control/source/output chains, not physical
  hardware or the complete original live Q-channel/queue timing.
- Original-style CD_Pause/Restart (Stop then Play with TO only) now restarts
  at the public whole CD sector, rather than the retained fractional sample.
  Original 4162e4/416314 and Wine MCICDA_Play's Q/MSF conversion establish the
  operation. A live Wine fixture with an explicit declared Q snapshot passes
  while the preceding native run resumes 338 stereo frames beyond that sector
  (/tmp/dd2-redbook-restart-sector-before/wine-proof.json and native-source.pcm).
  Both fixed ports match 70736 source bytes and all 317872 controlled mixed
  bytes; actual Wine prefixes match both literal source starts. The verifier
  rejects the old fractional restart and one altered accepted PCM bit
  (/tmp/dd2-redbook-restart-sector-negative-reviewed/report.json).
  make verify-redbook-restart supports REDBOOK_RESTART_ARGS='--wine --mingw
  <compiler> --output <dir>'. True MCI Pause/Resume still retain their buffer's
  sample cursor. All 18 Redbook Stop/TO-only expectations now include fractional
  sector repetition; full track/cross-track, completion controls, shared mixer,
  menu audio and DS cursor regressions pass
  (/tmp/dd2-redbook-sector-regression.log).
  The optional read-only DD2_CD_Q_POSITION device input contains absolute LBA
  and Linux audio status; 108 Q snapshots/positions and malformed/missing input
  tests pass. Wine ntdll ignores supplied addresses on AUDIO_NO_STATUS and
  clears its cached position. The fixture explicitly observes PLAY then
  COMPLETED snapshots to retain a known sector through Stop; no live transport
  clock or queue/control timing is established. Normal reference game captures
  still report NO_STATUS and remain invalid for full pause/resume acceptance.
- Original video capture now accepts --frames for several L9 checkpoints in
  one unmodified attract run; each port compares all counters in one run too.
  It waits for the actual green-light gate, because the countdown resets cf.
  Original Play_Game@423b50 budgets 1500 physics steps and divides the counter
  by two, so unreachable attract requests beyond cf700 now fail before Wine
  launches. Manifests are published only when all requested captures succeed;
  incomplete or mismatched metadata is rejected. Eight actual original
  checkpoints at cf1/25/50/150/300/450/600/650 match every framebuffer/palette
  byte on both ports (2457600 pixels and 8192 palette bytes per target;
  /tmp/dd2-original-l9-complete-checkpoints-reviewed/video-comparison.json).
  These are checkpoints on L9, not all original frames/tracks or audio timing.
  After the MCI completion fix, rebuilt native/Node/browser pass all ten full
  sound-enabled demos: 15255 frames/palettes, RNG/flip logs and all effects,
  final mix, music and raw CD bytes
  (/tmp/dd2-parity-redbook-controls/results.json). Browser CD/menu checks
  accept 3807/1469 exact combined WebAudio buffers with no missing/extra or
  differing buffers. Actual native SDL/X11 menus, CD, virtual controller,
  live physics and pause/resume pass ten rendering checkpoints (3072000
  pixels) and 7411264 accepted audio bytes matching the declared C-mix prefix
  (/tmp/dd2-native-redbook-controls-pad-reviewed/report.json). Canonical
  /tmp/dd2_native and /tmp/lvltest/dd2run.js/.wasm now hold these validated
  builds; native SHA 0692515984f2e3994ecab6af354f49ec103b66e7e5b84655fac8322ff400f40c.
- MCI Pause/Resume now preserve a completed transport's stopped state instead
  of reporting Pause/Play and restarting its retained source at a reset DS
  cursor. A fresh real Wine 10 fixture confirms all 11 drained/live and
  repeated-control records; the preceding native fixture failed four drained
  controls (/tmp/dd2-redbook-controls-before/native-records.json). Both fixed
  ports match those records, 50568 exact source PCM bytes and the entire
  515088-byte mixed/music stream, including completion/paused silence
  (/tmp/dd2-redbook-controls-mixed-reviewed/report.json). Explicit Play still
  creates a new transport. make verify-redbook-controls can run the same
  fixture with REDBOOK_CONTROLS_ARGS='--wine --mingw <compiler> --output <dir>'.
  Redbook all-18/full-track, shared ordered mix, menu audio and DS cursor
  regressions pass (/tmp/dd2-redbook-controls-regression.log). The one-second
  drain wait tests the final state, not Wine's asynchronous CD ring-end timing;
  complete original game A/V and Stop/TO-only Q-channel resume remain open.
- Native window QA's --controller option now attaches a virtual SDL device
  before boot, selects Configuration -> Control Method -> Joystick through
  real X11 keys, and drives the actual player through SDL axes/buttons.
  The separate test-only input driver uses SDL APIs; the output observer
  remains read-only and neither writes engine memory. Full-left/right raw
  bits and endpoints, live steering -512/496, throttle +32768/-32768/0,
  actual world movement, release and keyboard pause/resume all pass. Ten
  rendered checkpoints match 3072000 source pixels; the declared complete
  accepted SDL audio prefix matches 7380576 C-mix bytes
  (/tmp/dd2-native-sdl-pad-engine-reviewed/report.json). Native controllers
  honor the original Control Method choice; select Joystick in Configuration.
  Physical controller hardware and full original A/V timing remain unproved.
- make verify-browser-pad now explicitly launches a live L9 race and waits
  for the actual countdown/control gate. The old probe stayed in the default
  frontend, timed out without failing and printed heading/height as X/Z.
  Corrected probes use original world X/Z offsets 792a30/792a38; packed
  steering/throttle DWORDs and the 434-byte car stride use DataView reads.
  Actual Gamepad API polling passes boot detection, left/right endpoints,
  raw controller bits, steering -512/496, throttle +32768/-32768/0,
  player movement, release and no browser runtime errors
  (/tmp/dd2-browser-pad-steering-reviewed.log). The matrix now also waits
  for the countdown instead of assuming cf200, which can precede the green
  light and its cf reset under adaptive frame skips. This establishes the
  synthetic input-to-physics chain; physical HID and original-input/timing
  equivalence still require reference tests.
- Native SDL play is available through make play-native (DD2_WINDOW=1).
  It presents the existing indexed framebuffer/palette via an exact software
  renderer, queues the final shared Float32 mix, forwards physical SDL keys
  through Translate_Keypress/GetKeyState and detects a controller at boot.
  make provision-native extracts Debian i386 SDL headers locally; the i386
  SDL runtime is required. DD2_BUILD_HEADLESS=1 retains builds without SDL.
  Native ELF dependencies are isolated from reconstructed CRT definitions:
  the engine's unused unlink stub otherwise intercepted real POSIX unlink.
  Unix SIGINT/SIGTERM retain their normal behavior; window close follows the
  original WM_CLOSE handler. The handwritten mixer uses -O2 without fast-math
  or strict-alias assumptions: -O0 could take longer than the elapsed audio
  block, feed back into the next block and stall races at cf2.
  make verify-native-sdl passes 614400 exact renderer AND actual X11 pixels,
  7976 accepted Float32 bytes (including amplitudes outside [-1,1]), keyboard
  and a boot-time virtual controller. make verify-native-window uses actual
  X11 input to check menus, CD Play/Stop, a 20-car race, actual player movement
  after the countdown and pause/resume. Nine presentations match 2764800
  source pixels; 5770392 accepted SDL audio bytes match the complete declared
  C-mix prefix (/tmp/dd2-native-sdl-window-world-reviewed/report.json).
  Startup waits for the child's executable before opening proc mem: early
  descriptors could permanently pin Python's old mm (the observed garbage
  exactly matched /usr/bin/python3.13 code). Input QA acknowledges ReadPad
  and waits for the actual slab bounce, current keymap and countdown gate.
  Original assembly 442dc0..442de7 confirms world X/Z at 792a30/792a38;
  the older pad probes measured heading/height at 792a24/792a34 instead.
  The optimized native resampler still matches actual Wine hashes at all
  three device rates and the 1084900-result x87/WASM oracle. Cursor, gain,
  ownership, menu audio, ordered shared mix and Redbook regressions pass.
  Rebuilt all targets: original L9/cf150 pixels/palette remain exact; browser
  CD/menu output checks 3666/1184 exact combined buffers, with none missing
  or extra. All ten complete optimized-native/Node demos again match 15255
  frames/palettes, RNG/flip logs and every effects/combined/music/CD byte
  (/tmp/dd2-parity-native-sdl-mixopt/results.json). SDL dummy audio acceptance
  and a virtual controller do not prove physical hardware or full original
  A/V timing; the overall goal remains active.
- The original frontend-exit snapshot now excludes seven executable action
  slots. These historical entries undid startup relocation after newer menu
  handlers were registered: real native CD Play jumped to PE VA 0x45220c.
  A hardware watchpoint pinpoints dd2_apply_frontend_state's memcpy, and the
  corrected boot retains the actual FUN_0045220c pointer. The six currently
  registered affected actions are checked by native window QA. Real native
  CD Play/Stop now work; both rebuilt ports retain exact original L9/cf150
  pixels/palette and complete ten-track demo parity (15255 frames). The
  snapshot-only regression and optimized SDL build both pass all ten tracks;
  complete original A/V remains separate
  work (/tmp/dd2-native-cd-relocation-watch.log,
  /tmp/dd2-native-cd-relocation-fixed.log,
  /tmp/dd2-native-sdl-festate-original-video.log).
- CD-menu browser QA now waits for the real slab rotation/bounce to finish,
  acknowledges ReadPad press/release and checks each Prev selection before
  issuing the next input. Fixed-duration pulses could land in transition
  frames and falsely report missing actions under load. The updated test
  passes Play/Stop/Next/Prev, exact track2/3 source parts and 2920 actual
  combined WebAudio buffers with zero missing, extra or differing buffers
  (/tmp/dd2-native-sdl-browser-cd-ack.log). This changes test input timing,
  not engine behavior or waveform alignment.
- DirectSound duplicates now own a reference to shared PCM storage independently
  of the original COM buffer. Previously releasing the original then duplicating
  a survivor read freed memory in ds_dupbuffer; the actual Wine fixture passed
  while native ASan failed (/tmp/dd2-sound-lifetime-before.log). DS buffer
  AddRef/Release now retains the object until its last reference, and storage
  remains until its last sharing buffer is released. make verify-sound-lifetime
  exercises two original duplicates, original release, another duplicate from
  a survivor, shared mutations, independent stopped cursors, release ordering
  and retained/final COM reference counts. Both ports then play 3528 exact PCM
  bytes from the surviving buffer. Optional SOUND_LIFETIME_ARGS='--wine --asan
  --mingw <32-bit compiler> --report <path>' compares the same COM operations
  with actual Wine and checks native ownership under ASan. The fixed real-Wine/
  ASan/WASM run passes (/tmp/dd2-sound-lifetime-reviewed.json, including fixture
  source/EXE hashes and Wine version). Cursor, gain, shared
  CD/effects, Redbook boundary and all resampler-rate regressions still pass
  (/tmp/dd2-sound-lifetime-regression.log). Rebuilt native, Node and browser;
  original L9/cf150 pixels/palette remain exact on both ports. Browser actual
  combined output matches C on 3945 CD-menu buffers and 1279 menu-navigation
  buffers, with no missing buffers or bit mismatches. All ten complete
  sound-enabled demos again match: 15255 presented frames/palettes and all
  RNG/flip logs; each demo has 10569888 exact bytes in each of effects, combined
  and music-part Float32 captures, plus 4939200 raw CD bytes and format metadata
  (/tmp/dd2-parity-sound-lifetime/results.json). Canonical native/Node binaries
  now contain the validated lifetime fix. This corrects source/object lifetime;
  complete original stream/timing and hardware verification remain open.
- CD and effects now share the ordered C Float32 device, one sample clock and
  one WebAudio scheduling cursor/sink. MCI Play creates a CD source in the
  buffer list; Stop releases it, Pause retains its cursor. Two read-only CD
  pages cover source/FIR lookahead across page and track boundaries. Legacy
  DD2_SNDPCM remains effects-only and DD2_CDPCM remains consumed s16 CD source;
  DD2_MIXPCM is the final ordered device output, DD2_MUSICPCM its music part,
  both Float32 with rate/format sidecars. verify-parity now compares these
  additional complete streams and metadata as well as video/palettes/RNG.
  make verify-shared-audio checks a real WinMM/DirectSound fixture with an
  effect created before the CD buffer and another after it. Actual Wine 10
  captures match 27740 consecutive mixed waveform frames, including 2995
  frames that distinguish source summation order. A fresh --wine rerun passes
  27741 frames; timer-driven leading/trailing control phases are explicitly
  reported, not aligned away or claimed identical. Both ports also match the
  entire controlled 960ms output (338688 Float32 bytes), 30870 CD source frames
  and complete music summand. Calibration stores hashes/metadata only in
  tools/reference/shared_audio_wine10.json; game PCM stays outside Git.
  Logs: /tmp/dd2-shared-audio-wine-reviewed/report.json and
  /tmp/dd2-shared-audio-wine-recheck/report.json. Altered bits, reordered sums
  and lost CD are rejected. Optional SHARED_AUDIO_ARGS='--wine --mingw
  <32-bit compiler> --output <fresh-dir>' repeats the actual reference.
  Browser CD menu passes Play/Stop/Next/Prev and exact track2/3 source parts;
  all 4208 actually submitted mixed buffers match C bits, with zero missing
  or extra buffers and no separate CD cursor (/tmp/dd2-shared-mix-browser-cd.log).
  Menu navigation separately checks 1494 exact buffers while cf stays zero
  (/tmp/dd2-shared-mix-browser-menu.log). Redbook's 29821008 source bytes,
  cursor/menu/gain fixtures, all resampler device rates and original L9/cf150
  video still pass. All ten complete sound-enabled demos match on both ports:
  15255 presented frames/palettes and every RNG/flip log; each demo has
  10569888 effects bytes, 10569888 combined bytes, 10569888 music-part bytes
  and 4939200 raw CD bytes, with exact format sidecars
  (/tmp/dd2-parity-shared-mix/results.json). The earlier MCI entry now starts
  the common clock before the first Draw_All; this captures one additional
  40ms device tick compared with the previous separate clocks. Native/WASM
  equality does not prove that this start time matches the original device.
  This resolves separate CD/effects mixing, not
  full original game streams: Wine's 91728-byte CD ring/worker/end behavior,
  queued controls, device start timing, native hardware output and complete
  Windows hardware equivalence remain open.
  Additional Redbook regressions exercise MCI_PAUSE/MCI_RESUME with a 911ms
  device-clock advance during pause, and a four-sector interval crossing from
  the end of track02 into track03. Both ports preserve the exact source bytes
  through the new page reader and stop at the requested end: 29848052 bytes
  total (/tmp/dd2-shared-mix-redbook-boundary.log). These are explicit-clock
  source tests, not a Wine worker/queued-control timing claim.
- The effects device now defaults to Float32 stereo 44100Hz, matching the
  observed original reference device and CD source rate. Previously C emitted
  22050Hz and WebAudio resampled it again to 44100Hz. Source buffers retain
  their original rates; the calibrated C FIR performs the single conversion.
  DD2_SND_RATE selects 22050/44100/48000 for device calibration. Cursor phases,
  elapsed-time remainders, 40ms ticks, PCM metadata and WebAudio buffers all
  use that rate (default tick 1764 frames). make verify-sound-resample now
  checks 24 complete one-shots and six loop cycles against actual Wine captures
  at all three rates: 190977 active one-shot frames per target, with no byte
  tolerance (/tmp/dd2-device-rates-port-reviewed/report.json). New actual Wine
  captures are in /tmp/dd2-resample-device-rates-calibration; an independent
  --wine --device-rates 44100 rerun also passes every case and both loops
  (/tmp/dd2-device44-wine-reviewed/report.json). CPU x87/WASM comparison now
  checks 1084900 results, including all remainders at all three device rates.
  Existing cursor/menu/gain fixtures explicitly use their established 22050Hz
  device and still pass. Their standalone Node builders now import DD2_* via
  tools/node_env.js, just like the full Node build; otherwise that rate option
  was silently ignored by libc ENV.
  Rebuilt all targets. Original L9/cf150 framebuffer/palette remain exact
  (/tmp/dd2-device44-original-video.log). Browser effects compare actual channel
  bits with the C buffer and require exactly one delivered buffer per running
  mixer import: 1105 buffers, 417097 frames, zero mismatches/missing buffers,
  source/device rate 44100Hz, nonzero effects at cf=0
  (/tmp/dd2-device44-browser-audio-reviewed.log). The CD menu still delivers
  exact source bytes for tracks 2/3 and Play/Stop/Next/Prev all pass
  (/tmp/dd2-device44-browser-cd.log). The CD observer identifies effects through
  their actual WASM import rather than assuming every 44100Hz buffer is music.
  CD and effects still use separate scheduling cursors; complete mixed PCM,
  queued controls, stream-start timing and original full-output parity remain open.
  All ten complete sound-enabled native/WASM demos pass at the new default
  44100Hz device: 15255 presented frames/palettes, every RNG/flip log, 10555776
  Float32 effects bytes and 4939200 CD source bytes per demo, including format
  sidecars (/tmp/dd2-parity-device44/results.json). This compares the two ports;
  the original video proof remains the L9 checkpoint above, and the original
  audio proof remains the actual captured synthetic waveform/control fixtures.
- Effects resampling now follows the observed Wine 10 FIR instead of the Q16
  point sampler. Initially calibrated at 22050Hz, the cursor uses an exact
  remainder / device rate; seeking retains
  that remainder, and the equal-rate copy path leaves it unchanged. Loops wrap
  the forward filter window; one-shots zero-pad it. Gaussian-windowed sinc
  coefficients are generated mathematically by tools/generate_sound_fir.py,
  with the reference generator's ten-decimal rounding (7907 coefficients).
  The installed i386 Wine mixer accumulates with x87 precision: f32 summation
  differed on the first sample (0.6078085899 versus 0.6078086495). A portable
  explicit 64-significand-bit implementation now reproduces those operations
  on native/WASM, including f32 coefficient/rem spills. Its actual CPU x87
  oracle compares 532300 results, including every device-rate fractional
  remainder at three indices (/tmp/dd2-fir-final-port-tests/report.json).
  make verify-sound-resample checks eight complete nonconstant one-shot waves
  against recorded real Wine hashes (36890 active stereo frames), plus exact
  11025/176400Hz short-loop cycles. Optional SOUND_RESAMPLE_ARGS='--wine --mingw
  <32-bit compiler> --output <fresh-dir>' recaptures actual Wine PCM and compares
  every active byte; /tmp/dd2-fir-wine-reviewed/report.json passes all ten cases.
  Altered bits, shortened/duplicated waveforms and corrupt loop cycles fail.
  Device preroll/trailing silence is checked and reported separately; loop
  tone durations are not claimed equal. These are synthetic API fixtures, not
  the original game. Updated menu/cursor tests use the independently captured
  Wine filter cycles rather than point-sampler expectations. Menu/cursor/gain
  regressions pass. Rebuilt native, Node and browser; original L9/cf150 pixels
  and palette still match on both ports (/tmp/dd2-fir-original-video.log).
  Actual browser channel data matches the C mixer on 1169 buffers, 231547
  frames, zero mismatches, with nonzero effects at cf=0
  (/tmp/dd2-fir-browser-audio.log). Full original streams, frequency/control
  changes with queued audio, combined CD/effects, original timing and Windows
  hardware fidelity remain open; this calibrates the exercised Wine backend.
  All ten complete sound-enabled native/WASM demos pass after the FIR change:
  15255 presented frames/palettes, all RNG/flip logs, 5277888 Float32 effects
  bytes and 4939200 CD source bytes per demo (/tmp/dd2-parity-fir/results.json).
  Fresh full Configuration/Audio Volume highlight cycles also match the
  original on both targets: 448 framebuffer/palette pairs per target, zero
  differing bytes (/tmp/dd2-config-fir-comparison.json; captures under
  /tmp/dd2-native-config-fir and /tmp/dd2-browser-config-fir).
- Effects Float32 LE stereo was initially calibrated at 22050Hz (DD2_SNDPCM plus a format
  .json sidecar), preserving Wine's observed quantized gain and unclipped sums.
  The prior Q15/16-bit implementation produced 0.2505798340 at volume -600 for
  a constant 0.5 source; actual Wine PCM is 0.2499961853. Table generation uses
  high-precision Decimal to reproduce all 9600 integer attenuation steps; gain
  -9600 and below quantizes to zero. Products are rounded to f32 before summing
  so x87 and WASM agree. re_out/dd2_sound_gain.h is generated platform code;
  no pristine engine source was changed. make verify-sound-gain checks all
  10001 volume values, six pans and a three-buffer sum above 1.0. Optional
  SOUND_GAIN_ARGS='--wine --mingw <32-bit compiler> --output <fresh-dir>' captures
  actual Wine PCM through the clocked ALSA device, scoped to gain.exe. Default
  original capture scope remains dd2h.exe. /tmp/dd2-gain-wine-float-reviewed/report.json
  passes 13 gain/pan cases, unsigned mono8 normalization and a real three-source
  mixed plateau, with matching
  source/device rates (22050Hz; no resampler). It also exercises settings queries,
  caps/errors, DSBFREQUENCY_ORIGINAL and configured duplicate properties. The
  reference validators reject an altered bit, all-silent audible tone and clipped
  sum (automatically checked in the reviewed --wine run).
  This proves amplitudes and exercised controls, not stream-start alignment,
  full original game PCM, combined CD/effects or Windows hardware equality.
  FIR calibration is documented above; separate CD/sound scheduling remains open.
  Rebuilt all three targets. All ten complete sound-enabled native/WASM demos
  match: 15255 presented frames/palettes, RNG/flip logs, 5277888 Float32 effects
  PCM bytes and 4939200 CD source PCM bytes per demo
  (/tmp/dd2-parity-float-gain/results.json). The new standalone Float32 menu and
  cursor tests check 9696 and 2992 bytes respectively (older sizes below describe
  the previous s16 output). L9/cf150 still matches the original on both targets.
  Fresh Configuration/Audio Volume cycles match 448 original framebuffer/palette
  pairs per target (/tmp/dd2-config-float-gain-comparison.json). Browser real menu
  audio compares actual WebAudio channel data bit-for-bit with the C Float32
  buffer through a read-only WASM import observer: 1050 buffers, zero mismatches,
  nonzero effects, cf=0, shared device 44100Hz
  (/tmp/dd2-gain-float-browser-exact-audio.log).
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
  browser and native now match all 20 original names/points and all four pages' framebuffer
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
  original. tools/capture_native_menu.py now drives the native full frontend with
  its normal dd2_key_event bridge (one ReadPad call per held key) and captures
  Draw_All entry. Native Championship/Go/Pause/Retire/Yes/View League matches
  all four original pages byte-exact: /tmp/dd2-native-champ-live;
  /tmp/dd2-champ-native-verification.log. CHAMPNATIVE adds this comparison to
  verify-champ-names and requires a real race, Retire confirmation and results.
  Native window-system input/hardware audio, full seasons/promotion/relegation,
  other menu reports and complete original streams remain open. All ten native/WASM
  demos still match after patch 838: /tmp/dd2-parity-champ-names/results.json.
  New menu-cycle captures compare all 64 highlight phases at presentation, with
  no pixel masks/tolerance. Main-menu Down/Right/Right -> Configuration -> Right
  -> Audio Volume matches original/native/browser on all seven checkpoints:
  448 frames per port, every framebuffer/palette byte. Artifacts:
  /tmp/dd2-original-config-presented, /tmp/dd2-native-config-presented,
  /tmp/dd2-browser-config-presented; /tmp/dd2-config-presented-comparison.json.
  Browser capture checks actual canvas ImageData against indexed pixels/palette.
  tools/menu_cycle_gdb.py uses original hardware-only breakpoints. Draw_All
  presents the previous framebuffer before rasterizing current OT; return-stage
  capture was one image ahead of browser presentation (all 16 color-transition
  phases per cycle failed). Entry-stage captures align exactly without shifting
  recorded phases. Reproduce via --menu-cycle 64 on reference/native capture,
  tools/browser/capture_menu_cycle.js and make verify-menu-cycles. This proves
  the rendered highlight cycle on this path only; input-repeat/wall-clock/audio
  timing, other animations and reported menu-action failures remain open.
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
  The default adapter supplies a track2 Q address with AUDIO_NO_STATUS; Wine
  ntdll clears its Q cache and ignores that address. TO-only MCI_PLAY therefore
  cannot establish original live pause/restart cursor fidelity. Explicit Q
  sector/status inputs are available only for controlled API fixtures.
  Historical claims below are investigation context, not full acceptance evidence.
- Original mixed PCM can now be observed at Wine's actual `snd_pcm_writei`, with
  accepted-write extents, format, SHA256, monotonic call bounds and transport
  journals. `tools/reference/wine_audio.c` forwards the real ALSA functions and
  captures only the dd2h.exe process. `alsa_clock.c` supplies a private stereo
  device with a CLOCK_MONOTONIC sample clock; it discards device output and leaves
  Wine's DirectSound mixer intact. Both libraries build for 32/64-bit Wine.
  `make verify-audio-observer` passes real ALSA tests for exact 512-byte payloads,
  failed-write exclusion, two fresh streams, out-of-scope passthrough, six damaged
  capture cases, and reused-output rejection. Clock tests check exact timestamp
  bounds, polling, pause/resume, rewind, prepare reset and genuine underrun state.
  /tmp/dd2-original-audio-clock-timeline captures the unmodified EXE without GDB:
  stereo 44100Hz FLOAT_LE, 136701 accepted frames (3.099795918s) over 3.060396919s
  observed wall time. The former ALSA null device accepted 9.2625s in 2.5728s and
  cannot establish timing fidelity. Live state observations record the menu-to-L9
  transition; they are non-atomic, not a complete frame trace. Accepted/queued PCM
  is not a played-output timeline: apply drops/rewinds and align actual inputs,
  source cursors and clock before port comparison. Full original mixed PCM parity,
  Windows hardware equivalence and complete A/V timing remain unverified.
  Regression with audio enabled: /tmp/dd2-original-config-audio-clock records
  all seven Configuration/Audio Volume checkpoints and their 64 presented
  highlight phases. /tmp/dd2-config-audio-clock-comparison.json reports zero
  framebuffer/palette mismatches for all 448 comparisons on each of native and
  browser. Debugger pauses in this video regression do not prove audio timing.
- DirectSound cursor controls were incorrect: SetCurrentPosition ignored its
  offset, and every Play reset source position. The runtime now seeks/aligns byte
  offsets, preserves position across repeated Play and Stop/resume, implements
  GetCurrentPosition, resets at automatic one-shot end, and wraps a frequency step
  spanning several short loops. GetStatus follows actual sample consumption;
  the old cf-duration fallback could stop a live seeked/frequency-changed buffer.
  Original Play_Sound explicitly seeks zero at 0x415e19-0x415e21. No engine patch
  is required: these are handwritten platform-shim corrections.
  tools/sound_cursor_test.c runs the same stopped-buffer COM probe against real
  Wine DirectSound and native/WASM. The pre-fix run failed on the selected seek
  (/tmp/dd2-sound-cursor-before.log) after the Wine reference passed. The fixed
  /tmp/dd2-sound-cursor-wine-final.json passes all 14 mono8/stereo16 cursor/error
  records and 1496 exact controlled PCM bytes on each port. Reproduce via
  make verify-sound-cursor; SOUND_CURSOR_ARGS='--wine --mingw <32-bit MinGW gcc>'
  enables the real API fixture in its own temporary Wine prefix. The existing
  55ms menu test still matches 4848 bytes after explicitly seeking zero for replay.
  These tests verify exercised API transport, not full original PCM/timing parity.
  Rebuilt native, Node and browser. All ten sound-enabled native/WASM demos remain
  byte-exact: 15255 presented frames per target with palettes/RNG/flip logs,
  2638944 effect PCM bytes and 4939200 CD PCM bytes per demo
  (/tmp/dd2-parity-sound-cursor/results.json). Original L9/cf150 still matches
  both ports (/tmp/dd2-sound-cursor-original-video.log). Fresh native and browser
  Configuration/Audio Volume captures match all 448 original presented phases
  per target (/tmp/dd2-config-cursor-fixed-comparison.json). Real browser menu
  effects still deliver nonzero samples at cf=0
  (/tmp/dd2-sound-cursor-browser-audio.log).

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
  WIN pixels. **L9 attract audio evidence is limited to port PCM and game-side calls:** DirectSound COM shim (DD2_SOUND=1) + deterministic
  PCM mixdown (dd2h_stubs.c; engine-frame clock, quantized gain, Float32 FIR and rational cursor;
  DD2_SNDPCM/<path>, DD2_SNDLOG): PCM stream bit-identical across native runs AND native<->wasm;
  game-side DS call stream matches the SOUND-ENABLED wine reference EXACTLY (59/59 race sound-start
  groups incl. every volume/pan/frequency value; Modify_Sound streams exact in every clean
  measurement window — heavier gdb captures distort wine's wallclock channel timing/frame-skip,
  only ~10cf windows with few breakpoints are valid). Audio roots: patches 700-735 (boot order,
  2x scattered DSBUFFERDESC, dropped reg-args, volume-curve log10 reconstruction + CRT log10 body,
  mm-timer callback 0x41345c reconstruction + deterministic 10-cf/phase-5 driver, commentator gate
  = BYTE test of demo counter [0x7746c0] opening every 128 cf). Sound-enabled refs need the ALSA
  null device in those historical captures — current reference tools use a private
  clocked ALSA device and observe actual accepted PCM. Headless Wine without a configured
  device makes DirectSoundCreate fail (all pre-2026-07-03 refs ran no-sound). FAITHFUL BOOT PATH
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
  advanced (GetKeyState stubbed 0 + only ~13 keys forwarded), 830-833 repaired several
  championship failures (Sort_Leagues/Reset_League_Info bounds, View League dispatch,
  initialized sorting storage). The earlier "fully working" verdict exceeded these
  checks. Ordinary finishes, promotion/relegation and subsequent seasons remain open.
  The patch-834 retire-all original-crash attribution is disproved: original WORD
  classification returns elimination 5 for league=3/rank=4 and exits without decrement.
  The port's DWORD league load included rank and returned relegation 3 instead;
  patch 851 repairs that actual cause. The user-sanctioned season>=0 clamp remains
  the explicit deviation for an inconsistent season/league fixture, but is not on
  the genuine bottom-division elimination path. DD2_SEASONEND forces that fixture;
  it does not prove that real original retirement input underflows the season.
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
- Wine gotchas: NEVER `pkill -f dd2h.exe` (kills own shell) → use `pkill -x`; win32 prefixes belong under `/tmp/wasm-dd2/`.
- Full pre-2026-07-01 investigation history is in git history + `/tmp/CLAUDE_full_backup.md`.
