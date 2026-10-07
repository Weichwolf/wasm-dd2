Use `/tmp/wasm-dd2/` for verification captures, generated diagnostics and logs.
Never place these outputs in the repository or `deps/verification-artifacts`.
Original provisioned game data and reusable build dependencies may remain ignored
in the repository. `/tmp` is a 16 GiB tmpfs on this machine: keep each verification
run below 2 GiB and leave at least 1 GiB free. Capture only the memory checkpoints
needed for the current diagnosis; do not dump full memory images at every frame.
Delete successful raw comparison output after writing the report. Remove completed
captures when their diagnosis is finished, and run `make clean-logs` before and
after verification work. Do not delete files needed by running processes.

After each successfully verified improvement, commit and push it as requested
by the user. Do not claim original parity for a diagnostic or partial comparison.

On the `master` branch, read `docs/rewrite.md` before implementation. New game
modules are handwritten C11 under `src/`, using typed state and explicit ownership
instead of original absolute addresses or emulated registers. Keep the reference
reconstruction and original game data available for functional comparisons.
`re_out/` and `patches/` exist only on `ghidra`; use `make reference-prepare`
and frozen reference commands under `/tmp/wasm-dd2/`, never recreate them on `master`.
The rewrite may improve graphics and audio; bitidentical original output is not
an acceptance requirement. Functional correctness and native/WASM stability are.

Use the pinned `deps/softgl` submodule by default. Do not alter SoftGL sources
as part of a game change; deliberate dependency changes need their own evidence.
All rewrite C/header files must pass clang-format 19. All rewrite C units must
pass strict clang-tidy 19 and the compiler flags defined in `CMakeLists.txt`.
Fix findings instead of disabling checks or adding blanket suppressions. Run
`make rewrite-check`, `make rewrite-wasm` and `ctest --preset rewrite-wasm` for
changes affecting the shared build/renderer. Preserve functional comparisons
and add focused checks when a new gameplay subsystem is implemented.

Maintain `board/README.md` and the current work item. Directory determines work
item state (`open/active/closed`); preserve IDs and update evidence, next steps
and acceptance after each verified improvement. Close only the proved contract.
