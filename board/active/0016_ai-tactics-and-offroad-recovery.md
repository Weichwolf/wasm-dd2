Type: Work item
Title: Complete AI tactics and off-road recovery
Depends: 0002

## Contract

Provide reliable road/arena driving, damage-aware tactics, supported overturn and off-road recovery for every track and race mode.

## Evidence

Source path guidance, traffic avoidance, pursuit, timed reverse, accepted-motion stall detection and supported overturn righting exist. Damage-aware tactics and automatic off-road recovery remain incomplete. Long AI probes disable damage and therefore do not establish complete race AI.

A sparse, damage-enabled Native B replay of a326993 reaches the unchanged
120000-tick bound without solver aborts or unresolved collision steps. The
supported player remains alive in a slow contact-bound field; all nineteen
opponents retain human target zero. The player's front damage remains
0.588834/0.947100 at the terminal checkpoint. Final fleet snapshots include
actual post-damage controls, motion, wheel support, retirement, recovery and
reverse/progress counters. Report:
/tmp/wasm-dd2/rewrite-branch-arenaB-native-diagnosis/diagnosis-report.json.
All 24 player checkpoints and the last two complete fleet position/step/damage,
AI-target and recovery snapshots match the terminal production Native B trace
exactly; see production-checkpoint-match.json in the same directory.
These observations do not establish a specific tactical fix or full AI acceptance.

The current two-endpoint contact runtime (69d97d7) completes the first natural
Stockcar championship round with unchanged 287,087 ticks and twenty-driver scores.
Both production and logging-only attempts then reach the unchanged 300,000-tick
round-2 bound, matching all 120 JSON records. Every requested physical step
advances; there is no logged contact failure or failure checkpoint. The player
has zero credited laps and health 0.4003601829670652; race/championship stay RACING
with no scores manufactured. Both processes exit 1 without timeout. The same
production child was adopted after launcher termination, without engine restart.
This proves current natural noncompletion, not its tactical/kinematic cause.
Current terminal receipt: /tmp/wasm-dd2/rewrite-endpoints-233358/report.json.
Closed 0033 proves the separate contact saved-input/continuation correction.

A sparse current-runtime reproduction matches all 120 production records, with
13 late player samples and two full-fleet observations. The supported upright
player moves only 16.0535 XZ units over the final 100 simulated seconds, without
sampled unresolved sweeps. Its reverse/stuck counters sum to 299 at every sample
after tick 240,000, while the requested sampled throttle remains -0.6. Independent
source geometry confirms the path target is forward-facing (dot above 0.988)
at both fleet checkpoints. The low-speed timer counts reverse decisions and can
therefore rearm reverse immediately on expiry. This is a controller defect;
other dense-field tactics and complete natural outcome causes remain separate.
Receipt: /tmp/wasm-dd2/rewrite-stock-round2-movement/report.json.

The low-speed timer now counts forward decisions only. A stationary reverse
cannot immediately rearm that timer; the focused regression requires two full
forward/reverse cycles. The regression fails against the unchanged previous
controller and passes on Native, WASM and ASan/UBSan. Strict LLVM19, all 35 Native
and 33 WASM CTests, and actual eleven-level Native/sanitized/Chromium input checks
pass. Receipt: /tmp/wasm-dd2/rewrite-reverse-expiry/report.json.

The unchanged sixty-second guidance/driving suite completes all eleven levels
on three targets. All 29,214 independent path queries per target and pose,
decision-cadence and reset checks pass. Sustained movement passes 31 of 33 target
scopes: Native B slot 12 travels 9,728.9505 units; sanitized B slots 12/13 travel
9,194.5768/9,937.1477, below the unchanged 10,000-unit requirement. All retain
2,400 supported samples. WASM B passes. This is an open movement failure, not
complete AI acceptance; no thresholds, deadlines or simulation lengths change.
Completed receipt with all B observations:
/tmp/wasm-dd2/rewrite-reverse-expiry/ai/diagnosis-report.json.

The corrected controller exposes a new natural physical failure at round-2
step 83,450. AI/frame/championship inputs remain valid; all 76 records match
between production and failure-only diagnosis. A single 31,216-byte owner
checkpoint and three world contacts involve only physical body 14. Velocity
exhausts 4,096 passes at residual 0.000335284234228977; position is not reached.
Both the actual owner replay and the isolated full-field / one-body-remapped
queries reproduce failure against unchanged production libraries. This is a
separate contact advancement task; natural campaign acceptance remains open.
Receipt: /tmp/wasm-dd2/rewrite-reverse-expiry/failure-capture/report.json.
Production replay/query receipts: production-query/owner-replay-report.json and
production-query/report.json within the same capture directory.

Closed 0035 now advances this saved contact step and the natural campaign through
round-2 tick 160,000, with mandatory, three-target physical and actual-input gates.
The full campaign and the recorded Arena-B movement failures remain open.
Receipt: /tmp/wasm-dd2/rewrite-fitted-83450/report.json.

That natural campaign is now terminal at tick 190,078 with valid inputs, healthy
player engine and no credited laps. Active 0036 owns the next failure-only
advancement diagnosis; its cause is not yet proved. Terminal receipt:
/tmp/wasm-dd2/rewrite-fitted-83450/natural-season/terminal-report.json.

The current 268e3f9 incident-pressure campaign now advances all physical steps
through the unchanged 300,000-tick second-round bound without a contact failure.
It exits 1 without timeout after 1,231.59 seconds, with zero player laps and
health 0.2677000161305475. All 120 ordinary records are retained. Thirteen late
samples show an upright supported player moving only 221.786 sampled XZ units
over the last 300 simulated seconds, with no sampled unresolved sweeps. The
two twenty-car snapshots match the exact production player/header at ticks
280,000/300,000; optional copied helper destinations were relocated only after
they closed. Receipt: /tmp/wasm-dd2/rewrite-next-world-load-0042/natural-season/terminal-report.json.

A read-only typed reconstruction of the 280,000 core/controller fields matches
all twenty recorded next-copy controls exactly. An independent original road
graph matches all target points within 1.82e-12. The player's target-facing dot
is -0.43581666529655283. A retired car caps the goal-directed speed to zero;
another retired car also lies inside the current-heading traffic gate. Changing
one projection alone has not been proved to restore movement. The current
controller/geometry evidence identifies a concrete diagnosis input, not a cause
or an accepted tactical correction. Active 0044 owns one terminal owner/pilot
capture and the next movement/escape diagnosis.
Receipt: /tmp/wasm-dd2/rewrite-next-world-load-0042/movement-diagnosis/geometry-report.json.

After production owned event recording d6df3ba, the full sixty-second AI suite
passes levels 1..A on Native/WASM/instrumented C. Arena-B Native has improved
travel (every opponent exceeds 37,518.840 units) but slot 17 remains supported
for only 1,929 of 2,400 frames, below the unchanged 2,280-frame requirement.
WASM passes that arena (minimum travel 44,399.075 and support 2,352). The O3
ASan/UBSan arena exceeds its unchanged 360-second deadline and is not a
completed movement case. Diagnose the actual support-loss trajectory and its
physical/controller/recovery cause; do not infer a fix from increased travel.
Keep deadline/work diagnosis under 0046 and preserve every acceptance bound.
Receipt: /tmp/wasm-dd2/rewrite-owned-events-0045/ai-partial-report.json.

## Next

Follow the current terminal movement/owner diagnosis under 0044. Diagnose the completed
Native/sanitized B movement failures using the retained observations, before
choosing a tactical correction; keep the ordinary movement requirement and bound.
Preserve the Total Destruction player target and every engine, damage, lap and
score rule. Use ordinary complete races to identify off-road and
tactical failures; recover original behavior and implement explicit recovery/tactics
without manufactured score, damage or route progress.

## Accept

Every track/mode sustains valid AI decisions and physically reaches required race outcomes, including damaged/off-road/overturned states, reset and paused clocks on both targets.
