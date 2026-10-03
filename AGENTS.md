Use `/tmp/wasm-dd2/` for verification captures, generated diagnostics and logs.
Never place these outputs in the repository or `third_party/verification-artifacts`.
Original provisioned game data and reusable build dependencies may remain ignored
in the repository. `/tmp` is a 16 GiB tmpfs on this machine: keep each verification
run below 2 GiB and leave at least 1 GiB free. Capture only the memory checkpoints
needed for the current diagnosis; do not dump full memory images at every frame.
Delete successful raw comparison output after writing the report. Remove completed
captures when their diagnosis is finished, and run `make clean-logs` before and
after verification work. Do not delete files needed by running processes.

After each successfully verified improvement, commit and push it as requested
by the user. Do not claim original parity for a diagnostic or partial comparison.
