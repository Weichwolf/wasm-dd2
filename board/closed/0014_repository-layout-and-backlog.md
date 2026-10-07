Type: Work item
Title: Repository layout and maintained backlog

## Contract

Keep README a concise description, remove GitHub workflow files, place tests directly in tests/, move our SoftGL to deps/softgl and use deps/ instead of third_party. Keep re_out/ and patches/ only on ghidra and supply frozen reference tests from /tmp. Maintain this board in the wasm-fist style.

## Evidence

46dbb0e publishes the concise README and workflow deletion. SoftGL and ignored reusable dependencies have moved to deps locally; unchanged pin and both-target mandatory gates pass. Frozen reconstruction export now verifies every Git blob and SHA-256 under /tmp; the complete frozen patch check passes. re_out/ and patches/ are removed from master. All 54 test sources/headers are now directly under tests/, with CMake, strict-tooling and verifier references updated together. Final gates pass: make rewrite-check (31 Native CTests plus strict LLVM19 on every owned C unit), make rewrite-wasm and ctest --preset rewrite-wasm (30 CTests). All54 test C/header files retain byte-identical contents across the move. Five artifact-cleanup tests pass. The frozen reference integrity/patch/image checks and actual6240-case season test pass. Evidence: /tmp/wasm-dd2/rewrite-layout-final/reference-report.json and /tmp/wasm-dd2/rewrite-layout-final/report.json.

## Next

Maintain the accepted layout and update this board with every verified improvement. Continue natural-race/contact diagnosis in0002 and publish the proved league foundation in0015.

## Accept

Requested paths and concise README are present, workflows/third_party/re_out/patches/tests-rewrite are absent, all tests remain covered by strict tooling and both-target gates, deps ignores preserve local dependencies, and the result is committed/pushed.
