Type: Work item
Title: Restore actual dense round-2 movement
Depends: 0016, 0042, 0043, 0045

## Contract

Diagnose and correct the proved ordinary round-2 movement noncompletion from
the frozen 268e3f9 campaign. Preserve material/contact rules, damage, laps,
scores, AI ownership and all unchanged fixture bounds. Demonstrate actual
physical escape and sustained route progress on Native/WASM/instrumented C,
then verify ordinary continuation. Full seasons and complete AI remain separate
acceptance requirements under 0004/0016.

## Evidence

The production campaign exits 1 without timeout after 1,231.59 seconds. Round 1
completes at 287,087 ticks. All round-2 steps advance through the unchanged
300,000-tick bound without a contact failure, but race/championship remains
RACING with zero player laps and zero round-2 scores. Health remains
0.2677000161305475. All 120 production records and thirteen sparse late movement
observations are retained with frozen source/helper/binary identity.
Receipt: /tmp/wasm-dd2/rewrite-next-world-load-0042/natural-season/terminal-report.json.

The last 300 simulated seconds show only 221.786 sampled XZ units of player
travel, with upright Y above 0.9718, wheel support at every sample, no overturn
and no sampled unresolved sweeps. Two twenty-car snapshots match the exact
production player/header at 280,000/300,000. The copied helper's optional old
destinations were moved to the current run only after closing and matching.
Receipts: natural-season/fleet-relocation-280000.json and
fleet-relocation-300000.json in the same directory.

Read-only typed reconstruction of all twenty core/controller fields at 280,000
matches every recorded next-copy control exactly. An independent original road
graph matches all twenty target points within 1.82e-12. The player's target-facing
dot is -0.43581666529655283; retired slot 9 caps goal-directed speed at zero,
337.44 units along that ray and -13.14 along the current heading. Retired slot
15 also lies 24.46 forward/407.38 sideways inside the heading-based traffic
gate. This evidence does not prove that replacing one projection restores
movement. Controller counters, route location, barriers, available turning
space and physical contact response still need causal diagnosis.
Receipt: /tmp/wasm-dd2/rewrite-next-world-load-0042/movement-diagnosis/geometry-report.json.

The terminal-owner/pilot capture is complete: exit 1 without timeout after
1,198.36 seconds, with one 31,216-byte owner and 72-byte pilot. It preserves
ordinary inputs, the 300,000-tick bound and 3,600-second process budget. All 120
production records, thirteen movement observations and both twenty-car snapshots
match exactly. No full-frame memory dumps were taken. Correction and complete
AI movement remain unproved.
Receipt: /tmp/wasm-dd2/rewrite-next-world-load-0042/movement-owner-capture/capture-report.json.

Ten ordinary six-second control probes all advance 1,200 steps from this exact
owner without changing poses, damage, lap state or scores. Every step fills the
64-record contact list. Original AI accepts 5.054 of 168.710 proposed XZ path
units; forward control accepts 56.338 of 1,559.319. An independent prescribed
field query confirms global clipping for a distant free spectator: 0.602 of
4.998 units is accepted with the dense field, while all 4.998 is accepted alone.
That query has refreshed wheel hints and is not an actual-owner replay.
Receipts: /tmp/wasm-dd2/rewrite-next-world-load-0042/movement-escape/summary-report.json
and movement-diagnosis/collision-budget-report.json.

Active 0045 separates report storage from the 64-event/64-constraint solver
bounds. Its private Native prototype translates the old owner fields explicitly.
The same forward control accepts 3,704.412 path units and 3,686.500 net units
in six seconds. Unchanged AI accepts 66.827 path units and remains at zero laps.
Metadata clipping is a proved motion-loss factor; no complete tactic, production
correction or WASM acceptance is claimed. Two focused cases reject the old
solver and pass the prototype, including a later collision after more than 64
support records. Production requires owned heap storage and borrowed report
views because two enlarged reports exceed the unchanged 1 MiB WASM stack.
Receipt: /tmp/wasm-dd2/rewrite-next-world-load-0042/report-capacity-prototype/comparison-report.json.

Production owned event recording under 0045 now passes strict Native/WASM and
ASan/UBSan gates, independent free-body/later-impact cases and all reached ground,
fleet, actual input and scoped race checks. Explicitly translated terminal-owner
probes reproduce the private prototype's restored physical escape: forward
motion reaches 3,686.500 net XZ units in six seconds. Unchanged AI still credits
zero laps in that same short terminal-state probe. The new ordinary Native
application completes four first-season results and opens season 2; rounds 2
and 4 end by engine retirement. The full AI suite passes ten levels but still fails Native Arena-B support
and the instrumented Arena-B deadline. Those are retained under 0016/0046;
independent WASM/instrumented session seasons both time out at their unchanged
3,600-second bound. Their last round-2 checkpoints are 230,000/155,000 ticks,
with zero player laps. Each completes a real first-round finish, but neither
establishes a completed season. Sustained route
recovery remains a separate contract.
Receipts: /tmp/wasm-dd2/rewrite-owned-events-0045/component-report.json,
0-report.json, 2-report.json, natural-season/terminal-report.json and
season-owner/{wasm,sanitized}/terminal-report.json.

## Next

Finish the remaining 0045 reached gates and diagnose remaining AI choices from
the exact terminal state
against available physical escape motion. Preserve damage, scores, checkpoints,
collision laws, captured contact regressions and strict Native/WASM gates,
including the known Arena-B movement failures under 0016.

## Accept

The actual terminal state reproduces, a causal defect and correct behavior are
demonstrated, and a focused physical/control regression rejects the previous
implementation. The correction sustains physical escape/route progress on
Native/WASM/instrumented C without relaxed bounds or manufactured state. Strict
LLVM19, captured physical/material/rollback checks and reached original-data/
actual-input/race gates pass. Ordinary current-source continuation demonstrates
the corrected movement; no partial scope proves a complete campaign or game.

## Current ordinary baseline after reachable body support

At cae5001, a fresh unchanged public-owner Native season reaches the round-2
300000-tick bound with zero player laps after 1799.94 seconds and no solver
rejection. The exact terminal twenty-body core/controller/tire state is retained
at /tmp/wasm-dd2/rewrite-season-body-support-0016/native-terminal-field.json;
canonical frozen-source/binary receipt is beside it. Fresh O3 ASan/UBSan completes
four natural rounds and starts season 2; WASM instead rejects tick 137455.
Closed 0047 independently proves and corrects that captured world's stronger
linear-pressure root on Native/WASM/O1 ASan/UBSan, without proving this movement
contract. Continue with actual Native route/control progress and corrected-source
ordinary WASM outcomes; preserve the same bounds and physical acceptance.

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
