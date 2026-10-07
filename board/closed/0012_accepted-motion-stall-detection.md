Type: Work item
Title: Detect stalled AI from accepted movement

## Contract

Blocked cars with nonzero retained solver velocity must recognize insufficient accepted XZ movement and reverse; valid forward motion must not trigger stall recovery.

## Evidence

Published a72ecf0. Movement anchor requires 120 world units within 300 decisions; reverse resets the window. Meaningful stationary/creep/oscillation/moving/invalid/reset tests and 31 Native/30 WASM strict gates pass. Receipt: /tmp/wasm-dd2/rewrite-arenaB-progress/quality-report.json.

## Next

Full arena stability and remaining AI behavior are tracked in 0002 and 0016.

## Accept

The bounded stall-detection contract and transactional state/reset behavior are verified. This does not prove original AI parity or complete races.
