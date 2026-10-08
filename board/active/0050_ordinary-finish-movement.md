# 0050: Restore ordinary movement through late-race traffic

## Contract

Diagnose and correct the actual 570df9d ordinary finish approaches without
manufactured laps, places, health or scores, higher tick/time limits, or altered
physics acceptance. Keep player/finished-NPC controls and result ownership clear.

## Evidence

The frozen heading-safe 570df9d baselines are terminal without timeout or contact
rejection. Native reaches the round-1 300000 bound at credited lap 9; WASM/O3
ASan/UBSan naturally finish/retire rounds 1/2 but reach the round-3 bound at
credited lap 7. Wall times are 105.47/430.74/1955.86 seconds. Native player health
is 1; WASM/sanitized round-3 health remains positive. No season is complete.
The guardian records source_unchanged=false because later C changes began after
all baseline sources were verified against committed Git blobs and their target
executables were frozen. Keep these results at 570df9d, not current production.
Receipt: /tmp/wasm-dd2/rewrite-season-passing-return-0016/canonical-report.json.

The Native terminal twenty-body/pilot field exactly reproduces all copied next
controller requests. Independent original road data matches its source-offset
30792 target exactly. The controller chooses lane 0.32 and desired speed 43.486
behind stationary traffic at distance 1186.971. Nine NPCs already have credited
lap 10; the rewrite suppresses their AI and applies full brake. All fifteen
sampled lanes are considered obstructed by the current point-corridor predicate.
These are geometry/control observations, not complete owner replays.
Receipts: /tmp/wasm-dd2/rewrite-finish-movement-0050/native/.

The frozen reference game loop calls every opponent's AI server. Its finish flag
is latched in Get_Race_Positions; AI_Com_Server does not read that flag. Its
excluded handling state 5 belongs to rolling/recovery. This is reconstruction
source evidence, not an executed original comparison. Receipt:
/tmp/wasm-dd2/rewrite-finish-movement-0050/reference-finisher-report.json.

## Next

Prove the intended finished-NPC movement and frozen lap/place/score behavior in
the actual typed driving/race owner. Check original/reference behavior as needed.
Then rerun ordinary Native/WASM/instrumented campaigns at the same limits.

## Accept

The failing finish approaches reproduce, a causal correction passes focused
previous-source negatives and physical/rule/rollback/platform checks, and actual
ordinary owners publish natural correct results on both platforms. Finish places
and points remain exactly once; full campaigns/game remain open until proved.
