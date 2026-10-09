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

The bounded named V8 profile attributes 11.86 percent of samples to zero writes.
The counted scratch correction is proved under closed 0054: eight ordinary
6000-tick prefixes preserve every per-target field, with a scoped WASM child-CPU
reduction of 5.50..6.01 percent. This supplies no natural-result acceptance.
Strict LLVM19/170-file and 38/36 gates, all eleven original ground levels,
220 recovery cases and 45 prior race scenarios pass again per target.

The selected source-0323ba9 owners are terminal. Native and fresh O1 ASan/UBSan
again reach engine-retirement/coasting results at ticks 81219 and 13083, with
401.09 and 60.41 seconds survival respectively. WASM times out at the unchanged
1800-second deadline after 86042 complete ticks: state
[1,0,86042,85642,0,0,20], 428.21 seconds survival, no solver rejection and no
natural result. Every complete WASM row passes the independent prefix oracle.
Its full public field at tick 77251 matches the prior source exactly; this is
one checkpoint, not a full-trace or original-parity comparison. Completed raw
output and private binaries are removed after recording hashes and receipts.
Receipts: /tmp/wasm-dd2/rewrite-arena8-profile-0053/
{terminal-owner-report,late-checkpoint-report,terminal-cleanup-report}.json.

Diagnose late WASM field motion and simulation cost with bounded typed
checkpoints and sampling before another unchanged long retry. Preserve all
component/race contracts and unchanged deadlines; do not close on a component
or a pre-timeout prefix alone. More completed ticks in this run do not establish
an isolated whole-case speed improvement.

## Current natural owner receipts

The source-3145183 natural-owner retry follows the checked residual-motion
correction under closed 0059 and current original class handling. Actual Native
Arena 8 now reaches engine-retirement/coasting results at tick 15425 (72.12
seconds survival); WASM reaches the same natural ending at tick 25530 (122.645
seconds). The independent owner oracle checks every row, all twenty clocks,
monotonic damage, engine retirement, pursuit and the complete 600-step coast.
Inputs, event/material bounds and the 1800-second deadline remain unchanged.
These two accepted target receipts do not prove sanitized or all-arena results,
the former source's exact late field, or an isolated performance improvement.
Receipts: /tmp/wasm-dd2/rewrite-certified-arena-0053/owners/
{identity,8-total-survive-native,8-total-survive-wasm}.json.

## Verified late-motion diagnosis

The read-only late WASM diagnostic is now terminal after its planned 1740-second
stop at tick 83591, state [1,0,83591,83191,0,0,20]. It retains ordinary controls,
120000 fixture ticks and frozen production library code; it is not a standard
natural-result run. Its full public tick-77251 field matches the saved production
checkpoint exactly; tick 86042 was not reached. All 85 sparse field/motion windows
pass finite-state, physical clocks, monotonic damage and pursuit checks.

Between ticks 70000 and 83591, 13591 actual steps cover 67.955 simulated seconds.
The field averages 58.411 response events per step, with no unresolved sweeps.
The player actually travels 24557.623 horizontal units, compared with 49706.963
from integrating published post-solve horizontal speed; that integral is not a
prediction of free-step motion. Its wheels are grounded in 99.181 percent of
counted wheel steps. Only five player contact records exceed both existing
damage thresholds; front damage grows from 0.809816/0.241110 to 0.929339/0.256481.
This is moving dense-contact behavior, not a frozen player or natural retirement.

The late named V8 profile has 48181 samples: fleet resolver 56.03 percent,
static car SAT 9.63, surface traversal 7.10, memset 6.50 and rotation 6.02.
Compiled labels include inlined callees; these percentages do not attribute all
fleet work to one helper or isolate the earlier scratch optimization. Capture
one actual late fleet step's previous/proposed typed bodies and selected contact
times/rows, then distinguish repeated supports from legitimate dense pursuit.
Keep material laws, damage, controls, event/pass bounds and ordinary deadlines.
Receipts: /tmp/wasm-dd2/rewrite-arena8-late-0053/
{terminal-report,diagnosis-report}.json. Completed profiler/raw output is removed;
only three needed typed checkpoints and immutable reports remain.

## Accept

The captured query reaches an independently checked valid position root on all
three targets without altering equations or budgets. The ordinary owner advances
through the rejection with actual fields, damage, clocks and results; previous
physical/race/recovery contracts remain passing. Broader arena completion is
tracked under 0002.
