Type: Work item
Title: Choose stronger physical contact corrections
Depends: 0022

## Contract

Compare projected and sticking Newton directions from the same exact input motion
and impulses. Retain the accepted candidate with the smaller physical residual.
Preserve Coulomb friction, finite motion, unilateral contacts, exact rollback,
contact count, convergence tolerances and the shared 4,096-pass bound.

## Evidence

The published Native Stockcar season probe reaches a reproducible advancement
failure at second-circuit tick 59,399. One 31,216-byte input checkpoint reproduces
it in one step. Only driver 8 participates in the failed three-world-support group;
first-improvement branch selection exhausts all 4,096 passes with 0.0001371838
world units/s residual. Diagnosis: /tmp/wasm-dd2/rewrite-stock-season-diagnosis/.

A frozen remapped one-body query fails the published solver while all fifteen
previous queries pass. Comparing both directions fixes the step. All sixteen
queries now pass independent final contact, friction, reaction and energy checks
on Native, WASM and ASan/UBSan. The new query uses 2,273/2,723/1,988 passes,
respectively, and retains the single allowed coordinate restart. Strict LLVM19
for 157 files and all 34 Native/32 WASM CTests pass. Evidence is under
/tmp/wasm-dd2/rewrite-world-branch-selection/selection-report.json. Source
identities remain unchanged through the required checks.

The production checkpoint advances from tick 59,399 to 59,400. The unchanged-bound
Native natural season completes its first round at tick 287,081 and proceeds to
tick 66,449 in round 2, where a different four-world-support failure is reproduced
and retained under 0027. This is partial season progress, not season acceptance.

Two selected original-data race scenarios pass all six Native/WASM/sanitized
target scopes: a short Circuit-2 Stockcar/DNF session and a naturally completed
eight-lap twenty-car Circuit-5 race. The latter finishes at tick 167,472 with
75 player points on all three targets. Scoped report:
/tmp/wasm-dd2/rewrite-world-branch-selection/race/report.json.

All eleven actual Native, sanitized window and Chromium/WASM presentation/input
regressions pass: /tmp/wasm-dd2/rewrite-world-branch-selection/window-final/report.json.
The Native championship observer acknowledges changed presentation before pause;
the browser observer checks the trusted restart click at exact tick zero rather
than requiring a later Pause click to precede the countdown's end. Both modes
reset the real twenty-car field and retain their actual score/round ownership.
These observers do not modify the production application or extend timeouts.

## Next

Follow the independently reproduced mixed-world-support failure under 0027.
Complete championships and the remaining natural arena failure stay in 0025/0002.
This correction-selection contract is proved; completed raw captures are removed
after the identity-bearing receipts, with the next diagnosis retained.

## Accept

The new failing query and prior captured/analytic cases pass independent physical
conditions on Native/WASM and instrumented C. The saved failing input advances
with the production solver; actual scheduled racing reaches past that failure
without changing controls, race bounds, damage or scoring. Mandatory LLVM19 and
Native/WASM gates plus actual application presentation pass. This establishes the
correction-selection contract; complete game acceptance remains open.
