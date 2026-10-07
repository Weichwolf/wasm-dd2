Type: Work item
Title: Natural race completion and stable contact response

## Contract

Ordinary twenty-car circuits and destruction arenas reach valid natural results on Native, WASM and ASan/UBSan without solver aborts, state injection or relaxed scenario coverage.

## Evidence

At a72ecf0 the full race run yielded 139 passing target receipts. Arena B Native completed naturally; WASM failed dd2_driving_advance at frame 51704; sanitized B hit the wall-time bound. A bounded capture reproduces a five-contact solver cycle (~0.1 units/s residual). Reports: /tmp/wasm-dd2/rewrite-progress-race-verification/terminal-report.json and /tmp/wasm-dd2/rewrite-arenaB-wasm-failure/identity.json.

A private best-residual secant safeguard solves that frozen query and six prior queries on all three targets with independent material checks. Its natural B WASM replay still aborts at frame 20430. It is not integrated or accepted. A bounded next-failure capture is complete under /tmp/wasm-dd2/rewrite-arenaB-solver/capture-next/.

The full post-pruning AI run passes the first ten levels on all targets; arena B Native and sanitized slot12 miss sustained travel (9010.022/9769.014<10000), although all2400 samples are supported. Keep the original threshold and diagnose this behavior. Report: /tmp/wasm-dd2/rewrite-connected-ai-verification/terminal-report.json.

## Next

Inspect the next failed query, prove the cause and fix the solver without changing constitutive acceptance checks. Re-run the complete production race suite and all required gates. Expand ordinary physically completed circuit coverage beyond circuit 5.

## Accept

Source-identified complete race checks pass on all three targets; each failure reproducer passes independent motion, contact, energy and clock checks. Natural termination and full-suite coverage are required; a timeout or frozen-query pass is insufficient.
