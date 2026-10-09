Type: Work item
Title: Certify independent residual motion at the fleet response budget

## Contract

Preserve contact-free vehicle travel when unrelated bodies exhaust the shared
64-response budget. Accept only residual paths certified by the existing world
and pair sweeps against the final moving/stationary field. Freeze potentially
contacting or unresolved bodies and propagate newly created obstructions to
followers. Preserve material laws, response/pass limits, transaction rollback
and contact reporting. Prove the captured failure and focused geometry contracts
on Native, WASM and ASan/UBSan; retain unchanged driving comparisons.

## Evidence

A read-only linker observer on unmodified b981ba2 production libraries reproduces
the entire prior Native/WASM LEVB final player state exactly. The original 120
frames retain five fixed steps each, after the 200-step settle. A compact player
trace first exceeds the existing strict comparison in driving tick 534, vehicle
step 734. Earlier sub-picometer differences are not this failure.

Five NPC pairs among bodies 7..12 consume 64 WASM events while Native needs 58.
The player has no contact records. Both proposed player paths still match; WASM
retains only 34.633 percent of the step while preserving its velocity. The player
loses 10.9045504872 horizontal units solely because unrelated response work
exhausts the global clock. Records are 107 WASM/109 Native; unresolved sweeps
remain zero. Only the selected previous/proposed/accepted typed field is retained.

The private candidate checks residual paths without further responses. A
monotone stopped set rechecks moving followers after their predecessors freeze.
All 600 actual LEVB drive steps meet the unchanged strict state comparison;
maximum Native/WASM X difference is 6.46e-11. Four frozen-query cases pass on
both targets, retaining 64 events and 107 records: independent motion, a falling
leader with two followers, a future crossing pair and a wall-bound leader with
two followers. The original implementation fails independent motion on both
targets. Diagnostics: /tmp/wasm-dd2/rewrite-LEVB-step-0002/diagnosis-report.json.

Production LLVM19 checks cover 207 C/header files, with 47 Native and 45 WASM
CTests passing. The four focused residual-path cases, 48 position boundary
selections and all 742 prior material cases pass on Native, WASM and fresh O1
ASan/UBSan. Native Memcheck reports zero errors and frees every allocation.
A read-only production-library observer repeats all 600 LEVB player steps within
the unchanged strict tolerance, with maximum X difference 6.46e-11. All 22
unchanged drive/start comparisons now pass, including LEVB: the sanitized image
matches Native exactly; WASM differs in seven pixels within the existing SIMD
raster contract. All eleven original ground levels and 220 controlled recovery
drops pass per target.

The first concurrently launched sanitized timing check exceeded its unchanged
60-second deadline before any image comparison. A complete fresh rerun with
fewer concurrent checks passes under the same limit. Its failed receipt remains
separate. Completed raw LEVB traces/private binaries are removed after retaining
source hashes and receipts. Reports:
/tmp/wasm-dd2/rewrite-fleet-remainder-0059-gates/{quality-report,component-report}.json,
trace/production-trace-report.json and
/tmp/wasm-dd2/rewrite-fleet-remainder-0059-{driving-2,ground,recovery}/report.json.

All 45 previously proved race scenarios pass per target, including the complete
physical eight-lap Stockcar race and nine-lap continuous Time Trial on circuit 5.
The selected suite explicitly excludes the four full natural arena owners; its
pass is not complete arena acceptance. Receipt:
/tmp/wasm-dd2/rewrite-fleet-remainder-0059-race/report.json.

## Next

Continue current natural owners under 0002/0053 without relaxing deadlines or
claiming completion from a frozen query. The new budget-tail contract supplies
a changed implementation for that acceptance; it does not prove the late Arena-8
field's cause or natural results.

## Accept

The captured query preserves free motion on all targets and blocks unchecked
world/pair travel, including obstruction propagation. Existing physical checks
and unchanged driving comparisons pass with strict Native/WASM/sanitizer gates.
This closes only budget-tail motion; full natural-race acceptance remains 0002.
