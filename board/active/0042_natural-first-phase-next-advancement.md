Type: Work item
Title: Next ordinary championship advancement after first-phase refinement
Depends: 0040, 0041

## Contract

Reproduce and diagnose the actual round-2 tick-194,353 advancement failure from
the frozen first-phase ordinary campaign. Correct the proved defect while
preserving all physical/material checks, typed ownership, exact rollback and
finite solver/fixture bounds. Verify saved-owner advancement on current Native
and instrumented production, independent WASM checks and ordinary continuation.
Complete seasons and complete-game acceptance remain separate requirements.

## Evidence

The first-phase campaign exits 1 without timeout after 678.16 seconds. All 98
production records are retained with frozen source/build identity. It reaches
round-2 checkpoint 190,000, then fails valid advancement at 194,353. AI/frame/
championship validation remains true; health is 0.23930508697523567, laps are
zero and controls are throttle -0.6, brake 0 and steering 1. This is a different
trajectory from the d160c06 tick-223,337 input; no common cause is assumed.

A failure-only capture used the exact frozen first-phase code and libraries,
with the same ordinary controls and unchanged bounds. Its diagnostic executable
was fully linked and its source identity verified before the 0041 source changes.
Only one failed owner/query was captured, without full-frame memory snapshots.
Receipt: /tmp/wasm-dd2/rewrite-first-phase-0040/natural-season/terminal-report.json.
Capture: /tmp/wasm-dd2/rewrite-first-phase-0040/failure-capture/.

The capture is now terminal: exit 1 without timeout after 649.00 seconds,
matching all 98 production records exactly. One 31,216-byte owner and a
three-world-contact group on slot 14 are retained. Frozen first-phase and
current fixed-active production both reject its owner and full/remapped query.
Read-only current instruction-equivalent observation records 4,096 velocity
passes, one restart, 48 corrections and error 0.00014995866141797633; position
repair is not reached.
Receipts: /tmp/wasm-dd2/rewrite-first-phase-0040/next-failure-diagnosis/
production-report.json and terminal.json.

Independent geometry/inertia reconstructs terminal contact speeds within
1.23e-13. The initial all-eight-active-set/four-seed search finds no admissible
root; that limited numerical search does not prove nonexistence. Independent
wall-pressure continuation subsequently finds an admissible all-loaded root
with pressure 0.6727497996746545 and physical error 4.53e-15. The failed wall
pressure is only 0.002110427572973521; doubling it misses this basin. Independent
central differences agree within 1.69e-10, and a Jacobian-free solve from a
perturbed seed reaches physical error 7.11e-15. No uniqueness claim is made.
Receipts: independent-root/report.json, pressure-continuation-report.json and
finite-difference-report.json in the same diagnosis directory.

A private Native world-pressure candidate uses the larger of current pressure
and mean positive incident pressure as its starting scale. Existing small-slip
LOAD seeds retain their current-pressure scale. All materials, 260 models,
matrix/pass/refinement bounds and complete-root acceptance remain unchanged.
It solves the full/remapped query in 513 velocity/45 position passes, advances
the actual owner to 194,354 and passes all 702 existing Native physical checks.
Its motion agrees with the independent root within 3.58e-11. This is a private
prototype; no production, strict or multi-platform correction is claimed.
Receipts: /tmp/wasm-dd2/rewrite-next-world-load-0042/candidate-report.json,
candidate-owner-report.json and prototype-independent-motion-report.json.

The correction and both actual 194,353/142,893 queries are integrated with all
six permutations each: 38 captured cases plus 678 orderings (716 independent
checks). The private incident-pressure candidate also advances the newly
captured 142,893 owner; its independent diagnosis remains under 0043.

The initial strict build rejects adjacent convertible index/method parameters
in the pressure-scale helper. The implementation now passes the existing typed
contact model, keeping selection and method together. No check is disabled.
The initial Native gate failed. The frozen final source now passes strict
LLVM19 checks, all 35 Native and 33 WASM CTests, and all 716 independent physical
checks on Native, WASM and ASan/UBSan. The frozen previous solver rejects the
new first fixture, confirming regression sensitivity.
Failed-scope receipts: /tmp/wasm-dd2/rewrite-next-world-load-0042/before-model-argument/.

Final Native full/remapped queries solve at 513 velocity passes: 45 position
passes for 194,353 and 52 for 142,893. All seven retained failed Native owners
advance, including both current inputs; reached rewrite C under ASan/UBSan also
advances both current owners. SoftGL/system SDL are uninstrumented dependencies.
Independent root motion agrees within 3.58e-11 and 1.34e-10 respectively. Saved
Native layouts are not used as WASM owner layouts. The pinned SoftGL, 260 models,
finite bounds, physical acceptance and rollback remain unchanged. Broader gates
and fresh ordinary continuation are still pending; no complete season is proved.
Receipts: /tmp/wasm-dd2/rewrite-next-world-load-0042/component-report.json,
physical-report.json, replay-report.json, san-next-owner-report.json,
independent-motion-report.json and independent-next-motion-report.json.

## Next

Run broader original-data/input/race scopes and fresh ordinary continuation
from the verified incident-pressure source. Keep the two
older trajectories distinct; retain AI movement and zero-lap playability
acceptance under 0016/0004.

## Accept

The exact captured state advances on current production; independent physical,
material, motion/energy/position and rollback checks pass on Native/WASM/
instrumented C. Strict and reached original-data/input/race gates pass. Ordinary
continuation crosses the failed step and dense region without manufactured
state, scores or enlarged bounds; no partial scope proves a complete campaign.
