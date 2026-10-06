# DD2 → dd2h.exe 1:1 rebuild — make-driven pipeline; the target chain IS the documentation:
#
#   make decompile   dd2h.exe --[Ghidra headless]--> re_out/dd2_decomp.c (pristine reference layer)
#   make assemble    re_out/dd2_decomp.c --[assemble_dd2h.py, mechanical]--> re_out/dd2.c (engine unit)
#   make patch       re_out/*.c|h + patches/NNN-*.diff (ordered, commented) --> build/
#   make native      build/ --[gcc -m32]--> $(NATIVE)         (primary debug target)
#   make wasm        build/ --[emcc]--> $(OUTJS)              (browser/node target)
#   make verify / verify-wasm   run all 10 demo levels on the target, print crash-free N/10
#
# Every engine correction is a .diff in patches/ with its rationale in the file header.
# The decompile (re_out/dd2_decomp.c, dd2.c) is committed so `make patch` is reproducible without Ghidra.

ROOT    := $(CURDIR)
OUTJS   ?= /tmp/lvltest/dd2run.js
NODE    := $(firstword $(wildcard $(HOME)/Git/emsdk/node/*/bin/node) node)
GAMEDIR := $(ROOT)/DestructionDerby2
LEVEL   ?= 9
NATIVE  ?= /tmp/dd2_native
RACE_REFERENCE ?= /tmp/wasm-dd2/reference
RACE_COMPARISON ?= /tmp/wasm-dd2/comparison
LOG_MAX_AGE ?= 3600

.PHONY: all pipeline provision provision-native decompile assemble symbols image patch check native play-native wasm web verify verify-native-sdl verify-native-window verify-native-quit verify-browser-pad verify-browser-redbook-restart verify-browser-startup verify-wasm verify-parity run shot refcapture verify-cdrom verify-audio-observer verify-redbook verify-redbook-controls verify-redbook-restart verify-redbook-end verify-movie-params verify-movie-codec verify-movie-audio verify-movie-reference verify-shared-audio verify-menu-audio verify-sound-cursor verify-sound-lifetime verify-sound-device verify-keyboard verify-browser-keyboard verify-browser-keyboard-negative verify-clock-replay verify-random-reference refcapture-race-stream verify-reference-race-stream verify-sound-gain verify-sound-resample verify-menu-cycles verify-champ-names verify-reference-video clean help

all: rewrite-native   ## default on rewrite: readable C + SoftGL native build with strict clang-tidy

.PHONY: rewrite-native rewrite-wasm rewrite-check rewrite-format rewrite-format-check rewrite-tidy
.PHONY: rewrite-archive-verify
.PHONY: rewrite-level-verify rewrite-mesh-verify rewrite-scene-verify rewrite-road-verify rewrite-surface-verify
.PHONY: rewrite-barrier-verify
.PHONY: rewrite-ground-verify
.PHONY: rewrite-driving-verify
.PHONY: rewrite-vehicle-verify
.PHONY: rewrite-play rewrite-web rewrite-window-verify
rewrite-play: rewrite-native ## open the interactive track/car viewer with provisioned original assets
	/tmp/wasm-dd2/rewrite-native/dd2_app "$(GAMEDIR)/Dirinfo" "$(LEVEL)"

rewrite-web: rewrite-wasm ## serve the browser track/car viewer on localhost:8080; select Dirinfo locally
	python3 $(ROOT)/tools/rewrite/serve.py

rewrite-ground-verify: ## verify source body/road contacts and free driving on all targets
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_ground.py
	python3 $(ROOT)/tools/rewrite/verify_driving.py
	python3 $(ROOT)/tools/rewrite/verify_window.py
	$(MAKE) clean-logs

rewrite-barrier-verify: ## verify source walls, continuous contacts and driving response
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_barriers.py
	python3 $(ROOT)/tools/rewrite/verify_driving.py
	python3 $(ROOT)/tools/rewrite/verify_window.py
	$(MAKE) clean-logs

rewrite-driving-verify: ## verify free-driving starts, timing and presentation on native/WASM/sanitized
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_driving.py
	python3 $(ROOT)/tools/rewrite/verify_window.py
	$(MAKE) clean-logs

rewrite-window-verify: ## verify real native/browser presentation, input, resize and lifecycle
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_window.py
	$(MAKE) clean-logs

rewrite-native: ## configure/build the readable rewrite with LLVM 19 and pinned SoftGL
	cmake --preset rewrite-native
	cmake --build --preset rewrite-native

rewrite-wasm: ## configure/build the readable rewrite with Emscripten; native clang-tidy is a separate gate
	bash -c 'source "$(ROOT)/tools/emscripten_env.sh" && python3 "$(ROOT)/tools/rewrite/build_wasm.py"'

rewrite-format: ## format only handwritten rewrite C/header files with clang-format 19
	python3 $(ROOT)/tools/rewrite/quality.py format

rewrite-format-check: ## reject any formatting difference in handwritten rewrite C/header files
	python3 $(ROOT)/tools/rewrite/quality.py format-check

rewrite-tidy: rewrite-native ## analyze every rewrite C unit with all findings treated as errors
	python3 $(ROOT)/tools/rewrite/quality.py tidy --build-dir /tmp/wasm-dd2/rewrite-native

rewrite-check: rewrite-format-check rewrite-native ## strict format/build/analysis gate and native renderer bootstrap
	python3 $(ROOT)/tools/rewrite/quality.py tidy --build-dir /tmp/wasm-dd2/rewrite-native
	ctest --preset rewrite-native

rewrite-archive-verify: ## verify all original archive entries on native, WASM and ASan/UBSan
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_archive.py
	$(MAKE) clean-logs

rewrite-level-verify: ## compare all original level fields and textures on native, WASM and ASan/UBSan
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_levels.py
	$(MAKE) clean-logs

rewrite-mesh-verify: ## compare original scene placements and polygon fields on native, WASM and ASan/UBSan
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_meshes.py
	$(MAKE) clean-logs

rewrite-scene-verify: ## render original scene/car geometry on native, WASM and ASan/UBSan
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_scene_render.py
	$(MAKE) clean-logs

rewrite-road-verify: ## compare all playable road graphs, lane cells and contact planes on native/WASM/ASan
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_roads.py
	$(MAKE) clean-logs

rewrite-vehicle-verify: ## verify fixed-step vehicle dynamics on all original tracks/arenas
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_vehicles.py
	$(MAKE) clean-logs

rewrite-surface-verify: ## verify indexed surface queries against exhaustive original-cell search
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_roads.py
	python3 $(ROOT)/tools/rewrite/verify_surfaces.py
	$(MAKE) clean-logs

pipeline: decompile assemble patch native verify wasm verify-wasm ## FULL from-binary chain: dd2h.exe -> Ghidra -> assemble -> patch -> native+WASM -> 10-level crash test (both targets)
	@echo "pipeline OK: dd2h.exe -> decompile -> assemble -> patch -> compile (native + WASM) -> run"

help:                 ## list targets
	@grep -hE '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN{FS=":.*?## "}{printf "  %-18s %s\n",$$1,$$2}'

provision:            ## download and verify the original game, extract its image and lossless CD music
	python3 $(ROOT)/tools/provision_game.py

provision-native: ## extract Debian i386 SDL headers into ignored third_party without a system installation
	python3 $(ROOT)/tools/native_sdl_config.py provision

decompile: ## re-run Ghidra headless: dd2h.exe -> re_out/dd2_decomp.c
	bash $(ROOT)/tools/decompile.sh
	@echo "decompile: $$(grep -cE '/\* ===== .* @ ' $(ROOT)/re_out/dd2_decomp.c) functions exported"

assemble: ## mechanical: re_out/dd2_decomp.c -> re_out/dd2.c (game-code selection + fwd decls + register-file globals)
	python3 $(ROOT)/tools/assemble_dd2h.py

symbols: ## regenerate re_out/dd2_symbols.h from the Ghidra symbol export (tools/ghidra/ExportSymbols.java -> re_out/data_symbols.txt)
	python3 $(ROOT)/tools/gen_symbols.py $(ROOT)/re_out/data_symbols.txt $(ROOT)/re_out/dd2_symbols.h $(ROOT)/re_out/dd2_symbols.h

image: ## extract the dd2h memory image: dd2h.exe -> re_out/dd2_image.bin + runtime copy in DestructionDerby2/
	python3 $(ROOT)/re_out/extract_image.py $(GAMEDIR)/dd2h.exe $(ROOT)/re_out/dd2_image.bin
	cp $(ROOT)/re_out/dd2_image.bin $(GAMEDIR)/dd2_image.bin

patch: ## apply patches/NNN-*.diff onto re_out/ -> build/ (exact match; drift fails loudly)
	bash $(ROOT)/tools/patch.sh

check: ## dry-run the patch series against the pristine decompile (anchor check)
	@mkdir -p /tmp/wasm-dd2
	@set -eu; check_dir=$$(mktemp -d /tmp/wasm-dd2/patchcheck.XXXXXX); \
	trap 'rm -rf "$$check_dir"' EXIT; \
	cp $(ROOT)/re_out/*.c $(ROOT)/re_out/*.h "$$check_dir/"; \
	for p in $(ROOT)/patches/*.diff; do patch -p1 -s -F0 --fuzz=0 -d "$$check_dir" < $$p || { echo "CHECK FAILED: $$p"; exit 1; }; done; \
	echo "check: all patches apply cleanly"

native: patch ## native 32-bit build (no ASan by default = real crash semantics) -> $(NATIVE)
	ASAN=' ' bash $(ROOT)/tools/build_native.sh $(NATIVE)

play-native: native ## play native in an SDL window with shared Float32 audio and keyboard/boot-time gamepad input
	cd $(GAMEDIR) && DD2_WINDOW=1 DD2_FE=1 $(NATIVE)

wasm: patch ## WASM build -> $(OUTJS)
	bash $(ROOT)/tools/build.sh $(OUTJS)

web: patch ## browser build -> web/dd2
	bash $(ROOT)/tools/build_web.sh web/dd2

verify: native ## crash-free check: run the demo on all 10 levels (native), print N/10
	python3 $(ROOT)/tools/verify_demos.py native --native $(NATIVE) --game-dir $(GAMEDIR)

verify-native-sdl: ## verify actual renderer/X11 pixels, accepted audio and SDL keyboard/virtual-controller input
	python3 $(ROOT)/tools/verify_native_sdl.py $(NATIVE_SDL_ARGS)

.PHONY: verify-native-sdl-clock
verify-native-sdl-clock: patch ## verify recorded/live clocks through actual SDL initialization; NATIVE_SDL_CLOCK_ARGS required
	python3 $(ROOT)/tools/verify_native_sdl_clock.py $(NATIVE_SDL_CLOCK_ARGS)

.PHONY: verify-multimedia-timers
verify-multimedia-timers: patch ## verify original activation/independent timer lifetimes in native/ASan/WASM; MULTIMEDIA_TIMER_ARGS required
	python3 $(ROOT)/tools/verify_multimedia_timers.py $(MULTIMEDIA_TIMER_ARGS)

.PHONY: verify-original-timer-callbacks
verify-original-timer-callbacks: ## verify forwarding observer and original callback counters; ORIGINAL_TIMER_CALLBACK_ARGS required
	python3 $(ROOT)/tools/verify_original_timer_callbacks.py $(ORIGINAL_TIMER_CALLBACK_ARGS)

.PHONY: verify-engine-audio
verify-engine-audio: ## compare actual native/browser engine PCM with observed original services; ENGINE_AUDIO_ARGS required
	python3 $(ROOT)/tools/verify_engine_audio.py $(ENGINE_AUDIO_ARGS)

.PHONY: verify-replay-engine-audio
verify-replay-engine-audio: ## compare actual replay PCM with observed or diagnostic keys; REPLAY_ENGINE_AUDIO_ARGS required
	python3 $(ROOT)/tools/verify_replay_engine_audio.py $(REPLAY_ENGINE_AUDIO_ARGS)

.PHONY: export-original-keyboard
export-original-keyboard: ## export verified original window-procedure key edges; ORIGINAL_KEYBOARD_ARGS required
	python3 $(ROOT)/tools/reference/keyboard_messages.py $(ORIGINAL_KEYBOARD_ARGS)

.PHONY: export-original-video
export-original-video: ## export original successful DirectDraw uploads/palettes; ORIGINAL_VIDEO_ARGS required
	python3 $(ROOT)/tools/reference/video_frames.py $(ORIGINAL_VIDEO_ARGS)

.PHONY: verify-audio-callback-interleave
verify-audio-callback-interleave: patch ## check real suspended callback/main stacks and own status assertions; AUDIO_CALLBACK_INTERLEAVE_ARGS required
	bash -c 'source "$(ROOT)/tools/emscripten_env.sh" && python3 "$(ROOT)/tools/verify_audio_callback_interleave.py" $(AUDIO_CALLBACK_INTERLEAVE_ARGS)'

.PHONY: verify-keyboard-binding-menu
verify-keyboard-binding-menu: ## capture/compare actual original/native/browser key bindings and exact menu cycles; KEYBOARD_BINDING_MENU_ARGS required
	python3 $(ROOT)/tools/verify_keyboard_binding_menu.py $(KEYBOARD_BINDING_MENU_ARGS)

.PHONY: verify-configuration-persistence capture-browser-configuration-persistence
verify-configuration-persistence: ## capture/compare actual saved configuration, process restart and remapped racing input; CONFIGURATION_PERSISTENCE_ARGS required
	python3 $(ROOT)/tools/verify_configuration_persistence.py $(CONFIGURATION_PERSISTENCE_ARGS)

capture-browser-configuration-persistence: ## capture actual browser configuration save, Chromium restart and remapped racing input; BROWSER_CONFIGURATION_ARGS required
	node $(ROOT)/tools/browser/capture_configuration_persistence.js $(BROWSER_CONFIGURATION_ARGS)

.PHONY: verify-configuration-card-ui capture-browser-configuration-card-ui
verify-configuration-card-ui: ## capture/compare live volume/card UI and exact selected menu images; CONFIGURATION_CARD_UI_ARGS required
	python3 $(ROOT)/tools/verify_configuration_card_ui.py $(CONFIGURATION_CARD_UI_ARGS)

capture-browser-configuration-card-ui: ## capture live browser volume/card UI across Chromium restarts; BROWSER_CONFIGURATION_CARD_UI_ARGS required
	node $(ROOT)/tools/browser/capture_configuration_card_ui.js $(BROWSER_CONFIGURATION_CARD_UI_ARGS)

.PHONY: verify-championship-save capture-browser-championship-save
verify-championship-save: ## generate/compare actual original championship cards and live loading; CHAMPIONSHIP_SAVE_ARGS required
	python3 $(ROOT)/tools/verify_championship_save.py $(CHAMPIONSHIP_SAVE_ARGS)

capture-browser-championship-save: ## load an original championship card through production IDBFS and real keys; BROWSER_CHAMPIONSHIP_SAVE_ARGS required
	node $(ROOT)/tools/browser/capture_championship_save.js $(BROWSER_CHAMPIONSHIP_SAVE_ARGS)

.PHONY: verify-statistics-ui capture-browser-statistics-ui
verify-statistics-ui: ## generate/capture/compare actual persisted statistics menus; STATISTICS_UI_ARGS required
	python3 $(ROOT)/tools/verify_statistics_ui.py $(STATISTICS_UI_ARGS)

.PHONY: verify-statistics-regression
verify-statistics-regression: ## compare fresh menu output against an explicitly pinned accepted original manifest; STATISTICS_REGRESSION_ARGS required
	python3 $(ROOT)/tools/verify_statistics_regression.py $(STATISTICS_REGRESSION_ARGS)

capture-browser-statistics-ui: ## run genuine browser statistics navigation; BROWSER_STATISTICS_UI_ARGS required
	node $(ROOT)/tools/browser/capture_statistics_ui.js $(BROWSER_STATISTICS_UI_ARGS)

.PHONY: generate-multiplayer-save
generate-multiplayer-save: ## produce a positive-score original multiplayer card using real road-driving keys; MULTIPLAYER_SAVE_ARGS required
	python3 $(ROOT)/tools/multiplayer_save_fixture.py $(MULTIPLAYER_SAVE_ARGS)

.PHONY: capture-natural-multiplayer verify-natural-multiplayer verify-road-recovery
capture-natural-multiplayer: ## record natural original hotseat turns with clock/RNG and racing pictures; NATURAL_MULTIPLAYER_CAPTURE_ARGS required
	python3 $(ROOT)/tools/capture_natural_multiplayer.py $(NATURAL_MULTIPLAYER_CAPTURE_ARGS)

.PHONY: capture-multiplayer-driver-probe
capture-multiplayer-driver-probe: ## preflight living original hotseat finishes with one hardware observer; MULTIPLAYER_DRIVER_PROBE_ARGS required
	python3 $(ROOT)/tools/capture_multiplayer_driver_probe.py $(MULTIPLAYER_DRIVER_PROBE_ARGS)

verify-natural-multiplayer: ## compare positive-score original/native/browser hotseat racing histories; NATURAL_MULTIPLAYER_VERIFY_ARGS required
	python3 $(ROOT)/tools/verify_natural_multiplayer.py $(NATURAL_MULTIPLAYER_VERIFY_ARGS)

verify-road-recovery: ## regress host keyboard intentions against retained original observations; ROAD_RECOVERY_ARGS required
	python3 $(ROOT)/tools/verify_road_recovery.py $(ROAD_RECOVERY_ARGS)

.PHONY: pack-racing-archive verify-racing-archive
pack-racing-archive: ## losslessly archive closed racing images, optionally following a live recorder; RACING_ARCHIVE_ARGS required
	python3 $(ROOT)/tools/racing_archive.py $(RACING_ARCHIVE_ARGS)

verify-racing-archive: ## compare archive decoding literally with actual original images and reject damaged/open inputs; RACING_ARCHIVE_VERIFY_ARGS required
	python3 $(ROOT)/tools/verify_racing_archive.py $(RACING_ARCHIVE_VERIFY_ARGS)

.PHONY: capture-race-results-history verify-race-results-ui capture-browser-race-results-ui
capture-race-results-history: ## observe original/native single or multiplayer result-table clock/RNG histories; RACE_RESULTS_HISTORY_ARGS required
	python3 $(ROOT)/tools/capture_race_results_history.py $(RACE_RESULTS_HISTORY_ARGS)

verify-race-results-ui: ## compare aligned actual single or multiplayer result tables; RACE_RESULTS_UI_ARGS required
	python3 $(ROOT)/tools/verify_race_results_ui.py $(RACE_RESULTS_UI_ARGS)

capture-browser-race-results-ui: ## observe browser result tables with recorded original API inputs; BROWSER_RACE_RESULTS_UI_ARGS required
	node $(ROOT)/tools/browser/capture_champ_season.js $(BROWSER_RACE_RESULTS_UI_ARGS)

.PHONY: verify-original-replay capture-browser-original-replay
verify-original-replay: ## generate/compare real original replay cards and natural playback; ORIGINAL_REPLAY_ARGS required
	python3 $(ROOT)/tools/verify_original_replay.py $(ORIGINAL_REPLAY_ARGS)

capture-browser-original-replay: ## load a real original replay through IDBFS and trusted keys; BROWSER_ORIGINAL_REPLAY_ARGS required
	node $(ROOT)/tools/browser/capture_original_replay.js $(BROWSER_ORIGINAL_REPLAY_ARGS)

.PHONY: verify-replay-history capture-browser-replay-history
verify-replay-history: ## compare actual original replay racing video with recorded clocks and computed RNG; REPLAY_HISTORY_ARGS required
	python3 $(ROOT)/tools/verify_replay_history.py $(REPLAY_HISTORY_ARGS)

capture-browser-replay-history: ## capture actual browser replay racing canvas/physics with explicit API inputs; BROWSER_REPLAY_HISTORY_ARGS required
	node $(ROOT)/tools/browser/capture_replay_history.js $(BROWSER_REPLAY_HISTORY_ARGS)

verify-clock-replay: ## exact captured uint32 game-clock inputs; reject missing, partial and leftover records on native/WASM
	python3 $(ROOT)/tools/verify_clock_replay.py $(CLOCK_REPLAY_ARGS)

.PHONY: verify-clock-runs
verify-clock-runs: patch ## compare lossless actual clock DWORDs through native/ASan/WASM readers; CLOCK_RUNS_ARGS required
	bash -c 'source "$(ROOT)/tools/emscripten_env.sh" && python3 "$(ROOT)/tools/verify_clock_runs.py" $(CLOCK_RUNS_ARGS)'

verify-random-reference: ## compute actual original Watcom random records; reject state/result differences and arithmetic mutation
	python3 $(ROOT)/tools/verify_random_reference.py $(RANDOM_REFERENCE_ARGS)

.PHONY: verify-native-api-returns
verify-native-api-returns: ## check optional native API return observations against an actual original transcript; API_RETURN_LOG_ARGS required
	python3 $(ROOT)/tools/verify_api_return_log.py $(API_RETURN_LOG_ARGS)

refcapture-race-stream: ## complete selected original attract loop, clock/random returns and initial blink state; read-only hardware breakpoints
	python3 $(ROOT)/tools/reference/capture.py --race-stream $(RACE_CAPTURE_ARGS)

verify-reference-race-stream: native wasm ## compare every captured original racing-loop frame/palette and calculated RNG/blink states at identical API times
	python3 $(ROOT)/tools/reference/compare_race_stream.py --capture $(RACE_REFERENCE) --output $(RACE_COMPARISON) --native $(NATIVE) --wasm $(OUTJS) --node $(NODE) $(RACE_COMPARISON_ARGS)

verify-keyboard: ## compare actual Wine USER32 modifier/Alt/F10 messages with native SDL and WASM
	python3 $(ROOT)/tools/verify_keyboard.py $(KEYBOARD_ARGS)

.PHONY: verify-keyboard-focus
verify-keyboard-focus: ## compare actual native X11 focus/key states and message counts with Wine USER32; KEYBOARD_FOCUS_ARGS required
	python3 $(ROOT)/tools/verify_keyboard_focus.py $(KEYBOARD_FOCUS_ARGS)

.PHONY: capture-window-focus capture-original-focus-recovery capture-native-window-focus capture-browser-window-focus verify-window-focus
capture-original-focus-recovery: ## unchanged-original recovery/HRESULT diagnosis; fails if active key release is not processed
	python3 $(ROOT)/tools/capture_original_focus_recovery.py $(ORIGINAL_FOCUS_RECOVERY_ARGS)

capture-window-focus: ## bounded unchanged-original activation/held-key/message-wait observations; WINDOW_FOCUS_ARGS required
	python3 $(ROOT)/tools/capture_window_focus.py $(WINDOW_FOCUS_ARGS)

capture-native-window-focus: ## actual native SDL focus transitions and read-only engine states; WINDOW_FOCUS_NATIVE_ARGS required
	python3 $(ROOT)/tools/capture_native_window_focus.py $(WINDOW_FOCUS_NATIVE_ARGS)

capture-browser-window-focus: ## headed trusted browser focus with Playwright focus emulation disabled; WINDOW_FOCUS_BROWSER_ARGS required
	TMPDIR=/tmp/wasm-dd2 xvfb-run -a $(NODE) $(ROOT)/tools/browser/capture_window_focus.js $(WINDOW_FOCUS_BROWSER_ARGS)

verify-window-focus: ## fail on actual port focus-state differences from the Original; WINDOW_FOCUS_VERIFY_ARGS required
	python3 $(ROOT)/tools/verify_window_focus.py $(WINDOW_FOCUS_VERIFY_ARGS)

verify-browser-keyboard: web ## trusted browser named-key release/press through normal intro to menus
	$(NODE) $(ROOT)/tools/browser/qa_keyboard.js $(ROOT)/web/dd2 $(BROWSER_KEYBOARD_OUTPUT)

verify-browser-keyboard-negative: web ## reject the genuine old shell key filter using the same trusted-browser test
	$(NODE) $(ROOT)/tools/browser/qa_keyboard_negative.js $(ROOT)/web/dd2 $(BROWSER_KEYBOARD_NEGATIVE_OUTPUT)

verify-sound-device: ## compare actual Wine device lifetimes and exact native/WASM PCM across closed movie epochs
	python3 $(ROOT)/tools/verify_sound_device.py $(SOUND_DEVICE_ARGS)

.PHONY: verify-sound-hardware
verify-sound-hardware: patch ## compare Wine hardware rejection/software retry with native/ASan/WASM; SOUND_HARDWARE_ARGS required
	python3 $(ROOT)/tools/verify_sound_hardware.py $(SOUND_HARDWARE_ARGS)

verify-native-window: native ## use real X11 keys to test native menus/CD/race/pause and exact rendered/accepted output
	python3 $(ROOT)/tools/verify_native_window.py --binary $(NATIVE) --original-startup $(NATIVE_WINDOW_ARGS)

verify-native-quit: native ## normal original startup, cancel/confirm Quit, exact renderer pixels and complete SDL stream closure
	python3 $(ROOT)/tools/verify_native_quit.py --binary $(NATIVE) $(NATIVE_QUIT_ARGS)

verify-browser-pad: web ## check synthetic boot-time controller, steering, player movement, brake and release
	$(NODE) $(ROOT)/tools/browser/padtest.js $(ROOT)/web/dd2

verify-browser-redbook-restart: web ## check live race keys, CD sector restart and exact shared WebAudio output
	$(NODE) $(ROOT)/tools/browser/qa_redbook_restart.js $(ROOT)/web/dd2 $(BROWSER_RESTART_OUTPUT)

verify-browser-startup: web ## normal original intro/full/skip/gesture -> menus -> live race with exact sink bytes
	$(NODE) $(ROOT)/tools/browser/qa_startup.js $(ROOT)/web/dd2 $(BROWSER_STARTUP_ARGS)

verify-wasm: wasm ## crash-free check: run the WASM demo (node) on all 10 levels, print N/10
	python3 $(ROOT)/tools/verify_demos.py wasm --node $(NODE) --wasm $(OUTJS) --game-dir $(GAMEDIR)

verify-parity: native wasm ## compare all presented frames, palettes, RNG/flip logs and PCM bytes on ten demos
	python3 $(ROOT)/tools/verify_parity.py --native $(NATIVE) --node $(NODE) --wasm $(OUTJS)

.PHONY: verify-highlight
verify-highlight: patch ## compare all ten pit-camera highlight areas and model guards with original x86
	bash -c 'source "$(ROOT)/tools/emscripten_env.sh" && python3 "$(ROOT)/tools/verify_highlight.py" $(HIGHLIGHT_ARGS)'

.PHONY: verify-pit-camera
verify-pit-camera: patch ## compare live/replay pit-camera wrap, signed offsets and model guards with original x86
	bash -c 'source "$(ROOT)/tools/emscripten_env.sh" && python3 "$(ROOT)/tools/verify_pit_camera.py" $(PIT_CAMERA_ARGS)'

.PHONY: verify-print-number
verify-print-number: patch ## compare numeric text commands and cursor/guard bytes with original x86
	bash -c 'source "$(ROOT)/tools/emscripten_env.sh" && python3 "$(ROOT)/tools/verify_print_number.py" $(PRINT_NUMBER_ARGS)'

.PHONY: verify-print-rgb
verify-print-rgb: patch ## compare complete Print records and both glyph-packet buffers with original x86
	bash -c 'source "$(ROOT)/tools/emscripten_env.sh" && python3 "$(ROOT)/tools/verify_print_rgb.py" $(PRINT_RGB_ARGS)'

.PHONY: verify-position-pointers
verify-position-pointers: patch ## compare top-three position packets, ordering tables and GTE with original x86
	bash -c 'source "$(ROOT)/tools/emscripten_env.sh" && python3 "$(ROOT)/tools/verify_position_pointers.py" $(POSITION_POINTERS_ARGS)'

.PHONY: verify-champ-menu-fields
verify-champ-menu-fields: patch ## compare championship menu packed WORD fields and their neighbors with original x86
	bash -c 'source "$(ROOT)/tools/emscripten_env.sh" && python3 "$(ROOT)/tools/verify_champ_menu_fields.py" $(CHAMP_MENU_FIELDS_ARGS)'

.PHONY: verify-duplicate-font
verify-duplicate-font: patch ## compare copied font metadata and all glyphs with original x86 call chains
	bash -c 'source "$(ROOT)/tools/emscripten_env.sh" && python3 "$(ROOT)/tools/verify_duplicate_font.py" $(DUPLICATE_FONT_ARGS)'

.PHONY: verify-original-race-audio
verify-original-race-audio: patch ## compare chronological original racing PCM, source controls and CD ring writes with production mixing
	bash -c 'source "$(ROOT)/tools/emscripten_env.sh" && python3 "$(ROOT)/tools/verify_original_race_audio.py" $(ORIGINAL_RACE_AUDIO_ARGS)'

.PHONY: verify-original-game-clock
verify-original-game-clock: ## reject damaged original relay clock traces and invented terminal returns; ORIGINAL_GAME_CLOCK_ARGS required
	python3 $(ROOT)/tools/reference/test_game_clock.py $(ORIGINAL_GAME_CLOCK_ARGS)

.PHONY: verify-original-clock-storage
verify-original-clock-storage: ## compare every actual clock/observation and audio-service byte in raw/compact exports; CLOCK_STORAGE_ARGS required
	python3 $(ROOT)/tools/reference/test_clock_storage.py $(CLOCK_STORAGE_ARGS)

run: wasm ## run the WASM demo under node at LEVEL=$(LEVEL)
	cd $(GAMEDIR) && DD2_FRAMEDIR=/tmp/wrun $(NODE) $(OUTJS) $(LEVEL)

shot: web ## headless browser screenshot of the web build -> /tmp/dd2_shot.png
	node $(ROOT)/tools/browser/shot.js $(ROOT)/web/dd2 /tmp/dd2_shot.png

REFCAP ?= /tmp/dd2-reference
verify-reference-video: native wasm ## compare existing original L9 Draw_All checkpoints (REFCAP) with both ports
	python3 $(ROOT)/tools/reference/compare_video.py --capture $(REFCAP) --native $(NATIVE) --node $(NODE) --wasm $(OUTJS)

verify-redbook: ## compare all 18 complete CD tracks on patched native/ASan/WASM; REDBOOK_ARGS supplies output/cleanup
	python3 $(ROOT)/tools/verify_redbook.py --node $(NODE) $(REDBOOK_ARGS)

verify-redbook-controls: ## check drained/live Pause/Resume states and exact PCM; REDBOOK_CONTROLS_ARGS can enable Wine
	python3 $(ROOT)/tools/verify_redbook_controls.py --node $(NODE) $(REDBOOK_CONTROLS_ARGS)

verify-redbook-restart: ## check original Stop/TO-only Play sector restart; REDBOOK_RESTART_ARGS can enable Wine
	python3 $(ROOT)/tools/verify_redbook_restart.py --node $(NODE) $(REDBOOK_RESTART_ARGS)

verify-redbook-end: ## measure Wine CD end truncation and verify complete port ranges; REDBOOK_END_ARGS can enable Wine
	python3 $(ROOT)/tools/verify_redbook_end.py --node $(NODE) $(REDBOOK_END_ARGS)

verify-movie-params: patch ## check original movie MCI blocks/control flow on native/WASM; MOVIE_PARAMS_ARGS can check WinMM headers
	python3 $(ROOT)/tools/verify_movie_params.py --node $(NODE) $(MOVIE_PARAMS_ARGS)

verify-movie-codec: ## compare every original AVI Cinepak source frame with Wine ICCVID on native/WASM; MOVIE_CODEC_ARGS selects MinGW
	python3 $(ROOT)/tools/verify_movie_codec.py --node $(NODE) $(MOVIE_CODEC_ARGS)

verify-movie-audio: ## compare complete AVI ADPCM source PCM with Wine ACM on native/WASM; MOVIE_AUDIO_ARGS selects MinGW
	python3 $(ROOT)/tools/verify_movie_audio.py --node $(NODE) $(MOVIE_AUDIO_ARGS)

.PHONY: verify-movie-avi
verify-movie-avi: patch ## compare production AVI parsing/seeks and all decoded source bytes with Wine; MOVIE_AVI_ARGS selects MinGW/output
	python3 $(ROOT)/tools/verify_movie_avi.py --node $(NODE) $(MOVIE_AVI_ARGS)

.PHONY: verify-movie-surface
verify-movie-surface: ## compare movie RGB565 scaling/display with real Win32 GDI; MOVIE_SURFACE_ARGS selects source/output/MinGW
	python3 $(ROOT)/tools/verify_movie_surface.py --node $(NODE) $(MOVIE_SURFACE_ARGS)

.PHONY: verify-movie-window
verify-movie-window: patch ## compare every actual original intro window byte with production native/WASM components
	python3 $(ROOT)/tools/verify_movie_window.py $(MOVIE_WINDOW_ARGS)

.PHONY: verify-movie-window-axes
verify-movie-window-axes: patch ## compare all old/current movie window bytes and clipped transforms on native/ASan/WASM
	python3 $(ROOT)/tools/verify_movie_window_axes.py $(MOVIE_WINDOW_AXES_ARGS)

.PHONY: verify-movie-end verify-movie-drain verify-movie-video observe-movie-timing verify-movie-audio-sinks
verify-movie-end: ## observe actual Wine default exclusive AVI play endpoints; MOVIE_END_ARGS supplies MinGW/output
	python3 $(ROOT)/tools/verify_movie_end.py $(MOVIE_END_ARGS)

verify-movie-drain: patch ## compare real-time Wine final draw/drain with native/ASan/WASM components; MOVIE_DRAIN_ARGS required
	python3 $(ROOT)/tools/verify_movie_drain.py $(MOVIE_DRAIN_ARGS)

.PHONY: verify-movie-qpc
verify-movie-qpc: patch ## verify actual Wine QPC and native RAW/fallback/wrap provider; MOVIE_QPC_ARGS required
	python3 $(ROOT)/tools/verify_movie_qpc.py $(MOVIE_QPC_ARGS)

.PHONY: verify-movie-precision
verify-movie-precision: patch ## check microsecond frame boundaries on native/ASan/WASM; MOVIE_PRECISION_ARGS required
	python3 $(ROOT)/tools/verify_movie_precision.py $(MOVIE_PRECISION_ARGS)

.PHONY: verify-movie-wait
verify-movie-wait: patch ## compare Wine relative drain with independent native/WASM timers; MOVIE_WAIT_ARGS required
	python3 $(ROOT)/tools/verify_movie_wait.py $(MOVIE_WAIT_ARGS)

.PHONY: verify-movie-worker-clock
verify-movie-worker-clock: patch ## compare actual Wine producer QPC and sanitized native wake correction; MOVIE_WORKER_CLOCK_ARGS required
	python3 $(ROOT)/tools/verify_movie_worker_clock.py $(MOVIE_WORKER_CLOCK_ARGS)

.PHONY: verify-browser-movie-pause
verify-browser-movie-pause: ## compare current/previous browser video clocks during actual audio suspension; BROWSER_MOVIE_PAUSE_ARGS supplies builds/output
	node $(ROOT)/tools/browser/qa_movie_pause.js $(BROWSER_MOVIE_PAUSE_ARGS)

.PHONY: verify-movie-completion
verify-movie-completion: patch ## compare actual Wine/native worker-published audio completion; MOVIE_COMPLETION_ARGS required
	python3 $(ROOT)/tools/verify_movie_completion.py $(MOVIE_COMPLETION_ARGS)

.PHONY: verify-native-movie-device capture-native-movie-device compare-native-movie-device
verify-native-movie-device: patch ## verify actual ALSA movie start, repeat, cancel and errors under ASan/UBSan; NATIVE_MOVIE_DEVICE_ARGS required
	python3 $(ROOT)/tools/verify_native_movie_device.py $(NATIVE_MOVIE_DEVICE_ARGS)

capture-native-movie-device: ## record actual native ALSA movie frames and accepted/played PCM; NATIVE_MOVIE_DEVICE_ARGS required
	python3 $(ROOT)/tools/capture_native_movie_device.py $(NATIVE_MOVIE_DEVICE_ARGS)

compare-native-movie-device: ## compare unshifted native movie device PCM with Wine ACM and optional whole original intro
	python3 $(ROOT)/tools/compare_native_movie_device.py $(NATIVE_MOVIE_DEVICE_ARGS)

.PHONY: verify-browser-s16-output
verify-browser-s16-output: patch ## diagnose every S16 value through actual Chromium ALSA and optional Pulse; BROWSER_S16_OUTPUT_ARGS required
	python3 $(ROOT)/tools/verify_browser_s16_output.py $(BROWSER_S16_OUTPUT_ARGS)

.PHONY: capture-original-movie-audio
capture-original-movie-audio: ## audit full original Intro against fresh Wine ACM PCM on ALSA or Pulse; ORIGINAL_MOVIE_AUDIO_ARGS required
	python3 $(ROOT)/tools/capture_original_movie_audio.py $(ORIGINAL_MOVIE_AUDIO_ARGS)

.PHONY: observe-original-pulse-overflow
observe-original-pulse-overflow: ## observe bounded before/after overflow states in the supported live Wine Pulse driver; ORIGINAL_PULSE_OVERFLOW_ARGS required
	python3 $(ROOT)/tools/observe_original_pulse_overflow.py $(ORIGINAL_PULSE_OVERFLOW_ARGS)

.PHONY: verify-movie-write-recovery
verify-movie-write-recovery: patch ## compare recoverable ALSA source writes with real Wine and sanitized native; MOVIE_WRITE_RECOVERY_ARGS required
	python3 $(ROOT)/tools/verify_movie_write_recovery.py $(MOVIE_WRITE_RECOVERY_ARGS)

.PHONY: verify-frexp-abi
verify-frexp-abi: ## compare production Native/WASM libc ABI and original Watcom instructions; FREXP_ABI_ARGS required
	python3 $(ROOT)/tools/verify_frexp_abi.py $(FREXP_ABI_ARGS)

.PHONY: capture-original-outro
capture-original-outro: ## capture complete actual original Outro through real CREDITZ! menu inputs; ORIGINAL_OUTRO_ARGS required
	python3 $(ROOT)/tools/capture_original_outro.py $(ORIGINAL_OUTRO_ARGS)

.PHONY: verify-native-credits capture-browser-credits
verify-native-credits: ## check real native CREDITZ! name-grid, complete Outro and exit(0); NATIVE_CREDITS_ARGS required
	python3 $(ROOT)/tools/verify_credits_frontend.py $(NATIVE_CREDITS_ARGS)

capture-browser-credits: ## check real browser CREDITZ! inputs, original frame hashes and exit(0); BROWSER_CREDITS_ARGS required
	node $(ROOT)/tools/browser/capture_credits_frontend.js $(BROWSER_CREDITS_ARGS)

verify-movie-video: ## compare actual original Intro/Outro window bytes with SDL/canvas; MOVIE_VIDEO_ARGS required
	python3 $(ROOT)/tools/verify_movie_video.py $(MOVIE_VIDEO_ARGS)

observe-movie-timing: ## validate actual original movie QPC/audio clock observations; MOVIE_TIMING_ARGS required
	python3 $(ROOT)/tools/observe_movie_timing.py $(MOVIE_TIMING_ARGS)

.PHONY: observe-native-movie-timing
observe-native-movie-timing: ## validate actual SDL presentation brackets against consumed movie samples; MOVIE_TIMING_ARGS required
	python3 $(ROOT)/tools/observe_native_movie_timing.py $(MOVIE_TIMING_ARGS)

verify-movie-audio-sinks: ## compare complete original accepted/played intro PCM with native/browser sinks; MOVIE_AUDIO_SINK_ARGS required
	python3 $(ROOT)/tools/verify_movie_audio_sinks.py $(MOVIE_AUDIO_SINK_ARGS)

.PHONY: verify-movie-playback
verify-movie-playback: patch ## run original Play_Movie through the production MCI device; MOVIE_PLAYBACK_ARGS supplies source/output
	python3 $(ROOT)/tools/verify_movie_playback.py --node $(NODE) $(MOVIE_PLAYBACK_ARGS)

.PHONY: verify-native-movie verify-browser-movie
verify-native-movie: native ## compare complete actual SDL movie frames/accepted PCM and real skip keys; NATIVE_MOVIE_ARGS supplies source/output
	python3 $(ROOT)/tools/verify_native_movie.py --binary $(NATIVE) $(NATIVE_MOVIE_ARGS)

verify-browser-movie: web ## compare complete actual canvas/movie AudioBuffers and live skip keys; BROWSER_MOVIE_ARGS supplies source/output
	$(NODE) $(ROOT)/tools/browser/qa_movie.js $(ROOT)/web/dd2 $(BROWSER_MOVIE_ARGS)

verify-movie-reference: ## check full original intro accepted PCM; MOVIE_REFERENCE_ARGS supplies --capture/--source
	python3 $(ROOT)/tools/reference/verify_movie.py $(MOVIE_REFERENCE_ARGS)

verify-shared-audio: ## check ordered CD/effects mix against Wine PCM; SHARED_AUDIO_ARGS can recapture Wine
	python3 $(ROOT)/tools/verify_shared_audio.py --node $(NODE) $(SHARED_AUDIO_ARGS)

verify-menu-audio: ## check elapsed-time DirectSound playback and controls with a fixed menu frame counter
	python3 $(ROOT)/tools/verify_menu_audio.py --node $(NODE)

verify-sound-cursor: ## check seeking, Play/Stop/resume/end and exact PCM; SOUND_CURSOR_ARGS can enable the Wine API fixture
	python3 $(ROOT)/tools/verify_sound_cursor.py --node $(NODE) $(SOUND_CURSOR_ARGS)

verify-sound-lifetime: ## check duplicate sample ownership and COM references; SOUND_LIFETIME_ARGS can enable Wine/ASan
	python3 $(ROOT)/tools/verify_sound_lifetime.py --node $(NODE) $(SOUND_LIFETIME_ARGS)

verify-sound-gain: ## check all quantized gains and Float32 mix; SOUND_GAIN_ARGS can enable real Wine PCM calibration
	python3 $(ROOT)/tools/generate_sound_gain.py --check
	python3 $(ROOT)/tools/verify_sound_gain.py --node $(NODE) $(SOUND_GAIN_ARGS)

verify-sound-resample: patch ## check production waveforms against real Wine FIR hashes and CPU x87; SOUND_RESAMPLE_ARGS can recapture Wine
	python3 $(ROOT)/tools/generate_sound_fir.py --check
	python3 $(ROOT)/tools/verify_sound_resample.py --node $(NODE) $(SOUND_RESAMPLE_ARGS)

verify-menu-cycles: ## compare all 64 presented phases; MCREF, MCNATIVE/MCBROWSER, MCREPORT required
	python3 $(ROOT)/tools/verify_menu_cycles.py --reference $(MCREF) $(if $(MCNATIVE),--native $(MCNATIVE)) $(if $(MCBROWSER),--browser $(MCBROWSER)) --report $(MCREPORT)

.PHONY: verify-track-selection
verify-track-selection: ## capture/compare road and bowl menus; TRACK_SELECTION_ARGS required
	python3 $(ROOT)/tools/verify_track_selection.py $(TRACK_SELECTION_ARGS)

verify-champ-names: patch ## check naming/score builders; optional CHAMPREF, CHAMPBROWSER and CHAMPNATIVE captures
	python3 $(ROOT)/tools/verify_champ_names.py --node $(NODE) $(if $(CHAMPREF),--reference $(CHAMPREF)) $(if $(CHAMPBROWSER),--browser $(CHAMPBROWSER)) $(if $(CHAMPNATIVE),--native-capture $(CHAMPNATIVE))

.PHONY: verify-league-standing verify-champ-season
verify-league-standing: patch ## compare all outcome classifications with original x86; LEAGUE_STANDING_ARGS required
	python3 $(ROOT)/tools/verify_league_standing.py $(LEAGUE_STANDING_ARGS)

.PHONY: verify-season-transition
verify-season-transition: patch ## original-x86 transfers/reset/statistics/unlocks; SEASON_TRANSITION_ARGS required
	python3 $(ROOT)/tools/verify_season_transition.py $(SEASON_TRANSITION_ARGS)

.PHONY: verify-multiplayer-points
verify-multiplayer-points: patch ## original-x86 post-result score components; MULTIPLAYER_POINTS_ARGS required
	python3 $(ROOT)/tools/verify_multiplayer_points.py $(MULTIPLAYER_POINTS_ARGS)

.PHONY: verify-race-positions
verify-race-positions: patch ## original-x86 lap/finish/record-loading components; RACE_POSITIONS_ARGS required
	python3 $(ROOT)/tools/verify_race_positions.py $(RACE_POSITIONS_ARGS)

.PHONY: verify-lap-record-update
verify-lap-record-update: patch ## original-x86 record updates/serialization with controlled name callback; LAP_RECORD_UPDATE_ARGS required
	python3 $(ROOT)/tools/verify_lap_record_update.py $(LAP_RECORD_UPDATE_ARGS)

.PHONY: verify-driver-name-ui capture-browser-driver-name-ui
verify-driver-name-ui: ## actual original/native driver-name input and menu comparisons; DRIVER_NAME_UI_ARGS required
	python3 $(ROOT)/tools/verify_driver_name_ui.py $(DRIVER_NAME_UI_ARGS)

capture-browser-driver-name-ui: ## trusted browser driver-name input; BROWSER_DRIVER_NAME_UI_ARGS required
	node $(ROOT)/tools/browser/capture_driver_name_ui.js $(BROWSER_DRIVER_NAME_UI_ARGS)

verify-champ-season: ## actual full Retire/Yes season capture/compare; CHAMP_SEASON_ARGS required
	python3 $(ROOT)/tools/verify_champ_season.py $(CHAMP_SEASON_ARGS)

.PHONY: verify-champ-history verify-browser-champ-history verify-champ-rest-loop
verify-champ-history: ## compare actual championship API histories; CHAMP_HISTORY_ARGS required; browser failure remains diagnostic
	python3 $(ROOT)/tools/verify_champ_history.py $(CHAMP_HISTORY_ARGS)

verify-browser-champ-history: ## actual browser/original API checkpoint and retained image comparison; BROWSER_CHAMP_HISTORY_ARGS required
	python3 $(ROOT)/tools/verify_browser_champ_history.py $(BROWSER_CHAMP_HISTORY_ARGS)

verify-champ-rest-loop: ## actual original/browser post-Retire Steam caller diagnosis; CHAMP_REST_LOOP_ARGS required
	python3 $(ROOT)/tools/verify_champ_rest_loop.py $(CHAMP_REST_LOOP_ARGS)

.PHONY: verify-normal-arena-history
verify-normal-arena-history: ## exact natural arena pictures/API states against original; NORMAL_ARENA_HISTORY_ARGS required
	python3 $(ROOT)/tools/verify_normal_arena_history.py $(NORMAL_ARENA_HISTORY_ARGS)

.PHONY: verify-natural-season-history
verify-natural-season-history: ## compare five natural championship races, all standings and season ending; NATURAL_SEASON_HISTORY_ARGS required
	python3 $(ROOT)/tools/verify_natural_season_history.py $(NATURAL_SEASON_HISTORY_ARGS)

verify-cdrom: ## validate virtual CD TOC, raw audio, track boundaries and descriptor isolation
	python3 $(ROOT)/tools/reference/test_cdrom.py

verify-corner-lanes: patch ## check BYTE corner FD writes and reject old neighbor corruption on both targets
	python3 $(ROOT)/tools/verify_corner_lanes.py

.PHONY: verify-replay verify-replay-metadata verify-browser-replay-save verify-native-replay verify-card
verify-replay: patch ## compare replay recording/decoding with actual original x86; reject old DWORD cursors
	python3 $(ROOT)/tools/verify_replay.py $(REPLAY_ARGS)

verify-replay-metadata: patch ## compare packed replay bytes/load setup with original x86 at external API boundaries
	python3 $(ROOT)/tools/verify_replay_metadata.py $(REPLAY_METADATA_ARGS)

verify-browser-replay-save: web ## real replay save/overwrite/restart/load/playback and cancelled/confirmed deletion
	$(NODE) $(ROOT)/tools/browser/qa_replay_save.js $(ROOT)/web/dd2 $(if $(BROWSER_REPLAY_SAVE_OUTPUT),$(BROWSER_REPLAY_SAVE_OUTPUT),"") $(BROWSER_REPLAY_SAVE_ARGS)

verify-native-replay: native ## real X11 replay save/overwrite/process restart/load/playback and cancelled/confirmed deletion
	python3 $(ROOT)/tools/verify_native_replay.py --binary $(NATIVE) $(NATIVE_REPLAY_ARGS)

verify-card: patch ## compare empty/full/sparse card operations and complete block/directory bytes with original x86
	python3 $(ROOT)/tools/verify_card.py $(CARD_ARGS)

verify-audio-observer: ## check exact accepted/consumed PCM and 32/64-bit sample clocks; no port parity claim
	python3 $(ROOT)/tools/reference/test_audio.py $(AUDIO_OBSERVER_ARGS)

.PHONY: verify-original-menu-audio
verify-original-menu-audio: ## compare bounded original menu mix at traced sample positions; live input scheduling pending
	python3 $(ROOT)/tools/verify_original_menu_audio.py --node $(NODE) $(ORIGINAL_MENU_AUDIO_ARGS)

.PHONY: verify-menu-startup-controls
verify-menu-startup-controls: ## compare actual original/native/browser startup audio calls by presentation; MENU_STARTUP_ARGS supplies captures/report
	python3 $(ROOT)/tools/verify_menu_startup_controls.py $(MENU_STARTUP_ARGS)

.PHONY: verify-menu-engine-audio
verify-menu-engine-audio: ## compare real frontend PCM at the independently observed original device clock; MENU_ENGINE_AUDIO_ARGS supplies captures/clock/output
	python3 $(ROOT)/tools/verify_menu_engine_audio.py $(MENU_ENGINE_AUDIO_ARGS)

.PHONY: verify-startup-video
verify-startup-video: ## compare first actual frontend frames/palettes with original successful Flips; STARTUP_VIDEO_ARGS supplies captures/report
	python3 $(ROOT)/tools/verify_startup_video.py $(STARTUP_VIDEO_ARGS)

.PHONY: verify-video-archive
verify-video-archive: ## verify lossless closed-block video storage through actual Wine DirectDraw; VIDEO_ARCHIVE_ARGS required
	python3 $(ROOT)/tools/verify_video_archive.py $(VIDEO_ARCHIVE_ARGS)

.PHONY: verify-replay-video-stream
verify-replay-video-stream: ## check incremental literal comparison and safe cleanup with copied source records; REPLAY_VIDEO_STREAM_ARGS required
	python3 $(ROOT)/tools/verify_replay_video_stream.py $(REPLAY_VIDEO_STREAM_ARGS)

.PHONY: verify-car-preview
verify-car-preview: ## compare whole car preview rotations by observed model pose; CAR_PREVIEW_ARGS supplies captures/report
	python3 $(ROOT)/tools/verify_car_preview.py $(CAR_PREVIEW_ARGS)

refcapture: ## capture original at Draw_All entry under private Wine with a verified virtual audio CD
	bash $(ROOT)/tools/refcapture.sh

clean: ## remove generated build/ and outputs
	rm -rf $(ROOT)/build $(OUTJS) $(OUTJS:.js=.wasm)

.PHONY: clean-logs
clean-logs: ## remove logs older than one hour, preserving open files
	python3 $(ROOT)/tools/artifacts.py --age-seconds $(LOG_MAX_AGE)

.PHONY: capture-live-lap-record capture-browser-live-lap-record verify-live-lap-record
capture-live-lap-record: ## actual original/native lap/name/configuration save and restart; LIVE_LAP_RECORD_ARGS required
	python3 $(ROOT)/tools/capture_live_lap_record.py $(LIVE_LAP_RECORD_ARGS)

capture-browser-live-lap-record: ## actual browser lap/name/save/reload; BROWSER_LIVE_LAP_RECORD_ARGS required
	node $(ROOT)/tools/browser/capture_live_lap_record.js $(BROWSER_LIVE_LAP_RECORD_ARGS)

verify-live-lap-record: ## compare actual record persistence and selected original dialog rasters; LIVE_LAP_RECORD_VERIFY_ARGS required
	python3 $(ROOT)/tools/verify_live_lap_record.py $(LIVE_LAP_RECORD_VERIFY_ARGS)
