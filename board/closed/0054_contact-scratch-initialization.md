# 0054: Reduce redundant contact scratch initialization

## Contract

Remove redundant zero writes from internal counted contact scratch storage.
Preserve every live entry, source-axis tie order, earliest event, physical law,
solver/query bound and public output. Natural Arena-8 acceptance stays in 0053.

## Evidence

A bounded 120-second V8 sample of unchanged ordinary Arena-8 WASM reaches tick
9911 and attributes 11.86 percent of samples to memset. The largest identified
callers are the fleet resolver and static car SAT. The named private link and
sampling overhead prevent a production or whole-scenario timing claim. Kernel
perf sampling is unavailable with perf_event_paranoid=3; no kernel settings change.

The candidate fills live event/body/contact prefixes before reading them,
initializes neighborhood counters/overflow explicitly, writes the first pending
refinement window in full, and clears only the found flag on degenerate SAT axes.
Tie selection reads other axis fields only when found is true. No unused capacity
has public meaning, and no geometry, arithmetic or iteration bound changes.

Eight alternating actual 6000-tick owner prefixes produce exactly the frozen
per-target public fields. Two reversed-order comparisons reduce child CPU by
1.48..1.92 percent on Native and 5.50..6.01 percent on the private named WASM link.
This is prefix evidence, not full-case speed or natural-result acceptance.
Nine physical/ownership corpora pass with 24 fresh shared units and patterned
automatic storage under O1 ASan/UBSan, including all 742 independent physical
checks. Six Native Memcheck corpora pass with actually uninitialized scratch,
zero undefined reads/errors and no definite/indirect leaks. The private one-window
test initially omitted its required build definition; rebuilding that target
with the CMake definition passes. This was a verifier configuration error.

Receipts: /tmp/wasm-dd2/rewrite-arena8-profile-0053/
{profile-report,prefix-report,scratch-sanitizer-report,memcheck-report,
scratch-budget-build-error,production-identity}.json.
Production differs from the checked candidate only by comments/formatting;
preprocessed non-whitespace contents agree. Strict LLVM19 checks all 170 C/header
files; 38 Native and 36 WASM CTests pass. All eleven original ground levels,
220 recovery cases and 45 previously proved race scenarios pass again per target.
Receipt: /tmp/wasm-dd2/rewrite-arena8-profile-0053/verification-report.json.
Completed raw prefixes, profiler output and component binaries are removed after
recording their hashes and reports.

## Next

The counted-scratch contract is proved. Continue the actual natural Arena-8
owner under 0053 with the unchanged 120000-tick/1800-second limits. Its new
Native/sanitized runs reach the same natural results; WASM still times out
after 86042 complete ticks. The full public tick-77251 checkpoint matches the
prior source exactly. Diagnose late field motion/cost under 0053; no isolated
whole-case speed or natural WASM result claim.
Receipts: /tmp/wasm-dd2/rewrite-arena8-profile-0053/
{terminal-owner-report,late-checkpoint-report}.json.

## Accept

Counted live storage is fully initialized before use; bounded/degenerate/invalid
and rollback paths pass Memcheck and patterned sanitizer checks. The frozen
actual prefixes retain every public field with recorded scoped CPU comparisons.
Strict LLVM19, required Native/WASM gates and original ground/recovery/prior race
contracts pass. Full natural arena completion is explicitly outside this item.
