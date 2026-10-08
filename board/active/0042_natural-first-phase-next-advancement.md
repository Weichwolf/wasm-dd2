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

## Next

Follow the same live capture to its terminal result; require exact matching of
all 98 production records. Replay its actual owner/query against current and
frozen production before attributing a cause. Retain AI movement and zero-lap
playability acceptance under 0016/0004.

## Accept

The exact captured state advances on current production; independent physical,
material, motion/energy/position and rollback checks pass on Native/WASM/
instrumented C. Strict and reached original-data/input/race gates pass. Ordinary
continuation crosses the failed step and dense region without manufactured
state, scores or enlarged bounds; no partial scope proves a complete campaign.
