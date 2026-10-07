Type: Work item
Title: Complete AI tactics and off-road recovery
Depends: 0002

## Contract

Provide reliable road/arena driving, damage-aware tactics, supported overturn and off-road recovery for every track and race mode.

## Evidence

Source path guidance, traffic avoidance, pursuit, timed reverse, accepted-motion stall detection and supported overturn righting exist. Damage-aware tactics and automatic off-road recovery remain incomplete. Long AI probes disable damage and therefore do not establish complete race AI.

A sparse, damage-enabled Native B replay of a326993 reaches the unchanged
120000-tick bound without solver aborts or unresolved collision steps. The
supported player remains alive in a slow contact-bound field; all nineteen
opponents retain human target zero. The player's front damage remains
0.588834/0.947100 at the terminal checkpoint. Final fleet snapshots include
actual post-damage controls, motion, wheel support, retirement, recovery and
reverse/progress counters. Report:
/tmp/wasm-dd2/rewrite-branch-arenaB-native-diagnosis/diagnosis-report.json.
Production trajectory matching is pending the live full-suite B Native target.
These observations do not establish a specific tactical fix or full AI acceptance.

## Next

Match the sparse B replay against the production trajectory, then diagnose
contact-bound movement and reverse maneuvers while preserving the Total
Destruction player target. Use ordinary complete races to identify off-road and
tactical failures; recover original behavior and implement explicit recovery/tactics
without manufactured score, damage or route progress.

## Accept

Every track/mode sustains valid AI decisions and physically reaches required race outcomes, including damaged/off-road/overturned states, reset and paused clocks on both targets.
