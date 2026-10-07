Type: Work item
Title: Complete championships and season progression
Depends: 0015, 0002

## Contract

Implement actual championship races, standings, original schedules/divisions, season promotion/relegation, unlocks, history, names and terminal outcomes, including multiplayer championship behavior where supported by the original.

## Evidence

Practice results already use actual simulated twenty-driver scores. Published
single-player league rules match 6241 targeted original-x86 cases on Native,
WASM and ASan/UBSan (closed 0015). There is no championship owner, consuming
physical-grid integration, season UI or persistence yet.

A private driving constructor now copies a twenty-slot permutation while
keeping stable driver IDs and human zero. Invalid permutations fail before
allocation; reset and mode changes preserve assigned nominal starts. Prepared
checks cover all eleven original levels in all four fixture league divisions:
880 assigned starts per target, settled valid fields, real active Stockcar/Total
Destruction steps, withdrawal, caller-map ownership and all seven circuits'
twenty-car/one-car Time Trial transitions. Native, WASM and fully instrumented
LLVM19 ASan/UBSan pass. Promotion points initialize fixtures; they are not actual
championship results. Reports:
/tmp/wasm-dd2/rewrite-championship-grid-prototype/original-grid-report.json and
source-quality-report.json.

An isolated checkout of the prepared constructor, focused driving tests and
original-data export also passes 32 Native/31 WASM CTests and strict LLVM19 for
146 owned C/header files. All outputs remain under /tmp/wasm-dd2/; production
sources and race binaries remain unchanged during the live race suite. Report:
/tmp/wasm-dd2/rewrite-grid-preparation-verification/report.json.
This prepares physical-grid integration; the championship owner, real season
results, menus and persistence still need their consuming implementation.

## Next

Integrate the prepared stable-ID constructor after the current source-frozen
race run is terminal, retaining its source/build evidence. Implement the
championship owner and exactly-once
result consumption using game/league.h. Add original schedules, history/unlocks
and consuming front-end/save flows.

## Accept

Full seasons use real race results and progress correctly on both platforms; terminal and continuing outcomes, score ties, duplicate-result rejection, original save compatibility and complete menu flows are covered.
