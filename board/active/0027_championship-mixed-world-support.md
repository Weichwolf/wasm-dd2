Type: Work item
Title: Stabilize mixed world supports in scheduled racing
Depends: 0026

## Contract

Advance the reproduced ordinary Stockcar championship step while preserving
finite motion, unilateral contact, Coulomb friction, impulse accounting and
transactional rejection. Keep existing solver tolerances, iteration bounds,
regular lap requirements, damage, controls and championship score ownership.
Verify the reached contact behavior on Native/WASM and instrumented C before
using it in further natural scheduled races.

## Evidence

After the correction-selection improvement, the unchanged-bound Native natural
season completes its first ten-lap round at tick 287,081 and reaches second-round
tick 66,449. Advancement then fails with valid controls, AI, race and championship
state, player engine health 0.5166984465536384 and zero credited laps.

One 31,216-byte typed rewrite input checkpoint reproduces the failure in one
actual step. The fleet velocity solve exhausts 4,096 passes with residual
0.000001874264492151687; position correction is not reached. All four contacts
belong to driver 8 against the world: two sloped supports, a sliding wall and a
released sloped support. The final two supported contact velocities are below
0.000003 world units/s, while the wall retains approximately 4.45 units/s slip.
The diagnostic projected Jacobian is nearly singular; forcing all loaded
supports to stick does not provide an accepted correction.

Neither changing the forward-difference scale in isolated diagnostic copies nor
forcing any individual normal release resolves this query. These experiments
change no production code or acceptance tolerances. Source/checkpoint identity,
one-step reproduction and failed experiment scope are recorded in
/tmp/wasm-dd2/rewrite-stock-season-next-failure/report.json. The checkpoint is
retained because its diagnosis remains open. This is not a completed season or
original physics comparison.

A separate temporary material experiment permits at most 0.0001 world units/s
of low-speed world-contact creep, retaining saturated Coulomb friction. Its frozen
query converges in 513 passes and its saved actual step advances to tick 66,450.
Production remains unchanged. Ten independent analytic world-friction and ten
pair-friction cases pass, but the third prior frozen query (arena-B pressure-
sensitive supports) fails. The candidate therefore cannot be adopted. Source/
binary/log identities and this rejected broader acceptance are recorded in the
same diagnostic report. One successful step cannot prove the constitutive model
or scheduled racing.

## Next

Diagnose the earlier pressure-sensitive query before considering bounded
low-speed regularization as deliberate material tuning. Keep independent
analytic creep/friction, impulse and energy checks.
Preserve unilateral response, friction cones, saturated Coulomb behavior and
fixed budgets. Produce an independently checked failing contact fixture before
adopting a general correction; do not substitute a diagnostic step for acceptance.
Continue natural scheduled races after the saved step succeeds; complete seasons
and their presentation remain in 0025/0004.

## Accept

Independent final motion/contact/impulse checks pass for this case and prior
captured cases on all three targets. The saved input advances with production
code, and ordinary scheduled racing proceeds beyond this failure with no state,
lap, damage or result injection. Required LLVM19, Native/WASM and relevant actual
application checks pass. Close only this contact contract.
