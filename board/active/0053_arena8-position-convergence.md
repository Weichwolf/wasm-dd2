# 0053: Restore natural Arena-8 contact position convergence

## Contract

Correct the actual twelve-contact position rejection in ordinary Arena-8 Total
Destruction. Preserve geometry, least-norm translation stationarity, nonnegative
multipliers, all contact inequalities, 4096 passes and the 1e-9 position bound.
Do not manufacture owner states or weaken natural scenario deadlines.

## Evidence

The unchanged actual owner rejects step 10574 after the last successful state
[1,0,10573,10173,0,0,20]. A read-only linker observer reproduces the complete
public field exactly and captures the first failed joint query: twenty bodies,
twelve contacts on ten bodies, two world barriers and ten pair rows. It records
zero calls to the standalone-world API changed in closed 0052.

Unmodified stage evaluation prepares the query and converges velocity in 17
passes (error 7.128257095700974e-8). Position exhausts 4096 passes with
3.437523934198162e-8 error against the unchanged 1e-9 requirement. An independent
mobility calculation finds an admissible velocity root; this does not solve the
position failure. The same full suite's sanitized Arena-8 run naturally retires
the player at tick 13083, while WASM exceeds its unchanged 1800-second deadline.
All 45 earlier proved race contracts pass on all targets; no full-suite acceptance.

Receipts: /tmp/wasm-dd2/rewrite-arena8-rejection-0053/
{capture-report,stage-native,independent-root-report}.json and
/tmp/wasm-dd2/rewrite-ground-stability-0052/owner-report.json.

## Next

Independently solve the twelve-row least-norm translation complementarity problem.
Compare active/released rows and the production refit's restored iterate; isolate
why the finite correction cannot reach the existing position tolerance. Verify a
causal correction on Native/WASM/sanitized physical components, strict gates and
the unchanged real owner, keeping independent arena deadlines visible.

## Accept

The captured query reaches an independently checked valid position root on all
three targets without altering equations or budgets. The ordinary owner advances
through the rejection with actual fields, damage, clocks and results; previous
physical/race/recovery contracts remain passing. Broader arena completion is
tracked under 0002.
