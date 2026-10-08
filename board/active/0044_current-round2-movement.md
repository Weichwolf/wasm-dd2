Type: Work item
Title: Restore actual dense round-2 movement
Depends: 0016, 0042, 0043

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

One terminal-owner/pilot capture is running from the frozen production source
and libraries. It adds only one snapshot at the same round-2 bound and relocates
optional diagnostic paths into its own directory. It preserves ordinary inputs,
the 300,000-tick round bound and 3,600-second process budget. Require all 120
production records and thirteen movement observations to match before using
that Native ABI owner for physical escape probes. Reproduction and correction
are not yet proved; no full-frame memory dumps are requested.
Live receipt: /tmp/wasm-dd2/rewrite-next-world-load-0042/movement-owner-capture/identity.json.

## Next

Finish the exact terminal capture. Replay the captured owner with ordinary
controls to separate available escape motions from controller choices, without
altering damage, scores, checkpoints or collision rules. Independently verify
the relevant road/barrier/car geometry before choosing a readable typed tactic
or recovery correction. Preserve all captured contact regressions and strict
Native/WASM gates, including the known Arena-B movement failures under 0016.

## Accept

The actual terminal state reproduces, a causal defect and correct behavior are
demonstrated, and a focused physical/control regression rejects the previous
implementation. The correction sustains physical escape/route progress on
Native/WASM/instrumented C without relaxed bounds or manufactured state. Strict
LLVM19, captured physical/material/rollback checks and reached original-data/
actual-input/race gates pass. Ordinary current-source continuation demonstrates
the corrected movement; no partial scope proves a complete campaign or game.
