# 0049: Diagnose the next ordinary WASM round-2 rejection

## Contract

Explain and correct the actual public-owner failure at level-2 round-2 tick
155992 after d0a1bd7's correction, preserving physics, budgets, damage/lap/score
ownership and all prior regressions. Keep subsequent AI source epochs separate.

## Evidence

The ordinary WASM season at frozen d0a1bd7 naturally finishes round 1 at tick
287081 and advances beyond the previous tick-137455 failure. It exits 1 without
timeout at round-2 tick 155992 after 710.75 seconds. AI/input/championship state
remain valid; health is 0.2268437624545876 and player laps are zero. Actual
requested input equals the copied next controller input exactly. One typed
checkpoint preserves all twenty cores/controllers/tire outputs. Portable
prediction with the exact saved controls succeeds on both Native/WASM,
while coupled collision rejects on both. A read-only linker observer captures
one failed twenty-body query with nineteen contacts on each target.
The nineteen contacts join nine involved bodies (ten world rows and nine pair rows). A private typed
replay prepares successfully but its velocity stage exhausts 4096 passes at
residual 0.0007802487758, after one restart and 191 accepted accelerations;
position repair is never reached.
Guardian source_unchanged is true at termination. Completed raw logs/binaries
are removed after the canonical receipt; necessary typed inputs remain.
Receipt: /tmp/wasm-dd2/rewrite-season-linear-pressure-0047/canonical-report.json.

Diagnosis receipt: /tmp/wasm-dd2/rewrite-next-round2-rejection-0049/component-diagnosis.json.
The two typed failed groups are retained beside it; no complete memory dump is taken.

An independent signed rigid-body mobility matrix is symmetric within floating
roundoff. Fifteen bounded uniform/contact-3 pressure starts find no admissible
root; the smallest constitutive/complementarity equation residual is
0.0005647023494. Contact 3 is a world support on slot 13. Failed searches do not
prove that no physical root exists; retained invalid iterates support the next
active-branch diagnosis. Receipt: independent-root-report.json beside the diagnosis.

## Next

Independent continuation now proves a valid all-loaded root; the correction
and component acceptance are tracked under 0051. Continue actual full-owner
outcomes and retain the exact saved controls/contacts. Preserve every saved
actual control and captured contact regression. Preserve 0048's AI
verification and distinguish old-source rejection from corrected-AI campaigns.

## Accept

A reproducible causal defect and physically valid correction pass Native/WASM/
instrumented C and all reached strict/functional regressions, followed by actual
ordinary owner advancement beyond this failure. No partial result proves a
complete natural campaign or game.

## Verified root and separated ordinary epochs

Independent pressure continuation recovers pressure 0.44282582792513747 and
0.08963866964215116 slip on world row 3. A read-only trace shows the pressure
trial prematurely releases still-needed rows 5/16. Preserving warm normal
branches recovers the captured query and exact portable field on all three
targets; production gates and the broader component proof are under 0051.
The separate immutable 570df9d baselines are all terminal: Native is blocked at
round-1 bound/credited lap9, WASM/sanitized at round-3 bound/credited lap7. They
naturally finish/retire earlier rounds but do not complete a season or exercise
the newer pressure correction. Those actual movement contracts are under 0050.
