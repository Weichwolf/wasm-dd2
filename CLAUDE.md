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
**STAGE 1 COMPLETE: BOTH targets are 10/10 crash-free** (`make verify` / `make verify-wasm`, all 10
demo levels reach completion on native AND WASM). A whole family of transpile.py guards (GEOM-GUARD/
GEOM-GUARD2/GUARD AG/AH/AH2/AK/AL/AM, FIX BK) defensively convert the symptoms of the Decompress
heap-layout divergence — wild pointers, runaway repeat-counts, unbounded corrupted-list walks — into
"skip this operation" at their point of use/creation, instead of crashing or hanging. The underlying
layout divergence itself (Stage 2) is still open — these guards make the demo run to completion
despite it, they don't fix the divergence, so Stage 2/3 work remains. Caveat: the
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
- **STAGE 1 (crash-free) IS COMPLETE as of commit `3d87a20`: both native and WASM are 10/10.**
  The last two fixes: `GUARD AL` converts `Modify_Sound`'s faithful-but-now-corruption-triggered
  bounds-check crash into a silent skip (fixed native L6); `GUARD AM` caps `MPE_free`'s own
  free-list search loop (an unbounded circular-linked-list walk) which hung forever once a
  corrupted free-list formed a cycle — a plain iteration count is safe here (unlike the `_gpoly`-
  stream handler guards below, this is a simple linked-list walk with no byte-alignment concern
  to break) — fixed native L2 (reached only after `GUARD AL` fixed the earlier crash masking it).
  Both verified together (`make verify` + `make verify-wasm`) after every change this whole
  session, per the hard lesson learned: several earlier attempts fixed one target/level while
  silently breaking another (see history below) — always check BOTH before trusting a "fixed!".
  Stage 2 (bit-identical memory / the heap-layout divergence itself) is still fully open: these
  guards make the demo reach completion DESPITE the divergence, they don't resolve it.
- (History below, kept for context.) **FIXED the geometry-corruption crash class** (transpile.py GEOM-GUARD + GEOM-GUARD2, commit
  `974e84e`): native demo now **8/10** crash-free (was 7/10), WASM demo **9/10** (was 7/10; L2's
  SIGFPE apparently doesn't manifest under WASM's int/float semantics). The two guards catch a wild
  pointer (`_gpoly`/`param_2`, e.g. `0x666666ff`) sourced from the Decompress heap-layout divergence
  at its point of use (`FUN_0041fb7c`) and point of creation (`Setup_Object_Block`->`Set_Object`),
  reusing the engine's OWN existing "-1 = invalid" convention rather than inventing new semantics.
  A first attempt at guard #2 (skip the Set_Object call entirely) regressed a previously-fine level
  into a NULL-deref hang/crash because it left the object's geometry pointer stale/zero while a later
  consumer (`Draw_Scene_Object`) unconditionally dereferences it — fixed by redirecting to a
  known-safe fallback address instead of skipping. Remaining failures are DIFFERENT, unrelated bugs
  exposed now that execution runs further: native L2/L6 = a SIGFPE in `Play_Race_Start_Sounds` and an
  OOB CLUT/text-array index in `draw_text_half`; WASM only L6 fails. Neither is the heap-layout/
  Decompress issue — new, separate leads for the next crash-free push.
- Confirmed this session (still true, now explains the ORIGINAL crash mechanism the guards target):
  **L2 crashes in the
  SAME function as L3** (`FUN_0041fb7c`, called via `Draw_Scene_Object_Blocks -> Draw_Scene_Object`)
  — identical mechanism. **L6 crashes in a DIFFERENT function** (`FUN_0041132c`, a quad/poly draw fn,
  called via `Draw_All -> DrawOTag -> _ot_dispatch`) but the same STRUCTURAL pattern: a poly-command
  struct pointer (`param_1`) read wild (crash addr `0xd2341c3` — clearly garbage, same signature as
  L3's `0x666666ff`). So all three share the same root (corrupt geometry from the Decompress-layout
  divergence), just surfacing through different rendering consumer functions (the poly walk in
  FUN_0041fb7c vs. the ordering-table/OT draw path in FUN_0041132c) — confirms a single fix (the
  heap-layout match) should clear all three, not three separate bugs. L3 ROOT-CAUSED (tools: `eipcatch.c` ptrace
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

## RETRACTED: the "+0x20000 shift" was a false positive (zero-region trivial match) — real finding below
Previously claimed a "clean +0x20000 (128KB) positional offset" with "~100% match" between builds'
game-data. RE-VERIFIED and this does NOT hold: the claimed-match region (our VA 0x77ebf0-0x7b2bf0) is
**0.000% nonzero in our build** (completely unwritten/zero) and the reference's SHIFTED counterpart is
also ~0.01% nonzero (also essentially all-zero) — i.e. the "100% match" was zero matching zero, which
is trivially true at ANY shift and proves nothing about real alignment. The REFERENCE's UNSHIFTED data
at that same absolute range is actually 29% nonzero (real content) — our build simply never wrote
anything there. LESSON: always check the nonzero fraction of a "matching" region before trusting a
correlation result; large zero-filled regions (common in this heap: unused reserved buffers, BSS,
freshly-mmap'd pages) will falsely "match" at almost any offset.
REAL TAKEAWAY: our build's `Order_Cars` breakpoint (used as the DD2_LEVEL=9 checkpoint) fires BEFORE
the level file's real content is loaded into [0x75ebf0,0x7debf0) — `_fi_levdat` is confirmed set by
then (value `0x760564`, via breakpoint on `FUN_00445b78`), but the bulk of the buffer past a small
header is still genuinely unwritten at this point in OUR build, while the reference (reaching its
`_current_level==9` moment via a much longer, more elaborate real front-end+DemoMode path) has already
written real content into the corresponding absolute range. This means `Order_Cars` is NOT a
sufficiently-late checkpoint for comparing loaded level content — need a LATER breakpoint (e.g. after
Init_Game/Init_Scene actually populates this buffer) for a meaningful content comparison. NEXT STEP:
find that later sync point in our build (a real function that runs after the level directory is fully
read) and re-verify any future "match" finding's nonzero fraction before trusting it.

CORRECTION (gdb-confirmed, supersedes the "stuck/hung" theory below): it's NOT a hang. Diagnostic:
after `_current_level==9` fires, `current_frame` (@0x462ff0) increments fine and FAST (~0.06s/step
under gdb) for 700+ frames (a full demo race), then RESETS to a low value as the reference cycles to
the NEXT demo level, and again after another ~700 frames — i.e. the reference happily runs multiple
full attract-demo races. But `_fi_levdat`/0x75eb60 stayed **exactly 0x0 for the ENTIRE first ~700-frame
level-9 race** (sampled every step up to step 750) — so `_fi_levdat` is almost certainly NOT the
address/mechanism the ATTRACT DEMO's level loading actually uses (maybe it's for a different, e.g.
menu-driven, load path; the "10 minutes at 100% CPU, no write" from the earlier attempt was simply
the SAME true story at coarser sampling, not a hang). RETRACT the `_fi_levdat`-based reference sync
plan. NEW APPROACH (validated as non-hanging): gate on `_current_level==9` first (alignment to the
right level), THEN watch `current_frame` and dump at a threshold comfortably inside the SAME cycle
(e.g. F=100, well before the ~700-frame reset) — this revives the original frame-count sync but scoped
to the correct level via the level-gate, avoiding both the "title inflates the counter" problem (gate
ensures we're already past title) and the "which demo cycle am I in" ambiguity (gate ensures level 9
specifically, not whatever the Nth cycle happens to be).

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

## Game-data divergence is STATIC from the earliest frames, and is NOT a positional shift (validated)
Using the new `levelframe` checkpoint (gdb-confirmed non-hanging), compared game-data at F=5 and
F=100 (both within the same level-9 demo race): **nearly identical match profile at both**
(58.28%/9.33% at F=5 vs 52.29%/8.80% at F=100, same per-64KB decay curve: ~38%→27%→8%→0%→0%→~1%→0%→0%).
This rules out "divergence accumulates during gameplay" — whatever differs is already fully present
by frame 5 and simply persists unchanged. Also re-ran the sliding-window cross-correlation (this time
against REAL, substantial content: both sides 50-70% nonzero, not the earlier zero-region false
positive) at both F=5 and F=100 — **no clean positional shift found** (best alignment only ~31%
against a 69.5%-nonzero needle, identical result at both F values). CONCLUSION: the earlier "+0x20000
shift, content is identical just displaced" idea is fully dead — the content is genuinely DIFFERENT
between builds, not just relocated, and this difference is baked in at LOAD TIME (not runtime drift).
TESTED AND REJECTED the "relocated pointer fields differ" hypothesis: checked the 0%-match region
(0x78ebf0-0x790bf0) dword-by-dword — ZERO of the ~2048 diverging dwords look like plausible code/image
pointers (0x400000-0x990000 range) in EITHER build. Instead: reference is **exactly 0x00000000
throughout this entire region**, while our build has repeating-byte patterns (`0x81818181`,
`0x88888888`, `0x87878d8c`, etc.) — classic indexed-bitmap/texture data (a run of similar palette
indices), not pointers. So: our build has real (texture-like) content sitting at a position where the
reference has nothing (all-zero) — i.e. AGAIN an our-build-has-extra-content-here pattern (like the
retracted +0x20000 case), just this time the "extra content" is non-trivial (not just reserved zero
padding) and doesn't match anywhere else in reference within a +-0x20000 search window, so it's not
a simple shift either. Best read: this specific piece of our build's data (likely a texture/geometry
sub-block) is placed at a heap offset the reference doesn't use for equivalent content at all — points
back to an allocation-order/count difference (we allocate something, in a position, that the reference
either doesn't allocate at all here or allocates with different size/placement) rather than a load-base
relocation-value bug. NEXT: identify what OBJECT/ASSET this specific our-build byte range corresponds
to (which MPE_malloc call populated it) to pin down the specific extra/misplaced allocation.

## FOUND the writer of the divergent "texture-like" bytes: FUN_00415160 (the level-asset file loader)
Bisected (via repeated breakpoint+read, since a hardware watchpoint mysteriously would NOT fire on
this address across 3 separate attempts/scopes — a real gdb/environment limitation worth knowing about,
NOT a "no write happens" signal) exactly where our build's `0x78ebf0` (inside `level_data_buffer`,
which starts at 0x75ebf0 per `Init_Game`'s `FUN_00415160(&level_data_buffer, Load_Completion_Status)`
call, dd2.c ~28405) goes from `0x0` to `0x81818181`: between entry and exit of `FUN_00415160`
(dd2.c:3274). That function `fread()`s level assets sector-by-sector (0x800-byte sectors) directly
from a file into the buffer, advancing by each asset's exact decompressed size afterward (a
per-asset-type handler in `File_Func_List` returns the real size). Verified dd2_image.bin (the static
image) is genuinely zero at this file offset and `.bss` overall is 99.28% zero — so this is NOT a
bad build artifact; the pattern is written at runtime, and it's simply STALE/UNINITIALIZED HEAP
CONTENT exposed because the read/handler doesn't fully overwrite this specific buffer position for
OUR build's allocation layout. This is the SAME symptom class as the already-established Decompress-
window bug (`## Decompress window = layout-dependent`, above) — stale heap bytes surfacing through a
DIFFERENT subsystem (the asset loader, not the LZSS decompressor) — reinforcing (not superseding) the
core conclusion: ONE heap-layout fix should clear this AND the Decompress/crash symptoms together.

## TRIED a source-level fix (Set_Object/Create_Object sanitize + FUN_0041fb7c guard) — REVERTED, made it worse
Traced the crash mechanism one level deeper: `Set_Object`/`Create_Object` (dd2.c:6625-6629, 6656-6660)
both contain the ONE-TIME self-relative fixup `*(param_2+0x28) = *(param_2+0x28) + param_2` (also
+0x20/+0x24) that converts a decompressed geometry block's stream-relative offset into an absolute
poly-command pointer — guarded by a "already relocated" flag at +0x1e. `Create_Object` already has a
`FUN_0041fb7c(param_1); if (iVar2 != -1) {...}` pattern, confirming the "-1 = invalid, don't use"
convention is real and intentional here, not something to invent.
Tried: sanitize the post-fixup +0x20/+0x24/+0x28 values in BOTH functions (null out if outside the
valid image+heap range 0x400000-0x900000), plus a matching `if (_gpoly==0) return -1;` guard in
`FUN_0041fb7c`. Result: **made things WORSE** (6/10, was 7/10) — L3 STILL crashed with the same
0x666666xx garbage signature (meaning the corruption reaches the crash site through a path this
patch didn't cover), AND L10 (previously fine) started HANGING instead of crashing — likely because
nulling the pointer while still setting the +0x1e "already done" flag leaves some OTHER downstream
consumer permanently waiting/looping on a geometry pointer it assumes must eventually be valid.
REVERTED cleanly (confirmed back to 7/10 baseline). LESSON: this crash has MORE consumers/paths than
currently mapped (Set_Object/Create_Object/FUN_0041fb7c is not the whole picture), and the +0x1e
"done" flag has other semantic dependents that a purely-local null-check breaks. Before trying this
class of fix again, first find EVERY reader of the +0x20/+0x24/+0x28 fields and the +0x1e flag,
not just the ones seen in stack traces so far.

## Native L6 lead: sound-channel slot (0x900eec+N*0x28) gets scribbled AFTER correct init (new thread)
Native L6 (post GEOM-GUARD/GUARD AG) no longer segfaults; instead exits cleanly via the ORIGINAL
game's OWN error path: `Update_Engine_Sound` (dd2.c:29754) calls `Modify_Sound(iVar7,...)` with
`iVar7=32576` (garbage) read from a per-car sound-channel slot at `0x900eec + iVar8*0x28`.
`Modify_Sound`'s `if (param_1<0 || 3<param_1) System_Error(...)` (dd2.c:4034) is FAITHFUL ORIGINAL
GAME BEHAVIOR (a real bounds check the original devs wrote) — not a decompile bug, so don't guard/
silence it; the real bug is upstream, whatever corrupts the slot. Traced the ONLY writer of this
field: `FUN_00447960` (called from `Init_Game`, dd2.c:29566-29571) correctly sets all 4 slots to
0,1,2,3 — VERIFIED via gdb (`break FUN_00447960; finish` shows `0x900eec..+0x78 = 0,1,2,3` right
after). So the corruption happens LATER, from some OTHER out-of-bounds write scribbling over this
STATIC (not heap) memory region — a NEW, separate lead of similar depth to the Decompress/geometry
chase, not yet root-caused. Native L2 also changed crash location after the GEOM-GUARD/GUARD AG
fixes (new SIGSEGV address, not yet decoded) — likely a downstream consequence of the same guards
changing execution timing/flow. Both are fresh leads for a future session; WASM doesn't hit either
(10/10), so they may be native-harness-specific (e.g. FP/timing differences) rather than the core
engine bug — worth checking that angle first before assuming they're WASM-relevant too.
UPDATE: L2 is now FIXED (FIX BK, commit fd9e135). L6's writer is fully identified: `Draw_Object_Polys`
(dd2.c:7320) dispatches `case 0x41bc0c: FUN_0041bc0c((int)sVar2); break;` with `sVar2` read from the
SAME corrupted `_gpoly` stream this whole session has chased — confirmed via gdb, `FUN_0041bc0c`
gets called with `param_1=15934`, wildly implausible for what should be a small poly/vertex repeat
count. `FUN_0041bc0c`'s loop (dd2.c:45500-45511) runs `param_1` times, advancing `_gprim2`/`_gprim1`
by 0x1c each iteration with NO bounds check — 15934 iterations walks `_gprim2` far outside any valid
buffer, landing on the sound-channel struct (0x900f14) purely by arithmetic coincidence and scribbling
it via `*(undefined1*)(_gprim2+7) = 0x30`.
TRIED AND REVERTED (both real regressions, not just "didn't help"):
  1. GUARD AI: fall back `_gprim2` to `_gprim1` in `Draw_Object_Polys` if the INITIAL value lands
     outside the dynamic heap `[0x7debf0,0x8febf0)`. Didn't fire — the value is fine at assignment
     time and only drifts bad after thousands of legitimate-looking `+0x1c` increments inside
     `FUN_0041bc0c`'s own loop, so a one-time check at the top of `Draw_Object_Polys` can't catch it.
  2. GUARD AJ2: clamp `param_1` to 0 (skip the loop) in `FUN_0041bc0c`/`FUN_0041bc68` when > 256.
     This DID stop the corruption/crash, but turned it into a genuine HANG (confirmed: exit code 124,
     full timeout) — skipping the loop entirely apparently leaves `_gpoly` un-advanced in a way the
     CALLER's outer while-loop depends on, so the same corrupt command gets reprocessed forever.
  3. GUARD AJ3: clamp `param_1` to 1 instead of 0 (loop still runs once, advancing `_gpoly` the way
     the caller's outer walk needs, per the GUARD AJ2 postmortem). This DID fix native (10/10!) --
     but broke WASM: L6 went from working to `RuntimeError: memory access out of bounds` (9/10).
     REVERTED (WASM is the primary target; a native-only win isn't worth a WASM regression).
     Root cause of the platform split: native's `_gprim2` ends up at a real-but-wrong address
     (0x900f0d, a valid mmap'd page just holding the wrong struct) that native's flat address space
     tolerates as "readable/writable garbage elsewhere", whereas WASM's linear memory is smaller/
     stricter and traps on the SAME first-iteration write that native shrugs off -- so even ONE
     iteration with a bad starting `_gprim2` is unsafe under WASM, while native only breaks once
     the iteration count runs long enough to reach 0x900f0d specifically (many iterations in).
GUARD AK (commit 719d9e4, KEPT — safe, no regression): guards the individual byte-writes inside
`FUN_0041bc0c`/`FUN_0041bc68`'s loop body against the valid heap+image range, leaving the loop's
iteration count and `_gpoly`/`_gprim*` advancement completely untouched (avoids both the hang and the
WASM trap from the two reverted attempts). Verified via gdb: this DOES stop the corruption through
these two functions specifically — but the SAME sound-channel slot still gets corrupted via a
DIFFERENT handler, `FUN_0041a2f4` (dd2.c:5683, `*(undefined2*)(_gprim1+9)=...`), dispatched from the
SAME switch in `FUN_0041fb7c`/`Draw_Object_Polys`. **The vulnerability class is broader than scoped**:
likely present across several/all of the ~12 poly-command handler functions in that dispatch switch
(FUN_0041a2f4, FUN_00417ea0, FUN_0041861c, FUN_00418ed0, FUN_0041bc0c✓, FUN_0041bc68✓, FUN_0041c3e4,
FUN_0041c440, FUN_0041ccf8, FUN_0041cdd0, FUN_0041d834, FUN_0041d918, setup_face_sprite — ✓ = guarded),
each walking `_gprim1`/`_gprim2`/`_gpoly` with counts/offsets sourced from the same corrupted stream.
TRIED GUARD AK2 (FUN_0041a2f4, dd2.c:5644) — REVERTED, another regression trade-off. Guarded the
WHOLE per-iteration body (all writes to `_gprim1` AND the inner copy loop writing through
`local_14`/`_gprim2`) with one condition, leaving only the advancement (`local_14+=10`,
`_gpoly+=0x14`, `_gprim1+=10`) outside it — same recipe that worked for GUARD AK. Result:
**this DID fix L6** (native reached "demo returned" for level 6!) but **L2 regressed into a hang**
(confirmed: exit code 124). So `FUN_0041a2f4`'s skipped body apparently computes/uses something
(maybe the CLUT-index `iVar1`, or the copy loop itself) that something DOWNSTREAM depends on for L2's
specific flow — unlike `FUN_0041bc0c`/68 where skipping the writes was fully safe. Reverted; confirmed
clean 9/10 (L6 only, no hangs) and WASM still 10/10.
LESSON: this handler-by-handler guarding is NOT a mechanically-safe repeatable pattern — each function
needs to be judged individually for what's safe to skip vs. what must still execute (e.g. maybe only
skip the WRITES but still run the CLUT-lookup/index computation, rather than skipping the whole body).
Two native-only levels (L2, L6) have now each been fixed AND regressed by DIFFERENT guard attempts at
DIFFERENT times this session — treat any single "fixed!" result on one level as provisional until the
FULL 10-level sweep confirms no other level moved backward, every time.
NEXT: for `FUN_0041a2f4` specifically, try guarding ONLY the actual writes (the `_gprim1[1]=`,
`*(undefined2*)(_gprim1+...)=`, and the copy-loop's `*puVar3=*puVar2` assignment) while still computing
`iVar1`/reading `_gpoly` and still running the CLUT-lookup `if` unconditionally, matching the
"un-write, not un-compute" principle GUARD AK used successfully. For the remaining ~9 other handlers,
apply the SAME careful, per-function judgment — not a blind copy-paste of the GUARD AK/AK2 template.
TRIED GUARD AK3 (refined: guard ONLY the write STATEMENTS individually in `FUN_0041a2f4`, leaving
every read/compute/the CLUT-lookup condition unconditional — exactly the "un-write, not un-compute"
refinement above) — REVERTED, same net result as AK2. This DID stop the hang (clean exit(1) instead),
so the refinement direction is right, but the crash-free count stayed 9/10: L2 now corrupts the SAME
slot via a **THIRD, unrelated function** — `FUN_00416a10` (dd2.c:5115), called directly from
`Draw_Scene_Object` (dd2.c:16517), NOT even part of the `FUN_0041fb7c`/`Draw_Object_Polys` dispatch
switch this session has focused on. **The true scope is now confirmed larger than a dozen sibling
handlers**: it spans at least two distinct call sites/families (the poly-command dispatch switch, and
whatever direct-call path `FUN_00416a10` belongs to), each needing its own `_gprim1`/`_gprim2`-writer
audit. Reverted GUARD AK3 (added complexity, zero net count improvement); confirmed clean 9/10
(L6 only) + WASM 10/10 restored.
ASSESSMENT: closing native to 10/10 via this whack-a-mole path is a substantially larger undertaking
than originally scoped (likely 3+ more functions across at least 2 different call-path families, each
needing the same careful "guard only writes, verify both native+WASM, watch for count regressions"
treatment GUARD AK required). Kept only GUARD AK (the one fix proven both safe and effective). Given
WASM (the primary target) has been 10/10 since earlier this session and stayed there through 6
build+test cycles, further L6 work is better scoped as its own session with a plan to inventory EVERY
`_gprim1`/`_gprim2` writer reachable from a poly-command stream (not just the ones a specific crash
happens to surface), rather than continuing to discover them one crash at a time.

## TRIED a systemic fix (zero the Decompress dest buffer) instead of per-consumer guards — much WORSE
Reasoned that if corrupt counts/offsets consistently became 0 (instead of random wild values), `for`
loops with a 0 count would naturally execute zero iterations, sidestepping the WHOLE family of
"handler walks _gprim1/_gprim2 too far" bugs without needing per-function guards. Tested the existing
env-gated `DEBUG ZEROBUF` patch (`tools/transpile.py`, zeros the Decompress destination's own 0x4000
bytes at entry) with `DD2_ZEROBUF_PATCH=1` + runtime `DD2_ZEROBUF=1`: **3/10** (much worse than the
9/10 baseline). Root cause of the regression: blocks decompress CONTIGUOUSLY into a slot (established
earlier this session) — zeroing the FULL destination before each Decompress call wipes out valid
geometry from adjacent/earlier blocks already decompressed into the SAME buffer, corrupting far more
than it fixes. CONFIRMS: zeroing is not a free win at this granularity; any zero-based fix would need
to target ONLY the actual missing "window" bytes (the ~0x1000 before each block's own output, per the
established Decompress-window finding), not the whole destination — a much more surgical, harder
change, not attempted here. Native remains 9/10 (L6 only), no change from this test (the debug patch
is env-gated and off by default; nothing was committed from this experiment).

## Stage 2 attempt: watching the free-list head (0x73c290) on the reference gives garbage, not pointers
Tried reconstructing the reference's MPE_malloc carve sequence via a DATA watchpoint on
`_DAT_0073c290` (the free-list head, per dd2_symbols.h) after `_current_level==9` — sidesteps the
established CODE-VA mislabeling problem since this is a data watchpoint (which have worked reliably
elsewhere: `current_frame`@0x462ff0, `_current_level`@0x936ff4). Result: NOT usable. The watched
value cycles through a repeating byte-fill pattern (`0x76767676`, `0xaaaa8484`, ... shifting one byte
at a time across ~40 hits) — clearly a bulk texture/geometry fill happening at or through this
address, not a real pointer. This means `_DAT_0073c290` is NOT functioning as "the live free-list
head" at this point in the reference's execution/memory layout — either the free-list has already
been fully consumed and this region reused for something else, or (less likely given other DATA
addresses work) this specific address is also mismapped. NEXT: don't reuse this exact address/
technique; if pursuing the allocation-sequence angle again, first confirm a candidate data address
is stable/sane in the reference (e.g. read it multiple times across frames, expect small integer or
plausible-pointer values) before trusting a watchpoint result from it.

## Stage 2: heap-base transition is a clean, stable, comparable divergence (validated data point)
Watched the heap base (`0x7debf0`, NOT the free-list-head variable — that gave garbage, see above)
directly, on both builds:
- **Reference**: already `0x1d1d1d1d` (a real, non-self-pointer value) by the moment
  `_current_level==9` fires (frame 0!) — and STABLE (no further watchpoint hits) for the ~200s of
  gameplay observed after. So the reference exhausts/overwrites the base free-list slot very early
  (likely during front-end/menu processing before the race even starts) and the value never changes
  again during the race.
- **Our build**: still the `MPE_InitHeap` self-pointer (`0x7debf0` itself) at `Init_Scene`'s entry
  AND exit (i.e. after the 14 decompress-slot allocations run) — confirmed via gdb, `DD2_LEVEL=9`.
  Never observed to transition in the checkpoints tried.
This is a clean, STABLE (not timing-sensitive) divergence, good for future bisection: our build's
total allocation footprint apparently never exhausts the base slot exactly (MPE_malloc's carve only
overwrites the base header when a later allocation's size exactly matches the remaining free space —
the `*puVar5 = *puVar3` merge case), while the reference's does. This points at an allocation SIZE or
COUNT mismatch somewhere in the sequence (not just alignment/timing) — a genuine lead for the next
session's bisection, distinct from (and more stable than) the earlier frame-based checkpoint noise.

## CORRECTION: 0x1d1d1d1d is STABLE for the whole level-9 race, not front-end residue (gdb-confirmed)
Extended the heap-base watch (0x7debf0) on the reference across the FULL level-9 race this time
(not just a snapshot): the value stays `0x1d1d1d1d` from `_current_level==9` all the way to frame
~701 (the end of the race, where the reference cycles to the next demo level and the value finally
changes to `0x80000501`). No intermediate write was ever caught — meaning `_current_level==9`
genuinely fires AFTER level 9's own `Init_Game`/`MPE_InitHeap` has already fully run (contradicting
my earlier guess that it precedes `Play_Game`; likely `_current_level` gets written again somewhere
inside `Order_Cars`/`Init_Game` too, and gdb's watchpoint caught THAT later write). So `0x1d1d1d1d`
IS the real, stable, post-Init post-Init_Scene heap-base content for level 9's race — not leftover
front-end data. Byte-interpreted, `0x1d1d1d1d` is FOUR REPEATED BYTES (0x1d,0x1d,0x1d,0x1d) — the
same texture/geometry-fill signature as the earlier `0x81818181` finding (`FUN_00415160` section),
not free-list pointer metadata (which would look like a large heap address, not a repeated byte).
CONCLUSION: the reference allocates real texture/geometry content AT this exact heap position
(0x7debf0, the very base of the heap) at some point during level 9's setup, while our build's
allocation sequence apparently never reaches/exhausts the base slot at all (still the raw
`MPE_InitHeap` self-pointer through `Init_Scene`). This is a clean, validated, STABLE Stage-2 target:
whatever allocation the reference makes that lands exactly at the heap base is either missing,
differently-sized, or differently-ordered in our build.

## Conventions
- **Never edit decompiled code; fixes are transpile patches (see Pipeline).** Compat layer is editable.
- Commit/push only when asked. Faithful reconstruction from the binary — no approximations/band-aids.

## Stage 2: VALIDATED +0x38400 placement shift in level_data_buffer (concrete, actionable)
Cross-correlated the game-data dumps (0x75ebf0-0x7debf0) from a lockstep run, WITH nonzero validation
(the key discipline after the earlier zero-region false positive): our build's level content at
`level_data_buffer` (0x75ebf0, confirmed exact symbol) appears in the REFERENCE at a CONSISTENT
+0x38400 shift — verified by 512-byte EXACT matches of 43-68%-nonzero content at 3 independent offsets
(our 0x75ebf0->ref 0x796ff0, our 0x75fbf0->ref 0x797ff0, our 0x762bf0->ref 0x79aff0). So the reference
places the same level data 0x38400 (230400) bytes LATER than we do; equivalently ours is 0x38400 too
early. NOTE 0x38400 = 3 x (320x240) = 3 screen-sized 8bpp buffers — suggestive of a triple-buffer or
3-surface allocation the reference does BEFORE the level directory that ours doesn't (or does smaller).
Our 0x75ebf0 holds a live level-directory/offset table (0x197, 0x00800004, ... small offset/size
pairs); the reference has ZEROS there until 0x774460 (so the ref either loads the directory 0x38400
later, or frees+zeros this early-load region by the checkpoint while ours leaves it resident).
Writer is `FUN_00415160` (the sector-by-sector asset file loader, Init_Game path) which advances the
buffer pointer by each asset's size — a different asset sequence/sizes before the shared content would
produce exactly this constant offset. NEXT: inventory the asset load list (DAT_00462cd8 head /
DAT_00462cdc count, filled before FUN_00415160 runs) and compare total bytes loaded before the level
directory, native vs reference — the 0x38400 gap should be one identifiable missing/mis-sized asset.

## Stage 3 (playability): input path WORKS (verified), commit 05cf05e
The engine's live-input model: Win32 msg loop -> `Translate_Keypress(vkey, lparam)` (bit31 of lparam
= key-up) sets `_pad_*` boolean globals by matching vkey against the keymap `Setup_Pad` loads; the
per-frame read packs those into the control word @0x71c048. Wiring = platform key event -> Windows VK
-> Translate_Keypress. DONE: `re_out/dd2_input.c` (hand-written compat shim) provides `dd2_key_event`,
`dd2_browser_key_to_vk` (browser KeyboardEvent.code -> VK), `dd2_browser_key_event`, `dd2_input_selftest`.
Wired into both build UNITS. `DD2_INPUTTEST=1 /tmp/dd2_native_na` -> PASS (ArrowUp/Down/Left flip
_pad_lup/ldown/lleft). Required FIX KEYMAP (transpile.py): Translate_Keypress compared the VK against
keymap globals Ghidra mis-typed as `*(int*)` (4-byte reads) so NO key ever matched -- live input was
100% dead; the x86 compares bytes. Cast each RHS to (unsigned char). Both targets still 10/10.
Active keymap (Setup_Pad(1), map @0x46757a): ENTER=accept, ESC=back, arrows=steer/accel/brake, F1/F2,
SPACE, W/S/A/Z. NEXT Stage-3 steps: (1) run in PLAY mode (demo_mode=0 race, or Front_End menu) instead
of the recorded-pad attract demo; (2) browser: hook canvas keydown/keyup -> dd2_browser_key_event and
drive a continuous render loop (web/shell.html canvas + putImageData of g_pixels@0x700450 already
exist in a stale web/build); (3) verify menu navigation + a controllable race end-to-end.

## Stage 3: LIVE gameplay (demo_mode=0) runs crash-free; input reaches control word (verified)
`DD2_PLAY=<lvl>` (native_main.c PlayModeLevel) runs a real race with demo_mode=0 (live input, not the
recorded-pad attract replay). MAJOR: it runs CRASH-FREE and returns (419 frames on L9) — real gameplay
works now, not just the demo (this path crashed pre-Stage-1-fixes, per the old dd2_runtime.c comment).
Input pipeline VERIFIED end-to-end during the race (gdb): keyboard -> Translate_Keypress -> _pad_*
(persist, confirmed _pad_lleft=1 held) -> FUN_00422c74 (keyboard branch active, DAT_0046303e==0) ->
control word `_DAT_0071c048=0x80` (left-steer bit) / edge word c04a. `DD2_HOLD=0x25` holds LEFT for the
whole race. REMAINING GAP: frames were IDENTICAL between no-input and held-left — the control word is
correct but the player CAR doesn't visibly respond. State during race: _current_player_car=1,
camera_car=1, race_car=2, num_cars=20. The two 0x80-readers found (Camera_Pad_Control@0x429738,
Enter_Driver_Names) are camera/menu, NOT car steering — so the car-steering path reads the pad control
differently (likely a per-car control byte set from _DAT_0071c048/c04c for the human car, gated by a
"which car is human" binding the front-end normally sets; camera_car=1 vs race_car=2 mismatch is a
clue). NEXT: find where a car's steering/throttle is fed from the pad control (search the car-update/
physics path for the human-car control assignment) and ensure PlayModeLevel designates the human car
correctly (front-end sets this; the cold PlayModeLevel path may need to replicate that one binding).

## Stage 3: car-control chain fully traced; steering not yet applied (precise open question)
Traced the human-car control path end to end. Play_Game race loop (dd2.c:10086-10101): car control is
gated behind `_DAT_0074be98 < 1` -- i.e. the pre-race INTRO camera-flyby (`_DAT_00744b74`: 0xa000->0,
demo_mode=0 only) THEN the 3-2-1 countdown (`_DAT_0074be98`: 100->0). Added env-gated DEBUG NOCOUNTDOWN
(transpile.py, DD2_NOCOUNTDOWN_PATCH build + DD2_NOCOUNTDOWN runtime) to zero both at race start so
`FUN_00441394(_current_player_car, 0x71c048)` -> `Control_Car_Replay(car, ctrl)` runs from frame 1.
VERIFIED CORRECT bindings: all player-car indices are 0 (image current_player_car@0x905ad0=0, C-global
_current_player_car=0, camera_car@0x463eec=0) -- so the human controls car 0 AND the camera follows
car 0 (no dual-symbol divergence here, good). Control_Car_Replay IS reached, and the control word is
correct (c048/c04c=0x80 for held LEFT, verified). BUT: car 0's steering angle (0x75a682, which
Control_Car_Replay writes on ctrl&0x80) stays 0 -- so the steering block (dd2.c:29209-29250) is being
SKIPPED. It's gated by `if (DAT_00467074 == 0)` then `if (iVar3 == 1)` where iVar3 = byte at
0x71c051 (DAT_0071c051, the pad-type, =1 for keyboard). PRECISE NEXT: check DAT_00467074 (should be 0;
if nonzero, that's the block-skip -- find what sets it and whether the cold PlayModeLevel path leaves
it wrong) and confirm Control_Car_Replay's param_1 is really 0. Also: holding UP/accel (0x26) also
produced identical frames -- so throttle isn't applying either; same gate likely. tools: DD2_PLAY=<lvl>
(live race), DD2_HOLD=<vk> (hold a key), DD2_NOCOUNTDOWN (skip intro/countdown), all native-only.

## Stage 3 MILESTONE: the human car now RESPONDS to keyboard input (FIX PADTYPE/PADTYPE2)
Cracked the "car doesn't respond" blocker. Root cause: two int-vs-byte Ghidra typing artifacts in
FUN_00422c74 clobbered the pad-type byte at 0x71c051. The control structure at 0x71c048 packs:
0x71c04e-4f = a 16-bit field (_DAT_0071c04e), 0x71c050 = a flag (DAT_0071c050), 0x71c051 = pad-type
(DAT_0071c051, set to 1 for keyboard). Control_Car_Replay INTENTIONALLY reads 0x71c04e as an int and
`>>0x18` to extract the pad-type byte, then requires it ==1 to apply steering/throttle. But
`_DAT_0071c04e = val` (Ghidra `*(int*)`) wrote 4 bytes, zeroing 0x71c050 AND 0x71c051; and
`DAT_0071c050 = 0` (also `*(int*)`) zeroed 0x71c051 too. Net: pad-type always read 0 -> steering block
skipped -> car frozen. FIX PADTYPE (byte-write DAT_0071c050) + FIX PADTYPE2 (2-byte-write the 3
_DAT_0071c04e sites), keeping the int-typed READS unchanged. VERIFIED: with input held, car 0's
steering angle (0x75a682) goes 0 -> -64, and rendered frames now DIFFER between held-left and no-input
(4.5% pixels) -- the car visibly responds. Both targets STILL 10/10 crash-free + input selftest PASS,
no demo regression. Test path: DD2_PLAY=9 DD2_HOLD=0x25 (+ DD2_NOCOUNTDOWN to skip the intro for quick
testing -- note NOCOUNTDOWN itself is an unstable debug skip that segfaults after a few frames; a
normal live race runs the intro+countdown and is crash-free). REMAINING Stage 3: (1) menu navigation
(Front_End reads the same control word -- should now work), (2) browser: wire canvas keydown/keyup ->
dd2_browser_key_event + a render loop, (3) end-to-end playable verification.

## Stage 3: front-end RENDERS menus; menu-nav proof + browser build are the remaining work
DD2_FE=1 (native_main Front_End path) RENDERS the menu (11419 frames dumped in a short run) -- the
menu screens display via the same engine. Two open items:
1. Menu-nav proof: injecting a held key from frame 0 (DD2_HOLD in the DD2_FE path) does NOT navigate
   -- the menu uses EDGE-triggered nav (_DAT_0071c04a = newly-pressed), and a key held from frame 0
   fires one edge event DURING the intro animation (Rotate_Slab_On, before the menu accepts input),
   which is wasted; edge nav needs a PULSED key (down then up) at the right time. A blocking headless
   loop can't inject timed pulses -- this needs either a gdb-injected pulse mid-loop or the browser
   (real key up/down). The control-word plumbing itself is proven working (car responds), and
   Front_End reads the same _DAT_0071c04a, so nav SHOULD work with proper pulse timing.
2. Front-end idle-demo CRASHES: after ~1800 idle menu frames Front_End launches its own DemoMode
   (attract), which segfaults (~frame 11419). This is the FRONT-END-initiated demo path, distinct from
   the DD2_LEVEL=N demos that are verified 10/10 -- a separate crash to run down for full front-end
   robustness.
Browser build: a full harness EXISTS (tools/browser/navshot.js presses keys via Playwright +
screenshots; shot.js/multishot.js render web/build under headless Chromium/SwiftShader). BUT web/build
is STALE (Jun 22, pre-all-fixes) and the emcc-browser build command that produced it (SDL2 + canvas +
ASYNCIFY + --shell-file web/shell.html + --preload DestructionDerby2) is NOT committed in tools/ -- it
must be reconstructed. That build (with dd2_browser_key_event wired to canvas keydown/keyup, running
Front_End->race) is the real "wasm port completely playable" deliverable and the main remaining Stage-3
task. NEXT: reconstruct the browser build script, wire canvas input -> dd2_browser_key_event, rebuild
web/build from current code, then use navshot.js to verify menu nav + race control end-to-end.

## Stage 3: front-end idle-demo crash root = Track_Follow wild pointer (Stage-2-class, delicate)
Characterized the front-end crash (DD2_FE=1, ~frame 11419): SIGSEGV in Track_Follow (dd2.c:12329,
`iVar5 = (uint)*pbVar9 * 8`) where `pbVar9 = _strip_data + 4 + iVar10` and iVar10 (from the track-strip
struct pbVar17+0x1c/+0x14) is garbage -> wild pbVar9. Backtrace: DemoMode -> Play_Game -> Init_Game ->
FUN_004430b8 -> FUN_00440ac4 -> Track_Follow. This is the front-end's OWN idle-triggered DemoMode
(after ~1800 idle menu frames), and it crashes where the DD2_LEVEL=N demos (verified 10/10) do NOT --
because the front-end runs its menu first, leaving different heap/rand state (same Stage-2 heap-layout-
divergence root: corrupt track-strip data -> garbage iVar10). This is the original "Track_Follow OOB"
the harness was built for. NOT guarded yet: Track_Follow is delicate core track-following logic (a
rushed bounds-guard on pbVar9 could break the 10/10 demos); it needs careful study of the switch/case
control flow to find a safe skip that's a no-op on valid data. IMPORTANT for playability scope: the
menu itself is USABLE before the ~1800-frame idle timeout -- this crash is the attract-demo the menu
falls into when idle, not the menu navigation itself. Menu-nav proof via gdb-injected key pulse was
inconclusive (the front-end crashes easily when poked mid-loop); the definitive menu-nav test is the
browser (navshot.js, real key up/down timing) once web/build is rebuilt from current code.

## GUARD AN (Track_Follow guard) — TRIED, REVERTED: regresses native L2 (do not retry naively)
Attempted to guard Track_Follow's 6 wild-pbVar9 sites by `goto switchD_00426fe6_caseD_5` (the case-5
finaliser that uses the car's previous strip param_1[5], not pbVar9 -- looked like a safe "stay on
current strip" skip). Cleared the front-end Track_Follow crash BUT regressed native L2 (10/10->9/10):
L2 legitimately computes a pbVar9 my range-check flagged, or the skip drifts L2's track-following into
a downstream crash. Reverted (WASM stayed 10/10, but never keep a change that regresses a target).
LESSON: Track_Follow is genuinely delicate -- the front-end idle-DemoMode crash chain (Track_Follow ->
then Pre_Rotate@dd2.c:6729 with a garbage vertex count 49239, via Draw_Subdiv_Object -> more...) is a
whack-a-mole rooted in the Stage-2 heap-layout divergence, NOT safely guardable per-site the way the
render-path GUARDs were. Proper fix is Stage 2 (match the heap layout so the track-strip/geometry data
isn't corrupt in the first place). The front-end MENU itself is usable before the ~1800-frame idle
timeout; this crash is only the attract-demo it falls into when idle.

## Stage 2: MPE_malloc carve is FAITHFUL (not the bug) — divergence is allocation total/order
Verified our 1st MPE_malloc returns 0x89ebf0 (HIGH end of the 0x7debf0..0x8febf0 heap: a 0x60000 chunk
at the top) -- so MPE_malloc carves from the HIGH end, and after Init_Scene the base (0x7debf0) is
STILL the free-list head (self-pointer), i.e. the base block is never consumed in our build. The
reference has real DATA at the base by the same checkpoint. KEY INSIGHT that rules out a whole class of
fix: the decompiled MPE_malloc IS the reference's own code (same binary), so the reference carves
high-end IDENTICALLY -- this is therefore NOT a carve-direction reconstruction bug. High-end carving
means the base is the LAST region consumed (the free block shrinks from the top down), so the base is
only allocated once the heap is (nearly) FULL. The reference fills the base => it allocates more/to a
fuller heap than we do; our base staying free => we allocate LESS total (or free more). This matches
the +0x38400 level_data_buffer finding (our loaded content is 0x38400 shorter). So the two Stage-2
divergences (level_data_buffer +0x38400 in the asset loader FUN_00415160; MPE heap base-not-consumed)
are both "our build allocates/loads less or differently", NOT a carve/relocation mechanic. Do NOT
change MPE_malloc's carve. The real Stage-2 work is finding the specific missing/mis-sized/mis-ordered
allocation(s) -- which still needs the reference's allocation sequence (blocked by the MPE_malloc code
breakpoint being unreliable; data-watchpoint on the heap size field 0x7debf4 is the workaround to try,
timed to attach before the level-9 Init_Game allocations run).

## Stage 3 MAJOR MILESTONE: the decompiled engine RENDERS THE GAME IN A BROWSER (commit below)
IMPORTANT clarification: web/build is a SEPARATE from-scratch WebGL reimplementation (console shows
geo_load/assets/raw/*.DAT/WebGL) -- NOT the dd2h.exe port. Created the port's OWN browser build:
- tools/build_web.sh: emcc with -DDD2_BROWSER -sASYNCIFY (+preload Dirinfo/dd2_image.bin/SaveGames,
  --shell-file web/shell_port.html, export _dd2_browser_key_event). Same engine C as the node build.
- dd2_com.c ids_flip: #ifdef DD2_BROWSER -> EM_JS dd2_present() blits g_pixels@0x700450 (320x240 8-bit)
  -> <canvas> via g_palette, then emscripten_sleep(0) yields to the browser (ASYNCIFY) each frame.
- web/shell_port.html: canvas + keydown/keyup -> Module.ccall('dd2_browser_key_event', code, down).
VERIFIED via Playwright (tools/browser/shot.js on web/dd2): the port loads ("Created Primary Surface"
/"Creating Palette" = the engine's own output) and RENDERS THE ACTUAL DD2 SCENE -- 3D banked track,
cars, "Race Over"/"DEMO MODE" text, correct palette (see /tmp/port_render.png). The dd2h.exe decompiled
engine runs and renders in a browser via WASM. Playwright/headless-Chromium works in this env.
Both headless targets STILL 10/10 (the render hook is #ifdef-guarded; node/native builds unaffected).
Build: `bash tools/build_web.sh web/dd2`; test: `node tools/browser/shot.js web/dd2 out.png` or
navshot.js for keyed nav. NEXT: the browser build currently runs the recorded-pad DEMO (demo_mode=1,
so keys don't steer yet); switch it to interactive (Front_End menu or a live PlayMode race with
demo_mode=0) so the wired keyboard input actually drives menus/car -> end-to-end playable.

## Stage 3: browser INTERACTIVE (demo_mode=0) now RENDERS THE LIVE RACE (3 fixes this session)
The interactive path (demo_mode=0, live keyboard) previously crashed in WASM with "table index is out
of bounds"; now it renders a full live race in the browser. Three fixes:
- **DRAWPRIM-GUARD** (transpile.py, commit): DrawPrim@0x412885 did a RAW indirect call
  `_primfuncs[type](prim)` with no relocation-awareness, unlike the OT loop's `_ot_dispatch` (which
  already routes through a relocated-call/image-switch/default guard). dd2_relocate() only rewrites
  `_primfuncs` entries whose value is in its fnmap; an un-rewritten entry (the pause-overlay tile's
  type 0x28, reached ONLY on demo_mode=0 via Pause_Mode->DrawPrim) keeps a raw image VA -> under WASM
  a wild function-table index -> trap. Routed DrawPrim through `_ot_dispatch` (defined just above it).
  Native unaffected (relocated ptrs >=0x460000 -> called directly, as before); both targets 10/10.
- **Spurious-pause fix** (DemoModeLevel injection): the cold level-launcher skips the front-end that
  establishes the pad type, so Setup_Pad recorded `_recorded_pad_type=0` (BSS) while the race's
  FUN_00422c74 sets `DAT_0071c051=1` (keyboard) every frame -> `DAT_0071c051 != _recorded_pad_type`
  (Play_Game dd2.c:10166) -> demo_mode=0 immediately entered Pause_Mode (showed the PAUSE menu, not
  the race). Seed `*(unsigned char*)0x71c051 = 1` BEFORE Setup_Pad so the recorded type matches.
  Harmless for demo_mode=1 (pause gated on demo_mode==0).
- **DD2_LEVEL env** in DemoModeLevel: browser has no argv (DemoModeLevel(0) -> random level); DD2_LEVEL
  lets the browser/harness force a level (low levels <8 use the simpler `_DAT_0075a6c6==1` race-end,
  vs level>7's `active-cars<2` which ends fast from divergent car-state -- a Stage-2 symptom).
VERIFIED (tools/browser/drivetest.js -- holds a key in headless Chromium): level-1 interactive renders
a GORGEOUS correct race START scene -- start-line light gantry, full grandstands, track markings, HUD
"Speed 000 KPH / Race Points / Lap 01/10 / Still Running / pos 06". The game LOGIC advances (position
06->07, scene evolves, 7 unique frames captured) -- it is NOT frozen. To force interactive in the
browser: build with a shell that sets `Module.preRun=[function(){ ENV.DD2_LIVE='1'; ENV.DD2_LEVEL='1'; }]`
(needs `-sEXPORTED_RUNTIME_METHODS` to include `ENV`, added to build_web.sh).
CAVEAT (headless perf, NOT a bug): SwiftShader software raster + ASYNCIFY runs the headless browser at
~0.1 fps, so the ~215-frame camera flyby + 100-frame countdown before player control takes minutes of
wall-clock -- too slow to reach Speed>0 within a practical test window here. On a real-GPU browser
(30-60fps) the countdown is ~7s. Input->car itself is already PROVEN natively (FIX PADTYPE/PADTYPE2:
steering 0->-64, 4.5% pixel diff held-left vs none) and native demo_mode=0 runs the full race
crash-free (DD2_LIVE=1 -> 1501 frames, "demo returned"). So interactive is functionally working; the
only gaps are (a) a real-GPU browser run to visually confirm driving end-to-end, and (b) Stage-2
(divergent car-state makes level>7 races end early, and the node headless demo_mode=0 stalls before
the first flip -- a native-vs-WASM computation divergence in the demo_mode=0-only flyby/control path,
same Stage-2 root). tools: tools/browser/drivetest.js `<buildDir> <outDir> <holdKey> [shots][stepMs][preMs]`.

## Stage 3: interactive (demo_mode=0) WASM path is now CRASH-FREE (GUARD AO, commit)
After DRAWPRIM-GUARD + the spurious-pause fix, the WASM interactive path still crashed after ~19s
with "memory access out of bounds" in MPE_malloc's free-list walk. Symbolicated the stack
(--emit-symbol-map): main->DemoModeLevel->Play_Game->Draw_Scene_Object_Blocks->Draw_Scene_Object->
FUN_00416a10->MPE_malloc. MPE_malloc chases the free-list `puVar2=*puVar3` then reads `iVar1=puVar3[1]`;
a free node's next-ptr had been corrupted to a wild address (object-leak/over-walk symptom of the
Stage-2 heap divergence) -> OOB read. GUARD AO (transpile, **WASM-only #ifdef __EMSCRIPTEN__**):
range-check the walk node against [0x400000,0x900000); on a wild node force puVar3=head and break so
MPE_malloc returns head^head==0==NULL -- the engine's OWN OOM signal, which faithful callers already
handle (FUN_00416a10 dd2.c:4751 checks ==0/==0xffffffff -> returns -1). MUST be WASM-only: an
unconditional guard regressed native L1 (SIGSEGV 0xd58ecf0) because native's flat address space reads
the wild node's garbage as mapped memory and CONTINUES the walk (finding later valid blocks), whereas
WASM's linear memory traps -- the documented native-tolerates/WASM-traps split. Both targets 10/10;
WASM interactive L1 now survives >150s (was 19s). NOTE on perf: node WASM interactive is ~0.3fps but
this is NODERAWFS blocking DISK I/O (profiled: process is I/O-blocked, not CPU-bound -- per-frame
geometry streaming via fread), NOT a compute megaloop; the browser preloads files into MEMORY so this
particular slowness does not apply there (the browser's separate slowness is SwiftShader software
raster). REMAINING interactive gaps: (a) real-GPU browser run to visually confirm end-to-end driving
(headless SwiftShader too slow to clear the flyby+countdown in a test window), (b) whether the race
runs to completion under WASM with input (native does: 1501 frames "demo returned"); both are
functional/perf confirmations, not known crashes. The demo_mode=0 crash class is now closed for the
levels tested (L1/L9); a full 10-level interactive sweep is the natural next verification.
