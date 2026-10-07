Type: Work item
Title: Refit coupled contact active branches
Depends: 0021

## Contract

Resolve the production seven-body/ten-contact normal-release failure and
six-body/seven-contact transition to sticking. Preserve every previous
independent material case, all physical constraints, laws and tolerances, the
shared 4096-pass budget, automatic bounded storage and strict LLVM19.

## Evidence

The selected production run at 7dd248b fails on all targets. Native reaches
120000 fixture ticks without natural results; WASM aborts after race step 11664
(elapsed 11264), sanitized after 10106 (elapsed 9706). Terminal target receipts,
identities and compact snapshots are under
/tmp/wasm-dd2/rewrite-contact-accuracy-arenaB-7dd248b/terminal-report.json.
Completed raw traces were hashed and removed.

Both first-failure captures match the production 5000-step checkpoints and
failed poses. WASM uses unchanged library objects with linker wrapping; sanitized
uses identical source-unit names/flags with linker wrapping. Compact queries
independently fail on Native at 4096 passes with residuals 0.0247363 and
0.000119884. Capture identities/replays are under
/tmp/wasm-dd2/rewrite-ordinary-sweep-wasm-failure/ and
/tmp/wasm-dd2/rewrite-ordinary-sweep-sanitized-failure/.

The seven-body Newton direction predicts negative pressures at contacts 8/9.
Clipping them invalidates the coupled prediction. Monotone refitting with those
impulse coordinates at zero resolves the case in 545 passes on all targets.
The six-body saturated world support approaches a nearly singular sticking
branch; a guarded zero-slip trial resolves it in 1665 passes on all targets.
Neither trial changes physical acceptance or omits constraints. A rejected or
singular trial restores exact state before the ordinary projected-root trial.
Private fifteen-query and ten analytic material checks pass on release LLVM19,
WASM and fully instrumented LLVM19 ASan/UBSan.

Production passes make rewrite-check (32 Native CTests and strict LLVM19 for
145 owned C/header files), make rewrite-wasm and ctest --preset rewrite-wasm
(31 CTests). Fully instrumented production LLVM19 contact/vehicle/road code
passes all fifteen frozen independent cases and ten analytic laws, conditioned
chains and joint support. The new cases complete in 545/1665 passes on all
three targets. The old solver fails the added regression; independent replays
also prove both failures. Source/binary identities and scoped receipts are in
/tmp/wasm-dd2/rewrite-contact-branches-verification/quality-report.json,
sanitized-contact-report.json and baseline-regression-report.json.
Successful raw output and completed capture binaries/traces are removed;
compact queries and reports remain. Private analytic-Jacobian/alternative
pair-root experiments still fail and are not integrated.

## Next

Run the complete production race suite, including ordinary arena B, under 0002.
Native natural completion and remaining
AI/movement work remain open; component queries do not prove full gameplay.

## Accept

The old solver fails both added independent regressions. Production passes all
fifteen contact cases, analytic laws, required gates and instrumented checks.
This closes only the prediction component; 0002 and the complete game goal stay open.
