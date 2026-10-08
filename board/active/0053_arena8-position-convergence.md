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

## Verified component correction

The translation equality refit previously released every negative fitted
multiplier together. In the frozen query this releases rows 3 and 4, although
row 4 must remain loaded after row 3 leaves. The correction selects the first
nonnegative warm multiplier boundary, releases that one row and refits the
remaining equations. At most contact_count + 1 solves remain; contacts, geometry,
4096 passes, clearance and the 1e-9 residual requirement are unchanged.

Released identity rows now prescribe exact zero instead of importing elimination
roundoff. The reordered WASM query otherwise produces -1.2151947010832075e-20
on a released row and rejects an otherwise 1.8973538018496328e-19 residual.
This preserves its fixed branch equation; fitted loaded values are unchanged.

Independent enumeration of all 4096 position active sets finds one admissible
least-norm root. The actual captured full query now converges in 512 position
passes on Native/WASM/fresh O1 ASan/UBSan, matching that root within 1e-12.
Forty-eight zero-motion geometry variants independently check final offsets,
all inequalities, untouched motion/steps and row/body-order independence on every
target. Prior source fails the new check on all three. All 742 earlier physical
checks pass per target. Valgrind reports zero errors/leaks for the Native analytic
joint-support corpus. Strict LLVM19 checks all 170 C/header files; 38 Native and
36 WASM CTests pass. All eleven original ground levels, 220 recovery cases and the 45 previously
proved race scenarios pass again per target.

The selected ordinary Arena-8 runs are now terminal at source 6efaa44. Native
naturally reaches engine-retirement/coasting results at tick 81219 (401.09 seconds
survival); the independent private Native run finishes in 1535.956 seconds,
within the unchanged 1800-second limit. Fresh O1 ASan/UBSan naturally reaches
results at tick 13083 (60.41 seconds survival). WASM times out at 1800 seconds
after 77251 complete ticks; its last state is [1,0,77251,76851,0,0,20].
The independent oracle checks every complete WASM row for phase/order, clocks,
recovery, finite positions, monotonic damage, retirement and NPC pursuit. It
advances beyond the prior rejection but supplies no natural-result acceptance.
The full contract remains open; all run limits and ordinary inputs are unchanged.
Receipt: /tmp/wasm-dd2/rewrite-arena8-rejection-0053/terminal-owner-report.json.

Receipts: /tmp/wasm-dd2/rewrite-arena8-rejection-0053/
{position-root-report,position-fit-diagnosis,released-zero-diagnosis,
component-report,valgrind-report}.json.

## Next

Measure actual WASM simulation cost and address the cause before repeating its
natural owner check. Retain the passing Native/sanitized results and all prior
component/race contracts. Keep the unchanged scenario deadlines visible; do not
close on the component or a pre-timeout prefix alone.

## Accept

The captured query reaches an independently checked valid position root on all
three targets without altering equations or budgets. The ordinary owner advances
through the rejection with actual fields, damage, clocks and results; previous
physical/race/recovery contracts remain passing. Broader arena completion is
tracked under 0002.
