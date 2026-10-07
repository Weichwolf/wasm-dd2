Type: Work item
Title: Own championship rounds and consume actual results once
Depends: 0015, 0023

## Contract

Own single-player scheduled tracks and physical fields; consume their naturally
completed results once into typed standings. Preserve completed results through
interstitial continuation, season transfer, unlocks, a five-season history window
including the current season, terminal outcomes and unscored cancellation.
Failed next-track preparation must preserve current results and owned views.

## Evidence

The typed progression and consuming session are implemented. Strict LLVM19 for
152 owned C/header files and all 33 Native/32 WASM CTests pass. Synthetic rule
fixtures pass on Native, WASM and fully instrumented ASan/UBSan: both modes,
all four divisions, promotion/relegation/staying/champion/elimination, capped
points, copied results, stale/duplicate/malformed results, unscored cancellation,
history rollover and counter overflow. The schedule and placement points are
independently decoded from the supported original executable.

Natural first scheduled ten-lap circuit sessions pass in both modes on Native,
WASM and fully instrumented ASan/UBSan. Actual results update all twenty drivers
once; result-screen frames preserve scores/clocks, clear sound batches and validate
input. Continuation prepares original track 2 with the new standings' assigned
physical slots. The missing-next-track rollback passes on all three targets, including fully
instrumented ASan/UBSan. All twelve verification cases pass with final unchanged
source/binary identities. Nine actual first-round sessions complete ten laps;
Native and sanitized results take 287081 ticks, WASM 287089 ticks, including
400 countdown and 600 coasting ticks. This is functional integration evidence,
not bitidentical physics or complete season/game acceptance.

Reports/receipts are under `/tmp/wasm-dd2/rewrite-championship-integration/`;
`quality-report.json` records final shared-build gates. Successful raw output is
removed after receipts. All verification processes are terminal; copied failure
fixtures and sanitized executables were removed after final identity checks. An exploratory fixed-throttle player-input fixture did not finish
within its bound; its compact diagnosis is retained and not counted as acceptance.
Use `make rewrite-championship-verify` and see `src/game/championship.md`.

## Next

Connect the controller to actual application/menu flows, preparing renderer
materials before destroying their borrowed track data; persistence, naming and
complete physical campaigns remain in 0004. The containing championship contract
stays open.

## Accept

Strict LLVM19 and both production builds pass; rule fixtures cover score
ownership, duplicate/stale result rejection, all outcomes, overflow and history.
Actual original-data sessions reach natural results, score once, hold results,
prepare the next assigned field and preserve state when next-track loading fails.
This contract excludes completed multi-season physical campaigns, complete front
end, driver naming, multiplayer and compatible persistence; those remain in 0004.
