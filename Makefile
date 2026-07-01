# DD2 → WASM reproducible pipeline (single entry point).
#
# Stages:  dd2h.exe --[Ghidra headless]--> re_out/dd2_decomp.c   (decompile; reproducible reference)
#          re_out/  --[transpile.py]-->   build/                (apply pipeline patches)
#          build/   --[emcc / gcc-m32]-->  dd2run.js / dd2_native
#
# NOTE on reproducibility: the engine C is mechanically decompiled (decompile.sh) BUT the ~186
# irreducible GTE/jumptable functions are a hand-reconstructed OVERLAY committed in re_out/ (the
# P-code-lift path that would make this fully mechanical was dropped — "wasm is a build target").
# So `make build` reproduces the build from the committed re_out overlay; `make decompile` + `make
# verify-decompile` re-derive and check the raw Ghidra layer that the overlay sits on.

ROOT    := $(CURDIR)
OUTJS   ?= /tmp/lvltest/dd2run.js
NODE    := $(firstword $(wildcard $(HOME)/Git/emsdk/node/*/bin/node) node)
GAMEDIR := $(ROOT)/DestructionDerby2
LEVEL   ?= 9

NATIVE  ?= /tmp/dd2_native

.PHONY: all build wasm native decompile pipeline run check verify verify-wasm clean help

all: build            ## default: transpile + WASM build from committed re_out

pipeline: decompile check native verify build verify-wasm ## FULL from-binary chain: dd2h.exe -> Ghidra -> transpile -> native+WASM build -> 10-level crash test (both targets)
	@echo "pipeline OK: dd2h.exe -> decompile -> patch -> compile (native + WASM) -> run  (~186 GTE/jumptable fns = committed re_out overlay)"

help:                 ## list targets
	@grep -hE '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN{FS=":.*?## "}{printf "  %-18s %s\n",$$1,$$2}'

build wasm: ## transpile re_out -> build/, emcc -> $(OUTJS)
	bash $(ROOT)/tools/build.sh $(OUTJS)

check: ## verify all transpile patch anchors still match the pristine decompile
	python3 $(ROOT)/tools/transpile.py --check >/dev/null && echo "check: all patch anchors OK"

native: ## transpile + native 32-bit build, NO-ASan (real crash semantics) -> $(NATIVE)
	ASAN=' ' bash $(ROOT)/tools/build_native.sh $(NATIVE)

decompile: ## re-run Ghidra headless: dd2h.exe -> re_out/dd2_decomp.c (reproducible reference layer, 906 fns)
	bash $(ROOT)/tools/decompile.sh
	@echo "decompile: $$(grep -cE '/\* ===== .* @ ' $(ROOT)/re_out/dd2_decomp.c) functions exported"

verify-wasm: build ## crash-free check: run the WASM demo (node) on all 10 levels, print N/10
	@echo "== WASM demo crash-check (node) =="; ok=0; \
	for L in 1 2 3 4 5 6 7 8 9 10; do \
	  rm -rf /tmp/wfv; mkdir -p /tmp/wfv; \
	  o=$$(cd $(GAMEDIR) && DD2_FRAMEDIR=/tmp/wfv timeout 120 $(NODE) $(OUTJS) $$L 2>&1); \
	  if echo "$$o" | grep -qiE 'abort|RuntimeError|exception thrown|SIGSEGV'; then echo "  L$$L CRASH"; else echo "  L$$L ok"; ok=$$((ok+1)); fi; \
	done; \
	echo "== WASM crash-free: $$ok/10 =="

run: build ## run the WASM demo under node at LEVEL=$(LEVEL)
	cd $(GAMEDIR) && DD2_FRAMEDIR=/tmp/wrun $(NODE) $(OUTJS) $(LEVEL)

verify: native ## crash-free check: run the demo on all 10 levels (no-ASan), print N/10
	@echo "== native demo crash-check (no-ASan) =="; ok=0; \
	for L in 1 2 3 4 5 6 7 8 9 10; do \
	  o=$$(cd $(GAMEDIR) && DD2_LEVEL=$$L timeout 120 $(NATIVE) 2>&1); \
	  if echo "$$o" | grep -q 'demo returned (no crash!)'; then echo "  L$$L ok"; ok=$$((ok+1)); else echo "  L$$L CRASH"; fi; \
	done; \
	echo "== crash-free: $$ok/10 =="

clean: ## remove generated build/ and outputs
	rm -rf $(ROOT)/build $(OUTJS) $(OUTJS:.js=.wasm)
