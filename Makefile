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

.PHONY: all pipeline provision provision-native decompile assemble symbols image patch check native play-native wasm web verify verify-native-sdl verify-native-window verify-browser-pad verify-wasm verify-parity run shot refcapture verify-cdrom verify-audio-observer verify-redbook verify-shared-audio verify-menu-audio verify-sound-cursor verify-sound-lifetime verify-sound-gain verify-sound-resample verify-menu-cycles verify-champ-names verify-reference-video clean help

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
	@rm -rf /tmp/dd2_patchcheck && mkdir -p /tmp/dd2_patchcheck && cp $(ROOT)/re_out/*.c $(ROOT)/re_out/*.h /tmp/dd2_patchcheck/ && \
	for p in $(ROOT)/patches/*.diff; do patch -p1 -s -F0 --fuzz=0 -d /tmp/dd2_patchcheck < $$p || { echo "CHECK FAILED: $$p"; exit 1; }; done && \
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

verify-native-window: native ## use real X11 keys to test native menus/CD/race/pause and exact rendered/accepted output
	python3 $(ROOT)/tools/verify_native_window.py --binary $(NATIVE) $(NATIVE_WINDOW_ARGS)

verify-browser-pad: web ## check synthetic boot-time controller, steering, player movement, brake and release
	$(NODE) $(ROOT)/tools/browser/padtest.js $(ROOT)/web/dd2

verify-wasm: wasm ## crash-free check: run the WASM demo (node) on all 10 levels, print N/10
	python3 $(ROOT)/tools/verify_demos.py wasm --node $(NODE) --wasm $(OUTJS) --game-dir $(GAMEDIR)

verify-parity: native wasm ## compare all presented frames, palettes, RNG/flip logs and PCM bytes on ten demos
	python3 $(ROOT)/tools/verify_parity.py --native $(NATIVE) --node $(NODE) --wasm $(OUTJS)

run: wasm ## run the WASM demo under node at LEVEL=$(LEVEL)
	cd $(GAMEDIR) && DD2_FRAMEDIR=/tmp/wrun $(NODE) $(OUTJS) $(LEVEL)

shot: web ## headless browser screenshot of the web build -> /tmp/dd2_shot.png
	node $(ROOT)/tools/browser/shot.js $(ROOT)/web/dd2 /tmp/dd2_shot.png

REFCAP ?= /tmp/dd2-reference
verify-reference-video: native wasm ## compare one existing original L9 Draw_All checkpoint (REFCAP) with both ports
	python3 $(ROOT)/tools/reference/compare_video.py --capture $(REFCAP) --native $(NATIVE) --node $(NODE) --wasm $(OUTJS)

verify-redbook: ## compare MCI playback, stop/resume and complete CD PCM on native/WASM against provisioned CDDA
	python3 $(ROOT)/tools/verify_redbook.py --node $(NODE)

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

verify-champ-names: patch ## check naming/score builders; optional CHAMPREF, CHAMPBROWSER and CHAMPNATIVE captures
	python3 $(ROOT)/tools/verify_champ_names.py --node $(NODE) $(if $(CHAMPREF),--reference $(CHAMPREF)) $(if $(CHAMPBROWSER),--browser $(CHAMPBROWSER)) $(if $(CHAMPNATIVE),--native-capture $(CHAMPNATIVE))

verify-cdrom: ## validate virtual CD TOC, raw audio, track boundaries and descriptor isolation
	python3 $(ROOT)/tools/reference/test_cdrom.py

verify-audio-observer: ## check 32/64-bit ALSA sample clocks and exact accepted-write capture; no port parity claim
	python3 $(ROOT)/tools/reference/test_audio.py

refcapture: ## capture original at Draw_All entry under private Wine with a verified virtual audio CD
	bash $(ROOT)/tools/refcapture.sh

clean: ## remove generated build/ and outputs
	rm -rf $(ROOT)/build $(OUTJS) $(OUTJS:.js=.wasm)
