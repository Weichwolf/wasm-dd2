Type: Work item
Title: Resolve the next championship world-support advancement failure
Depends: 0029

## Contract

Advance the production-equivalent second-round Stockcar failure at tick 118,647
with finite state, complementarity, friction cones, dissipative response and exact
impulse accounting. Preserve transactional rollback, material laws, ordinary pass
bounds, final tolerances and regular controls, AI, damage, laps and scoring.
Identify the actual failed stage and cause before choosing a correction.

## Evidence

The production Native natural first-season probe completes round 1 at tick
287,087 with identical previous twenty-driver scores and correct consumption.
It passes all previously recorded advancement failure ticks, then fails round 2
at 118,647. AI, input frame and championship remain valid; race/championship stay
RACING. Health is 0.24004616190228145, credited laps zero, throttle -0.6, brake
zero and steering 1. This is a failed full-season attempt with unchanged race
bounds, not complete gameplay. Production receipt:
/tmp/wasm-dd2/rewrite-release-93231/report.json.

A failure-only diagnostic matches all eighty-three production JSON records,
including first-round scores and the terminal failure. Both production and
diagnostic replayers reproduce the same failed step without advancement.
The single 31,216-byte input checkpoint has SHA-256
7fc23f6a0651aa6b712c495dd14a537811ed9b9c4c35f9daa80b9d0570b19904.
Driver 15 has three world contacts and no car pair; the contact query SHA-256 is
d22cbb00389ed1996e1684e2b18af1d561e41ae149b62bc5892d2a3b6b1eb6e5.
Velocity exhausts 4,096 passes at residual 0.000838176120091046. Position repair
is not reached. This identifies the failure stage, not its cause or correction.
Input/query/source/binary receipt:
/tmp/wasm-dd2/rewrite-season-after-93231/report.json.
The unresolved input/query and their owned replay helpers remain under `/tmp`.

A separate remapped one-body diagnostic retains only passes 512/4,096 and
unit-mobility/Jacobian snapshots. It reproduces the velocity failure at
0.0008381761251268624, about 5.1e-12 from the full-owner residual. The
constitutive matrix at pass 4,096 is nonsingular, with condition about 5.07e5.
Both ground supports stay on linear friction branches; the loaded wall remains
sliding and ordinary directions release its pressure. These are sparse
standalone observations, not a valid root or production correction.
Receipt: /tmp/wasm-dd2/rewrite-season-after-93231/sparse-analysis.json.

## Next

Diagnose the three-world-support velocity failure from the saved input using
sparse iteration checkpoints and source/binary-identified experiments. Retain
existing frozen-query, analytic, actual application and original-data regression
checks. Complete physical campaigns remain under 0025/0004; natural arena
completion and movement/tactics retain 0002/0016.

## Accept

The actual failed step and independently checked contact reproducer pass with
production code on the reached targets. Existing physical and real application
checks remain valid on Native/WASM, with sanitizer coverage and mandatory strict
LLVM19/Native/WASM gates. Ordinary racing continues beyond this failure without
state, damage, lap or result injection. Close only this advancement contract;
full-season or all-game acceptance requires its own complete evidence.
