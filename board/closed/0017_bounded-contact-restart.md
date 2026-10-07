Type: Work item
Title: Bounded restart for cycling accelerated contacts
Depends: 0002

## Contract

When accelerated normal/friction response stalls, restart once from exact input
motion with zero accumulated impulses and ordinary coordinate steps. Retain all
contacts, material laws, initial-normal-speed metadata and position repair. Both
phases share the unchanged 4096-pass bound; reserve half for ordinary response.
Publish explicit restart count without turning trial iterations into collisions.

## Evidence

The arena-B five-contact failure at frame51704 is reproduced from captured rewrite
state on original assets. Its old accelerated solve cycles around0.1 units/s;
ordinary coordinate response converges. A private best-residual-only safeguard
introduced a second failure at frame20430, with one body/three world contacts;
ordinary response also solves that query. The private safeguard remains rejected.

The published implementation restarts after64 accelerated passes without a0.1%
residual improvement, or at pass2048. The original5contact failure now completes
in477 total passes with one restart on Native, WASM and ASan/UBSan. The3world
junction completes in40 passes. Six older frozen contact failures, ten analytic
material cases and conditioned chains pass unchanged independent constitutive,
impulse, energy, clearance, orientation and clock checks. All required strict
LLVM19 gates plus31 Native/30 WASM CTests pass. Receipts:
/tmp/wasm-dd2/rewrite-contact-restart-final/quality-report.json and
/tmp/wasm-dd2/rewrite-contact-restart-final/sanitized-contact-report.json.

## Next

Continue natural arena/circuit and full-game acceptance in0002. The focused
ordinary arena-B race is running on all three targets; frozen-query success does
not close its natural race behavior. AI sustained motion remains open in0016.

## Accept

The bounded restart contract and independent eight-query/material checks pass
on all three targets with mandatory builds/style gates. No contact or material
check is dropped, and reported passes include both phases. This closes the solver
mechanism only, not original parity, natural races or complete gameplay.
