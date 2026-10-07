Type: Work item
Title: Natural race completion and stable contact response

## Contract

Ordinary twenty-car circuits and destruction arenas reach valid natural results on Native, WASM and ASan/UBSan without solver aborts, state injection or relaxed scenario coverage.

## Evidence

At a72ecf0 the full race run yielded 139 passing target receipts. Arena B Native completed naturally; WASM failed dd2_driving_advance at frame 51704; sanitized B hit the wall-time bound. A bounded capture reproduces a five-contact solver cycle (~0.1 units/s residual). Reports: /tmp/wasm-dd2/rewrite-progress-race-verification/terminal-report.json and /tmp/wasm-dd2/rewrite-arenaB-wasm-failure/identity.json.

A private best-residual secant safeguard solves that frozen query and six prior queries on all three targets with independent material checks. Its natural B WASM replay still aborts at frame 20430. It is not integrated or accepted. A bounded next-failure capture is complete under /tmp/wasm-dd2/rewrite-arenaB-solver/capture-next/.

The full post-pruning AI run passes the first ten levels on all targets; arena B Native and sanitized slot12 miss sustained travel (9010.022/9769.014<10000), although all2400 samples are supported. Keep the original threshold and diagnose this behavior. Report: /tmp/wasm-dd2/rewrite-connected-ai-verification/terminal-report.json.

Closed0017 now supplies a bounded ordinary-coordinate restart while retaining all contacts/material checks and4096 total passes. The5contact failure completes in477 passes and the3world junction in40 on all targets. Required31Native/30WASM gates and independent material checks pass. The fresh production B-total-survive run under /tmp/wasm-dd2/rewrite-restart-arenaB-verification reaches natural Native retirement after93.975 seconds; WASM fails at race step52320; sanitized also aborts advancement at race step48037. This selected run is terminal and fails overall. Report: /tmp/wasm-dd2/rewrite-restart-arenaB-verification/terminal-report.json. A bounded ten-contact/eight-body capture shows that forced restart at2048 interrupts a healthy accelerated solve needing3467 iterations. Follow-up0018 removes that unconditional trigger while preserving stagnation detection and the total budget; production31Native/30WASM and strictLLVM19 gates pass, as do independent nine-query/material checks on all three targets.

Native B AI capture at6fc2fe2 confirms slots12/13 target one another after step~2000 and spend much of the remaining drive reversing with all wheels supported. A private alternate-target escape prototype lowers travel to8072.943/8644.802 and is rejected. Report: /tmp/wasm-dd2/rewrite-arenaB-ai-diagnosis/report.json. These are control/movement diagnostics, not complete AI acceptance.

A production-equivalent private WASM replay of4f0aeac passes the frozen query but aborts at race step52337, seventeen steps later. Report: /tmp/wasm-dd2/rewrite-arenaB-restart-failure/candidate-race-report.json. The fresh bounded capture under /tmp/wasm-dd2/rewrite-arenaB-stagnation-failure/ is terminal. Its nine contacts connect seven bodies; Native independently reproduces the4096-pass failure with0.0004257565 units/s residual and no coordinate restart. A private best-state fallback also fails, because this query never enters fallback. Reports: report.json and replay-report.json. Improve accelerated convergence for the coupled slow modes without changing material laws or relaxing the pass/tolerance budget. No full natural-race pass is claimed.

Private follow-up diagnostics are recorded in
/tmp/wasm-dd2/rewrite-coupled-secants/report.json. Two-direction and deeper
secant histories, exact tangential coordinates and local normal/friction blocks
still fail the seven-body query. Sparse residuals locate the dominant slow mode
at its two world contacts. A private finite-difference Newton step solves the
query on Native, WASM and ASan/UBSan within the unchanged 4096-pass budget. Native
and sanitized independent ten-query material checks pass; WASM fails the
native15 final constitutive oracle. Its actual WASM arena also aborts at step
18062. That new four-body/seven-contact state reproduces the private failure on
Native while the unchanged production solver succeeds. The private candidate
is rejected and is not integrated; its temporary matrix allocation also does
not meet the production no-allocation contract. Terminal experiment binaries
and successful raw diagnostics were removed after recording identities and
reports. Compact queries and private sources remain for the next diagnosis.

## Next

Diagnose the captured step52337 failure, retain independent material checks, then run the complete production suite and fix the solver without changing constitutive acceptance checks. Re-run the complete production race suite and all required gates. Expand ordinary physically completed circuit coverage beyond circuit 5.

## Accept

Source-identified complete race checks pass on all three targets; each failure reproducer passes independent motion, contact, energy and clock checks. Natural termination and full-suite coverage are required; a timeout or frozen-query pass is insufficient.
