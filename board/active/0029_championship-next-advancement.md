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

Production now has a bounded multiplier transfer toward a stronger,
near-parallel inequality with the same partners. It rebuilds offsets from all
response columns, projects only recipients and checks every original constraint.
It accepts only a smaller finite full position residual and restores exact
state on rejection. No material law, tolerance, relaxation or pass bound changes.

Strict LLVM19 format/tidy, all 34 Native and 32 WASM CTests pass. Native, WASM
and instrumented rewrite C pass nineteen captured queries, twenty-eight rotated
or reversed captured-query orderings, twenty analytic friction cases and
twenty-four independent analytic least-norm position cases. The new production
query takes 515 position passes with one prediction and residual
5.9027074081428182e-10; velocity remains 12 passes at
5.3753121290722745e-8. The saved actual input advances to 107,551 with rebuilt
production libraries. Actual Native SDL, instrumented SDL and Chromium/WASM
checks pass all eleven levels and existing mode/championship/pause/reset/focus/exit controls.
Verification receipt: /tmp/wasm-dd2/rewrite-position-107550/report.json.
Successful raw verification output has been hashed, reported and removed.

The new production natural first-season probe again completes round 1 at
287,087 ticks with exactly the previous twenty driver scores and consumes them
correctly. Its second-round trajectory now fails advancement at tick 66,070:
AI/frame/championship validity pass, health is 0.48667347418173146 and credited
laps are zero. This is another failed full-season attempt. The old saved input
is fixed, but ordinary full-campaign acceptance remains unproved. A sparse
diagnostic reproduces every production JSON checkpoint/result/failure exactly. Its 31,216-byte saved input fails identically under production and
diagnostic replay. Drivers 2/8 form four contacts (three world, one pair).
Velocity converges in 12 passes at 2.4753696444140237e-8; position exhausts
4,096 passes at 2.2943640119247986e-9. This is another position failure, with
different support normals. Receipt: /tmp/wasm-dd2/rewrite-season-66070/report.json.


Two sparse position-iteration records identify the new failure: all four
multipliers remain positive, including two distinct ground-plane supports.
Their common position Gram system has condition number about 79,107. Ordinary
relaxation still leaves active-row residual 2.2943640119247986e-9 after 4,096
passes. An independent double-precision solve yields four positive multipliers
and residuals below 3e-20, confirming a feasible least-norm translation.

Production now fits active position equations using exact unit translations.
Satisfied unloaded rows enforce zero pressure. Negative fitted pressures are
released and refitted, at most contact_count + 1 times. Singular systems leave
state unchanged. All original inequalities remain checked; non-finite or
non-improving candidates restore exact offsets/multipliers. The existing delay,
period, solver bounds, tolerances and material laws remain unchanged.

Native/WASM/ASan/UBSan pass twenty captured queries, fifty-two captured-query
orderings, twenty analytic friction cases and seventy-two analytic position
cases (24 coincident-row, 48 tilted-plane with active/released supports).
Strict LLVM19 and all 34 Native/32 WASM CTests pass. The new captured query
converges at position pass 512 with residual 5.4210108624275222e-20 and one
prediction; the saved production step advances from 66,070 to 66,071.
The production natural season again completes round 1 at tick 287,087 with
exactly the previous twenty driver scores, advances past 66,070 and fails
round 2 at tick 81,622. AI/frame/championship validity pass; player health is
0.29167128978005974 and credited laps remain zero. This is a failed season
attempt, not complete gameplay. A sparse diagnostic matches every production checkpoint/result exactly, and
production/diagnostic replays reproduce the same failed 31,216-byte input.
Drivers 13/14 form eight contacts (seven world, one pair). Velocity exhausts
4,096 passes at residual 1.5853764431892486e-5; position is not reached.
Receipt: /tmp/wasm-dd2/rewrite-season-81622/report.json. Actual Native SDL,
instrumented SDL and Chromium/WASM input checks pass all eleven levels.
Original-data ground checks pass all eleven levels on Native/WASM/instrumented
C (79,100 queries and 26,400 physical steps per target). Scoped Circuit-2 live
and complete Circuit-5 Stockcar races pass six target scopes; full_suite=false.
Each eight-lap Circuit-5 run naturally completes 167,472 ticks with 3,341,440
vehicle steps, player place 2/75 points and two finishers. These scopes do not
prove all races or seasons. Successful raw output is hashed, reported and
removed; the failed 81,622 input/query remains available. Receipt:
/tmp/wasm-dd2/rewrite-position-66070/report.json.

## Next

Freeze and inspect sparse velocity multipliers, material branches and correction
attempts in the reproduced 81,622 eight-contact query before choosing further
changes. Preserve all constraints, rollback, material laws and bounds. Continue
ordinary racing after its correction; keep full campaigns and remaining gameplay
under their existing owners.

## Accept

The actual failed step and any contact reproducer pass with production code.
Existing physical and actual application checks remain valid on both platforms,
with sanitizer coverage and mandatory LLVM19/Native/WASM gates. Ordinary racing
continues past this failure without state, damage, lap or result injection.
Close only the proved advancement contract.
