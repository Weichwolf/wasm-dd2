Type: Work item
Title: Prune world queries outside the contact component

## Contract

Query road/barrier support only for cars connected to the primary contact, preserving selected supports, order, solver/material rules and budgets.

## Evidence

Published dcb29dd. Required 31 Native/30 WASM gates and the complete 220-grid/132000-step-per-target fleet checks pass. Full arena8 samples match baseline Native exactly and WASM/Sanitizer complete baseline prefixes exactly. Receipts: /tmp/wasm-dd2/rewrite-connected-final/quality-report.json and /tmp/wasm-dd2/rewrite-connected-fleet-verification/report.json. Full eleven-level sixty-second AI verification is terminal: the first ten levels pass all targets, but B Native slot12 travels9010.022<10000 units and sanitized slot12 travels9769.014<10000. Receipt: /tmp/wasm-dd2/rewrite-connected-ai-verification/terminal-report.json. This separate motion failure remains under 0002/0016.

## Next

Keep the full AI result current and continue 0002. Sanitizer optimization now matches production O3, with unchanged scenario lengths, checks and deadlines.

## Accept

The scoped query-pruning contract is verified without dropping connected supports; mandatory gates and actual fleet checks pass. Full natural race behavior remains outside this item.
