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

A failure-only capture is now running from the exact frozen first-phase code
and libraries, with the same ordinary controls and unchanged bounds. Its
diagnostic executable was fully linked and its source identity verified before
the new 0041 source changes. Capture only one failed owner/query; no full-frame
memory snapshots are requested. Reproduction and cause remain unproved.
Receipt: /tmp/wasm-dd2/rewrite-first-phase-0040/natural-season/terminal-report.json.
Live capture: /tmp/wasm-dd2/rewrite-first-phase-0040/failure-capture/.

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
candidate-owner-report.json and independent-motion-report.json.

## Next

Integrate the proved seed scale with the actual three-contact fixture and all
six permutations; run strict Native/WASM/instrumented gates and prior owner
regressions, broader checks and fresh ordinary continuation. Keep the new
fixed-active trajectory's 142,893 failure separate under 0043. Retain AI movement
and zero-lap playability acceptance under 0016/0004.

## Accept

The exact captured state advances on current production; independent physical,
material, motion/energy/position and rollback checks pass on Native/WASM/
instrumented C. Strict and reached original-data/input/race gates pass. Ordinary
continuation crosses the failed step and dense region without manufactured
state, scores or enlarged bounds; no partial scope proves a complete campaign.
