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
**WASM is 10/10 crash-free** (`make verify-wasm`, all 10 demo levels reach completion with no
abort/RuntimeError/exception). Native is **9/10** (only L6 remains — a sound-channel-slot
corruption, see "Current state" below). GEOM-GUARD/GEOM-GUARD2/GUARD AG/GUARD AH/GUARD AH2/FIX BK
(transpile.py) defensively guard the symptoms of the Decompress heap-layout divergence at their
points of use/creation — the underlying
layout divergence itself (Stage 2) is still open, but no longer crashes the demo. Caveat: the
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
- **FIXED the geometry-corruption crash class** (transpile.py GEOM-GUARD + GEOM-GUARD2, commit
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

## Conventions
- **Never edit decompiled code; fixes are transpile patches (see Pipeline).** Compat layer is editable.
- Commit/push only when asked. Faithful reconstruction from the binary — no approximations/band-aids.
