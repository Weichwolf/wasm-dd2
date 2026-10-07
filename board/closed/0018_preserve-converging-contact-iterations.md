Type: Work item
Title: Preserve iterations for converging contact chains
Depends: 0017

## Contract

Restart accelerated contacts only after64 passes without a0.1% residual improvement. A slowly converging solve can use the entire existing4096-pass budget. Preserve the single restart, every contact, material law, tolerance and independent acceptance check.

## Evidence

Production6fc2fe2 arena-B WASM fails at race step52320. A bounded first-failure capture contains ten contacts across nine connected bodies. The accelerated solve reaches tolerance in3467 passes, while the unconditional restart at2048 exhausts4096 total passes with0.00072143 units/s residual. Ordinary coordinates alone also fail the unchanged budget. The new independent-material regression rejects the old implementation.

The candidate restarts only on measured stagnation. The new ten-contact query completes in3467 passes with no restart; the prior five-contact cycle still completes in477 passes with one restart. All nine frozen queries and ten analytic material cases pass on Native, WASM and fully instrumented ASan/UBSan, with contact/cone/energy/impulse/clearance/clock checks unchanged. Conditioned chains and joint supports also pass. Reports: /tmp/wasm-dd2/rewrite-arenaB-restart-failure/replay-report.json, candidate-material-report.json and sanitized-contact-report.json.

## Next

Production gates pass: make rewrite-check (31 Native CTests, clang-format19 and strict clang-tidy19 across141 owned C/header files), make rewrite-wasm and ctest --preset rewrite-wasm (30 CTests). Exact source/binary/log receipts are in /tmp/wasm-dd2/rewrite-arenaB-restart-failure/quality-report.json. Continue natural race acceptance in0002; a private WASM arena-B replay is running. The old6fc2fe2 race run is terminal: Native passes natural retirement; WASM and sanitized both abort advancement. Its compact terminal report is /tmp/wasm-dd2/rewrite-restart-arenaB-verification/terminal-report.json. The Native-only alternative-target AI prototype worsens travel for slots12/13 and is rejected.

## Accept

The old implementation fails the new independent regression; all nine contact regressions and analytic laws pass on Native/WASM/ASan with strict LLVM19 and required shared build gates. Publish only this convergence contract; complete natural races remain0002.
