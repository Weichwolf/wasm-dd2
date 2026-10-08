Type: Work item
Title: Advance dependent world supports in scheduled racing
Depends: 0031

## Contract

Advance the production-equivalent second-round Stockcar failure at tick 220,415
with finite state, complementarity, friction cones, dissipative response and exact
impulse accounting. Preserve transactional rollback, material laws, ordinary pass
bounds, final tolerances and ordinary controls, AI, damage, laps and scoring.
Diagnose the actual failed stage and cause before choosing its correction.

## Evidence

The unchanged-bound natural Native first-season attempt completes round 1 at
287,087 ticks with identical previous twenty-driver scores and correct result
consumption. It advances beyond the old 128,036 failure, then fails round 2 at
220,415. AI, frame and championship are valid; race/championship stay RACING.
Health is 0.4230288876319067, credited laps zero, throttle -0.6, brake zero and steering
-0.6664023079031487. This is a failed full-season attempt, not complete gameplay.
Parent verification: /tmp/wasm-dd2/rewrite-release-128036/report.json.

The failure-only diagnostic matches all 104 production JSON records. Both
production and diagnostic replayers reproduce the same failed step without
advancement. The single 31,216-byte input has SHA-256
56a4118bb739060fda06ff2f99fdaee36f2427c7489e72be509b4feaf98c337c.
Drivers 0/1/4/13 form ten contacts: six world supports and four pairs. Query SHA-256:
9f915f45161365ec385871213f275b9b30e96259f7f9f2baeb09fd4e46af9af5.
Velocity exhausts 4,096 passes at residual 0.0001827040936497326; position repair
is not reached. This initial capture identifies the failed stage; the diagnosis
below establishes the dependent-row cause and admissible branch.
Receipt: /tmp/wasm-dd2/rewrite-season-after-128036/report.json.

A remapped four-body standalone query also fails velocity at
0.00018271467689423559 with one restart and no position repair, about 1.06e-08
from the full-owner residual. Only passes 512/4,096 and unit-mobility/Jacobian
checkpoints are retained. The constitutive matrix condition at pass 4,096 is
about 1.59e+18. This suggests dependent equations need investigation; it does
not prove the physical cause or a correction. Sparse receipt:
/tmp/wasm-dd2/rewrite-season-after-128036/diagnosis.json (archived sparse analysis).

Independent branch enumeration finds a physical root loading wall contacts 1/5
and releasing contacts 2/3/4. Its constitutive error is below 6.2e-13; released
normal speeds are positive (about 0.000354/0.000236/0.000118 units/s). The two
collinear normal-row second differences are below 3.9e-14. Retaining those rows
as loaded leaves dependent equations, while the admissible root uses simultaneous
releases. This is equation-level diagnosis, not original-output parity.

Private ablations show that choosing releases only from current separating
velocities misses rotation 4. A fixed geometric patch mask, an active retained
normal and exact zero impulse targets pass the captured query and all twenty
rotations/reversals, alongside the previous 389 physical cases. Tiny reconstructed
loads otherwise amplify through pressure-dependent friction softness.
A Native private owner replay advances the actual saved step to 220,416.
Production Native and instrumented C now also advance the saved owner input
from 220,415 to 220,416. Their captured queries converge at velocity pass 513
with one restart, residual below 5.3e-15 and 99 position passes below 6.8e-10.
All strict LLVM19 checks, 34 Native and 32 WASM CTests pass. Twenty-eight queries
and 382 orderings preserve independent physical checks on Native/WASM/ASan/UBSan.
The Native solver frame is 353,016 bytes; actual WASM fixtures retain the reserved
1 MiB stack. All eleven original-data ground checks and actual Native/sanitized/
Chromium window/input checks pass. Six scoped Circuit-2/live and Circuit-5/complete
eight-lap race targets pass. The latter is partial race-suite coverage.
The unmodified natural season and logging-only companion both exhaust the
900-second wallclock allowance after reaching round-2 tick 160,000. Their complete
JSON prefix matches exactly; neither logs a contact failure or produces a failure
checkpoint. This is an incomplete attempt, not a physical advancement proof.
Cumulative CPU time is about 56% of elapsed wallclock time; no algorithmic
performance cause is established. Receipt:
/tmp/wasm-dd2/rewrite-patch-220415/natural-timeout-report.json.
The same production binary and a fresh logging-only companion now run with a
3,600-second wallclock allowance, unchanged 300,000-tick round bounds and ordinary
AI controls. Outputs are under /tmp/wasm-dd2/rewrite-patch-220415-natural-long/
and /tmp/wasm-dd2/rewrite-season-after-220415-long/.
Both longer-wallclock runs now reach round-2 tick 230,000, with exactly matching
complete JSON prefixes. Round 1 retains its previous 287,087 ticks and twenty
scores. Ordinary advancement beyond 220,415 is proved; no position, damage, lap
or result injection supplies it. Close this saved-input/continuation contract.
Complete physical seasons and all-game acceptance remain unproved. Receipt:
/tmp/wasm-dd2/rewrite-patch-220415/advancement-report.json.
Completed input/query/matrix/private-ablation captures are removed after their
hashes, independent diagnosis and correction receipt are archived in
/tmp/wasm-dd2/rewrite-season-after-128036/diagnosis.json. Current live natural
processes and required files remain intact.
Verified scope: /tmp/wasm-dd2/rewrite-patch-220415/gates-report.json.
Independent diagnosis/ablation receipt:
/tmp/wasm-dd2/rewrite-patch-220415/independent-diagnosis.json.

## Next

Follow the current longer-wallclock natural attempts to their terminal outcomes
under 0025/0004. Record any next contact failure, healthy round bound or timeout
separately from this proved saved-input/continuation correction.
Preserve frozen-query, analytic, original-data and actual application checks.
Complete physical seasons remain under 0025/0004; natural arena completion and
movement/tactics retain 0002/0016.

## Accept

The actual failed step and independently checked contact reproducer pass with
production code on reached targets. Existing physical and actual application
checks remain valid on Native/WASM, with sanitizer coverage and mandatory strict
LLVM19/Native/WASM gates. Ordinary racing continues past this failure without
state, damage, lap or result injection. Close only this advancement contract;
full-season and all-game acceptance require their complete evidence.
