Type: Work item
Title: Resolve slow coupled contacts with delayed bounded corrections
Depends: 0017, 0018

## Contract

Resolve the captured seven-body/nine-contact and twelve-body/fifteen-contact
slow modes without changing contact identities, friction laws, tolerances or the
4,096-pass budget. Preserve the previously successful four-body fallback.
Use readable C11, no allocation and strict LLVM19 on Native and WASM.

## Evidence

The published solver fails the seven-body capture at 4,096 passes with a
0.0004257565 residual and no restart. It also fails the later twelve-body query
with a 0.0001769569 residual after restart. The unchanged independent material
fixture rejects the old solver; its four-body control still passes.

Production now forms guarded numerical Newton directions after 512 ordinary
sweeps, every 32 passes in either phase. Pivoted elimination uses a bounded
192-axis automatic matrix. Singular directions are skipped; cone-projected
trials are backtracked and accepted only for finite motion with smaller physical
residual. Rejection restores exact motion/impulses. Accepted corrections clear
ordinary-sweep secant history. The caller reserves a 1 MiB WASM stack.

The seven-body query completes in 2917/2789/2694 passes on Native/WASM/sanitized;
the twelve-body query in 1218/1250/1314; the four-body fallback in 640 on all three.
All twelve frozen contact queries and ten analytic material cases pass their
independent constitutive, impulse, energy, clearance, rotation and clock checks.
Conditioned chains and joint support also pass with fully instrumented
ASan/UBSan contact/vehicle/road code.

Mandatory gates pass: make rewrite-check (32 Native CTests and strict LLVM19
format/tidy for 145 owned C/header files), make rewrite-wasm and ctest --preset
rewrite-wasm (31 CTests). Source/binary/log identities and scoped receipts are in
/tmp/wasm-dd2/rewrite-delayed-contact-verification/quality-report.json,
sanitized-contact-report.json and baseline-regression-report.json.

## Next

Run ordinary arena B on all three targets, then the complete production race
suite and additional physically completed circuits under 0002. Private captures
and component tests do not establish full natural completion or original parity.

## Accept

The old solver fails the new independent regression. Production passes all
twelve contact queries, analytic laws, strict LLVM19 and required Native/WASM
gates. This closes the coupled-correction component only; 0002 remains active.
