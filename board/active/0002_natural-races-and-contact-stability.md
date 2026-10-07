Type: Work item
Title: Natural race completion and stable contact response

## Contract

Ordinary twenty-car circuits and destruction arenas reach valid natural results on Native, WASM and ASan/UBSan without solver aborts, state injection or relaxed scenario coverage.

## Evidence

At a72ecf0 the full race run yielded 139 passing target receipts. Arena B Native completed naturally; WASM failed dd2_driving_advance at frame 51704; sanitized B hit the wall-time bound. A bounded capture reproduces a five-contact solver cycle (~0.1 units/s residual). Reports: /tmp/wasm-dd2/rewrite-progress-race-verification/terminal-report.json and /tmp/wasm-dd2/rewrite-arenaB-wasm-failure/identity.json.

A private best-residual secant safeguard solves that frozen query and six prior queries on all three targets with independent material checks. Its natural B WASM replay still aborts at frame 20430. It is not integrated or accepted. A bounded next-failure capture is complete under /tmp/wasm-dd2/rewrite-arenaB-solver/capture-next/.

The full post-pruning AI run passes the first ten levels on all targets; arena B Native and sanitized slot12 miss sustained travel (9010.022/9769.014<10000), although all2400 samples are supported. Keep the original threshold and diagnose this behavior. Report: /tmp/wasm-dd2/rewrite-connected-ai-verification/terminal-report.json.

Closed0017 now supplies a bounded ordinary-coordinate restart while retaining all contacts/material checks and4096 total passes. The5contact failure completes in477 passes and the3world junction in40 on all targets. Required31Native/30WASM gates and independent material checks pass. The fresh production B-total-survive run under /tmp/wasm-dd2/rewrite-restart-arenaB-verification reaches natural Native retirement after93.975 seconds; WASM fails at race step52320; sanitized also aborts advancement at race step48037. This selected run is terminal and fails overall. Report: /tmp/wasm-dd2/rewrite-restart-arenaB-verification/terminal-report.json. A bounded ten-contact/eight-body capture shows that forced restart at2048 interrupts a healthy accelerated solve needing3467 iterations. Follow-up0018 removes that unconditional trigger while preserving stagnation detection and the total budget; production31Native/30WASM and strictLLVM19 gates pass, as do independent nine-query/material checks on all three targets.

Native B AI capture at6fc2fe2 confirms slots12/13 target one another after step~2000 and spend much of the remaining drive reversing with all wheels supported. A private alternate-target escape prototype lowers travel to8072.943/8644.802 and is rejected. Report: /tmp/wasm-dd2/rewrite-arenaB-ai-diagnosis/report.json. These are control/movement diagnostics, not complete AI acceptance.

A production-equivalent private WASM replay of4f0aeac passes the frozen query but aborts at race step52337, seventeen steps later. Report: /tmp/wasm-dd2/rewrite-arenaB-restart-failure/candidate-race-report.json. The fresh bounded capture under /tmp/wasm-dd2/rewrite-arenaB-stagnation-failure/ is terminal. Its nine contacts connect seven bodies; Native independently reproduces the4096-pass failure with0.0004257565 units/s residual and no coordinate restart. A private best-state fallback also fails, because this query never enters fallback. Reports: report.json and replay-report.json. Improve accelerated convergence for the coupled slow modes without changing material laws or relaxing the pass/tolerance budget. No full natural-race pass is claimed.

Private follow-up diagnostics are recorded in
/tmp/wasm-dd2/rewrite-coupled-secants/report.json. Two-direction and deeper
secant histories, exact tangential coordinates and local normal/friction blocks
still fail the seven-body query. Sparse residuals locate the dominant slow mode
at its two world contacts. A private finite-difference Newton step solves the
query on Native, WASM and ASan/UBSan within the unchanged 4096-pass budget. Native
and sanitized independent ten-query material checks pass; WASM fails the
native15 final constitutive oracle. Its actual WASM arena also aborts at step
18062. That new four-body/seven-contact state reproduces the private failure on
Native while the unchanged production solver succeeds. The private candidate
is rejected and is not integrated; its temporary matrix allocation also does
not meet the production no-allocation contract. Terminal experiment binaries
and successful raw diagnostics were removed after recording identities and
reports. Compact queries and private sources remain for the next diagnosis.

A restricted private follow-up delays Newton until 512 ordinary sweeps and
keeps it out of the coordinate-only fallback. All ten frozen material queries
and ten analytic laws pass on Native and WASM, and the previously rejected
four-body query again completes through the preserved fallback. Its actual
WASM arena passes the old step52337 failure but aborts at step52379. The next
compact query connects twelve bodies through fifteen contacts. Capture, source
identity and terminal report are under
/tmp/wasm-dd2/rewrite-coupled-secants/natural-cold/.

A readable private version uses a bounded automatic matrix instead of allocation
and passes repository-configured clang-format/tidy 19 plus all ten material
queries on Native/WASM/ASan/UBSan. It needs a larger WASM stack; its scoped report
is /tmp/wasm-dd2/rewrite-coupled-secants/fixed-material-report.json. These private
results do not establish production gates or natural completion. No solver
change is integrated. Terminal binaries and completed raw captures are removed;
compact failing queries and the prepared source remain for diagnosis.

Closed 0020 now integrates delayed bounded coupled corrections in both phases.
The private step52379 twelve-body query independently reproduces on Native with
the old production solver. The new seven-body and twelve-body queries and the
four-body fallback pass all independent material/motion checks on Native, WASM
and fully instrumented ASan/UBSan. Required 32 Native/31 WASM CTests and strict
LLVM19 for 145 owned files pass. Reports are under
/tmp/wasm-dd2/rewrite-delayed-contact-verification/. No complete natural-race
pass is established by these component checks.

The selected production run at ea6a440 is terminal and fails overall. Native
reaches natural engine retirement after 93.975 simulated seconds. WASM aborts
after race step 52778 (elapsed 52378); ASan/UBSan aborts after race step 51297
(elapsed 50897). Scoped receipts and compact failure summaries are under
/tmp/wasm-dd2/rewrite-delayed-arenaB-ea6a440/terminal-report.json. Terminal raw
logs were hashed and removed; successful Native evidence is retained.

Linker wrappers of the unchanged production WASM library objects reproduce all
five 10000-step player checkpoints and the exact failed pose. The compact
thirteen-body/eighteen-contact query independently fails on Native and WASM
within 4096 passes, with residuals 2.1006819e-5 and 1.8766935e-5. Capture identity
and replay evidence are under
/tmp/wasm-dd2/rewrite-delayed-wasm-failure/replay-report.json. Controlled probes
at identical saved iterates locate forward-difference truncation in late
Newton directions; 0021 owns the bounded numerical correction. The sanitized
natural failure still needs its own production-equivalent capture if it remains
after that correction. No full natural-race pass is claimed.

Closed 0021 now reduces the forward-difference probe to 1e-6 and certifies
convergence after an ordinary sweep, preventing predicted residuals from
publishing tiny separating impulses. The new thirteen-body contact query and
all twelve prior independent cases pass on production Native/WASM/LLVM19
ASan/UBSan. Required 32 Native/31 WASM CTests and strict LLVM19 for 145 files
pass. Report: /tmp/wasm-dd2/rewrite-contact-accuracy-verification/quality-report.json.
These component checks do not establish natural completion.

The selected production run at 7dd248b is terminal and fails on all targets.
WASM aborts after race step 11664 (elapsed 11264); sanitized aborts after 10106
(elapsed 9706). Native completes the 120000-step fixture bound without a solver
abort, but remains active with seventeen engines alive and player front damage
0.594363/0.952254, so it has no natural result. Do not extend the bound or inject
damage/results to make this pass. Target receipts, hashes and compact snapshots:
/tmp/wasm-dd2/rewrite-contact-accuracy-arenaB-7dd248b/terminal-report.json.
Completed raw traces, including the 555 MB Native trace, were removed after
reporting.

First-failure wrappers match both production trajectories at the 5000-step
checkpoints and failed poses. The compact WASM query has seven bodies/ten
contacts; the sanitized query six bodies/seven contacts. Both independently
fail on Native within 4096 passes. Refit/branch direction work belongs to 0022;
the independent fifteen-query private checks pass, but complete natural
behavior remains unproved. Reports are under
/tmp/wasm-dd2/rewrite-ordinary-sweep-wasm-failure/ and
/tmp/wasm-dd2/rewrite-ordinary-sweep-sanitized-failure/.

Closed 0022 now refits predicted negative pressures and tries guarded sticking
directions for loaded world contacts. Both new queries and all thirteen prior
queries pass independent material/motion/energy/clock checks on production
Native/WASM/fully instrumented LLVM19 ASan/UBSan. Required 32 Native/31 WASM
CTests and strict LLVM19 for 145 files pass. Reports:
/tmp/wasm-dd2/rewrite-contact-branches-verification/. These component checks
do not establish complete natural races or fix the nonterminal Native behavior.

The source-identified production run at a326993 is terminal: 140 of its 141
attempted target receipts pass. All 43 short physical/rule scenarios pass on all three targets; injected
route checks do not prove physically completed circuits. Arenas 8, 9 and A reach
natural results on all three targets. Arena B WASM reaches engine retirement
after 134.045 simulated seconds and sanitized after 114.895 seconds. Native B
remains running at the unchanged 120000-tick fixture bound, so the suite fails
and skips its last two Circuit-5 scenarios. Report:
/tmp/wasm-dd2/rewrite-contact-branches-full-a326993/terminal-report.json.

The separate Circuit-5 follow-up passes all six target receipts: eight physical
twenty-car Stockcar laps, then nine one-car Time Trial laps beyond the original
eight-lap race limit. Stockcar finishes after 835.36 simulated seconds and Time
Trial withdraws after 935.12 seconds on all three targets. Reports:
/tmp/wasm-dd2/rewrite-contact-branches-circuit5-a326993/report.json and
/tmp/wasm-dd2/rewrite-contact-branches-full-a326993/combined-scope-report.json.
Together these runs account for all 49 scenarios and 147 target scopes, with
146 passes and the Native B natural-completion failure. This is not a passing
full-suite or complete-game result.

A separate sparse Native B diagnosis using unchanged production library objects
reaches all 120000 fixture ticks without an advancement or solver failure, but
has no natural result. Fourteen cars remain available. The player has four
supported wheels and front damage 0.588834/0.947100; no sampled interval contains
an unresolved collision step. Its last 125 simulated seconds have a 131.138-unit
sum of checkpoint chords, a lower bound on travel rather than a full motion
trace. Retained player checkpoints and the last two fleet snapshots are under
/tmp/wasm-dd2/rewrite-branch-arenaB-native-diagnosis/diagnosis-report.json.
All 24 player checkpoints and the final two twenty-car position, step, damage,
AI-target and recovery snapshots now match the production Native B trace
exactly; see production-checkpoint-match.json in the same diagnostic directory.
Completed raw output and the temporary capture binary were hashed and removed.
This identifies a movement/tactics investigation, not a complete physics or
natural-race pass.

The shared world/body low-speed material law is separately proved under 0027:
all seventeen captured queries, analytic material cases, original-data ground/
recovery checks and selected natural Circuit-5 racing pass on all three targets.
The current Native championship probe progresses through the saved tick-66,449
step, but a two-body support failure at tick 86,643 remains under 0028. This new
scoped evidence does not rerun or replace the historical full race/arena suite.

Closed 0028 separately proves the analytic mixed-contact correction with all
eighteen captured queries and fourteen row orderings on three targets, mandatory
quality gates, the actual saved step and eleven-level real window/browser checks.
The fresh Native Stockcar season passes the old tick-86,643 failure and reaches
107,550 before a position-repair failure, reproduced under 0029. Original-data
ground checks and six selected race target scopes also pass, including the
natural eight-lap Circuit-5 Stockcar race on all three targets. Current scope: /tmp/wasm-dd2/rewrite-coupled-86643-final/report.json.
This is not a passing full race/arena suite or completed physical season.

Closed 0029 now proves the saved advancement cases through tick 93,231, with
25 captured queries, 334 orderings and strict Native/WASM/sanitizer gates.
Actual window/input and original-data ground checks pass all eleven levels;
six selected race target scopes include the complete eight-lap Circuit-5 race.
The unchanged-bound Native season passes every previously recorded failure tick,
including 107,550, then fails round 2 at 118,647. Sparse production-equivalent
capture and actual replays identify a three-world-contact velocity failure on
driver 15 under 0030. Complete physical seasons remain unproved.
Current evidence: /tmp/wasm-dd2/rewrite-release-93231/report.json and
/tmp/wasm-dd2/rewrite-season-after-93231/report.json.

## Next

Diagnose the reproduced nonterminal Native B movement/tactics behavior while
preserving material laws, tolerances and the shared pass budget. Expand ordinary
physically completed circuit coverage beyond circuit 5. Do not replace a missing
natural result with a longer bound, forced retirement, health changes or scores.

## Accept

Source-identified complete race checks pass on all three targets; each failure reproducer passes independent motion, contact, energy and clock checks. Natural termination and full-suite coverage are required; a timeout or frozen-query pass is insufficient.
