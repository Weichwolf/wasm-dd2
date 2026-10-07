Type: Work item
Title: Resolve coupled supports in the second scheduled circuit
Depends: 0027

## Contract

Advance the reproduced Stockcar step at second-round tick 86,643 with the shared
bounded low-speed friction model. Preserve finite state, normal complementarity,
friction cones, dissipative response, impulse accounting, rollback, convergence
tolerances and the shared 4,096-pass budget. Keep regular controls, AI, damage,
laps, championship scoring and race bounds.

## Evidence

The earlier Native natural season stopped at second-round tick 86,643 in drivers
11/13's seven-contact group: six world supports and one car pair. A 31,216-byte
input checkpoint reproduced the 4,096-pass velocity failure with residual
0.0034676348145833424; position repair was not reached. Near-dependent support
rows and pressure-sensitive projected derivatives motivated the alternate root.

Mixed car/world groups now compare the projected correction with an analytic
constitutive friction correction. The derivative includes current pressure and
the existing linear/saturated transition. Unit-impulse body responses provide
contact mobility, cached once per column. World-only groups retain their prior
projected/zero-gradient treatment. Material laws, singular-direction rejection,
active-set releases, cone projection, rollback and all budgets/tolerances remain.
The initial broad application to world-only groups failed a WASM ground unit
check and was discarded; it is not the accepted runtime.

All eighteen immutable captured queries and fourteen rotations/reversals of the
new seven-contact query pass independent normal/friction, impulse, energy,
clearance, orientation and clock checks on Native, WASM and ASan/UBSan. Ten
analytic world and ten analytic pair cases per target pass. The new captured
case uses 833 Native, 1,185 WASM and 961 sanitized passes, within the unchanged
4,096 bound, with one restart. The released-normal regression retains its
required restart. No contact is dropped to obtain these results.

LLVM19 format/tidy covers all 157 owned C/header files; 34 Native and 32 WASM
CTests pass. All eleven actual Native, sanitized SDL and Chromium/WASM
scene/car/driving/race/trial/arena/championship input/presentation flows pass.
Pinned SoftGL is unchanged. The production saved input advances from 86,643 to
86,644. Fresh ordinary scheduled racing completes the first ten-lap round at
287,087 ticks with actual twenty-driver scores, then proceeds past the old failure
to second-round tick 107,550. AI, frame and championship state remain valid at
that next failure; player health is 0.40180162706486544 and no complete lap is
credited. No state, health, lap, score or result injection supplies progression.
This proves the reproduced contact contract, not a complete season or original
physics parity. Source/binary/input identities and scopes:
/tmp/wasm-dd2/rewrite-coupled-86643-final/report.json.

## Next

Diagnose the next natural advancement failure under 0029; its sparse capture is
running. Supplemental original-data ground and selected race regressions are
running against this unchanged source. Complete campaigns and front-end/save
flows remain under 0025/0004/0002/0003/0008.

## Accept

The reproduced query and all prior physical fixtures pass on all three targets.
The saved actual input advances and ordinary racing continues past this failure
without state, lap, health or result injection. Mandatory LLVM19/Native/WASM and
relevant actual application checks pass. Close only this coupled-support contract.
