# 0051: Preserve warm normal branches during world-pressure refinement

## Contract

Recover the captured nine-body/nineteen-contact physical root while preserving
material, all original constraints, rollback, work limits and prior regressions.
Close only this captured physical component; full owner continuation is 0049.

## Evidence

Independent signed rigid-body mobility and pressure continuation find an
admissible all-loaded root: contact-3 pressure 0.44282582792513747, slip
0.08963866964215116 and equation residual below 6e-12. The first higher-pressure
trial wrongly releases rows 5 and 16 from a large unconstrained step; further
releases obstruct its complete root. Those contacts have positive loads in the
independent solution. The corrected private trial freezes the projected warm
normal branches before adding pressure, keeps the selected world row loaded,
and refines friction before the same complete physical acceptance. Existing
small-load trials and the 324-model/4096-pass/16-step budgets remain unchanged.

Final Native solve uses 513 velocity and 64 position passes. Motion and pressures
agree with the independent root within 5.07e-11. The typed Native/WASM projections,
full twenty-body fields and remapped nine-body six-ordering cases pass the
independent normal/friction/cone/energy/reaction/pose/clearance checks. All 742
physical checks per target and fresh O1 ASan/UBSan on 34 reached units pass.
Previous source rejects the new first fixture on Native/WASM. Exact twenty-body
prediction/collision passes on all three targets with zero unresolved sweeps.
Strict LLVM19 checks cover 169 files; 38 Native/36 WASM CTests pass. Valgrind reports
zero errors/live heap bytes for the exact nineteen-contact root.
The unchanged full eleven-level AI suite passes on all three targets: 29214
independent original path queries and 2640000 vehicle steps per target, with
unchanged reset, decision, support/travel and deadline contracts. Source hashes
remain unchanged throughout. A separate ground comparison
fails identically at prior 570df9d and current source; its unchanged 120 Arena-A
states and original angular tolerance remain recorded under 0052.
Canonical receipt:
/tmp/wasm-dd2/rewrite-next-round2-rejection-0049/component-report.json.

## Next

Continue complete actual-owner outcomes under 0049/0050 and ground stability
under 0052; neither diagnostics nor component success proves a complete game.

## Accept

The independent root, both exact target projections, full/remapped physical
constraints, all prior checks, fresh sanitizer, strict/platform, previous-source
negatives and unchanged eleven-level AI contracts pass. No tolerances, physical
budgets, source scenarios or deadlines are relaxed.
