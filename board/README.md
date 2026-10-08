# Rewrite work queue

Directory is state: `open/active/closed`. Read `../AGENTS.md` and the current work
item before implementation. IDs remain stable; move the file when its state
changes. This board follows the structure used by `wasm-fist` without importing
its game requirements or acceptance rules.

## Work order

| WI | Deliverable | State |
| --- | --- | --- |
| [0001](closed/0001_rewrite-reference-boundary.md) | master rewrite and ghidra reference | closed |
| [0002](active/0002_natural-races-and-contact-stability.md) | Natural races and contact stability | active |
| [0014](closed/0014_repository-layout-and-backlog.md) | Repository layout and backlog | closed |
| [0015](closed/0015_typed-league-foundation.md) → [0004](open/0004_championships.md) | League foundation and complete championships | closed / open |
| [0003](open/0003_frontend-and-menus.md) | Complete front end and menu actions | open |
| [0005](open/0005_vehicles-and-damage.md) | Every vehicle/class/livery and damage behavior | open |
| [0016](active/0016_ai-tactics-and-offroad-recovery.md) | AI tactics and off-road recovery | active |
| [0006](open/0006_replays.md) | Replays | open |
| [0007](open/0007_keyboard-gamepad-and-platforms.md) | Keyboard/gamepad/platform lifecycle | open |
| [0008](open/0008_settings-save-load-and-records.md) | Settings, save/load and records | open |
| [0009](open/0009_complete-audio-behavior.md) | Complete effects and music behavior | open |
| [0010](open/0010_graphics-and-performance.md) | Continuous graphics improvement | open |
| [0011](open/0011_complete-functional-acceptance.md) | Complete requested game on both targets | open |
| [0012](closed/0012_accepted-motion-stall-detection.md) | Accepted-motion AI stall detection | closed |
| [0013](closed/0013_connected-world-query-pruning.md) | Connected contact world queries | closed |
| [0017](closed/0017_bounded-contact-restart.md) | Bounded restart for cycling accelerated contacts | closed |
| [0018](closed/0018_preserve-converging-contact-iterations.md) | Preserve iterations for converging contact chains | closed |
| [0019](closed/0019_english-repository-and-browser-layout.md) | English repository text and browser layout | closed |
| [0020](closed/0020_delayed-coupled-contact-corrections.md) | Delayed coupled-contact corrections | closed |
| [0021](closed/0021_contact-jacobian-accuracy.md) | Late contact Jacobian accuracy | closed |
| [0022](closed/0022_contact-active-branch-predictions.md) | Contact active-branch predictions | closed |
| [0026](closed/0026_contact-correction-selection.md) | Choose stronger physical contact corrections | closed |
| [0027](closed/0027_championship-mixed-world-support.md) | Mixed world supports in scheduled racing | closed |
| [0028](closed/0028_championship-coupled-supports.md) | Coupled supports in scheduled racing | closed |
| [0029](closed/0029_championship-next-advancement.md) | Scheduled racing advancement through the captured failures | closed |
| [0030](closed/0030_championship-world-support-advancement.md) | Linear world-friction refinement in scheduled racing | closed |
| [0031](closed/0031_championship-mixed-support-advancement.md) | Coupled world-release directions in scheduled racing | closed |
| [0032](closed/0032_championship-dependent-support-advancement.md) | Dependent world supports in scheduled racing | closed |
| [0033](closed/0033_championship-two-support-advancement.md) | Two retained world supports in scheduled racing | closed |
| [0034](closed/0034_original-font-glyph-data.md) | Original font glyph data | closed |
| [0035](closed/0035_championship-world-support-advancement.md) | Championship world-support advancement | closed |
| [0036](active/0036_championship-next-advancement.md) | Next natural championship advancement | active |
| [0037](active/0037_dense-contact-search-cost.md) | Avoid redundant dense contact model search | active |
| [0038](active/0038_cold-constitutive-contact-root.md) | Recover a complete cold constitutive contact root | active |
| [0025](active/0025_championship-application-flows.md) | Play championships through Native/browser input and presentation | active |
| [0024](closed/0024_championship-result-owner.md) | Owning scheduled championship rounds and actual results | closed |
| [0023](closed/0023_stable-driver-physical-grids.md) | Stable driver IDs in assigned physical grids | closed |

Continue stabilizing ordinary races, then connect the proven league foundation
to full championship/front-end/save flows. Audio, input and visual work
remain part of the full goal; no component milestone replaces final acceptance.

The terminal a326993 race attempt and separate Circuit-5 follow-up account for
all 49 scenarios and 147 target scopes: 146 pass; Native B fails natural
completion at the unchanged fixture bound. Its sparse movement diagnosis
matches the production trace exactly. Assigned physical grids now preserve
driver IDs on all eleven original levels; complete championships remain open
under 0004. Shared championship entry, prepared round transitions, named
standings and first-round Native/browser/sanitized application checks now pass
under 0025. Correction selection now advances the saved round-2 tick-59,399 input;
the shared low-speed material law advances the saved tick-66,449 input under
0027. Analytic mixed-contact corrections now advance the saved tick-86,643 input
under 0028; all eighteen frozen queries and fourteen reorderings pass on three
targets. The unchanged-bound natural season proceeds to tick 107,550, whose
position-repair failure is reproduced under 0029. The production position prediction advances that saved step; nineteen captured
queries, twenty-eight reorderings and twenty-four independent analytic position
cases pass on Native, WASM and ASan/UBSan. Actual Native/sanitized/Chromium input checks pass all eleven levels. The new
natural season again
completes round 1 with the same scores, then fails round 2 at tick 66,070; a
31,216-byte replay and exact sparse trace identify a different four-contact
position failure. A guarded active-position fit now advances that input. Twenty captured queries,
fifty-two reorderings and seventy-two analytic position cases pass on three
targets. The natural season advances past 66,070 to a new second-round failure
at 81,622. Production-equivalent input/query replay identifies an eight-contact
velocity failure; position is not reached. All eleven actual Native/sanitized/
Chromium input checks, all eleven original-data ground checks and six scoped
Circuit-2/live plus Circuit-5/eight-lap target scopes pass.
A bounded nonlinear branch refinement now advances the saved 81,622 input.
Twenty-one captured queries and sixty-eight orderings pass Native/WASM/ASan/
UBSan alongside strict gates. All eleven actual Native/sanitized/Chromium input
checks and original-data ground checks pass; Circuit-2/live and the complete
eight-lap Circuit-5 race pass on all three targets. The natural season completes
round 1 with unchanged scores, crosses 81,622 and next fails at second-round
tick 105,335. Sparse production-equivalent replay identifies a five-contact
velocity failure. A saturated constitutive search direction now advances the
saved input to 105,336 on Native and sanitized C, with unchanged regularized
material acceptance. Twenty-two captured cases and 188 orderings pass all three
targets and strict gates. Actual application, original-data ground and the
scoped Circuit-2/live plus complete eight-lap Circuit-5 checks pass on all three
variants. Natural racing passes 105,335 and next fails at 105,932; sparse
production-equivalent replay identifies a four-body, ten-contact velocity
failure. Local selective car-pair friction directions now advance that saved
input to 105,933 on Native and instrumented C, retaining all material rules and
solver bounds. Twenty-three captured queries and 208 orderings pass all three
targets and strict gates. The changed natural collision trajectory completes
round 1 with unchanged scores, then fails round 2 at 101,640, before the previous
failure tick. Sparse production-equivalent replay isolates three world supports
on one car; velocity remains above tolerance at the existing pass bound.
A positive world-load seed with fixed-load friction equilibrium now advances
the saved 101,640 input to 101,641, with unchanged material rules and ordinary
solver bounds. Twenty-four captured queries and 214 orderings pass all three
targets and strict gates. The changed natural season again completes round 1
with unchanged scores, then fails round 2 at 93,231, before the previous failure.
Sparse production-equivalent replay identifies a three-body/five-contact velocity
failure. Typed higher/released world-pressure seeds now advance the saved input
to 93,232. Twenty-five captured queries and 334 orderings pass all three variants,
with strict gates, eleven-level real window/input and original-data ground checks,
and six scoped race targets including the complete eight-lap Circuit-5 race.
The unchanged-bound natural season passes every previously recorded failure tick,
including 107,550, then fails round 2 at 118,647. This completes the reproduced
advancement contract under 0029; a new failure-only capture and two actual replays
identify three world supports on driver 15 with a velocity failure under 0030.
A bounded private refinement of the existing linear world-friction direction now
advances the saved 118,647 input to 118,648. Twenty-six queries and 340 orderings,
strict LLVM19/Native/WASM gates, eleven-level actual input and original-data
ground checks, and six scoped race targets pass on all three variants.
The unchanged-bound natural season crosses 118,647 and next fails round 2 at
128,036. Production-equivalent sparse capture and two actual replays identify
six cars with eleven contacts (six world/five pair); velocity remains above
tolerance at 4,096 passes. The advancement contract closes under 0030; the next
failure is subsequently resolved under 0031. Full physical seasons, remaining arena
completion and all-game acceptance remain open. Current receipts:
/tmp/wasm-dd2/rewrite-linear-118647/report.json and
/tmp/wasm-dd2/rewrite-season-after-118647/report.json.

Direct zero-impulse release directions now advance the saved 128,036 input to
128,037 on Native and instrumented C. Twenty-seven captured queries and 362
orderings, strict LLVM19/Native/WASM gates, eleven-level actual input and
original-data ground checks, and six scoped race targets pass on three variants.
The unmodified natural season retains identical round-1 results and reaches
round-2 tick 150,000; its diagnostic prefix matches exactly. The advancement
contract closes under 0031. The terminal season attempt next fails at round-2 tick 220,415. Its failure-only
diagnostic matches all 104 production records, and two actual replayers retain
the failure. Four cars form ten contacts; velocity remains above tolerance at
4,096 passes. Sparse diagnosis identifies dependent normal rows; the correction
and natural continuation are tracked under 0032. Full seasons, the remaining arena
and all-game acceptance remain open. Receipts:
/tmp/wasm-dd2/rewrite-release-128036/report.json and
/tmp/wasm-dd2/rewrite-season-after-128036/report.json.

A private retained normal with a fixed co-oriented patch release mask and exact
zero-impulse targets now advances the saved 220,415 step on Native and instrumented
C. Twenty-eight captured queries and 382 orderings pass on Native/WASM/ASan/UBSan,
with strict LLVM19 gates. All eleven original-data ground and actual Native/
sanitized/Chromium input checks pass. Six scoped race targets pass, including the
complete eight-lap Circuit-5 race. The longer natural production and diagnostic
attempts now reach round-2 tick 230,000 with exact matching prefixes and identical
previous round-1 results. This closes advancement under 0032; complete seasons
and all-game acceptance remain open. Verified scope:
/tmp/wasm-dd2/rewrite-patch-220415/gates-report.json.
The first production/diagnostic attempt stops at its 900-second wallclock budget
after round-2 tick 160,000, with exactly matching records and no logged solver
failure. It does not close the continuation contract. A longer-wallclock attempt
uses the same physical bounds, binary and ordinary control profile. Advancement
receipt: /tmp/wasm-dd2/rewrite-patch-220415/advancement-report.json.

Both longer-wallclock attempts are now terminal: the Native season fails round 2
at tick 233,358, with exactly matching production/diagnostic records. Six cars
form fourteen contacts; two actual owner replays reproduce the velocity failure.
Read-only inspection preserves identical diagnostic machine code. Independent
analytic and finite-difference roots require two retained wall supports and two
released supports. A private Native two-endpoint branch advances the saved step
and passes 438 physical query/order checks; production correction and natural
continuation remain under active 0033. Full seasons remain unproved. Receipts:
/tmp/wasm-dd2/rewrite-patch-220415/report.json and
/tmp/wasm-dd2/rewrite-season-after-220415-long/diagnosis.json.

The production two-endpoint branch now advances the saved 233,358 input on
Native and instrumented C. Twenty-nine queries and 410 orderings pass all three
targets with strict LLVM19, 34 Native and 32 WASM CTests. Full material laws and
ordinary pass bounds remain unchanged. Broader original-data/actual-input/scoped-
race checks and fresh natural continuation remain in progress under 0033;
its advancement contract is still active. Component receipt:
/tmp/wasm-dd2/rewrite-endpoints-233358/component-report.json.

Current two-endpoint production code also passes all eleven actual Native/
sanitized/Chromium input flows and original-data ground checks. Six scoped race
targets pass, including the complete eight-lap Circuit-5 race on all three
variants; this is partial race-suite coverage. Fresh natural production and
diagnostic prefixes match, with unchanged previous first-round results. The
same live production child continues under read-only exit monitoring after its
launcher terminated; no engine restart occurred. Natural advancement under 0033
and full physical seasons remain unproved. Verified scope:
/tmp/wasm-dd2/rewrite-endpoints-233358/gates-report.json.

Both unchanged-bound natural runs now reach round-2 tick 240,000 with exact
matching prefixes and identical previous round-1 results. The saved-input and
ordinary-continuation contract closes under 0033. The same Native process has not
restarted; read-only exit monitoring preserves its original allowance. Full
physical seasons remain unproved under 0025/0004, and the player has zero credited
laps at this checkpoint. Follow the live runs to their actual terminal outcomes.
Advancement receipt: /tmp/wasm-dd2/rewrite-endpoints-233358/advancement-report.json.

The same natural attempts are now terminal at round-2 tick 300,000, with all 120
JSON records matching. Every requested step advances; no contact failure or
failure checkpoint is recorded. Player health is 0.4003601829670652 and credited
laps remain zero, so the season is incomplete at the unchanged fixture bound.
Sparse movement/controller diagnosis now belongs to active 0016 alongside the
remaining arena-B tactics. No specific tactical cause or correction is proved.
Terminal receipt: /tmp/wasm-dd2/rewrite-endpoints-233358/report.json.

Original Information/Configuration subordinate entry and cancel routes are now
observed through 19 actual keys: lap times, Credits, Keyboard remapping prompt
and Audio Volume. The isolated/provisioned saves stay unchanged. Remaining
editing, persistence, joystick and rewrite-menu acceptance stay open under 0003.
Receipt: /tmp/wasm-dd2/rewrite-frontend-submenus/report.json.

Closed 0034 supplies owned original font glyph tables, with all 288 decoded
characters checked on Native/WASM/ASan and all 576 loaded original base/duplicate
characters checked at normal Wine frontend startup. Twelve original mutations,
input lifetime and mandatory quality gates pass. Rendering/menu actions remain
open under 0003. Receipt:
/tmp/wasm-dd2/rewrite-font-verification-complete/report.json.

The step-83,450 natural world-support failure after the
controller's forward-retry correction was reproduced under 0035. All 76 production/diagnostic records
match; one saved owner and its three world contacts reproduce failure against
production libraries, both with twenty bodies and a single-body remap. A contact
correction and complete physical campaign remain unproved. Receipt:
/tmp/wasm-dd2/rewrite-reverse-expiry/failure-capture/report.json.

The low-speed reverse timer now permits a fresh forward attempt, with a focused
before-fail/after-pass regression on Native/WASM/ASan and passing mandatory
35/33 CTests and eleven-level actual input checks. The completed guidance suite
passes all 29,214 path queries per target, but sustained movement passes only
31 of 33 scopes: Native and sanitized B remain below the unchanged movement
requirement under active 0016. Full AI and physical seasons remain open. A faithful
terminal observer and independent physical root identify a feasible released-wall
branch for 0035; the production contact correction remains unproved. Receipts:
/tmp/wasm-dd2/rewrite-reverse-expiry/report.json and
/tmp/wasm-dd2/rewrite-reverse-expiry/failure-capture/independent-root/report.json.

The private 0035 diagnostic now reproduces that admissible root by retaining
fitted friction through its bounded released-wall refinement. Both isolated
query layouts pass independent material checks; the saved actual Native owner
advances to 83,451. Production integration must restore cone projection before
outer acceptance and pass shared-platform/regression/natural-continuation gates.
This is a private diagnostic milestone, not a production correction or completed
season. Receipt:
/tmp/wasm-dd2/rewrite-reverse-expiry/failure-capture/private-release-constitutive/physical-report.json.

Closed 0035 now proves the production fitted-release correction, with restored
friction cones before finite lower-residual acceptance and unchanged physical
rules/bounds. The saved owner advances to 83,451 on Native and instrumented C;
thirty captured queries / 416 orderings, strict 35/33 CTests, eleven-level actual
input/original-data ground checks and six scoped race targets pass on all three
variants. Those race targets include the complete eight-lap Circuit-5 race.
The unchanged-bound natural season retains the same first-round result and
crosses the former failure through round-2 tick 160,000. That same process remains
running; full seasons and the remaining Arena-B AI movement remain open.
Receipt: /tmp/wasm-dd2/rewrite-fitted-83450/report.json.

That natural process is now terminal at round-2 tick 190,078 (exit 1, no timeout).
AI/frame/championship inputs remain valid; player health is 0.23458961198271322
and laps remain zero. All 98 production records are retained. Active 0036 owns
the next advancement diagnosis; a failure-only capture is running and its cause
is not yet proved. The closed 0035 continuation contract remains valid; complete
physical seasons remain open. Terminal receipt:
/tmp/wasm-dd2/rewrite-fitted-83450/natural-season/terminal-report.json.

The 0036 capture is terminal and matches all 98 production records. Actual owner
replay and twenty-body/two-body contact queries reproduce the failure. A
byte-identical read-only observer and independent material root isolate a private
release-refinement merit obstacle in the five-contact group. A bounded diagnostic
trajectory reaches the admissible root, but production correction and natural
continuation remain unproved under active 0036. Receipt:
/tmp/wasm-dd2/rewrite-next-190078/diagnosis-report.json.

The production private constitutive merit now advances that saved step to
190,079 on Native and instrumented C while preserving outer cone/physical
acceptance and all solver bounds. Strict LLVM19, 35/33 CTests, all 567 captured
query/order checks per target and eleven-level original-data ground checks pass.
Actual-input, scoped-race and natural-continuation gates remain running; 0036
stays active and full seasons remain unproved. Component receipt:
/tmp/wasm-dd2/rewrite-merit-190078/component-report.json.

All eleven actual Native/sanitized/Chromium input/view checks and all six selected
race targets now pass for the constitutive-merit correction, including the
complete eight-lap Circuit-5 race on every target. The same fresh ordinary
campaign retains identical first-round results and continues in round 2;
advancement past 190,078 remains pending under active 0036. Full physical seasons,
the remaining Arena-B AI movement and all-game acceptance remain open. Receipt:
/tmp/wasm-dd2/rewrite-merit-190078/gates-report.json.

## Workflow and acceptance

The complete cold constitutive fallback is now integrated under active 0038,
after existing warm branches and with stricter complete-root acceptance. Exact
rollback, the physical law, initial refitted competition and ordinary certification
remain; only one bounded base model is added (259 total). Strict LLVM19, 35/33
CTests and all 633 independent physical checks per target pass. Actual saved
115,420/190,078 owners advance, independent root motion agrees within 1.72e-9,
and the dense-query Native CPU benefit remains 20.48-fold. A fresh ordinary
campaign retains identical first-round results; continuation and broader gates
are running. Full physical seasons/all-game acceptance remain open. Receipt:
/tmp/wasm-dd2/rewrite-cold-root-0038/component-report.json.

The 115,420 capture now matches all 83 production records exactly. Current and
pre-optimization solvers both reject the saved owner and full/remapped query.
Independent geometry and all sixteen normal active sets establish an admissible
all-loaded root. A private complete cold constitutive refinement advances the
actual owner and passes all 608 existing Native physical checks; integration,
other platforms and natural continuation remain under active 0038. Partial cold
improvements are rejected to preserve existing warm progress. Receipt:
/tmp/wasm-dd2/rewrite-dense-search-0037/diagnosis/diagnosis-report.json.
The old immutable 18c0a01 run separately terminates at 168,287 (exit 1, no timeout),
before proving the 0036 continuation contract. Its 93 records/source/binary remain
identified; an exact old-runtime failure capture is running. Receipt:
/tmp/wasm-dd2/rewrite-merit-190078/natural-season/terminal-report.json.

All current-source actual-input/window checks for 0037 now pass on Native,
sanitized Native and Chromium/WASM, including all eleven scene/car views and
31/31/144 comparisons. No browser errors are reported. Shared-build, focused
physical, ground and selected-race gates pass; natural campaign acceptance is
still contradicted by the recorded 115,420 advancement failure. Its targeted
capture remains running and no causal correction is claimed. Receipt:
/tmp/wasm-dd2/rewrite-dense-search-0037/window-gate-receipt.json.

The 0037 current-source selected race gate passes all six scenario/target pairs,
including the complete eight-lap Circuit-5 Stockcar race on Native/WASM/ASan.
Its 167,472 ticks, place 2 and 75 player points match independent geometry/result
checks. Full race-suite and campaign acceptance remain unproved; the separate
115,420 natural failure is still being captured. Actual-input checks are running.
Receipt: /tmp/wasm-dd2/rewrite-dense-search-0037/race-gate-receipt.json.

The new 0037 natural campaign is terminal at round-2 tick 115,420 (exit 1, no
timeout) after identical first-round results. All 83 production records are
retained; AI/frame/championship inputs are valid, player health is 0.2677000161305475
and laps remain zero. Component/ground proofs remain scoped; natural acceptance
has failed and 0037 stays active. A failure-only owner/query capture is running
to reproduce the exact records and distinguish a regression from another exposed
contact state. The old immutable 18c0a01 campaign is followed separately. Receipt:
/tmp/wasm-dd2/rewrite-dense-search-0037/natural-season/terminal-report.json.

Active 0037 now verifies a bounded dense-contact search improvement: further
private models stop once an accepted full physical residual already reaches the
existing tolerance; both initial refitted directions and ordinary certification
remain. Strict LLVM19, 35/33 CTests, all 608 independent physical checks per target,
and the saved actual 190,078 owner pass. One captured dense query averages 20.49
times less Native CPU work across five paired runs; no frame-rate claim is made.
The expanded oracle includes the existing capped law, rather than interpreting
vanishing-load uncapped-oracle failures as solver failures. A fresh natural run
retains identical first-round results; its dense-region continuation and broader
gates are pending. The old natural 18c0a01 binary is followed separately.
Receipt: /tmp/wasm-dd2/rewrite-dense-search-0037/component-report.json.

The dense-search implementation now also passes the eleven-level original-data
ground gate on Native/WASM/ASan, including 79,100 independent queries and 26,400
drop steps per target. Current-source actual-input/scoped-race checks and natural
continuation remain pending under 0037/0036. Receipt:
/tmp/wasm-dd2/rewrite-dense-search-0037/ground-gate-receipt.json.

Additional original File Manager input now establishes directional slot selection,
the empty-load Not a DD2 file error, empty-delete return without confirmation,
and cancellation with unchanged isolated/provisioned saves. A separate actual
X11 client-window capture visibly confirms the selected slot and error dialog;
torn internal framebuffer images do not support complete-image claims. Empty-slot
routes are evidence for open 0003/0008; handwritten menus, saving, occupied-slot
load/delete, reload and storage failures remain open. Receipts:
/tmp/wasm-dd2/rewrite-frontend-file-manager-errors-confirmed/report.json and
/tmp/wasm-dd2/rewrite-frontend-file-manager-window/report.json.

The frontend migration inventory in docs/frontend.md records the original eight
main slots and source-backed session/cancel/confirm constraints. Actual original
input now identifies CARD as File Manager and LINK as CD Audio Player, with
entry/cancel observations and unchanged provisioned saves. Complete navigation,
submenu actions and handwritten backend routes remain open under 0003;
inventory and two observed entry routes do not establish a functioning frontend.
Additional original input now opens/cancels Information and Configuration with
the default View Lap Times/Select Control Method actions and unchanged saves.
Those observations constrain entry/cancel behavior; subordinate actions and
the complete rewrite frontend remain open. Receipt:
/tmp/wasm-dd2/rewrite-frontend-info-config/report.json.

Use RFC 822 headers `Type`, `Title`, optional `Depends`, then **Contract**,
**Evidence**, **Next**, **Accept**. Keep one owner for each behavior contract.

1. Reproduce the current failure and state the intended behavior and evidence.
2. Implement readable typed C11 and meaningful checks for the reached behavior.
3. Run strict LLVM19 and required Native/WASM gates plus relevant functional,
   original-data and actual presentation/output checks.
4. Record source/build identity, commands, scope, results and limitations in the
   work item. Diagnostics remain bounded under `/tmp/wasm-dd2/`.
5. Close only after **Accept** is proved. Commit/push each verified improvement,
   clean completed raw output and update affected work items.

Original runs and reconstructed C supply evidence. They do not replace the
handwritten game. A partial comparison, successful unit test or static preview
cannot prove complete gameplay, original parity or a feature it does not reach.
