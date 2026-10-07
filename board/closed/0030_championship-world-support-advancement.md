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

An independent finite-difference solve on captured unit mobility proves an
admissible all-linear root: wall impulse 0.7246122100558153, wall tangent speed
0.08332354482023452, with physical branch residual below 5.4e-14. Ordinary
directions instead decrease wall pressure toward zero on the sliding branch.
Higher pressure seeds for sliding supports fix the captured order but fail the
reversed row order; that prototype is not production acceptance.

Production now privately refines the existing linear world-friction direction
for world-only groups after the existing restart. Its full fitted seed and up
to sixteen steps allow the wall to return to the linear branch as pressures
settle. All intermediate states remain private; normal refits, cone projection,
bounded backtracking, exact rollback and every original constraint are retained.
Only a finite smaller residual under the unchanged physical material law can
replace the outer state. The world-only base model fits the existing 130-model
array. Mixed-group models, first-phase behavior, ordinary 4,096-pass bound,
material laws and final tolerances are unchanged.

The actual 31,216-byte input advances from 118,647 to 118,648 under Native
and ASan/UBSan.
The remapped query takes 513 velocity passes at 9.8559282121881802e-10 (Native)
and 9.8558840435383105e-10 (sanitized), with one
restart, followed by 50 position passes at 8.3662229894813052e-10.
All six contact permutations retain independent material, complementarity,
cone, energy, impulse, clearance, orientation and physical-clock checks.
Native compiler static solver frame remains 350,904 bytes and the matrix frame
21,944 bytes. Verification directory: /tmp/wasm-dd2/rewrite-linear-118647/.
Strict LLVM19 format/tidy and all 34 Native/32 WASM CTests pass. Native,
WASM and instrumented rewrite C pass twenty-six captured queries, 340 contact
orderings, twenty analytic friction cases and seventy-two analytic position
cases. The new query and all six permutations exercise the existing 1 MiB WASM
stack; no solver storage increase is needed.

The unchanged-bound Native natural season completes round 1 at 287,087 ticks
with identical twenty-driver scores and correct championship consumption.
Its changed collision trajectory passes 118,647 and next fails round 2 at
128,036. AI/frame/championship validity remain true; race/championship remain
RACING, health is 0.22552210658073613, credited laps zero, throttle -0.6, brake
zero and steering 0.39842831080676594. Full-season acceptance remains false.

A failure-only diagnostic matches all eighty-five production JSON records.
Both actual replayers reproduce the same 31,216-byte input without advancement
at 128,036. Drivers 4/8/9/14/16/19 form eleven contacts: six world supports and
five pairs. Velocity exhausts 4,096 passes at 0.006737579411726455; position is
not reached. This identifies the next stage, not its cause or correction.
The new failure belongs to 0031; its input/query and helpers remain available.
Receipt: /tmp/wasm-dd2/rewrite-season-after-118647/report.json.

Actual Native SDL, instrumented SDL and Chromium/WASM window/input checks pass
all eleven levels. Original-data ground comparisons pass all eleven levels:
79,100 queries and 26,400 physical steps per target. Scoped Circuit-2/live and
complete Circuit-5/eight-lap checks pass all six target scopes (full_suite=false).
Each Circuit-5 run takes 167,472 ticks and 3,341,440 physical vehicle steps, with
167,072 independent player geometry queries, player place 2/75 points and two
finishers. These scopes do not prove complete all-track racing or full campaigns.
Source/binary/command receipt: /tmp/wasm-dd2/rewrite-linear-118647/report.json.
Successful verification raw output and resolved 118,647 captures have been
hashed, reported and removed. The original sparse receipt remains unchanged;
its separate diagnosis.json retains the independent root and correction evidence.
The new unresolved input/query remain available under 0031.

The advancement contract is complete: the saved actual input and all contact
fixtures pass, and ordinary racing proceeds beyond 118,647 with unchanged bounds
and no state, damage, lap or result injection. The separate 128,036 failure
remains active under 0031; full-season acceptance stays false.

## Next

Diagnose the separate mixed-support tick-128,036 failure under 0031.
Complete physical campaigns and remaining gameplay retain their existing owners.

## Accept

The actual failed step and independently checked contact reproducer pass with
production code on the reached targets. Existing physical and real application
checks remain valid on Native/WASM, with sanitizer coverage and mandatory strict
LLVM19/Native/WASM gates. Ordinary racing continues beyond this failure without
state, damage, lap or result injection. Close only this advancement contract;
full-season or all-game acceptance requires its own complete evidence.
