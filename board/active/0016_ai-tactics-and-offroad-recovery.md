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

Two read-only Native Arena-B probes preserve all 300 production rows exactly,
including the same slot-17 support failure. The steady-loss probe exits zero
within the unchanged 180-second exporter deadline. The first continuous loss
starts at frame 1,278 and lasts at least 460 observed frames. Two needed
matching-ABI Native owner/event captures are only 24,712 bytes each; they are
not WASM memory inputs. At frame 1,477, all four lower body corners are
402.877..404.937 vertical units above independently queried original terrain.
Actual pair reports carry upward pressure from body 18, while body 17 also
contacts the radial arena barrier and body 16. The car rides another car's roof
down the outer slope; this sample does not show terrain penetration or a
supported overturned car. Wheel refresh remains unsupported there.
This identifies the traction/support failure to address, not a proved tactical
or recovery fix. Receipts: /tmp/wasm-dd2/rewrite-arena-cost-0046/support/support-report.json
and support-steady/report.json. Contact cost is tracked separately under 0046.

Typed fleet pair preparation preserves both complete Native/WASM Arena-B
inventories byte for byte, including Native slot-17 support at 1,929 frames;
it does not correct roof riding. The separate prior-source WASM/instrumented
public-session seasons both time out at 3,600 seconds, after natural first-round
finishes and round-2 checkpoints 230,000/155,000 with zero player laps. Complete
route recovery on those targets remains open. Receipts:
/tmp/wasm-dd2/rewrite-pair-preparation-0046/production-arena-verified/{native,wasm}/report.json
and /tmp/wasm-dd2/rewrite-owned-events-0045/season-owner/{wasm,sanitized}/terminal-report.json.

The dynamic body-supported tire experiment is not adopted. Its controlled
six-second saved-input probe restores road contact, and its complete Native B
case reaches at least 2,315 supported frames. Levels 1..A pass the unchanged
three-target AI gates. However, WASM B slot 17 overturns and has only 1,172
supported frames, below 2,280; the complete instrumented B inventory passes
movement but cannot establish a safe three-target correction. Two read-only
WASM observers preserve all 300 ordinary rows byte for byte. They show an
overturned resting car whose recovery counter saturates at 400 without righting.
The archived experiment, including its separately checked mixed-height tie
correction, remains under /tmp/wasm-dd2/rewrite-roof-support-0016/unadopted-experiment/.
Receipt: /tmp/wasm-dd2/rewrite-roof-support-0016/diagnosis-report.json.

An independent original-triangle and quaternion-matrix calculation identifies
the blocked recovery: the first touching source corner samples opposite bank
cell 843 and extrapolates center height -4,432.3935, while roof-facing cell 812
supports the actual center road at -2,942.7954. Recovery now selects the actual
contact normal most aligned with the roof, with stable source-corner ties.
A separate two-ramp valley regression fails against the previous production
code and passes the correction. The portable WASM-derived core pose rights
with four real tire contacts on Native, WASM and fresh O1 ASan/UBSan; the exact
previous recovery still fails the instrumented negative control. Vehicle clock,
rest deadline, damage, NPC distance and local landing window are unchanged.
Strict LLVM19/163-file checks, 37 Native/35 WASM CTests and all 220 original-grid
physical drops per Native/WASM/ASan/UBSan target pass. This proves a landing
selection correction, not complete B AI, roof traction or natural campaigns.
Receipt: /tmp/wasm-dd2/rewrite-bank-recovery-0016/component-report.json.

The fleet now uses reachable body-supported tires with immutable simultaneous
prediction and equal/opposite spring/traction forces at one common contact point.
An initial retest of the vertical prototype plus the accepted bank-recovery fix
still fails WASM B support at 2,211 frames. A read-only observer preserves all
300 ordinary rows and captures a 5,417-unit rear-tire load on another car's nose.
Independent sphere/plane geometry requires suspension displacement -264.224,
outside the unchanged -15..70 window. The corrected finite-face query follows
the actual suspension axis; a banked two-car fixture checks sphere contact and
load independently. The previous vertical algorithm fails both new geometry
checks. Native/WASM replay of the portable twenty-body checkpoint replaces the
false nose support with a real road contact and 572-unit load.

The final source passes strict LLVM19/167-file formatting, 38 Native/36 WASM
CTests, ten fresh O1 sanitizer targets with 47 instrumented library units, and
716 physical contact checks per target. Valgrind reports zero errors/leaks for
the focused subsystem and the original-data twenty-body replay. All eleven
unchanged sixty-second AI cases pass on Native/WASM/fresh O3 ASan/UBSan:
2,640,000 vehicle steps and 29,214 independent path queries per target. Arena-B
minimum supported counts are 2,374/2,323/2,340 of 2,400, above the unchanged
2,280 requirement. Its observed target times are 66.42/89.73/263.70 seconds,
within the existing 180/180/360 deadlines. The full-suite launcher terminates
in level 9; completed 1..8 checks are independently revalidated, repeated 9
results are byte-identical with observed bounded exits, and A/B retain complete
receipts. No interrupted target is counted as a successful run.

Original-data scalar vehicle and fleet checks pass on all eleven levels, as do
31 Native/31 instrumented window and 144 Chromium comparisons with real input
and no browser errors. This accepts reachable moving-body suspension and the
existing sustained-driving suite, not complete tactics, natural races, seasons
or original handling parity. Receipts:
/tmp/wasm-dd2/rewrite-strut-support-0016/{component-report,field-report,negative-report}.json
and ai/canonical-report.json.

## Next

Resume fresh ordinary damage-enabled races and public-session seasons on the
accepted source. Diagnose remaining round-2 dense-route blockage with portable
typed checkpoints and actual controls; expanded wheel state makes older binary
owner captures valid only for their matching frozen build. Preserve every
physical work bound, damage/lap/score rule and unchanged acceptance deadline.

Follow the terminal route/movement diagnosis under 0044. Body-supported tires
now address the proved roof-support gap; they do not establish damage-aware
traffic escape or general off-road rescue. Keep contact-cost profiling under
0046 and older failed trajectories/timings attached to their source epochs.

Preserve the Total Destruction player target and require naturally published
results in complete races. Implement remaining tactics/recovery from observed
behavior without manufactured score, damage or route progress.

## Accept

Every track/mode sustains valid AI decisions and physically reaches required race outcomes, including damaged/off-road/overturned states, reset and paused clocks on both targets.

## Verified heading-safe lane return

Closed 0048 preserves immediate heading avoidance and prevents a return into
an obstructed base lane. Exact portable terminal-state controls and independent
original-level geometry identify the false merge. Final Native/WASM/O3 ASan/UBSan
public components advance more than 4000 net units in six seconds; health is
frozen and the damage/recovery/lap/result owner is not replayed. Strict LLVM19,
38/36 Native/WASM CTests, mirrored previous-source negatives, fresh instrumented
synthetic tests, zero-error/leak Valgrind and unchanged sixty-second AI cases on
all eleven levels pass. Complete ordinary corrected-AI movement/campaigns remain
open. The frozen solver-only d0a1bd7 WASM trajectory rejects tick 155992 under 0049;
it is not a corrected-AI trajectory. Receipt:
/tmp/wasm-dd2/rewrite-native-movement-0044/component-report.json.

## Terminal ordinary heading-safe baseline

All immutable 570df9d baselines are terminal without timeout/contact rejection:
Native reaches round-1 bound at credited lap9; WASM/sanitized naturally finish/
retire rounds1/2 but reach round-3 bound at credited lap7. The new source does not
inherit those baseline trajectories as proof. Finished NPCs brake in the rewrite;
the original reference keeps calling their AI. Active 0050 owns actual finish
movement; closed 0051 resolves the independent nineteen-contact component while
0049 retains full-owner continuation. Existing Arena-A cross-target angular
acceptance remains open under 0052. Receipts:
/tmp/wasm-dd2/rewrite-season-passing-return-0016/canonical-report.json and
/tmp/wasm-dd2/rewrite-finish-movement-0050/reference-finisher-report.json.
