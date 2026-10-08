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
| [0016](open/0016_ai-tactics-and-offroad-recovery.md) | AI tactics and off-road recovery | open |
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

## Workflow and acceptance

The frontend migration inventory in docs/frontend.md records the original eight
main slots and source-backed session/cancel/confirm constraints. Actual original
input now identifies CARD as File Manager and LINK as CD Audio Player, with
entry/cancel observations and unchanged provisioned saves. Complete navigation,
submenu actions and handwritten backend routes remain open under 0003;
inventory and two observed entry routes do not establish a functioning frontend.

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
