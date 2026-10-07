Type: Work item
Title: Resolve the next scheduled racing advancement failure
Depends: 0028

## Contract

Advance the reproduced second-round Stockcar failure with finite state,
complementarity, friction cones, dissipative response and exact impulse accounting.
Preserve rollback, material laws, solver budgets/tolerances and regular controls,
AI, damage, laps, scoring and race bounds. Verify the actual failure stage before
choosing its correction.

## Evidence

The production Native natural first-season attempt completes round 1 at tick
287,087, consumes actual scores for all twenty drivers, and fails advancement at
second-round tick 107,550. AI, frame and championship state remain valid; race
and championship remain RACING. Player health is 0.40180162706486544, credited
laps zero, throttle -0.6, brake zero and steering 1. Source identity stays
unchanged. This is a failed full-season attempt, not natural completion.
Production receipt: /tmp/wasm-dd2/rewrite-coupled-86643-final/report.json.

The sparse diagnosis under /tmp/wasm-dd2/rewrite-season-107550/ is running.
It will write only the needed 31-KiB input checkpoint and first failed contact
query. The failure stage, participants and residual are not yet established.

## Next

Finish and compare the sparse probe with the production checkpoints. Replay the
actual saved step, identify the failing stage, freeze the relevant query and
verify a general correction independently on Native/WASM and instrumented C.
Continue ordinary scheduled racing; keep full seasons and remaining gameplay
under their existing owners.

## Accept

The actual failed step and any contact reproducer pass with production code.
Existing physical and actual application checks remain valid on both platforms,
with sanitizer coverage and mandatory LLVM19/Native/WASM gates. Ordinary racing
continues past this failure without state, damage, lap or result injection.
Close only the proved advancement contract.
