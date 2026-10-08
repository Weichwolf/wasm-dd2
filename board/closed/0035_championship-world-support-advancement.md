Type: Work item
Title: Advance the championship world-support failure after reverse retry
Depends: 0016

## Contract

Resolve the actual round-2 step-83,450 world-contact failure exposed after the
corrected forward/reverse timer. Preserve all physical inequalities, material
acceptance and ordinary driving/damage/lap/score ownership. Verify the saved
owner step, three-target query behavior and subsequent natural continuation.

## Evidence

The corrected controller's natural first Stockcar round retains 287,087 ticks
and its original twenty-driver scores. Round 2 fails at tick 83,450 with valid
AI/frame/championship inputs. Failure-only diagnostics match all 76 production
records and capture one 31,216-byte owner input plus the contact query.

Three static-world contacts involve physical body 14: two distinct upward
road normals and a horizontal wall normal. Velocity exhausts 4,096 passes at
residual 0.000335284234228977; position is not reached. Both the saved actual
owner and the isolated full-field / one-body-remapped query reproduce failure
against unchanged production libraries. A production correction has not yet
been proved.

Receipt: /tmp/wasm-dd2/rewrite-reverse-expiry/failure-capture/report.json.
Replay/query receipts: production-query/owner-replay-report.json and
production-query/report.json within the same capture directory.

A read-only GDB observer uses instruction bytes identical to the production
contact object and reproduces its exact 4,096-pass terminal residual. An
independent geometry/inertia mobility reconstructs terminal contact velocity
within 1.19e-14 and enumerates every normal active set. It finds an admissible
regularized root with the wall released, both road supports loaded, wall
separation speed 0.0899708 and full physical residual below 3.6e-15. The two
road slip speeds straddle the unchanged 0.1 transition (0.173528/0.095204).
This proves a feasible branch, not a production fix or original parity.
Receipts: debug-query/identity.json, debug-query/terminal.json and
independent-root/report.json within the same capture directory.

Read-only observation of all 108 selected wall-release attempts shows that
their fitted seeds and refinement endpoints remain above the outer residual;
the smallest observed endpoint error is 0.207525. A private Native candidate
keeps fitted friction unclipped throughout that bounded constitutive refinement.
It solves the full-field and one-body query in 673 velocity / 50 position passes,
matches the independently admissible impulses within 7.98e-12 and has independently
checked physical residual 5.35e-12 with no cone violation. The actual saved owner
then advances from 83,450 to 83,451. Exact zero for the selected release alone,
or omitting clipping only in the first seed, does not solve the input.
These are private diagnostic results; production sources remain unchanged.
Receipt: private-release-constitutive/physical-report.json, with query and actual
owner receipts in that directory. Faithful branch observations and independent
acceptance analysis remain in debug-query/ within the same capture directory.

The production correction retains fitted tangent directions only inside the
bounded selected-release refinement. It restores all friction cones before
outer acceptance, recomputes the unchanged full physical error, and requires
finite reduction or exact rollback. Material rules, model/pass/refinement/search
bounds, normal inequalities and all game-state ownership remain unchanged.

All mandatory LLVM19/Native/WASM gates pass (35 Native / 33 WASM CTests).
Thirty captured queries and 416 orderings retain independent physical checks
on Native, WASM and ASan/UBSan, including all six new permutations. The actual
saved owner advances to 83,451 on Native and instrumented C. Production full-field
and remapped responses match the independent admissible root within 7.53e-12.
Eleven-level actual Native/sanitized/Chromium input and original-data ground
checks pass; six scoped race targets include the complete eight-lap Circuit-5
race. The unchanged-bound natural campaign retains round-1 tick 287,087 and all
twenty scores, then crosses the old failure through round-2 tick 160,000.
That same natural process remains running; complete physical seasons and the
remaining Arena-B AI motion/tactics are not proved by this contract.
Receipt: /tmp/wasm-dd2/rewrite-fitted-83450/report.json.

## Next

Follow the same natural campaign to its actual terminal outcome under 0025/0004.
Keep ordinary controls, damage, score ownership and fixture bounds. Continue
complete AI tactics and the recorded Arena-B movement failures under 0016.

## Accept

The actual saved owner advances, the captured query and independent physical
checks pass on Native/WASM/ASan, and the same ordinary natural campaign continues
beyond this failure with normal controls and no state or score injection.
