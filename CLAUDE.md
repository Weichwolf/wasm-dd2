# Destruction Derby 2 → WASM

Port `dd2h.exe` (1996 PC game) to a reproducible C build (native-primary, WASM as a build target).
**Code is the truth.** Verify every claim against the binary (`DestructionDerby2/dd2h.exe`) or a run — do not trust prose.

## Goal / acceptance
The deterministic attract-demo must be **crash-free on all 10 levels** and **bit-identical** to reference
`dd2h.exe` (compare the 8-bit indexed framebuffer `@0x700450`, 320x240) plus audio sample-for-sample.
Then: fully playable (menus, track-select, race; keyboard + pad). Engine C is mechanically derived from
Ghidra; only the platform/runtime shim is hand-written (DirectDraw→g_pixels/WebGL, DirectSound→WebAudio, Win32/CRT).

## Pipeline (RULE: never edit decompiled code)
`dd2h.exe → decompile → patch → compile → run` — and it must run identically, in Wine too.
- **NEVER hand-edit the decompiled engine source (`re_out/dd2.c`, `re_out/dd2_*.c`).** It must stay the
  pristine Ghidra output so re-decompiling reproduces it. Re-running `decompile.sh` would wipe hand edits.
- **All engine fixes go in `tools/transpile.py` as anchor-asserted `sub()` patches** (`re_out/*.c → build/*.c`).
  Each patch asserts its anchor still exists, so decompile drift is caught loudly. `--check` verifies anchors.
- The ONLY hand-written code is the platform/runtime compat layer (`re_out/dd2_com.c` DirectDraw shim,
  `dd2_win32.c`, `dd2_filio.c`, `tools/native_main.c`) — editing those is fine.
- `tools/decompile.sh` → `re_out/dd2_decomp.c` (+ `re_out/dd2.c`, pristine). Ghidra can't decompile the
  register-convention GTE fns (drops reg args as `unaff_E**` / bare `GTERPT()`); `tools/recover_regargs.py`
  recovers those from the binary disasm (feed into transpile patches, don't hand-write).
- NOTE: `re_out/dd2.c` currently still holds legacy hand-edits (pre-rule). Those should be migrated into
  transpile patches over time so `re_out/` is fully regenerable.

## One-command pipeline
`make pipeline` = `dd2.exe → Ghidra decompile → check anchors → native build+verify →
WASM build (emcc→dd2run.js)+verify` — runs the attract demo on all 10 levels for BOTH
targets and prints N/10. (`make verify` = native only; `make verify-wasm` = WASM only.)
Both targets are **7/10** crash-free (L2/L3/L6 = the Decompress-layout bug). Caveat: the
~186 GTE/jumptable fns are the committed `re_out` overlay (P-code lift = criterion 1,
abandoned), so the chain reproduces from that overlay, not fully mechanically.

## Build & run (native debug)
- `ASAN=' ' bash tools/build_native.sh /tmp/dd2_native_na`  — **use no-ASan for real crashes**: ASan
  false-positives on the engine's hand-computed global-pointer arithmetic (faults at low addrs like 0x20).
- Demo: from `DestructionDerby2/`, `DD2_LEVEL=N /tmp/dd2_native_na` (prints "demo returned (no crash!)").
- Front-end: `DD2_FE=1 /tmp/dd2_native_na`.
- Reference (works here — no install needed): `WINEPREFIX=… xvfb-run -a wine dd2h.exe`.
  Extract its 8-bit framebuffer for bit-compare: ptrace-read `0x700450` (320x240) from the live
  process (ptrace_scope=1 → the reader must launch wine as its own descendant; see /tmp/refcap.c).
  Our build dumps the same surface via the `ids_flip` hook (`DD2_FRAMEDIR=…`). Compare byte-for-byte.

## Current state (verify, don't trust)
- Demo **7/10** crash-free (no-ASan); L2/L3/L6 crash. L3 ROOT-CAUSED (tools: `eipcatch.c` ptrace
  SIGSEGV→EIP catcher + `DD2_PLOG_PATCH`/`DD2_PLOG` BSS ring buffer of poly commands, both env-gated):
  faulting insn = walk-loop `while(*(char*)(_gpoly+3))` at dd2.c:6600 with `_gpoly=0x666666ff`. The ring
  buffer proved the poly stream up to the crash is VALID (small counts) — so it is NOT a per-handler
  stride bug. The crashing object's `FUN_0041fb7c` loads `_gpoly = *(iVar3+0x28)` already = `0x666666ff`
  and crashes on the FIRST while-check (before any command records). => a SCENE OBJECT reaches the draw
  path with a corrupt data pointer. Ties to the object LEAK/over-walk: in L3 `_free_mem` drops
  monotonically (objects created, never freed via `Remove_Object`; obj+0x19 lifetime countdown gated by
  obj+0x1a/obj+6 doesn't fire) → `num_scene_objects`/array over-walk → garbage object. TRUE heisenbug
  (shifts under any in-loop probe). FULLY TRACED via `tools/wpcatch.c` (ptrace HW watchpoint = debug
  registers, NO memory perturbation → doesn't move the heisenbug): the crashing object is VALID +
  in-bounds (block6/idx3, num=22, geom ptr p1[1]=0x8020c8 valid heap); a HW watchpoint on 0x8020c8+0x28
  caught the WRITER = **`Decompress`** (build/dd2.c:3543, the LZ geometry decompressor) writing decompressed
  `0x66`('f') bytes into that buffer. So the on-demand geometry streamer (`Decompress(&dec_info)` from the
  scene-object paths re_out:16660/16677/16801/16842/16992) DECOMPRESSES INTO A BUFFER A LIVE SCENE OBJECT
  STILL REFERENCES → corrupt +0x28 → wild `_gpoly` → crash. FIX = the streaming buffer lifecycle (dec_info
  dest must not collide with a referenced object; or the object must be removed before its buffer is reused).
  Tools (all env-gated/standalone): `eipcatch.c` (fault EIP), `wpcatch.c` (HW watchpoint), `DD2_PLOG`/`DD2_OBJLOG`.
- Renders the 3D demo scene AND the front-end **title screen** ("DESTRUCTION DERBY 2" logo) — both verified
  by capturing `0x700450`. The menu uses the SAME 3D engine as the race but is static/deterministic + far
  fewer objects (no desync) → the cleanest bit-exact target. Title compare still low (palette/align), WIP.
- **Reference capture works** (`tools/refcap.c` ptrace-reads `0x700450`; `tools/eipcatch.c` catches faults).
  Our build dumps the same surface (`DD2_FRAMEDIR=…`, `DD2_CFDUMP=1` keys on `current_frame`@0x462ff0).
  Bit-compare blocked by alignment: our `DemoModeLevel` skips the intro the reference shows; counters differ.

## Dispatch-table VAs are WRONG for at least MPE_InitHeap/MPE_malloc/Decompress (gdb-confirmed)
Tried for hours to breakpoint the reference at the dispatch-table VAs (0x4235c0/0x4235e4/0x415550).
Every attempt (gdb `break`, raw ptrace POKETEXT, even a from-scratch hardware breakpoint via
`gdb wine` before any code runs) either silently never fired or, when it did stop, disassembled to
garbage — e.g. `x/10i 0x4235c0` in the LIVE reference process shows nonsense opcodes, not the simple
`_mem_size=param_2; *param_1=param_1; ...` leaf function Ghidra decompiled. Traced it BYTE-EXACT:
the real function boundary (clean `push ebp; mov ebp,esp` prologue) for the code Ghidra assigned to
"MPE_InitHeap @ 0x4235c0" is actually at **0x423594** — i.e. 0x4235c0 falls INSIDE an unrelated
function (a block-copy routine calling 0x456770). Sanity-checked the METHOD itself is sound by
disassembling the PE's real AddressOfEntryPoint (0x456c34, independently computed from the PE header)
in the live process — it decoded perfectly. So: the tooling is fine, but Ghidra's recorded VA for
these specific functions (at least these 3; unclear how many others) does NOT match a real function
start in the raw binary. This does NOT affect OUR build (dd2_dispatch.c's addresses are apparently
vestigial/cosmetic for these entries — our build calls MPE_malloc etc. as normal compiled C functions,
which is presumably why it still works). It DOES mean: raw-VA reference breakpoints on these specific
functions are unreliable; use DATA watchpoints (current_frame @0x462ff0, _current_level @0x936ff4,
which DO work and are gdb/gdb-gdbstub-confirmed) instead, or find the real VA independently before
trying to breakpoint code. QEMU-user (qemu-i386) was tried as an alternative to ptrace/gdb but Wine
hangs under qemu-user's linux-user-mode emulation (its threading model isn't fully supported) — not
a viable path. WINEDEBUG=+virtual,+module IS useful (confirms load base 0x400000, section mapping,
matches static PE headers exactly) but only shows top-level VirtualAlloc/module-map events, not
in-game MPE_malloc calls (those are pure game-internal C logic, invisible to Wine's own tracing).

## Lockstep alignment: sync on _current_level==9, NOT current_frame (measured, use this)
Frame-count sync (current_frame >= F) is weak: the reference's title screen inflates current_frame
before the demo/level even starts, so a raw threshold doesn't line up the same engine state in both
builds. Switched `tools/lockstep.sh` (MODE=level, now the default) to sync on **_current_level
transitioning to 9** (ref @0x936ff4; ours via breaking at `Order_Cars` with `DD2_LEVEL=9`) — a
semantically meaningful checkpoint (start of Init_Game for the demo level) both builds reach
regardless of front-end/title cycling length. Measured improvement in the SAME run: heap match
58%->74.44%, game-data match 9%->54%. First divergence still at heap VA 0x7debf0 (base free-list
header): reference already has real content there (`1d 1d 1d 1d 1d 1d 1f 1f 1f 1c 1a...`) while ours
is still all-zero (`00 00 00...`) — i.e. by this checkpoint the reference has already carved through
and OVERWRITTEN the base header (a later allocation's carve exactly consumed the remaining free block,
triggering MPE_malloc's `*puVar5 = *puVar3` merge-forward), while ours hasn't — a concrete, actionable
allocation-order difference to chase next. (MODE=frame preserved in lockstep.sh for comparison.)

## BREAKTHROUGH LEAD: clean +0x20000 (128KB) positional offset in the game-data region (gdb-confirmed)
Cross-correlated our build's game-data dump against the reference's (both at the `_current_level==9`
checkpoint, DD2_LEVEL=9 baseline — see below for why that's the right one to use). Found: for our
build's game-data region starting at VA 0x77ebf0 (0x20000 bytes into the [0x75ebf0,0x7debf0) window),
comparing against the REFERENCE's data 0x20000 bytes EARLIER (ref_va = our_va - 0x20000) gives a
**perfect ~100% match for a ~0x35000-byte stretch** (was 54% unshifted). This is the strongest,
cleanest evidence in the whole project so far: **the level-file content itself is byte-IDENTICAL
between builds — this is purely a PLACEMENT/offset bug, not a data or decompression bug.** Our build
has an extra (or misplaced) ~128KB somewhere before this point that the reference doesn't have, so
everything from there on is shifted +0x20000 in our layout relative to reference's. (Match degrades
again past our VA ~0x7b6bf0 — a SECOND, separate divergence further out, not yet characterized.)
NEXT STEP: find what occupies our build's [0x75ebf0, 0x75ebf0+0x20000) that the reference either
doesn't have or sizes differently. RE-CONFIRMED this session: `_fi_levdat` (the level-data pointer,
stored AT VA 0x75eb60 per dd2_symbols.h) holds VALUE `0x760564` in our build once set (breakpoint
`FUN_00445b78`, the fn containing dd2.c:28343-28345's `_level_data=_fi_levdat; *_fi_levdat =
*_fi_levdat + (int)_fi_levdat` self-relative fixup — gdb-verified this session). Reference's
corresponding value not yet captured (watchpoint on 0x75eb60 didn't fire within budget — may be set
much later than our build's equivalent point, or need more patience/a background run). Likely still
ties to an in-place relocation/offset-table bug in the level-data loader
(re_out/dd2.c:28343-28345: `*_fi_levdat = *_fi_levdat + (int)_fi_levdat` — a self-relative offset
table fixup whose result depends on the LOAD ADDRESS, which is exactly the kind of thing that would
produce a clean, constant, size-independent-of-content shift like this). To reproduce: dump
[0x75ebf0,0x7debf0) from both builds at DD2_LEVEL=9's Order_Cars breakpoint (see git history /
tools/lockstep.sh for the exact commands), then cross-correlate with a sliding-window byte search
(see the analysis in the commit adding this section) rather than a fixed-offset diff.

## Front-end-history hypothesis: tested, did NOT improve alignment (negative result, keep DD2_LEVEL shortcut)
Hypothesis: the reference runs ~1800 front-end idle-loop iterations (`re_out/dd2.c:34635-34661`,
`local_18 = 0x708` countdown in the menu loop) before its OWN `DemoMode()` call — menu/title screen
allocations before level 9's race heap gets built — while our `DD2_LEVEL=9` harness path
(`DemoModeLevel` in `tools/native_main.c`) skips straight to the demo, with ZERO prior heap history.
Tested by running our build the FAITHFUL way (`DD2_FE=1`, which confirmed it DOES naturally reach
`Order_Cars`/`_current_level=9` after the same idle loop) and comparing its heap/game-data against
the same reference dump. Result: heap match got slightly WORSE (74.44%->72.83%) and game-data match
got MUCH worse (54.02%->10.33%). So "did the front-end run or not" is NOT the (or not the whole)
explanation — likely our front-end's OWN behavior (timing, which menu screens/assets get touched,
maybe non-deterministic real-time-based counters rather than pure loop-count) diverges from the
reference's specific front-end path in ways that hurt more than the skip helps. CONCLUSION: keep
using the `DD2_LEVEL=9` shortcut (DemoModeLevel) as the baseline for lockstep comparisons — it's
measurably better-aligned than the "faithful" front-end path. The heap-layout divergence root is
elsewhere (allocation/carve order within Init_Game itself, not pre-existing front-end history).

## Memory transaction log (how to journal heap changes for the bisect)
A "transaction log" of memory changes is the right tool to find the first divergence. Granularities:
- ALLOCATION journal (best for layout): log every MPE_malloc(size)→addr / MPE_free(addr); the heap
  layout is fully determined by this sequence. `tools/refmalloc.c` does this for the REFERENCE via raw
  ptrace `PTRACE_POKETEXT` software breakpoints (attach to dd2h.exe, INT3 @MPE_malloc 0x4235e4, read
  size at [esp+4]). KEY: gdb's `break` CANNOT write INT3 to Wine's read-only code pages (silently never
  hits) — only POKETEXT works; gdb is fine for DATA watchpoints (hardware) like lockstep's 0x462ff0.
  Our build: just `gdb break MPE_malloc` (symbol) — works (captured baseline below). CAVEATS: (1) the
  wine launch in this env is flaky (pid timing, prefix ownership) — use `WINEPREFIX=<scratchpad>/wineprefix`
  and a wait-for-pid loop; (2) NEVER `pkill -f dd2h.exe` — it matches and kills the agent's OWN shell
  (the cmdline contains "dd2h.exe") → "exit 144, no output"; use `pkill -x dd2h.exe`; (3) the agent shell's
  120s default timeout cuts off long wine+gdb runs — pass a longer tool timeout.
- WRITE journal (every byte change): mprotect the heap region read-only, SIGSEGV handler logs {PC,addr,
  old→new}, single-step, re-protect. No VA needed. Easy in our native build; scope it to Init_Game/stream
  so it isn't unusably slow. Reference: same via ptrace single-step (slow) or 4 HW watchpoints (targeted).

## Stage-2 layout bisect — baseline + method
Compare the MPE_malloc SIZE sequence after the level-9 `MPE_InitHeap` (alignment-insensitive code
point; the first differing size = the first divergent allocation). OUR build's level-9 sequence:
`0x60000 0x60000 0x14800 0x14800 0x10 0x10 0x10 0x10 0x4000×14 0x81c 0xfec 0xc0c 0x81c 0x81c 0x81c 0x8a4 …`
(0x60000×2 = prim buffers; 0x14800×2; 0x10×4; 0x4000×14 = the 14 active_object_blocks decompress slots;
then 0x81c/0x8a4 object descriptors). Get the reference's via gdb: `break *0x4235c0 if *(int*)($esp+4)==0x7debf0`
(MPE_InitHeap, level heap) then `break *0x4235e4` (MPE_malloc), log `*(unsigned*)($esp+4)`. CAVEAT: gdb
on the Wine process is SLOW (init under gdb can take minutes) and the agent's shell timeout (120s default)
cuts it off — run the reference logging with a long timeout / patiently. Tools: `tools/lockstep.sh` (heap
dump at a frame, fast via watchpoint), `tools/refcap.c`/`wpcatch.c`/`eipcatch.c`.

## Decompress window = layout-dependent (the precise mechanism, gdb-confirmed)
Blocks decompress CONTIGUOUSLY into a slot (gdb @Decompress L3: out=0x801ba0, 0x801faa, 0x8023b5…).
Block N's LZSS back-refs read up to 0x1000 BEFORE its output, i.e. into the previous blocks AND, for
the first block in a slot, into the **heap memory preceding the slot base** — the "window". That window
in our build is uninitialized/stale (zeros or 0x66), not the valid bytes the reference's heap layout puts
there → garbage decompressed pointer fields (+0x28=0x666666ff). The source at param_1[0] is the
faithfully-loaded compressed level file (not the bug). So there is NO decompressor-logic or source bug —
the fix is purely the HEAP LAYOUT (Stage 2): the memory around each decompress slot must byte-match the
reference. => Stage 1 ⊆ Stage 2; chase the layout via the harness, not the decompressor.

## Stage 1 ↔ Stage 2 entanglement (verified)
Crash-free (Stage 1) is NOT independent of bit-identity (Stage 2): the L2/L3/L6 crash IS the
corrupt-decompressed-geometry symptom of the heap-LAYOUT divergence. A `_gpoly` bounds guard
only moved the fault (FUN_0041fb7c → Draw_Object_Polys → Set_Object/Setup_Object_Block), since
the garbage block is consumed in object SETUP and DRAW alike. Zeroing the decompress window
flips the garbage to `0x0` (NULL deref), so `Decompress` produces wrong POINTER fields regardless
of window value → its back-ref reads the wrong position. DECISIVE next test: harness-compare our
decompressed block bytes vs the reference's — same source+offsets ⇒ window/buffer-position bug;
different ⇒ source/relocation (load-base) bug. Either way the fix is the layout/load, not a guard.

## Conventions
- **Never edit decompiled code; fixes are transpile patches (see Pipeline).** Compat layer is editable.
- Commit/push only when asked. Faithful reconstruction from the binary — no approximations/band-aids.
