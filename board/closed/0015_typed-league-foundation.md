Type: Work item
Title: Publish typed single-player league rules

## Contract

Own original single-player score/rank/division permutations, initial league, tie sorting, promotion/relegation and stable-ID physical-grid mapping in handwritten C11.

## Evidence

Production league.c/h is integrated into master and both game libraries. It
matches 6240 original component cases plus the original initial field on Native,
WASM and fully instrumented ASan/UBSan. The compact original-x86 fixture emits
only targeted league fields and classifications, below one MiB. Source/binary
identities and comparisons are recorded in
/tmp/wasm-dd2/rewrite-league-integration/original-comparison/report.json.

Meaningful checks cover score accumulation, division ties, invalid late scores,
signed 16-bit overflow, malformed permutations, null inputs, unchanged rejected
state/output and the initial physical grid. Required gates pass: make
rewrite-check (32 Native CTests and strict LLVM19 format/tidy for 145 owned
C/header files), make rewrite-wasm and ctest --preset rewrite-wasm (31 CTests).
Report: /tmp/wasm-dd2/rewrite-league-integration/quality-report.json.

This proves a single-player rules component. The game does not yet consume it
through a championship controller, season menu, race grid or saved season.

## Next

Use this component in 0004: championship ownership, stable-ID physical grids,
exactly-once result consumption, schedules, history/unlocks and front-end/save
flows. Continue natural-race/contact diagnosis under 0002 independently.

## Accept

Production Native/WASM builds and the reproducible 6241-case original check pass with strict LLVM19; malformed state/score overflow preserves outputs. Complete championships remain under 0004.
