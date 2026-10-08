# 0052: Restore the original Arena-A body-state comparison

## Contract

Correct the existing spinning-drop discrepancy without relaxing original-data
geometry, material, contact budgets or component tolerances. Preserve the proved
race and recovery contracts. Full natural-arena completion is separate.

## Evidence

The serial standalone-world entry point reproduces the old Arena-A seed-3,
step-560 angular-Z discrepancy (0.00034940996934543033 Native versus
0.0015511486170937353 WASM; unchanged tolerance 0.001). A single corner impulse
can leave another simultaneously touching corner closing. The focused inverted
fast-drop check fails the old implementation on Native/WASM.

The world entry point now delegates to the existing count-one joint field solver.
Simultaneous normal/friction supports are solved together, with unchanged source
box, sweep geometry, clearance, material coefficients and joint solver budgets.
The independent roof-corner normal-velocity inequalities now pass both targets.

The unchanged full original-data ground verifier passes all eleven levels on
Native/WASM/fresh 24-unit O1 ASan/UBSan: 79100 geometry queries and 26400 body
steps per target. Position/velocity/quaternion/angular limits remain
0.1/0.2/0.0001/0.001, with exact non-spinning contact counts and the existing
spinning allowance of two. Recovery passes all 220 original-grid drops per
target with independent source-plane, tire support and acceleration checks on
Native/WASM/fresh 15-unit O1 ASan/UBSan. Strict LLVM19 checks all 169 files;
38 Native and 36 WASM CTests pass.

All 45 previously proved race scenarios per target pass: 29 short physical,
14 ordered-rule and the natural eight-lap Stockcar/nine-lap Time Trial on
circuit 5. The full arena suite does not pass: Native Arena 8 rejects owner
step 10574 in the joint position phase; WASM exceeds the unchanged 1800-second
deadline, while sanitized Arena 8 reaches natural engine retirement. A read-only
linker observer reproduces the exact Native field and records zero calls to the
changed standalone-world API. Those separate findings remain active under 0053
and 0002; they do not establish complete-game or original parity.

Receipts: /tmp/wasm-dd2/rewrite-ground-stability-0052/
{component-report,simultaneous-support-report,owner-report}.json,
ground/report.json, recovery/report.json, circuit-races/report.json and
/tmp/wasm-dd2/rewrite-arena8-rejection-0053/capture-report.json.

## Next

The bounded ground contract is closed. Continue the independently captured arena
position failure under 0053, retaining every original physics acceptance bound.

## Accept

Proved: original sweep geometry and all four body drops pass unchanged contracts
on eleven levels and all three targets; recovery, strict gates, CTests and the
45 previously accepted race contracts remain intact. Full-game and natural-arena
completion remain outside this closed contract.
