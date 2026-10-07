Type: Work item
Title: Resolve the next championship dependent-support advancement failure
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
is not reached. This identifies the failed stage, not a proved cause or root.
Receipt: /tmp/wasm-dd2/rewrite-season-after-128036/report.json.

A remapped four-body standalone query also fails velocity at
0.00018271467689423559 with one restart and no position repair, about 1.06e-08
from the full-owner residual. Only passes 512/4,096 and unit-mobility/Jacobian
checkpoints are retained. The constitutive matrix condition at pass 4,096 is
about 1.59e+18. This suggests dependent equations need investigation; it does
not prove the physical cause or a correction. Sparse receipt:
/tmp/wasm-dd2/rewrite-season-after-128036/sparse-analysis.json.

## Next

Diagnose dependent normal/friction rows and active branches in the actual query.
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
