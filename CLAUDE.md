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
- Original accepted PCM: `python3 tools/reference/capture.py --mode audio --audio
  --audio-tail 3 --output /tmp/fresh-original-audio` (no debugger). Observer/clock
  calibration: `make verify-audio-observer`; needs ALSA headers and 32/64-bit runtimes.

## Current acceptance check (2026-10-03, Debian 13)
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
