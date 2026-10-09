# Handwritten C11 rewrite; frozen reconstruction commands run under /tmp.
ROOT := $(CURDIR)
GAMEDIR := $(ROOT)/DestructionDerby2
LEVEL ?= 9
LOG_MAX_AGE ?= 3600
REFERENCE_TARGET ?= check

all: rewrite-native   ## default on rewrite: readable C + SoftGL native build with strict clang-tidy

.PHONY: rewrite-native rewrite-wasm rewrite-check rewrite-format rewrite-format-check rewrite-tidy
.PHONY: rewrite-archive-verify rewrite-audio-assets-verify rewrite-music-output-verify rewrite-effects-output-verify
.PHONY: rewrite-level-verify rewrite-mesh-verify rewrite-scene-verify rewrite-road-verify rewrite-surface-verify
.PHONY: rewrite-barrier-verify
.PHONY: rewrite-ground-verify
.PHONY: rewrite-fleet-verify
.PHONY: rewrite-accidents-verify
.PHONY: rewrite-race-verify

.PHONY: rewrite-car-livery-verify
.PHONY: rewrite-car-class-verify
rewrite-car-class-verify: ## verify original class handling and actual Native/browser selection
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_car_classes.py
	$(MAKE) clean-logs

rewrite-car-livery-verify: ## verify original car palettes/numbers and cross-target high-detail rendering
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_car_liveries.py
	$(MAKE) clean-logs

.PHONY: rewrite-player-profile-verify
rewrite-player-profile-verify: ## verify player/car/audio profiles and actual Native/browser save dialogs
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_player_profile.py
	$(MAKE) clean-logs

.PHONY: rewrite-preferences-verify
rewrite-preferences-verify: ## verify original-compatible audio preferences in the actual application
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_preferences.py
	$(MAKE) clean-logs

.PHONY: rewrite-save-store-verify
rewrite-save-store-verify: ## verify durable Native/IndexedDB card storage and owner lifetimes
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_save_store.py
	$(MAKE) clean-logs

.PHONY: rewrite-save-profile-verify
rewrite-save-profile-verify: ## verify typed original configuration/profile blocks and bounded edits
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_save_profile.py
	$(MAKE) clean-logs

.PHONY: rewrite-save-card-verify
rewrite-save-card-verify: ## verify owned original save container and transactional block operations
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_save_card.py
	$(MAKE) clean-logs

.PHONY: rewrite-font-verify
rewrite-font-verify: ## verify owned original font glyphs and live original font loading
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_fonts.py --original
	$(MAKE) clean-logs

rewrite-race-verify: ## verify race phases/results, source rules and a complete physical race
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_race.py
	python3 $(ROOT)/tools/rewrite/verify_window.py
	$(MAKE) clean-logs


.PHONY: rewrite-championship-verify
rewrite-championship-verify: ## verify typed championship rules and actual scheduled race ownership
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_championship.py
	$(MAKE) clean-logs

.PHONY: rewrite-championship-application-verify
rewrite-championship-application-verify: ## verify actual championship application/canvas and Native/browser input
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_championship_application.py
	python3 $(ROOT)/tools/rewrite/verify_window.py
	$(MAKE) clean-logs

.PHONY: rewrite-grid-verify
rewrite-grid-verify: ## verify stable-ID physical grids and mode/reset ownership on every original level
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_grid.py
	$(MAKE) clean-logs

.PHONY: rewrite-league-verify
rewrite-league-verify: ## compare typed single-player league rules with unmodified original x86
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_league.py
	$(MAKE) clean-logs

.PHONY: rewrite-recovery-verify
rewrite-recovery-verify: ## verify supported overturn recovery and original-data wheel support
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_recovery.py
	$(MAKE) clean-logs

.PHONY: rewrite-laps-verify

rewrite-laps-verify: ## verify original course equivalents, lap rules and physical progress
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_laps.py
	$(MAKE) clean-logs

rewrite-accidents-verify: ## verify source-inspired accident rules in full fields on original tracks
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_accidents.py
	$(MAKE) clean-logs

.PHONY: rewrite-ai-verify
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

rewrite-ai-verify: ## verify source-linked guidance and sixty-second opponent drives on all targets
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_ai.py
	python3 $(ROOT)/tools/rewrite/verify_fleet.py
	python3 $(ROOT)/tools/rewrite/verify_driving.py
	python3 $(ROOT)/tools/rewrite/verify_window.py
	$(MAKE) clean-logs

rewrite-fleet-verify: ## verify all source grid slots and coupled vehicle collisions on all targets
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_fleet.py
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

.PHONY: rewrite-original-music-selection-verify
rewrite-original-music-selection-verify: ## verify original CD context selection and green-light transport
	$(MAKE) clean-logs
	python3 $(ROOT)/tools/rewrite/verify_original_music_selection.py
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

rewrite-effects-output-verify: ## verify gameplay effects through real SDL/WebAudio output and sanitizers
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_effects_output.py
	$(MAKE) clean-logs

rewrite-music-output-verify: ## verify real SDL/WebAudio Redbook output, controls and lifetime
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_music_output.py
	$(MAKE) clean-logs

rewrite-audio-assets-verify: ## verify all original sound effects and Redbook PCM on native/WASM/sanitized
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	python3 $(ROOT)/tools/rewrite/verify_audio_assets.py
	$(MAKE) clean-logs

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

.PHONY: help provision provision-native image reference reference-prepare native wasm patch check clean-logs
help: ## list targets
	@grep -hE '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN{FS=":.*?## "}{printf "  %-24s %s\n",$$1,$$2}'

provision: ## download verified original game data and lossless Redbook music
	python3 $(ROOT)/tools/provision_game.py

provision-native: ## provision reusable i386 SDL headers under ignored deps/
	python3 $(ROOT)/tools/native_sdl_config.py provision

image: ## extract the original image with the frozen extractor under /tmp
	python3 "$$(python3 $(ROOT)/tools/rewrite/reference.py --file re_out/extract_image.py)" "$(GAMEDIR)/dd2h.exe" "$(GAMEDIR)/dd2_image.bin"

reference-prepare: ## provision hash-checked frozen reconstruction files under /tmp
	python3 $(ROOT)/tools/rewrite/reference.py

reference: ## run a frozen reference target; use REFERENCE_TARGET=target
	python3 $(ROOT)/tools/rewrite/reference.py --make "$(REFERENCE_TARGET)"

native wasm patch check: ## run the named reconstruction target under /tmp
	python3 $(ROOT)/tools/rewrite/reference.py --make "$@"

clean-logs: ## remove completed logs older than one hour, preserving open files
	python3 $(ROOT)/tools/artifacts.py --age-seconds $(LOG_MAX_AGE)

.PHONY: assets-generate assets-preview assets-check assets-content-verify
assets-content-verify: ## check owned C mesh/PNG loading on Native, WASM and fresh sanitizers
	$(MAKE) clean-logs
	$(MAKE) rewrite-check rewrite-wasm
	ctest --preset rewrite-wasm
	PYTHONDONTWRITEBYTECODE=1 python3 $(ROOT)/tools/assets/verify_content.py
	$(MAKE) clean-logs

assets-generate: ## regenerate authored Blender, procedural maps and new PCM; no original game input
	PYTHONDONTWRITEBYTECODE=1 python3 $(ROOT)/tools/assets/build_assets.py

assets-preview: ## regenerate authored content and render review images under /tmp/wasm-dd2/
	$(MAKE) clean-logs
	PYTHONDONTWRITEBYTECODE=1 python3 $(ROOT)/tools/assets/build_assets.py --preview /tmp/wasm-dd2/authored-assets-preview
	$(MAKE) clean-logs

assets-check: ## independently validate checked-in authored meshes, maps and PCM
	$(MAKE) clean-logs
	PYTHONDONTWRITEBYTECODE=1 python3 $(ROOT)/tools/assets/verify_assets.py --report /tmp/wasm-dd2/authored-assets-check/report.json
	$(MAKE) clean-logs
