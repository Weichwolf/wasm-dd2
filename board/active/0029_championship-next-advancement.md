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

One 31,216-byte input checkpoint reproduces the actual failed step with both
production and diagnostic code. Every production JSON checkpoint, first-round
result, failure and summary matches the sparse probe exactly. Drivers 11/14
form seven contacts: six world supports and one car pair. Velocity converges in
12 passes with residual 5.3753121290722745e-8. Position repair exhausts 4,096
passes at 3.290564012534082e-9, above the unchanged 1e-9 tolerance.
Five nearly identical wall normals have requirement differences spanning about
7.9e-9. Two sparse iteration reports show positive position multipliers retained
on weaker inequalities while the strongest remains violated. This identifies
slow multiplier release in the position solve, not a friction/velocity failure.
Reproduction/input/source/binary receipt:
/tmp/wasm-dd2/rewrite-season-107550/report.json.

A private Native-only candidate transfers multipliers toward a stronger,
near-parallel inequality, rebuilds offsets and predicts its exact coordinate
response. It checks every original constraint, accepts only a smaller finite
position residual and restores exact offsets/multipliers on rejection. The
nineteen captured queries and fourteen existing velocity-query reorderings pass
independent physical conditions. The new query needs 515 position passes at
5.902707408142818e-10; the saved actual input advances to 107,551. Production is
unchanged. Strict LLVM19, WASM, sanitizer, reordered position-query/analytic
checks and ordinary racing have not established acceptance for this candidate.

## Next

Review and implement the general bounded position prediction in readable C11.
Preserve all constraints and existing physical acceptance, add focused independent
position checks and row reorderings, and verify Native/WASM and instrumented C.
Then replay the saved input with production code, run mandatory/application
checks and continue ordinary racing. Keep full seasons and remaining gameplay
under their existing owners.

## Accept

The actual failed step and any contact reproducer pass with production code.
Existing physical and actual application checks remain valid on both platforms,
with sanitizer coverage and mandatory LLVM19/Native/WASM gates. Ordinary racing
continues past this failure without state, damage, lap or result injection.
Close only the proved advancement contract.
