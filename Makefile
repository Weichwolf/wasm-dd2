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

.PHONY: all pipeline decompile assemble symbols image patch check native wasm web verify verify-wasm run shot refcapture clean help

all: wasm             ## default: patch + WASM build

pipeline: decompile assemble patch native verify wasm verify-wasm ## FULL from-binary chain: dd2h.exe -> Ghidra -> assemble -> patch -> native+WASM -> 10-level crash test (both targets)
	@echo "pipeline OK: dd2h.exe -> decompile -> assemble -> patch -> compile (native + WASM) -> run"

help:                 ## list targets
	@grep -hE '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN{FS=":.*?## "}{printf "  %-18s %s\n",$$1,$$2}'

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

wasm: patch ## WASM build -> $(OUTJS)
	bash $(ROOT)/tools/build.sh $(OUTJS)

web: patch ## browser build -> web/dd2
	bash $(ROOT)/tools/build_web.sh web/dd2

verify: native ## crash-free check: run the demo on all 10 levels (native), print N/10
	@echo "== native demo crash-check =="; ok=0; \
	for L in 1 2 3 4 5 6 7 8 9 10; do \
	  o=$$(cd $(GAMEDIR) && DD2_LEVEL=$$L timeout 120 $(NATIVE) 2>&1); \
	  if echo "$$o" | grep -q 'demo returned (no crash!)'; then echo "  L$$L ok"; ok=$$((ok+1)); else echo "  L$$L CRASH"; fi; \
	done; \
	echo "== crash-free: $$ok/10 =="

verify-wasm: wasm ## crash-free check: run the WASM demo (node) on all 10 levels, print N/10
	@echo "== WASM demo crash-check (node) =="; ok=0; \
	for L in 1 2 3 4 5 6 7 8 9 10; do \
	  rm -rf /tmp/wfv; mkdir -p /tmp/wfv; \
	  o=$$(cd $(GAMEDIR) && DD2_FRAMEDIR=/tmp/wfv timeout 120 $(NODE) $(OUTJS) $$L 2>&1); \
	  if echo "$$o" | grep -qiE 'abort|RuntimeError|exception thrown|SIGSEGV'; then echo "  L$$L CRASH"; else echo "  L$$L ok"; ok=$$((ok+1)); fi; \
	done; \
	echo "== WASM crash-free: $$ok/10 =="

run: wasm ## run the WASM demo under node at LEVEL=$(LEVEL)
	cd $(GAMEDIR) && DD2_FRAMEDIR=/tmp/wrun $(NODE) $(OUTJS) $(LEVEL)

shot: web ## headless browser screenshot of the web build -> /tmp/dd2_shot.png
	node $(ROOT)/tools/browser/shot.js $(ROOT)/web/dd2 /tmp/dd2_shot.png

refcapture: ## Stage-2 reference capture: run original dd2h.exe under Wine, dump live memory (needs WINEPREFIX)
	bash $(ROOT)/tools/refcapture.sh

clean: ## remove generated build/ and outputs
	rm -rf $(ROOT)/build $(OUTJS) $(OUTJS:.js=.wasm)
