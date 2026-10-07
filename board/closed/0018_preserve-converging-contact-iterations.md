Type: Work item
Title: Preserve iterations for converging contact chains
Depends: 0017

## Contract

Restart accelerated contacts only after 64 passes without a 0.1% residual
improvement. A slowly converging solve can use the entire existing 4,096-pass
budget. Preserve the single restart, every contact, material law, tolerance and
independent acceptance check.

## Evidence

Production 6fc2fe2 arena-B WASM fails at race step 52,320. A bounded first-failure
capture contains ten contacts across eight connected bodies. Acceleration reaches
tolerance in 3,467 passes; an unconditional restart at 2,048 exhausts the budget
with a residual of 0.00072143 units/s. Ordinary coordinates alone also fail the
unchanged budget. The new independent-material regression rejects the old code.

Published 4f0aeac restarts only on measured stagnation. The new query completes
in 3,467 passes without restarting; the prior five-contact cycle still completes
in 477 passes with one restart. All nine frozen queries and ten analytic material
cases pass on Native, WASM and fully instrumented ASan/UBSan. Contact, cone,
energy, impulse, clearance and clock checks are unchanged. Conditioned chains and
joint supports also pass.

Mandatory gates pass: make rewrite-check (31 Native CTests, clang-format 19 and
strict clang-tidy 19 across 141 owned C/header files), make rewrite-wasm and
ctest --preset rewrite-wasm (30 CTests). Source, binary and log identities are
recorded in /tmp/wasm-dd2/rewrite-arenaB-restart-failure/quality-report.json;
replay-report.json and sanitized-contact-report.json record the diagnosis and
instrumented checks.

## Next

Continue natural race acceptance in 0002. The production-equivalent private WASM
replay passes the captured query but aborts at step 52,337. Its completed trace
was compacted after recording candidate-race-report.json. A fresh bounded capture
of that next failure runs under /tmp/wasm-dd2/rewrite-arenaB-stagnation-failure/.
The older 6fc2fe2 race run is terminal: Native reaches natural retirement; WASM
and sanitized builds abort advancement. Full natural races remain unaccepted.

## Accept

The old implementation fails the new independent regression; all nine contact
regressions and analytic laws pass on Native/WASM/ASan with strict LLVM 19 and
mandatory shared build gates. This closes the convergence contract only;
complete natural races remain in 0002.
