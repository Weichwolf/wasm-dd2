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

all: wasm             ## default: patch + WASM build

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

verify-clock-replay: ## exact captured uint32 game-clock inputs; reject missing, partial and leftover records on native/WASM
	python3 $(ROOT)/tools/verify_clock_replay.py $(CLOCK_REPLAY_ARGS)

verify-random-reference: ## compute actual original Watcom random records; reject state/result differences and arithmetic mutation
	python3 $(ROOT)/tools/verify_random_reference.py $(RANDOM_REFERENCE_ARGS)

refcapture-race-stream: ## complete selected original attract loop, clock/random returns and initial blink state; read-only hardware breakpoints
	python3 $(ROOT)/tools/reference/capture.py --race-stream $(RACE_CAPTURE_ARGS)

verify-reference-race-stream: native wasm ## compare every captured original racing-loop frame/palette and calculated RNG/blink states at identical API times
	python3 $(ROOT)/tools/reference/compare_race_stream.py --capture $(RACE_REFERENCE) --output $(RACE_COMPARISON) --native $(NATIVE) --wasm $(OUTJS) --node $(NODE) $(RACE_COMPARISON_ARGS)

verify-keyboard: ## compare actual Wine USER32 modifier/Alt/F10 messages with native SDL and WASM
	python3 $(ROOT)/tools/verify_keyboard.py $(KEYBOARD_ARGS)

verify-browser-keyboard: web ## trusted browser named-key release/press through normal intro to menus
	$(NODE) $(ROOT)/tools/browser/qa_keyboard.js $(ROOT)/web/dd2 $(BROWSER_KEYBOARD_OUTPUT)

verify-browser-keyboard-negative: web ## reject the genuine old shell key filter using the same trusted-browser test
	$(NODE) $(ROOT)/tools/browser/qa_keyboard_negative.js $(ROOT)/web/dd2 $(BROWSER_KEYBOARD_NEGATIVE_OUTPUT)

verify-sound-device: ## compare actual Wine device lifetimes and exact native/WASM PCM across closed movie epochs
	python3 $(ROOT)/tools/verify_sound_device.py $(SOUND_DEVICE_ARGS)

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

run: wasm ## run the WASM demo under node at LEVEL=$(LEVEL)
	cd $(GAMEDIR) && DD2_FRAMEDIR=/tmp/wrun $(NODE) $(OUTJS) $(LEVEL)

shot: web ## headless browser screenshot of the web build -> /tmp/dd2_shot.png
	node $(ROOT)/tools/browser/shot.js $(ROOT)/web/dd2 /tmp/dd2_shot.png

REFCAP ?= /tmp/dd2-reference
verify-reference-video: native wasm ## compare existing original L9 Draw_All checkpoints (REFCAP) with both ports
	python3 $(ROOT)/tools/reference/compare_video.py --capture $(REFCAP) --native $(NATIVE) --node $(NODE) --wasm $(OUTJS)

verify-redbook: ## compare MCI playback, stop/resume and complete CD PCM on native/WASM against provisioned CDDA
	python3 $(ROOT)/tools/verify_redbook.py --node $(NODE)

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
verify-movie-avi: ## compare production AVI parsing/seeks and all decoded source bytes with Wine; MOVIE_AVI_ARGS selects MinGW/output
	python3 $(ROOT)/tools/verify_movie_avi.py --node $(NODE) $(MOVIE_AVI_ARGS)

.PHONY: verify-movie-surface
verify-movie-surface: ## compare movie RGB565 scaling/display with real Win32 GDI; MOVIE_SURFACE_ARGS selects source/output/MinGW
	python3 $(ROOT)/tools/verify_movie_surface.py --node $(NODE) $(MOVIE_SURFACE_ARGS)

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

verify-sound-resample: ## check complete waveforms against real Wine FIR hashes and CPU x87; SOUND_RESAMPLE_ARGS can recapture Wine
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
