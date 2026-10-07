Type: Work item
Title: Preserve late contact Jacobian accuracy
Depends: 0020

## Contract

Resolve the captured production thirteen-body/eighteen-contact failure while
preserving every prior independent material case, unchanged constitutive laws,
physical residual acceptance and the 4096-pass budget. Keep bounded automatic
storage, strict LLVM19 and Native/WASM stability.

## Evidence

The unchanged ea6a440 WASM arena B fails at race step 52778, elapsed 52378.
Linker-wrapped unchanged library objects match five production checkpoints and
the failed pose. The compact query fails on Native/WASM with residuals
2.1006819e-5/1.8766935e-5 after 4096 passes and one restart.

Identical saved Newton states show direction differences decreasing as the
relative forward difference falls from 1e-5 through 1e-6 and 1e-7. A private
1e-6 candidate passes all thirteen independent contact queries and ten analytic
material cases with GCC Native/sanitized and WASM. The new query takes
2881/2913/2846 passes with no restart. A sqrt(DBL_EPSILON) variant is rejected:
it fails the existing twelve-body constitutive oracle under ASan/UBSan.
Changing residual row weights alone also fails the new query. Reports and
source/binary identities: /tmp/wasm-dd2/rewrite-delayed-wasm-failure/.

The first production candidate is rejected: its Native release material test
finds a 6.3693e-22 predicted normal impulse at a contact separating at 0.59757
units/s. Projected residual acceptance permits stopping before the ordinary
unilateral response clears it; the independent final friction oracle rejects
the resulting direction. Report:
/tmp/wasm-dd2/rewrite-contact-accuracy-verification/rejected-fd-only/.

The corrected candidate certifies convergence only after an ordinary sweep,
which clears separating impulses without extra work outside the pass budget.
Its thirteen independent queries and analytic laws pass with release LLVM19,
WASM and fully instrumented LLVM19 ASan/UBSan. The new query takes
3073/2913/2657 passes; the old five-contact cycle still needs one restart and
477 passes.

Production passes make rewrite-check (32 Native CTests and strict LLVM19 for
145 owned C/header files), make rewrite-wasm and ctest --preset rewrite-wasm
(31 CTests). Fully instrumented production LLVM19 contact/vehicle/road code
passes all thirteen independent cases and analytic laws, conditioned chains and
joint support. The published old solver fails the new independent regression.
Source/binary identities, mandatory receipts and scoped reports are under
/tmp/wasm-dd2/rewrite-contact-accuracy-verification/quality-report.json,
sanitized-contact-report.json and baseline-regression-report.json. Successful
raw comparison output is removed after reporting.

## Next

Rerun production arena B under 0002; this component cannot establish
complete natural-race behavior.

## Accept

The old solver fails the added independent regression. Production passes all
thirteen contact queries, analytic laws, required gates and instrumented checks.
This closes only the numerical component; 0002 and the full game stay open.
