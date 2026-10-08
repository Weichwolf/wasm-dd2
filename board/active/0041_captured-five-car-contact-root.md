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

## Next

Identify why existing bounded release/refinement models miss the verified root
before changing production. Compare private merit, branch selection and
rollback against complete physical acceptance. Continue
observing the frozen first-phase ordinary campaign as a separate scope.

## Accept

Current production advances the actual captured owner and passes focused
independent physical/material/motion/energy/position checks on Native/WASM/
instrumented C. Mandatory strict and reached original-data/input/race gates
pass. Ordinary continuation crosses the failure and dense region within the
unchanged bounds; complete-game and physical-season acceptance remain separate.
