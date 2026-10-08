# 0052: Restore the original Arena-A body-state comparison

## Contract

Explain and correct the existing Native/WASM spinning-drop discrepancy while
preserving the original-data ground suite, body/contact/material rules, budgets
and its component tolerances. Do not hide the failed comparison.

## Evidence

The unchanged ground verifier passes levels 1..9, then rejects Arena A, spinning
seed 3 at step 560: angular Z is 0.00034940996934543033 on Native and
0.0015511486170937353 on WASM, exceeding the unchanged 0.001 angular tolerance.
The cumulative contact counts are 318/317, within the existing spinning allowance.
A separately compiled frozen 570df9d solver reproduces the same failure. Every
one of the 120 Arena-A body states per target is exactly unchanged by the new
warm-pressure correction, so that correction is not causal for this finding.
No complete eleven-level ground acceptance is claimed.
Receipts: /tmp/wasm-dd2/rewrite-next-round2-rejection-0049/
{preexisting-ground-A-diagnosis,ground-baseline-report}.json.

## Next

Isolate the grazing contact and numerical divergence with necessary typed
checkpoints. Preserve the original 0.1 position, 0.2 velocity, 0.0001 quaternion,
0.001 angular, contact-count and finite-state contracts. Fix the causal behavior
and repeat the unchanged full ground suite after the correction.

## Accept

Independent original sweep geometry and the same four body drops pass their
unchanged contracts on all eleven levels and Native/WASM/instrumented C.
Physical regression, strict gates and actual-owner stability remain intact.
