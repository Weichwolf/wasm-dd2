# 0048: Keep a clear passing lane while its base path is blocked

## Contract

Correct the false merge from a clear passing path into an obstructed base path.
Preserve immediate body-heading avoidance, source road ownership, rate limits,
speed/steering limits, physics, damage, lap and score behavior. Prove mirrored
hold/return cases, actual original-level movement and the unchanged full AI
suite on Native/WASM/instrumented C. Complete ordinary campaigns and movement
remain under 0002/0004/0016/0044.

## Evidence

At d0a1bd7 the Native terminal typed field at round-2 tick 300000 exactly
reproduces all twenty copied next controls. Independent original-level-2 lane
geometry matches the player target exactly. Its current goal is clear, while
base lane 0.375 points toward slot 8; returning from lane 0.441 enters that
traffic gate. Candidate 0.555 is clear, while 0.195 remains obstructed.

With the same portable twenty-body/pilot observations and frozen scalar engine
health, original AI travels 1272.856 but ends 643.421 units backward in six
seconds. Prescribed forward motion travels 4433.842, while prescribed right
lane travels 4290.750 without any player body contact. A current-goal-only
trigger still oscillates; it is not adopted. These are component isolations,
not full-owner damage/recovery/lap/result replays.

The first base-goal-only implementation restores over 4000 units of net
movement on all three targets, but the unchanged full AI suite rejects Native
level 1: slot 8 has only 1650 supported frames, below the unchanged 2280.
That source is rejected. Retaining immediate body-heading avoidance as well
as checking the return-to-base path passes the independent mirrored semantics
and the full unchanged eleven-level sixty-second suite on Native/WASM/O3
ASan/UBSan. No thresholds, original scenarios or deadlines are weakened.
Receipts: /tmp/wasm-dd2/rewrite-native-movement-0044/.

Final source passes LLVM19 format/tidy across 168 files, 38 Native/36 WASM
CTests, mirrored old-source negatives on both targets, a freshly compiled
34-unit O3 ASan/UBSan synthetic test and Valgrind with zero errors/leaks.
The exact six-second public-component field ends 4034.968/4021.720/4044.720
units forward on Native/WASM/instrumented C, with no player body/world contacts.
Scalar health remains frozen in this component; damage/recovery/lap/results are
not replayed. All eleven original-level AI cases pass their unchanged sixty
seconds, reset, decision cadence, travel and 2280/2400 support requirements,
covering 29214 independent path queries and 2640000 vehicle steps per target.
The final test source replaces a literal with a named enum; its fresh synthetic
receipt independently covers the final file after the full-suite synthetic
binary was compiled. Game/path/export units remain unchanged during that suite.
Canonical evidence: /tmp/wasm-dd2/rewrite-native-movement-0044/component-report.json.

## Next

Continue ordinary corrected-source owner campaigns under 0016/0044 and the
independent tick-155992 rejection under 0049. Keep full game/season acceptance
open; this item closes only the proved heading-safe lane-return contract.

## Accept

Old source fails mirrored blocked-base/clear-passing regressions; corrected
Native/WASM/instrumented C holds the safe lane, respects immediate obstacles
and returns when both paths clear. The exact portable original-level-2 component
advances physically without manufactured damage/progress/results or changed
material. Full strict gates, original-path comparisons and unchanged sixty-second
AI/reset/support/travel contracts pass on all eleven levels and all targets.
This closes lane-return semantics, not complete natural campaigns.
