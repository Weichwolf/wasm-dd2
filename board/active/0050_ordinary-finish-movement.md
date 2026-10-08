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

Finished opponents now continue ordinary AI decisions and physical movement;
only the player retains finish/coasting braking. Completion remains owned by
lap/race rules, so continuing damage/accidents do not invent another finish.
The focused original-circuit-5 Stockcar race naturally completes all eight laps
on Native, WASM and fresh O1 ASan/UBSan. Each target checks 1570 post-finish
opponent steps, with 21688.791 units of accepted horizontal travel and unchanged
finish place, finish tick and lap records. Restoring only prior-source driving.c
makes this check fail immediately after the first natural opponent finish.
Independent original-data geometry and placement tables validate the final field
and scores. The nine-lap Time Trial and all 29 short physical/14 ordered-rule
scenarios pass on all three targets: 45 scenarios per target, excluding the four
still-open long Total Destruction completions. Strict LLVM19 checks cover all
169 C/header files; 38 Native and 36 WASM CTests pass.
Receipts: /tmp/wasm-dd2/rewrite-finish-movement-0050/{races,short-races,arena-races}/report.json
and /tmp/wasm-dd2/rewrite-finish-movement-0050/negative/report.json.

Fresh unchanged-bound public-owner campaigns at published d7dfdd4 naturally
complete all four first-season rounds on Native and WASM and open season 2.
Each target has three player finishes and one physical engine retirement.
Native round ticks: 290349/112745/166518/276588, 406.58 wall seconds.
WASM: 287118/118018/170193/233338, 445.36 wall seconds. All original-schedule
rounds consume actual twenty-driver results exactly once, retain the original
points table and clear totals for season 2. Native thereby clears its original
round-1 bound; WASM clears its original round-3 bound. Accepted vehicle-step
counts are 16892000/16141340. No renderer, menu, saved-game or original-parity
claim follows. The fresh 34-unit O3 ASan/UBSan season remains running; preserve
its 300000-tick round bounds and 3600-second deadline and keep this item active
until its terminal outcome is recorded.
Receipt: /tmp/wasm-dd2/rewrite-season-finisher-movement-0050/native-wasm-report.json.
Guardian/source/binary identity: {launch,identity}.json in the same directory.
The build identity records HEAD before the improvement was committed; the
publication receipt verifies every compiled source hash against d7dfdd4 Git
blobs, so these are correction evidence rather than a mislabeled old baseline.

## Next

Collect terminal fresh ordinary Native/WASM/instrumented campaigns at the
unchanged limits and compare the originally blocked round-1/round-3 finish
approaches. Diagnose any remaining physical or movement failures separately.
Keep this item active until its actual-owner acceptance is proved.

## Accept

The failing finish approaches reproduce, a causal correction passes focused
previous-source negatives and physical/rule/rollback/platform checks, and actual
ordinary owners publish natural correct results on both platforms. Finish places
and points remain exactly once; full campaigns/game remain open until proved.
