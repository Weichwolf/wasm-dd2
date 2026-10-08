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
All 24 player checkpoints and the last two complete fleet position/step/damage,
AI-target and recovery snapshots match the terminal production Native B trace
exactly; see production-checkpoint-match.json in the same directory.
These observations do not establish a specific tactical fix or full AI acceptance.

The current two-endpoint contact runtime (69d97d7) completes the first natural
Stockcar championship round with unchanged 287,087 ticks and twenty-driver scores.
Both production and logging-only attempts then reach the unchanged 300,000-tick
round-2 bound, matching all 120 JSON records. Every requested physical step
advances; there is no logged contact failure or failure checkpoint. The player
has zero credited laps and health 0.4003601829670652; race/championship stay RACING
with no scores manufactured. Both processes exit 1 without timeout. The same
production child was adopted after launcher termination, without engine restart.
This proves current natural noncompletion, not its tactical/kinematic cause.
Current terminal receipt: /tmp/wasm-dd2/rewrite-endpoints-233358/report.json.
Closed 0033 proves the separate contact saved-input/continuation correction.

## Next

Capture sparse public owner/controller observations of the current Stockcar
noncompletion, match its unchanged production trace and diagnose supported motion,
orientation, damage, guidance and reverse/progress decisions before choosing a fix.
Keep every engine, damage, lap and score rule and the ordinary fixture bound.
Diagnose the reproduced contact-bound B movement and reverse maneuvers while preserving the Total
Destruction player target. Use ordinary complete races to identify off-road and
tactical failures; recover original behavior and implement explicit recovery/tactics
without manufactured score, damage or route progress.

## Accept

Every track/mode sustains valid AI decisions and physically reaches required race outcomes, including damaged/off-road/overturned states, reset and paused clocks on both targets.
