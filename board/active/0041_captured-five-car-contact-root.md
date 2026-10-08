Type: Work item
Title: Resolve the captured five-car contact failure
Depends: 0039, 0040

## Contract

Diagnose and correct the actual five-car/fourteen-contact tick-223,337 input
from the frozen d160c06 ordinary campaign. Preserve every contact, complete
physical/material acceptance, impulse/motion accounting, finite solver bounds
and exact rollback. Verify actual saved-owner advancement on current production,
independent Native/WASM/instrumented checks and ordinary continuation without
manufactured state or larger fixture bounds.

## Evidence

The original campaign fails valid advancement at round-2 tick 223,337, exiting
1 without timeout after 889.72 seconds. Its failure-only capture exits 1 without
timeout after 785.09 seconds and matches all 104 production records exactly.
One 31,216-byte owner and one fourteen-contact group are retained under /tmp;
participating slots are 1, 4, 11, 14 and 17. Recorded velocity error is
0.00075162998424393003 at 4,096 passes; position repair is not reached.

Frozen d160c06 and the current first-phase solver both reject the actual owner,
full twenty-body query and five-body remap. Current production and diagnostic
instruction bytes match; the cause and an admissible root remain unproved.
The 0040 saved-input correction does not resolve this separate state.
Receipt: /tmp/wasm-dd2/rewrite-sliding-pressure-0039/diagnosis/production-report.json.

Faithful read-only terminal observation records 4,096 velocity passes, one
restart, 191 accepted corrections and 111 rejected extrapolations. The private
branches are eligible in this solve; the initial-phase exclusion from 0040
does not explain this failure.

Independent geometry/inertia reconstructs terminal contact speeds within
4.26e-14. Selected active-set trials find an admissible root releasing world
row 1 on slot 14 and retaining all other rows. Full physical error is 3.29e-14,
and the released row separates at speed 0.04087746227902925. This selected
search does not establish uniqueness. A central-difference step sweep confirms
the analytic Jacobian within 9.23e-8; a Jacobian-free solve from a perturbed
seed independently reaches physical error 2.36e-14. The initial 1e-5 difference
step misses the 1e-6 check threshold; its 1e-6 step reduces the error by about
100 times, with smaller steps exposing rounding cancellation. The complete
sweep is retained rather than weakening the threshold.
Receipts: /tmp/wasm-dd2/rewrite-sliding-pressure-0039/diagnosis/terminal.json,
independent-root/report.json and finite-difference-report.json.

The private terminal-state comparison now identifies provisional normal
releases as an obstacle. The unchanged release trial reaches final physical
error 0.048178166027913513 after its sixteen steps; retaining its other normal
equations reaches 7.94e-15 within ten steps. Removing that release behavior from
all existing production release trials preserves the old 673 Native checks but
still fails the full actual query. That ablation is not adopted.

A separate complete fixed-active-set fallback preserves the existing warm/cold
trials and freezes the current projected normal branch. Exact zero targets apply
to released rows; retained normal equations survive negative full-step
predictions while nonnegative candidates and bounded backtracking settle the
nonlinear friction direction. Partial roots restore exact warm state. One base
model raises the finite capacity from 259 to 260; the existing 4,096 shared
passes, one matrix and sixteen refinement/backtracking steps remain.

The private full/remapped query succeeds in 673 velocity/61 position passes.
Its actual five-car query and twenty-eight orderings are now integrated, with
all 36 captured cases plus 666 orderings (702 independent physical checks) passing
the preliminary Native run. Mandatory strict Native/WASM and instrumented gates
are running; no multi-platform or actual-owner correction is yet claimed.
Receipts: /tmp/wasm-dd2/rewrite-five-root-0041/private-model-report.json,
private-retain-model-report.json, retain-release-report.json,
fixed-active-report.json and integrated-friction-report.json.

Integrated production now passes strict LLVM19/160-file formatting, 35 Native
and 33 WASM CTests, and all 702 independent contact checks on Native/WASM/ASan/
UBSan. Both full/remapped Native queries solve within the unchanged bounds;
motion agrees with the independent root within 9.15e-15. The actual 223,337 owner
advances to 223,338 on Native and instrumented C. The 168,287, 108,716, 115,420
and 190,078 Native owners still advance; frozen first-phase production fails
the new first regression. Pinned SoftGL is unchanged. Current-source broader
checks and ordinary continuation remain unproved; the work item stays active.
Receipt: /tmp/wasm-dd2/rewrite-five-root-0041/component-report.json.

All current-source broader gates now pass. Eleven original levels each pass
Native/WASM/ASan ground checks (79,100 independent queries and 26,400 drop steps
per target). Actual Native/instrumented/Chromium input checks pass 31/31/144
comparisons and all eleven scene/car views, with empty browser error lists.
Six selected race targets pass, including the complete eight-lap Circuit-5 race
on every variant; this is partial race-suite coverage. Source hashes match.
Receipt: /tmp/wasm-dd2/rewrite-five-root-0041/broader-report.json.

The fresh ordinary fixed-active campaign is now terminal: exit 1 without timeout
after 424.42 seconds, failing valid advancement at round-2 tick 142,893. All 88
records are retained. Player health is 0.18102661831774292 and laps remain zero.
Its collision trajectory differs from the older campaigns; the saved component
correction does not prove natural continuation. The exact frozen-source
failure-only capture is running under 0043; full season acceptance remains false.
Receipt: /tmp/wasm-dd2/rewrite-five-root-0041/natural-season/terminal-report.json.

## Next

Follow the exact current-trajectory capture under 0043. Preserve the saved
223,337 component correction while integrating the separate 0042 diagnosis;
require new current-source ordinary continuation and keep full seasons open.

## Accept

Current production advances the actual captured owner and passes focused
independent physical/material/motion/energy/position checks on Native/WASM/
instrumented C. Mandatory strict and reached original-data/input/race gates
pass. Ordinary continuation crosses the failure and dense region within the
unchanged bounds; complete-game and physical-season acceptance remain separate.
