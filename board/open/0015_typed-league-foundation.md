Type: Work item
Title: Publish typed single-player league rules

## Contract

Own original single-player score/rank/division permutations, initial league, tie sorting, promotion/relegation and stable-ID physical-grid mapping in handwritten C11.

## Evidence

Private code under /tmp/wasm-dd2/rewrite-championship-worktree matches 6240 original component cases plus the original initial field on Native/WASM/ASan/UBSan. A fresh compact original-x86 producer proves these fields/classifications. Report: /tmp/wasm-dd2/rewrite-league-original/compact-target-report.json. This code is not yet integrated into master or consumed by a championship.

## Next

Integrate league.c/h, meaningful validation/overflow tests, the compact original fixture, CMake targets and make rewrite-league-verify using the final repository layout. Run all mandatory gates and publish independently of unfinished solver experiments.

## Accept

Production Native/WASM builds and the reproducible 6241-case original check pass with strict LLVM19; malformed state/score overflow preserves outputs. Complete championships remain under 0004.
