Type: Work item
Title: Stabilize mixed world supports in scheduled racing
Depends: 0026

## Contract

Advance the reproduced Stockcar championship step with finite motion, unilateral
contacts, friction cones, dissipative response and exact impulse accounting.
Use a documented bounded low-speed material transition, retaining Coulomb
saturation, existing convergence tolerances, iteration bounds, regular laps,
damage, controls and score ownership. Verify the reached contact behavior on
Native/WASM and instrumented C before further natural scheduled races.

## Evidence

The earlier Native season failed at second-round tick 66,449 in driver 8's
four-world-support group. One 31,216-byte input checkpoint reproduced the
4,096-pass velocity failure with residual 0.000001874264492151687. The initial
0.0001-unit/s world-creep prototype fixed that step but failed the prior
pressure-sensitive fixture; it was not adopted.

World supports now use the same 0.1-unit/s low-speed friction transition already
used by car pairs. Above it, Coulomb saturation is unchanged. Below it, the
opposing impulse varies linearly with final slip. This bounds creep to 1/9,000
of the standard 900-unit body length per second in the low-speed regime. The
alternate Newton branch includes the same pressure-dependent friction gradient.
This is deliberate rewrite material tuning, not original physics parity.

All seventeen frozen contact queries pass independent complementarity, cone,
constitutive, impulse, energy, clearance and clock checks on Native, WASM and
ASan/UBSan. Ten analytic world and ten analytic pair cases per target check the
linear/saturated transition and exact equal-mass reaction. The new query converges
in 35 passes on all three targets; the prior eighteen-contact pressure query
converges in 115. The released-normal fixture still requires the single bounded
restart. The former world/car cycle now converges without one; its physical
conditions and the global restart/pass bounds remain checked.

LLVM19 format/tidy for 157 files and all 34 Native/32 WASM CTests pass. Actual
Native, sanitized window and Chromium/WASM presentation/input pass on all eleven
levels. Original-data ground checks cover 79,100 queries and 26,400 physical steps
per target; recovery checks cover all 220 original starts per target. Two selected
race scenarios pass six target scopes, including a natural eight-lap twenty-car
Circuit-5 Stockcar race. This is scoped coverage, not full-game acceptance.

The production saved step advances from 66,449 to 66,450. The unchanged-bound
Native season completes its first ten-lap round at tick 287,087, awards actual
scores, then reaches a different two-body/seven-contact failure at second-round
tick 86,643. That sparse input and diagnosis remain under 0028. No lap, position,
health, result or score injection supplies progression.

Current source/binary identities, commands, physical checks and exact scopes:
/tmp/wasm-dd2/rewrite-world-friction-transition/report.json. Successful raw output
and the completed tick-66,449 checkpoint are removed after their receipts.

## Next

Investigate the independently reproduced two-body support group under 0028.
Complete physical seasons, presentation and remaining arena behavior stay under
0025/0004/0002. This mixed-world material contract is proved.

## Accept

Independent final motion/contact/impulse checks pass for the new case and prior
captured cases on all three targets. The saved input advances with production
code, and ordinary scheduled racing proceeds beyond this failure without state,
lap, damage or result injection. Required LLVM19, Native/WASM and actual application
checks pass. Close only this contact contract.
