Type: Work item
Title: Stable driver IDs in assigned physical grids
Depends: 0015

## Contract

Consume a valid permutation of twenty physical start slots while preserving
driver IDs, human zero, copied start ownership and reset/mode behavior.
The existing default constructor preserves identity placement. Invalid
permutations fail before allocation.

## Evidence

Production Native/WASM/fully instrumented LLVM19 ASan/UBSan checks cover all eleven
original levels and four fixture league divisions, 880 assigned starts per
target, settled valid fields, short actual Stockcar/Total Destruction drives,
withdrawal, copied-map reset and all-seven-circuit Time Trial transitions.
Use `make rewrite-grid-verify`. Source/binary-identified reports:
/tmp/wasm-dd2/rewrite-grid-integration/original-data/report.json and report.json
in its parent directory. All 32 Native/31 WASM CTests and strict LLVM19 for
146 owned C/header files pass. Actual Native/Sanitizer windows and Chromium
input/canvas checks pass on every original level; report:
/tmp/wasm-dd2/rewrite-grid-integration/presentation/report.json.

The browser check initially fails because a fixed one-second throttle delay
does not guarantee crossing the start line. It now holds trusted forward input
and waits at most fifteen seconds for actual lap timing, preserving the positive
lap-time condition and pause/reset assertions. The browser-only recheck passes;
passing Native/Sanitizer presentation checks reuse the same runtime sources and
binaries. Initial failure evidence and the final identity remain in the report
directory. Successful raw captures/logs and temporary sanitizer binaries are
hashed and removed.

Fixture promotion scores initialize divisions; they do not represent actual
championship results. The consuming championship owner, complete seasons,
front end and persistence remain open under 0004.

## Next

Use the verified grid constructor in the championship owner under 0004.
Keep actual season results, front-end flows and saved seasons in that contract.

## Accept

Production source-identified checks pass on all three targets for the contract,
including malformed maps, copied storage, assigned nominal starts, physical
steps, stable player/pursuit IDs, reset and mode changes. Strict LLVM19 and
Native/WASM gates pass. Close only this grid contract.
