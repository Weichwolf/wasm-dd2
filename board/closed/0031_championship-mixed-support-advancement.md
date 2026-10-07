Type: Work item
Title: Coupled world-release directions in scheduled racing
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
Archived diagnosis/source-output identities:
/tmp/wasm-dd2/rewrite-season-after-118647/diagnosis.json.

Independent unit-mobility analysis finds a physical root with contact 1 (driver
4's wall) unloaded and separating at 0.07392688476743503 units/s. The other ten
normal loads remain positive; the constitutive residual is below 8.4e-13.
Keeping every normal loaded found no admissible root in the tested branches.
This is an independent contact-equation diagnosis, not original-output parity.

Private ablations compare the old fixed-load friction seed with selected
zero-normal/tangent rows. Keeping those rows alone advances the canonical query
but fails rotation 2. Starting the coupled release fit directly from the exact
outer state passes the query and all 22 rotations/reversals, plus all existing
physical regressions. The production change uses that direct fit, preserving
all ordinary bounds, material laws, tolerances and physical acceptance.
Root calculations, ablations and sparse traces are archived in
/tmp/wasm-dd2/rewrite-release-128036/independent-diagnosis.json.
Production Native and instrumented C advance the saved owner input to tick
128,037. Both standalone queries converge at velocity pass 641, with one restart,
residual below 1.4e-13 and 56 position passes below 9.7e-10. All strict LLVM19
checks, 34 Native and 32 WASM CTests pass. Twenty-seven frozen queries and 362
orderings retain independent physical checks on Native, WASM and instrumented C.
All eleven original-data ground checks and actual Native/sanitized/Chromium
window/input flows pass. Circuit-2/live and the complete eight-lap Circuit-5
race pass on all three variants; the scoped race receipt marks full_suite false.

The unmodified natural first-season attempt retains identical round-1 results,
and the production prefix reaches round-2 tick 150,000 without the old failure.
Its diagnostic prefix matches exactly. This proves advancement under unchanged
controls and bounds; the full season is still running and remains unproved.
The solver's Native static frame remains 350,904 bytes; the actual WASM fixtures
pass with the reserved 1 MiB stack. Immutable advancement receipt:
/tmp/wasm-dd2/rewrite-release-128036/advancement-report.json.

## Next

Continue the live unchanged-bound first-season attempt under 0025/0004, and
record its terminal outcome. Capture any new contact failure only if it occurs.
Natural arena completion and movement/tactics remain under 0002/0016. This work
item closes only the captured advancement contract.

## Accept

The actual failed step and independently checked contact reproducer pass with
production code on reached targets. Existing physical and real application checks
remain valid on Native/WASM, with sanitizer coverage and mandatory strict
LLVM19/Native/WASM gates. Ordinary racing continues beyond this failure without
state, damage, lap or result injection. Close only this advancement contract;
full-season and all-game acceptance require their complete evidence.

The advancement contract is proved by the saved-input checks, independent
physical fixtures, strict gates, original-data/actual-input regressions and the
natural prefix in advancement-report.json. Full-season/all-game acceptance
remains open.

Completed window/ground/race captures and the resolved tick-128,036 raw input
and diagnostic helpers have been removed after archiving their identities and
proof. The terminal season outcome and next unresolved sparse diagnosis are recorded
separately. Retained reports record the published code commit 9795cc3.

The completed unchanged-bound attempt next fails round 2 at tick 220,415.
All 104 production/diagnostic records match, and both actual failure replayers
agree. The new four-body/ten-contact velocity failure stays active under 0032.
Final parent receipt: /tmp/wasm-dd2/rewrite-release-128036/report.json.
