Type: Work item
Title: Complete championships and season progression
Depends: 0015, 0002

## Contract

Implement actual championship races, standings, original schedules/divisions, season promotion/relegation, unlocks, history, names and terminal outcomes, including multiplayer championship behavior where supported by the original.

## Evidence

Practice results already use actual simulated twenty-driver scores. Published
single-player league rules match 6241 targeted original-x86 cases on Native,
WASM and ASan/UBSan (closed 0015). Driving now copies a twenty-slot permutation while
keeping stable driver IDs and human zero. Invalid permutations fail before
allocation; reset and mode changes preserve assigned nominal starts. Production
checks cover all eleven original levels in all four fixture league divisions:
880 assigned starts per target, settled valid fields, real active Stockcar/Total
Destruction steps, withdrawal, caller-map ownership and all seven circuits'
twenty-car/one-car Time Trial transitions. Native, WASM and fully instrumented
LLVM19 ASan/UBSan pass. Promotion points initialize fixtures; they are not actual
championship results. Use `make rewrite-grid-verify`. Production reports:
/tmp/wasm-dd2/rewrite-grid-integration/original-data/report.json and report.json
in its parent directory. All 32 Native/31 WASM CTests and strict LLVM19 for
146 owned C/header files and actual Native/browser presentation checks pass.
Grid ownership is verified separately under closed 0023.
The typed single-player championship owner now connects actual scheduled races
to these standings and assigned fields. Closed-round copies, original schedules,
explicit result/season continuation, unlocks, terminal outcomes and the five-record
history window are implemented. Strict LLVM19 for 152 files, all 33 Native/32 WASM
CTests, synthetic progression rules on all three targets and natural first ten-lap
circuit results in both modes on Native/WASM/ASan/UBSan pass. Native/WASM next-track
failure rollback passes; its sanitized run and final identity checks remain pending
in active 0024. Reports are under
`/tmp/wasm-dd2/rewrite-championship-integration/`. These scoped first-round and
synthetic-season checks do not establish full physically driven championships.
Complete campaigns, menus, names, multiplayer and persistence still need their
consuming implementation.

## Next

Finish the remaining 0024 verification, then connect the owning session to real
Native/browser championship entry, scores, results and continuation. Prepare
renderer resources transactionally before replacing borrowed track data. Add
names, profile-wide unlock retention, complete physical campaigns, multiplayer
and compatible save/load.

## Accept

Full seasons use real race results and progress correctly on both platforms; terminal and continuing outcomes, score ties, duplicate-result rejection, original save compatibility and complete menu flows are covered.
