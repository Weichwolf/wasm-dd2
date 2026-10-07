Type: Work item
Title: Resolve the next championship mixed-support advancement failure
Depends: 0030

## Contract

Advance the production-equivalent second-round Stockcar failure at tick 128,036
with finite state, complementarity, friction cones, dissipative response and exact
impulse accounting. Preserve transactional rollback, material laws, ordinary pass
bounds, final tolerances and regular controls, AI, damage, laps and scoring.
Diagnose the actual failed stage and cause before choosing its correction.

## Evidence

The production Native natural first-season probe completes round 1 at 287,087
ticks with identical previous twenty-driver scores and correct consumption.
It passes the old tick-118,647 failure, then fails round 2 at 128,036.
AI, input frame and championship remain valid; race/championship stay RACING.
Health is 0.22552210658073613, credited laps zero, throttle -0.6, brake zero and
steering 0.39842831080676594. This is a failed full-season attempt with unchanged
race bounds, not complete gameplay. Production verification receipt:
/tmp/wasm-dd2/rewrite-linear-118647/report.json.

A failure-only diagnostic matches all eighty-five production JSON records.
Production and diagnostic replayers reproduce the same failed step without
advancement. The single 31,216-byte checkpoint has SHA-256
717abcfdde9431a5be51a47d65ac641bef5c09c395af019e9aff4997cbf7c80d.
Drivers 4/8/9/14/16/19 form eleven contacts: six world supports and five pairs.
The query SHA-256 is
b9bc7bdee8b987f36d31fc6063416166ff067580078a87087b7e005910e39bb3.
Velocity exhausts 4,096 passes at residual 0.006737579411726455; position repair
is not reached. This identifies the failed stage, not its cause or correction.
Input/query/source/binary receipt:
/tmp/wasm-dd2/rewrite-season-after-118647/report.json.
The unresolved checkpoint, query and owned replay helpers remain under /tmp.

A remapped six-body standalone diagnostic retains only passes 512/4,096 and
unit-mobility/Jacobian snapshots. It reproduces the velocity failure at
0.0067375793504768407, about 6.2e-11 from the full-owner residual,
with one restart and no position repair. The constitutive matrix condition at
pass 4,096 is about 4.11e+05. These are sparse
standalone observations, not a valid root or production correction.
Receipt: /tmp/wasm-dd2/rewrite-season-after-118647/sparse-analysis.json.

## Next

Diagnose the six-body, eleven-contact velocity failure using sparse iteration
and unit-mobility/Jacobian checkpoints. Preserve existing frozen-query, analytic,
actual application and original-data checks. Complete physical campaigns remain
under 0025/0004; natural arena completion and movement/tactics retain 0002/0016.

## Accept

The actual failed step and independently checked contact reproducer pass with
production code on reached targets. Existing physical and real application checks
remain valid on Native/WASM, with sanitizer coverage and mandatory strict
LLVM19/Native/WASM gates. Ordinary racing continues beyond this failure without
state, damage, lap or result injection. Close only this advancement contract;
full-season and all-game acceptance require their complete evidence.
