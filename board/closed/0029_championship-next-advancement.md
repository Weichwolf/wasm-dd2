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


Sparse 1,024/4,096 velocity records and two Jacobian snapshots identify the
81,622 failure. Both existing directions release two loaded wall supports;
the resulting friction/load change needs a much larger branch transition.
Full steps increase physical error, so backtracking accepts factors near
6e-5 and convergence remains slow. A private unrefitted direction regresses
an existing case. Analytic-only speculative refinement fixes the captured order
but misses a reordered query whose analytic seed is singular. Those experiments
are not production acceptance.

Production now compares bounded speculative refinement after the existing
coordinate restart, leaving the first phase and world-only behavior intact.
Up to sixteen Newton steps re-evaluate loaded normals/friction from a full seed.
Co-oriented world patches permit the existing projected Jacobian if the analytic
seed is singular or its refinement cannot improve. One matrix is reused.
Every original constraint remains checked; only a finite lower final physical
residual replaces the outer iterate, otherwise exact state/better candidates
are restored. No extra ordinary sweeps, material/tolerance/bound changes.

Strict LLVM19 and all 34 Native/32 WASM CTests pass. Native/WASM/instrumented C
pass twenty-one captured queries, sixty-eight captured-query orderings, twenty
analytic friction cases and seventy-two analytic position cases. The new query
takes 545 Native velocity passes (error 3.2301169239730312e-8), 513 WASM and
513 sanitizer passes, with one restart; position takes 54 passes. The actual
saved production input advances from 81,622 to 81,623. Native static stack
usage is bounded; actual WASM cases exercise refinement within the 1 MiB stack.
All eleven actual Native SDL, instrumented SDL and Chromium/WASM window/input
checks pass. All eleven original-data ground checks pass on Native, WASM and
instrumented C: 79,100 queries and 26,400 vehicle steps per target. Scoped
Circuit-2/live and full eight-lap Circuit-5 race checks pass on all three targets.
Circuit 5 takes 167,472 ticks and 3,341,440 vehicle steps, with 167,072 independent
player geometry queries, player place 2 and 75 points; this is a scoped
regression, not complete all-track racing acceptance.
Receipt directory: /tmp/wasm-dd2/rewrite-refinement-81622/.

The unchanged-bound production natural season again completes round 1 at
287,087 ticks, with unchanged scores for all twenty drivers and correct
championship consumption. Ordinary racing crosses 81,622 and next fails
second-round advancement at 105,335. AI/frame/championship validity remain true;
race and championship remain RACING. Player health is 0.36360009215113553,
credited laps zero, throttle -0.6, brake zero and steering -1. This is a failed
full-season attempt, despite the separately verified saved-input correction.
The advancement contract remains active.

A sparse diagnostic reproduces all eighty-one production JSON records exactly,
including checkpoints, round-1 scores, failure and summary. Its single
31,216-byte input fails identically under both production and diagnostic
replayers: no advancement, step 105,335. Drivers 3/4 form five contacts, four
world supports and one car pair. Velocity exhausts the unchanged 4,096-pass
bound at residual 0.0015343071284616529; position repair is not reached. This
identifies the failed stage, not its correction or full-season completion.
Input/query/source/binary receipt: /tmp/wasm-dd2/rewrite-season-105335/report.json.
Successful raw verification output is hashed, reported and removed; unresolved
105,335 input/query evidence remains available.


Two sparse iteration records and numerical differentiation confirm that the
105,335 Jacobian is nonsingular (condition about 6.07e4), with maximum analytic/
numerical derivative difference about 1.01e-7. Changing private error merit or
allowing unconstrained private friction does not fix the captured failure; the
latter also regresses an existing query. These are rejected diagnostic trials.
An independent fixed-material-branch search finds a valid all-sliding root with
nonnegative loads, normal complementarity and friction cones. Its car-pair slip
is above 0.1, while the stalled production direction remains on the linear
small-slip branch. This is diagnostic root evidence, not original parity.

Production now compares an additional saturated constitutive branch after the
existing restart, only for mixed groups with a positive regularized load.
Both private branches start from the same exact outer state and retain sixteen
bounded refinements, fitted nonnegative pressure, cone projection, finite checks,
bounded backtracking and exact restoration. The unchanged regularized physical
residual retains only the better final candidate. Material laws, final tolerances,
total ordinary-pass bound, first phase, world-only behavior and race inputs
remain unchanged. No allocations or extra ordinary sweeps are introduced.

Strict LLVM19 format/tidy and all 34 Native/32 WASM CTests pass. Native, WASM
and instrumented rewrite C pass twenty-two captured queries, 188 captured-query
orderings, twenty analytic friction cases and seventy-two analytic position
cases. All 120 permutations of the new five-contact query independently check
normal complementarity, friction cones/constitutive law, dissipation, impulse
accounting, clearance, orientation and physical clock. The captured order takes
513 velocity passes on all three targets; Native residual is
2.0088650465321933e-9 and sanitizer residual 2.0088706739751494e-9.
Position takes 53 passes at 7.2634431975970859e-10. The actual 31,216-byte
production input advances from 105,335 to 105,336 on Native and with sanitizers.
Receipt directory: /tmp/wasm-dd2/rewrite-saturation-105335/.
All eleven actual Native SDL, instrumented SDL and Chromium/WASM window/input
checks pass. All eleven original-data ground checks pass on Native, WASM and
instrumented C: 79,100 queries and 26,400 vehicle steps per target. Scoped
Circuit-2/live and the complete eight-lap Circuit-5 race pass on all three
targets; Circuit 5 retains 167,472 ticks, 3,341,440 vehicle steps, player place 2,
75 points and 167,072 independent player geometry queries. Reports retain
source/binary identity; this is scoped regression, not full-suite acceptance.

The unchanged-bound natural season again completes round 1 at 287,087 ticks
with unchanged scores and correct championship consumption for all twenty
drivers. Ordinary racing passes 105,335, then fails advancement at 105,932.
AI/frame/championship validity remain true, race/championship stay RACING,
player health remains 0.36360009215113553 and credited laps are zero. Inputs
are throttle -0.6, brake zero and steering -0.83222504759486837. Full-season
acceptance is false; this owner remains active.

A sparse failure-only capture matches all eighty-one production JSON records
exactly. Both production and diagnostic replayers reproduce its single
31,216-byte failed input without advancement at 105,932. Drivers 3/4/7/19 form
ten contacts: seven world supports and three car pairs. Velocity exhausts
4,096 passes at 0.0032146159982845519; position is not reached. This identifies
the next failure stage, not its cause or correction.
Receipt: /tmp/wasm-dd2/rewrite-season-after-105335/report.json.
Completed successful raw output and resolved 105,335 captures have been hashed,
reported and removed; the unresolved next input/query remain available.

Two sparse velocity records at passes 512/4,096 show that regularized and
fully saturated directions stall on the 105,932 query. Three loaded ground
supports and two car pairs remain on linear branches; treating every contact as
saturated does not find a lower outer physical residual. A diagnostic selective
pair-direction prototype passes the independently checked query and twenty
rotated/reversed orders. This identifies a useful search direction, not original
trajectory parity.

Production now compares local typed models that saturate each loaded small-slip
car pair individually while retaining the other contacts' constitutive directions.
Every trial starts from the same exact motion and impulses and uses the existing
refinement, pressure-fit and backtracking bounds. The unchanged regularized
physical residual selects only a smaller finite candidate; rejected trials and
the final best candidate restore exact state. No global selection state, material
change, extra ordinary sweeps or altered tolerances are introduced.

The saved production input advances from 105,932 to 105,933 under both Native
and ASan/UBSan. The Native remapped query converges in 513 velocity passes at
6.3802401372722457e-10 and 52 position passes at 8.3132451499155855e-10, with
one coordinate restart. Strict LLVM19, all 34 Native/32 WASM CTests, twenty-three
captured queries, 208 captured-query orderings, twenty analytic friction cases
and seventy-two analytic position cases pass on Native/WASM/instrumented C.
Actual Native SDL, instrumented SDL and Chromium/WASM input checks pass all
eleven levels. Original-data ground checks pass all eleven levels with 79,100
queries and 26,400 vehicle steps per target. Scoped Circuit-2/live and complete
Circuit-5/eight-lap checks pass all six target scopes (full_suite=false).
Each Circuit-5 run takes 167,472 ticks and 3,341,440 vehicle steps, with 167,072
independent player geometry queries, player place 2/75 points and two finishers.
These scopes do not prove complete all-track racing or natural seasons.
Source/binary/command receipts: /tmp/wasm-dd2/rewrite-selective-105932/report.json.

The unchanged-bound natural season still completes round 1 at 287,087 ticks,
with unchanged scores and correct consumption for all twenty drivers. Its
changed collision trajectory fails round 2 at tick 101,640, before the previous
105,932 failure. AI/frame/championship validity remain true; race/championship
remain RACING, health is 0.36360009215113553, credited laps zero, throttle 1,
brake zero and steering -0.37918715958729865. The saved-input correction is
verified separately; natural advancement past the previous failure and full-season
acceptance remain false. This work item stays active.

A failure-only sparse capture matches all eighty production JSON records.
Production and diagnostic replayers reproduce the same 31,216-byte input without
advancement at 101,640. Driver 7 has three world contacts and no car pair.
Velocity exhausts 4,096 passes at 0.00047813492909413535; position is not reached.
This identifies the next stage, without claiming its cause or correction.
Receipt: /tmp/wasm-dd2/rewrite-season-after-105932/report.json.
Completed successful raw output and resolved 105,932 captures have been hashed,
reported and removed. The original diagnostic receipt remains unchanged;
its separate diagnosis.json records the correction and resolved capture hashes.
The unresolved 101,640 input/query remain available.

Sparse records at velocity passes 512/4,096 reproduce the 101,640 standalone
failure exactly. The wall load decreases toward zero while every fitted
Newton direction releases that contact; the resulting wall velocity remains
negative. The constitutive Jacobian is nonsingular (condition about 4.27e5).
An independent fixed-branch numerical solve finds an admissible all-linear root
with wall load 0.2472825724622389 and residual below 1.7e-13. Starting near the
stalled pressure instead reaches an inadmissible negative wall-load root.
Additional saturated branches or an increased load alone do not fix the query.
Those trials remain diagnostics, not production acceptance.

Production now compares positive world-load seeds after the existing restart.
Each local model doubles one loaded small-slip support, holds all normal loads
fixed while solving linear friction equilibrium, projects friction into its
cones, then applies the existing bounded analytic constitutive refinement.
The fixed-load solve and at most sixteen refinements reuse one matrix. Every
trial starts from the same exact outer state; only finite lower final physical
error replaces the best candidate. Singular/non-improving trials restore exact
state. First-phase behavior, ordinary-pass limits, material laws, tolerances,
controls, AI, damage and race rules remain unchanged.

The saved actual Native input advances from 101,640 to 101,641 under production
and ASan/UBSan. The remapped query takes 513 velocity passes at
1.7622010286531244e-11 and 46 position passes at 9.015950237095411e-10, with one
restart. All six row permutations retain independent material, complementarity,
cone, energy, impulse, clearance, orientation and clock checks. Strict LLVM19,
all 34 Native/32 WASM CTests, twenty-four frozen queries, 214 captured-query
orderings, twenty analytic friction and seventy-two analytic position cases
pass on Native/WASM/instrumented C. Native static solver stack is 350,392 bytes;
actual WASM cases exercise the seed/refinement within the existing 1 MiB stack.
Verification directory: /tmp/wasm-dd2/rewrite-load-101640/.
Actual Native SDL, instrumented SDL and Chromium/WASM checks pass all eleven
levels. Original-data ground checks pass all eleven levels with 79,100 queries
and 26,400 vehicle steps per target. Scoped Circuit-2/live and complete
Circuit-5/eight-lap checks pass all six target scopes (full_suite=false).
Each Circuit-5 run takes 167,472 ticks and 3,341,440 vehicle steps, with 167,072
independent player geometry queries, player place 2/75 points and two finishers.
These scopes do not prove full campaigns or complete all-track racing.
Source/binary/command receipt: /tmp/wasm-dd2/rewrite-load-101640/report.json.

The unchanged-bound natural season completes round 1 at 287,087 ticks with
unchanged scores and correct consumption for all twenty drivers. Its changed
collision trajectory fails round 2 at 93,231, before the previous 101,640
failure. AI/frame/championship validity remain true; race/championship remain
RACING. Health is 0.26430849622175878, credited laps zero, throttle -0.6,
brake zero and steering -1. Saved-input correction is proved separately;
natural advancement past the previous failure and full-season acceptance remain
false, so this work item stays active.

A sparse failure-only diagnostic matches all seventy-eight production JSON
records exactly. Production and diagnostic replayers reproduce the same
31,216-byte failed input without advancement at 93,231. Drivers 1/16/17 form
five contacts: three world supports and two car pairs. Velocity exhausts
4,096 passes at 0.004628929193779555; position is not reached. The next failure
stage is identified; its cause and correction remain unproved.
Receipt: /tmp/wasm-dd2/rewrite-season-after-101640/report.json.
Successful verification raw output and resolved 101,640 captures have been
hashed, reported and removed. The original diagnostic receipt remains unchanged;
its separate diagnosis.json records the independent root, correction and raw
capture identities. The unresolved 93,231 input/query remain available.


Sparse velocity records at passes 512/4,096 isolate the 93,231 failure.
The constitutive Jacobian is nonsingular (condition about 9.74e5), but ordinary
and fully saturated directions do not find the admissible active branch.
An independent finite-difference solve on captured unit mobility proves a root
with zero wall load and separating wall velocity 0.03741068281093929. Both
ground supports remain linear and both car pairs saturated; final physical
residual is below 6.5e-12. Doubling world pressure alone fixes the captured order
but fails a permutation, and applying it to sliding supports regresses an older
arena query. These rejected trials are diagnostics, not acceptance.

Production now compares higher and released world-pressure seeds in both mixed
and world-only fields after the existing restart. A local typed model records
the direction and selected support. Each seed applies its normal impulse change,
holds all normal loads fixed during friction equilibrium, projects the cones and
uses the existing bounded constitutive refinement. Released contacts remain in
the system and can load again. Every original constraint is checked under the
unchanged final material law. Rejected/nonfinite candidates restore exact motion
and impulses; only a smaller full physical residual replaces the best candidate.
The bounded model array permits at most two models per contact plus two base
models; the ordinary 4,096-pass bound, sixteen-refinement bound and final
tolerances are unchanged.

The actual 31,216-byte input advances from 93,231 to 93,232 under Native and
ASan/UBSan. The remapped three-body query takes 513 velocity passes at
1.045344366623624e-14 (Native) and 1.2434497875801753e-14 (sanitized), with one
restart. Position takes 53 passes at 9.6082740036278723e-10. All 120 contact
permutations pass independent material, complementarity, cone, energy, impulse,
clearance, orientation and physical-clock checks on Native and sanitized C.
Native compiler static solver frame is 350,904 bytes; the matrix remains
21,944 bytes. Verification directory: /tmp/wasm-dd2/rewrite-release-93231/.
Strict LLVM19 format/tidy, all 34 Native/32 WASM CTests, twenty-five captured
queries, 334 captured-query orderings, twenty analytic friction cases and
seventy-two analytic position cases pass on Native/WASM/instrumented C.
The new case and all 120 permutations exercise the existing 1 MiB WASM stack.


The unchanged-bound Native natural season completes round 1 at 287,087 ticks
with identical twenty-driver scores and correct championship consumption.
Its changed trajectory passes every previously recorded failure tick, including
107,550, and next fails advancement at round-2 tick 118,647. AI/frame/championship
validity remain true; race/championship stay RACING, health is
0.24004616190228145, credited laps zero, throttle -0.6, brake zero and steering 1.
This proves advancement beyond this work item's reproduced failures, not a
complete season. The new failure has its own owner under 0030.

A failure-only diagnostic matches all eighty-three production JSON records
exactly. Production and diagnostic replayers reproduce the same 31,216-byte
input without advancement at 118,647. Driver 15 has three world supports and no
car pair. Velocity exhausts 4,096 passes at 0.000838176120091046; position is not
reached. The failed stage is identified, without a claim about its cause or
correction. Receipt: /tmp/wasm-dd2/rewrite-season-after-93231/report.json.

Actual Native SDL, instrumented SDL and Chromium/WASM window/input checks pass
all eleven levels. Original-data ground comparisons pass all eleven levels:
79,100 queries and 26,400 physical steps per target. Scoped Circuit-2/live and
complete Circuit-5/eight-lap checks pass all six target scopes (full_suite=false).
Each Circuit-5 run takes 167,472 ticks and 3,341,440 physical vehicle steps, with
167,072 independent player geometry queries, player place 2/75 points and two
finishers. These scopes do not prove complete all-track racing or full campaigns.
Source/binary/command receipt: /tmp/wasm-dd2/rewrite-release-93231/report.json.
Successful verification raw output and resolved 93,231 captures have been
hashed, reported and removed. The original sparse receipt remains unchanged;
its separate diagnosis.json retains the independent root and correction evidence.
The new unresolved input/query remain available under 0030.

The reproduced advancement contract is complete: all saved physical cases pass,
the actual latest failed input advances on Native/instrumented C, and ordinary
racing proceeds beyond every recorded failure tick from this item. The separate
118,647 failure remains active under 0030; full-season acceptance stays false.

## Next

Resolve the separately reproduced tick-118,647 world-support failure under 0030.
Complete natural seasons and remaining gameplay retain their existing owners.

## Accept

The actual failed step and any contact reproducer pass with production code.
Existing physical and actual application checks remain valid on both platforms,
with sanitizer coverage and mandatory LLVM19/Native/WASM gates. Ordinary racing
continues past this failure without state, damage, lap or result injection.
Close only the proved advancement contract.
