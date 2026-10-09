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
| [0005](active/0005_vehicles-and-damage.md) | Every vehicle/class/livery and damage behavior | active |
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
| [0039](active/0039_sliding-world-pressure-transition.md) | Recover a loaded sliding world support's linear branch | active |
| [0040](active/0040_captured-three-car-contact-root.md) | Resolve the captured three-car contact failure | active |
| [0041](active/0041_captured-five-car-contact-root.md) | Resolve the captured five-car contact failure | active |
| [0042](active/0042_natural-first-phase-next-advancement.md) | Next ordinary championship advancement after first-phase refinement | active |
| [0043](active/0043_natural-fixed-active-next-advancement.md) | Next ordinary championship advancement after fixed-active refinement | active |
| [0044](active/0044_current-round2-movement.md) | Restore actual dense round-2 movement | active |
| [0045](active/0045_complete-fleet-event-recording.md) | Complete fleet event recording with explicit ownership | active |
| [0046](active/0046_arena-b-contact-cost.md) | Diagnose and reduce actual Arena-B contact cost | active |
| [0047](closed/0047_linear-world-pressure-basin.md) | Recover a stronger linear world-pressure basin | closed |
| [0048](closed/0048_passing-lane-return.md) | Keep a clear passing lane while its base path is blocked | closed |
| [0049](active/0049_next-wasm-round2-rejection.md) | Diagnose the next ordinary WASM round-2 rejection | active |
| [0050](closed/0050_ordinary-finish-movement.md) | Restore ordinary movement through late-race traffic | closed |
| [0051](closed/0051_warm-pressure-normal-branches.md) | Preserve warm normal branches during world-pressure refinement | closed |
| [0052](closed/0052_arena-a-body-state-stability.md) | Restore the original Arena-A body-state comparison | closed |
| [0053](active/0053_arena8-position-convergence.md) | Restore natural Arena-8 contact position convergence | active |
| [0054](closed/0054_contact-scratch-initialization.md) | Reduce redundant contact scratch initialization | closed |
| [0055](closed/0055_owned-save-card-container.md) | Own the original Windows save-card container | closed |
| [0056](closed/0056_typed-save-profile.md) | Typed original configuration and profile payload | closed |
| [0057](closed/0057_durable-save-store.md) | Durable Native/browser save-card owner | closed |
| [0058](active/0058_configuration-and-saved-game-actions.md) | Configuration and saved-game application actions | active |
| [0059](closed/0059_checked-residual-fleet-motion.md) | Certify independent motion at the fleet response budget | closed |
| [0060](active/0060_automatic-redbook-selection.md) | Automatic game/frontend Redbook selection | active |
| [0061](active/0061_browser-modal-redraw.md) | Browser modal presentation after viewport changes | active |
| [0062](active/0062_standalone-authored-assets.md) | Standalone Blender assets, procedural textures and new audio | active |
| [0025](active/0025_championship-application-flows.md) | Play championships through Native/browser input and presentation | active |
| [0024](closed/0024_championship-result-owner.md) | Owning scheduled championship rounds and actual results | closed |
| [0023](closed/0023_stable-driver-physical-grids.md) | Stable driver IDs in assigned physical grids | closed |

The expanded product goal is standalone Native/WASM with every asset rebuilt
and committed: Blender models/cockpits, procedural textures and newly created
audio. Visual/acoustic quality must substantially exceed the original throughout.
Original game files are optional development references; the current runtime
still requires them and has not met this new contract. Active 0062 owns removal
of those dependencies and the reproducible authored content pipeline. Prior
original-asset diagnostics remain evidence, not standalone product acceptance.

Continue stabilizing ordinary races, then connect the proven league foundation
to full championship/front-end/save flows. Audio, input and visual work
remain part of the full goal; no component milestone replaces final acceptance.
Visual inspection of actual Native/browser play is a primary acceptance check,
alongside pixel regressions. Current sampled HUD/dialogs are readable, but sky,
shadows, texture quality and browser playfield placement need visual improvement.
The review also reproduces a black browser canvas after a full-page viewport
capture in the F2 modal; 0061 owns diagnosis and correction. Physics checks alone
do not establish an acceptable player experience.

Current vehicle work is 0005: original class drive/grip/axle parameters now
affect actual physical fields. Closed 0059 restores all 22 strict drive/start
comparisons by certifying independent motion after the 64-response budget.
Four captured geometry contracts, 48 position boundary checks, 742 prior material
checks, all eleven original ground levels, 220 recovery drops and all 45 earlier
race scenarios pass per Native/WASM/O1 sanitized target. LLVM19 covers 207 C/header
files; 47 Native and 45 WASM CTests pass. Memcheck reports zero errors and frees
all allocations. Next, resume complete natural-arena acceptance under 0002/0053
against this changed source. Complete arenas and campaigns remain open.

The earlier 0005 class integration makes original drive/grip/axle parameters
affect actual physical fields. Native F1/browser selection exposes class names,
ratings and paint; reset/mode/track and championship restart retain owned human
selection and Pro NPC classes. Thirty unmodified-original component cases,
120 short original level/mode/class prefixes per Native/WASM/sanitized target,
real selection/lock checks, LLVM19/205-file checks and 46 Native/44 WASM CTests
pass. Class Memcheck frees all 12 allocations with zero errors. The existing
660 original material cases and 264 body images per target and all eleven
scene/car datasets pass again. The class integration initially passed
21 of 22 strict short-drive cases; 0059 now restores the complete 22-case corpus
by checking independent motion at the fleet response budget. Material laws and
state/image tolerances remain unchanged. Saved car choice is consumed under
0058; complete class campaigns/body-support coverage and damage effects remain
open. The class catalog supplies the 0058
configuration dependency; it does not close complete vehicles.
The complete window regression passes 31 Native/31 sanitized and 144 Chromium
comparisons again with the selected-class preview and current class forces.

Actual source-3145183 Arena-8 Native/WASM owners now reach natural
engine-retirement/coasting results at ticks 15425/25530, with 72.12/122.645 seconds
survival. Their complete independent target receipts pass. Sanitized Arena 8
times out after 41732 independently valid steps without natural results.
The frozen remaining owner suite passes Native Arena 9 but WASM exits after
tick 18935 without results; sanitized Arena 9 is running and A/B have not started.
Sanitized/all-arena acceptance remains unproved. See active 0053 and
/tmp/wasm-dd2/rewrite-certified-arena-0053/owners/.

Current audio preparation is 0060: it establishes the actual original CD contexts:
48 protected-code transport/countdown checks, six caller contexts and eleven
independent menu/asset mappings pass. Main/practice results use track 13;
race music follows loaded level + 1 and starts at GO; championship results use
14 and season completion 15. Automatic Native/browser application integration
remains open. The existing saved gains are proved under 0058.
The shared audio owner now supports transactional READY preparation separately
from immediate playback. Native/sanitized ownership checks and actual browser
silence/start PCM are verified under the music-output command. Next connect
committed contexts, countdown/GO and asynchronous browser file availability;
the preparation primitive does not close 0060.

Optional diagnostic packages are installed and their access probes are recorded
under 0010. Native Memcheck/Callgrind work without elevated privileges; perf and
PCM counters remain unavailable to this user. Use bounded contact cases for
instrumented cost diagnosis and separate WASM sampling. This supplies diagnostics
for remaining contact work; it does not establish a performance improvement.

Current application integration is 0058 under 0008/0003: player/car/audio profiles
now have actual Native/browser name/save/load/delete dialogs, restart and
validated rollback. All three classes persist after fresh processes; failed
class preparation retains every live owner, and same-class loads keep active
motion. Restored dialogs redraw class paint/ratings once. LLVM19/205-file checks,
46 Native/44 WASM CTests, 47 original/Native/sanitized profile cases,
62 Native/sanitized dialog checks and 95 browser checks
pass, including deletion confirmation, complete physical images, empty/duplicate/
GAME/REPLAY entries, external-writer conflicts and pending/aborted IndexedDB
transactions. Full configuration/records, remaining Native configuration actions
and validated playable saved championships remain open. Natural-race work continues under
0053/0002: complete WASM Arena-8 natural-result acceptance.
After closed 0054, the selected Native/sanitized owners reach natural results;
WASM still times out at the unchanged 1800-second limit after 86042 complete ticks.
The bounded late observer now proves moving dense-contact behavior at a matched
production checkpoint. Capture one actual late fleet step and contact times
before another unchanged natural-owner retry.
Closed 0052 proves ground support and closed 0050
proves ordinary first-season movement. The broader 0016/0044/0049 contracts and
0046/0045 remain active.
The following owned-report comparison is historical evidence, before 0050. Owned collision reports preserve
physical work bounds and restore previously clipped movement. The prior
owned-report source's ordinary
Native first Stockcar season completes four real results (two finishes, two
engine retirements) and opens season 2. The separate WASM/instrumented public
session seasons both time out at 3,600 seconds in round 2 with zero player laps;
their first-round finishes do not establish complete seasons.

Moving-body suspension is now integrated: tires contact reachable finite faces
along their actual suspension axis and exchange equal/opposite forces at one
common point. Independent banked/nose cases reject the earlier vertical
prototype; a portable twenty-body replay replaces its false nose load with
real road support. The accepted road-bank recovery remains in place.
Strict LLVM19/167-file checks, 38 Native/36 WASM CTests, ten fresh O1 sanitizer
targets and 716 physical checks per target pass. The complete unchanged
sixty-second AI suite passes all eleven original playable levels on
Native/WASM/O3 ASan/UBSan, with 2,640,000 vehicle steps and 29,214 independent
path queries per target. Arena-B minimum support is 2,374/2,323/2,340 frames,
above the unchanged 2,280 requirement; all target deadlines and reset checks
pass. Original-data vehicle/fleet and actual window/browser input checks also
pass. The interrupted full-suite launcher has a separately revalidated prefix
and bounded continuation receipts. This proves the suspension and sustained
AI-driving improvement; tactics, off-road rescue and complete natural seasons
remain open. Current receipt:
/tmp/wasm-dd2/rewrite-strut-support-0016/component-report.json.
Older timings and failed movement evidence below retain their source epochs.

Typed immutable fleet pair preparation now avoids repeated body validation and
pose construction, retaining caller-owned ordered inventories and transactional
failure. Frozen actual-field O3 ASan/UBSan trials reduce sweep query CPU by
40.2..41.5 percent and neighborhood query CPU by 61.3..62.7 percent. Strict LLVM19
(163 C/header files), 37 Native/35 WASM CTests, 716 physical checks per target,
six focused instrumented targets and all eleven original-data ground/fleet/
actual-input levels pass. These are verified component gains, not whole-game
speedup or complete AI acceptance. Both full Native/WASM Arena-B exporters
preserve all 300 previous rows byte for byte. Native still fails slot-17 support
(1,929 versus 2,280 frames); two small read-only captures show it riding body
18's roof above intact terrain. Its tactics/traction correction remains under
0016. The current instrumented case times out at 360.172 wallclock/360.015 CPU
seconds with 232 complete rows and no completed summary/reset inventory; its
full-case deadline remains under active 0046. The prior
sphere-ordering candidate remains unadopted. Complete tactics, modes, menus,
audio, visuals and all-game acceptance remain open. Current receipts:
/tmp/wasm-dd2/rewrite-pair-preparation-0046/ and
/tmp/wasm-dd2/rewrite-owned-events-0045/{natural-season,season-owner}/.

The road index now also prepares immutable sweep planes once, retaining the
original final-point triangle test, tolerances, bridge separation and tie order.
All 79,100 original-data short/fast sweeps preserve every frozen previous result
field and work count; four O3 ASan/UBSan trials reduce query CPU by 19.0..20.8
percent. Strict LLVM19/163-file checks, 37 Native/35 WASM CTests, 716 physical
checks per target, eight instrumented targets and eleven-level ground/fleet
checks pass. Actual Native/sanitized/Chromium input/presentation passes 31/31/144
comparisons after an unchanged retry of a terminal browser reverse-to-barrier
wait timeout. Both full Native/WASM Arena-B outputs still match their preceding
300-row inventories exactly. Its Native support failure remains; the private
and current-production instrumented cases still exceed 360 seconds. The latter
uses only 217.797 CPU seconds, so its wallclock cannot isolate code speedup or
regression. These are component gains; complete AI, campaigns and game
acceptance remain open. Current receipts:
/tmp/wasm-dd2/rewrite-world-cost-0046/component-report.json.
Historical evidence below is scoped to its recorded source epoch.

The next current-source pair-window diagnostic preserves all 60 available
production rows and measures 50,739,303 box evaluations/7,310,589 refinement
windows by frame 400. Three private bounded pose caches match every result in
the actual 190-pair Native field but increase isolated query CPU by 4.4..10.1
percent against the current batch API. Neither full eight-entry candidate
completes the unchanged 360-second instrumented case; their 126/108 complete
prefix rows match. No cache is adopted and no whole-case gain is claimed.
0046 remains active; next measure reuse within surviving refinement trees and
evaluate sharing window endpoints without slowing cheap initial rejection.
Current receipt: /tmp/wasm-dd2/rewrite-pair-windows-0046/diagnosis-report.json.

Production now rejects an already empty swept-axis time interval immediately.
The measured actual frame-400 work drops from 109,577,906 to 87,658,746 axis
evaluations, preserving all other window/world/group counts. Four alternating
O3 ASan/UBSan actual-field trials reduce query CPU by 19.0..19.5 percent; 4,096
additional frozen pair comparisons preserve every result field. Strict
LLVM19/163-file gates, 37 Native/35 WASM CTests, 716 physical checks per target
and eight fresh instrumented targets pass. Full Native/WASM Arena-B exporters
preserve all 300 prior rows exactly; Native slot-17 support still fails. The
production-equivalent fully instrumented attempt still times out at 360 seconds
with 215 matching prefix rows and no summary/reset completion. A fourth private
pose cache remains unadopted despite fewer box constructions. 0046 stays active;
next locate the remaining cost by per-pair refinement/unresolved work. Current
receipt: /tmp/wasm-dd2/rewrite-refinement-reuse-0046/component-report.json.

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

The new 0038 natural run has now terminated at round-2 tick 108,716 (exit 1, no
timeout, 310.03 seconds) after identical first-round results. All 81 records are
retained and input validity remains true. Component correction of the saved
115,420 case is proved; ordinary continuation is not. The targeted next-failure
capture is running, with no causal diagnosis yet. Active 0038 stays open and
full campaign acceptance remains unproved. Receipt:
/tmp/wasm-dd2/rewrite-cold-root-0038/natural-season/terminal-report.json.

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

A subsequent 38-key original configuration run saves A, cancels occupied
overwrite, commits a staged Sound Effects edit, loads A to restore volume,
cancels deletion and confirms deletion. Nine actual X11 windows and six bounded
card checkpoints establish these paths; the provisioned save is unchanged.
Cancellation/load preserve all bytes; deletion retains the old payload after
clearing the occupied marker/name prefix. Open 0003/0008 still require rewrite
menus/codecs, accepted overwrite, game/replay data and storage-failure handling.
Receipt: /tmp/wasm-dd2/rewrite-frontend-occupied-card-0003-5/report.json.

A further 58-key original run proves same-name occupied configuration overwrite
and cancellation at the filename prompt after Yes. Twelve actual X11 windows
and eight bounded card checkpoints are reviewed. Cancellation preserves all
old bytes; accepted overwrite preserves headers and writes the updated packed
configuration, subsequently restored by load after another volume edit.
The provisioned save remains unchanged. The original delete-before-save
intermediate state requires waiting for completed replacement; rewrite storage
still needs atomic ownership. Renamed overwrite, multiple slots, game/replay
data, restart, storage failures and rewrite menus/adapters remain open in 0003/0008.
Receipt: /tmp/wasm-dd2/rewrite-frontend-overwrite-card-0003-5/report.json.

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

The current 0038 cold-root implementation passes eleven-level original-data
ground checks on Native/WASM/ASan (79,100 queries and 26,400 drop steps per target),
actual Native/instrumented/Chromium input comparisons (31/31/144), and six
selected Circuit-2 live/Circuit-5 complete eight-lap race target scopes. Recorded
source hashes match current production. Its natural campaign still fails round 2
at tick 108,716; exact failure capture and diagnosis remain pending. The saved
115,420 component fix does not prove natural continuation or complete seasons.
Receipt: /tmp/wasm-dd2/rewrite-cold-root-0038/broader-report.json.

The 0039 selected world-pressure refinement now advances the actual 108,716
saved owner on Native/instrumented C. Strict 160-file format/LLVM19 checks,
35 Native/33 WASM CTests and 656 independent contact checks per target pass.
Current motion agrees with the independent root within 7.97e-12; prior 115,420
and 190,078 Native saved steps still advance. A fresh unchanged-bound campaign
and current-source original-data/input/scoped-race checks are running. Natural
continuation and complete seasons remain unproved; the work item stays active.
Receipt: /tmp/wasm-dd2/rewrite-sliding-pressure-0039/component-report.json.

Final 0039 source also passes the eleven-level original-data ground gate on all
three targets (79,100 queries and 26,400 drop steps each). Its frozen ordinary
campaign reaches round-2 checkpoint 110,000 beyond the actual failure, with no
terminal season acceptance. Actual input/scoped-race gates and dense-region
continuation remain live. Receipt:
/tmp/wasm-dd2/rewrite-sliding-pressure-0039/ground-gate-receipt.json.

Final 0039 actual Native/instrumented/Chromium input checks pass 31/31/144
comparisons and all eleven scene/car views; six selected Circuit-2 live/
Circuit-5 complete eight-lap race target scopes also pass. Receipt:
/tmp/wasm-dd2/rewrite-sliding-pressure-0039/broader-report.json.
The older immutable 18c0a01 failure-only capture now matches all 93 production
records and saves its separate three-car/eight-contact tick-168,287 state.
Current d160c06 still rejects that query and owner; 0040 owns its diagnosis.
This older trajectory does not establish the outcome of the current campaign.

The d160c06 ordinary campaign has now ended with valid advancement failure at
round-2 tick 223,337 (exit 1, no timeout, 889.72 seconds, 104 retained records).
It crosses the old failing ticks and reaches 220,000; lap count remains zero.
Targeted new-state capture is running, and complete physical seasons remain
unproved. Separately, current read-only observation of the old 168,287 state
finds no restart, excluding private branch models. A first-phase Native trial
advances that saved owner but still fails an existing regression; 0040 owns
further diagnosis and no production fix is claimed for that input.

The 0040 trial regression is now explained solely by one historical restart
expectation. Production eligibility after the unchanged delay now applies in
both phases, with unchanged materials, bounds, rollback and physical checks.
All 673 contact checks per target, strict LLVM19/160-file formatting and 35/33
Native/WASM CTests pass. The actual 168,287 owner advances on Native/instrumented
C, with independent root motion agreement within 4.98e-14. A fresh frozen
campaign and broader checks are being followed. The separate 223,337 capture
matches all 104 records; its five-car/fourteen-contact owner still fails current
production and is tracked under 0041. Full seasons remain unproved.
Receipt: /tmp/wasm-dd2/rewrite-first-phase-0040/component-report.json.

Current 0040 source now also passes all eleven original-data ground and actual
Native/instrumented/Chromium input scopes, with 31/31/144 view comparisons and
six selected race targets including the complete eight-lap Circuit-5 race.
The ordinary campaign is still live; these checks do not prove a full season.
Independent 0041 equations confirm an admissible root releasing one world row,
while the faithful production observer confirms private branches were already
eligible. Production correction of that separate five-car state remains open.
Receipts: /tmp/wasm-dd2/rewrite-first-phase-0040/broader-report.json and
/tmp/wasm-dd2/rewrite-sliding-pressure-0039/diagnosis/finite-difference-report.json.

The same first-phase ordinary campaign now terminates at round-2 tick 194,353
(exit 1, no timeout, 678.16 seconds, 98 retained records). Inputs remain valid;
player health is 0.23930508697523567 and laps remain zero. Its exact failure-only
capture is running under 0042, separately from the older five-car input.
The private 0041 diagnosis confirms premature extra normal releases can obstruct
the independent root. A separate complete fixed-active-set fallback preserves
old trials, adds one bounded base model and passes the actual fourteen-contact
query plus all 702 preliminary Native checks. Strict/multi-platform/owner gates
are running; full production and complete-season acceptance remain unproved.
Receipts: /tmp/wasm-dd2/rewrite-first-phase-0040/natural-season/terminal-report.json
and /tmp/wasm-dd2/rewrite-five-root-0041/integrated-friction-report.json.

Integrated 0041 production now passes strict LLVM19/160-file formatting, 35/33
Native/WASM CTests and 702 independent contact checks per target. The actual
223,337 owner advances on Native/instrumented C; four prior Native owners still
advance. Full/remapped motion agrees with the independent root within 9.15e-15.
The new first regression rejects frozen first-phase production. Broader checks
and fresh ordinary continuation remain pending; no full season is proved.
Receipt: /tmp/wasm-dd2/rewrite-five-root-0041/component-report.json.

Current 0041 source now passes all eleven original-data ground and actual
Native/instrumented/Chromium input scopes plus six selected race targets.
Its fresh ordinary campaign nevertheless fails valid advancement at round-2
tick 142,893 (exit 1, no timeout, 424.42 seconds, 88 records, zero player laps).
The exact fixed-source capture is running under 0043. Separately, the 0042
capture exactly matches all 98 older first-phase records and isolates three
world contacts on slot 14. Independent pressure continuation confirms an
admissible stronger wall-pressure root. A private incident-pressure seed advances
that saved owner and passes all 702 Native checks, but production adoption,
other platforms and ordinary continuation remain pending. Complete seasons
and the full game remain unproved.
Receipts: /tmp/wasm-dd2/rewrite-five-root-0041/broader-report.json,
natural-season/terminal-report.json and
/tmp/wasm-dd2/rewrite-next-world-load-0042/candidate-owner-report.json.

The 0043 capture now exactly matches all 88 fixed-active production records and
saves another three-world-contact owner on slot 14. Independent mobility and
active-set/pressure-continuation roots identify an admissible all-loaded root.
The 0042 incident-pressure candidate also advances this saved 142,893 owner.
Both real queries and all six permutations each are integrated, bringing the
physical corpus to 716 checks. The first strict build rejects a swappable
index/method argument pair; the final implementation passes a typed contact
model. The final source passes strict LLVM19, 35 Native/33 WASM CTests and all
716 independent physical checks per Native/WASM/ASan/UBSan target. Both new
Native full/remapped queries solve, seven retained Native failed owners advance,
and instrumented rewrite C advances both new owners. Independent root motion
agrees within 3.58e-11 and 1.34e-10. The frozen previous solver rejects the new
first fixture. The adopted 268e3f9 source also passes all eleven original-data
ground levels (79,100 queries/26,400 body steps per target), actual input/canvas
checks (31 Native/31 sanitized/144 Chromium, no browser errors), and selected
short Circuit-2/full eight-lap Circuit-5 races on all three targets. The fresh
ordinary campaign completes round 1 and advances all second-round steps to the
unchanged 300,000-tick bound without a contact failure. It exits 1 without
timeout after 1,231.59 seconds, with zero player laps and zero round-2 scores.
Thirteen late observations show only 221.786 sampled XZ units of supported
upright travel in 300 simulated seconds. Bounded contact advancement does not
prove dense-region spatial escape or a complete season. Active 0044 now owns
the actual movement diagnosis; a read-only typed 280,000 reconstruction matches
all twenty next-copy controls and independent original road targets. The terminal
owner/pilot capture now matches all 120 records, thirteen movement observations
and both twenty-car snapshots. Ten ordinary control probes fill the contact
list and lose most proposed motion. A distant free spectator also loses motion
when unrelated contacts fill the report. Active 0045 owns this proved coupling.
Its private Native prototype preserves 64 CCD events and 64 constraints per group
while allowing all derived 4,096 records; forward control reaches 3,686.500 net
units in six seconds, compared with 55.746 previously. Two focused cases reject
the previous implementation and pass the prototype. Native and Emscripten
sizeof checks require owned heap buffers and borrowed views to retain the
1 MiB WASM stack. Production, multi-platform and tactical correction remain
open, as do the full game/seasons.
Receipts: /tmp/wasm-dd2/rewrite-five-root-0041/next-failure-diagnosis/production-report.json
and /tmp/wasm-dd2/rewrite-next-world-load-0042/component-report.json,
broader-report.json, natural-season/terminal-report.json,
movement-diagnosis/geometry-report.json, movement-owner-capture/capture-report.json,
movement-escape/summary-report.json and report-capacity-prototype/comparison-report.json.


Current per-pair profiling locates frequent adjacent-car refinement rather than
observed budget exhaustion through frame 600. A private shared-rotation candidate
reduces captured-field pair-query CPU by 17.1..20.4 percent and passes functional
pair/rotation comparisons and all three-target sixty-second checks on levels
1..A. It still fails Arena-B supported motion (Native slot 16: 1,419 frames;
sanitized slot 19: 1,362; required 2,280), despite full inventories/reset and a
passing WASM case. It is rejected and production geometry remains at 9511875.
Sixteen independent public vehicle-rotation checks are retained; final strict
LLVM19, 37 Native/35 WASM CTests, nine sanitizer targets and 716 physical checks
per target pass. The complete
contact-cost, support/traction and full-game contracts remain open. Receipts:
/tmp/wasm-dd2/rewrite-pair-distribution-0046/{diagnosis-report,ai/canonical-report}.json.

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

Current ordinary-season baseline at cae5001: fresh O3 ASan/UBSan completes four
natural Stockcar rounds and opens season 2. WASM instead rejects level-2 round-2
tick 137455. Its saved twenty-body field isolates a three-world-contact velocity
root failure in slot 13 on both Native/WASM; an independent mobility calculation
proves a stronger loaded linear-pressure root. The small-load direction is
preserved while the existing complete-root incident-pressure trial becomes
eligible for positive linear world contacts. The correction passes strict LLVM19/168-file checks,
38 Native/36 WASM CTests, 728 physical checks per target plus the independent
pressure root, fresh O1 ASan/UBSan on 34 reached units, the saved twenty-body
field on all three targets, and Valgrind with zero errors/leaks. Native's baseline
reaches the 300000-tick round-2 bound with zero player laps after 1799.94 seconds;
it is frozen prior-source evidence, not verification of the correction. All
baseline processes are terminal; successful raw logs/binaries are removed.
Receipts: /tmp/wasm-dd2/rewrite-linear-pressure-0047/ and the baseline
/tmp/wasm-dd2/rewrite-season-body-support-0016/canonical-report.json. Complete
movement and cross-platform ordinary campaigns remain open.

The frozen d0a1bd7 ordinary WASM season advances beyond tick 137455 but rejects
round-2 tick 155992; its exact typed checkpoint is retained under 0049.
Native's late movement reveals a false return into a blocked base lane. Closed 0048
retains immediate heading avoidance and checks the base path before merging.
A base-only variant fails Native level-1 support and is rejected. The final
source passes strict LLVM19/168-file checks, 38 Native/36 WASM CTests, both-target
old-source negatives, fresh 34-unit O3 ASan/UBSan and zero-error/leak Valgrind.
Its six-second public-field component moves over 4000 net units on all three
targets. The unchanged eleven-level AI suite passes 29214 independent path
queries and 2640000 vehicle steps per target. Component health is frozen; actual
corrected-source damage/recovery/lap/result campaigns remain pending under
0016/0044. The nineteen-contact velocity failure remains active under 0049.
Receipt: /tmp/wasm-dd2/rewrite-native-movement-0044/component-report.json.

Closed 0051 preserves warm normal branches inside incident world-pressure
refinement. The independent nineteen-contact root and exact twenty-body field
advance on Native/WASM/O1 ASan/UBSan, with 742 physical checks per target, strict
LLVM19/169-file checks, 38/36 CTests, previous-source negatives and zero-error/leak
Valgrind. All eleven unchanged original AI levels pass 29214 path queries and
2640000 vehicle steps per target. Complete owner outcomes remain under 0049;
full seasons are not proved. All frozen 570df9d ordinary baselines are terminal:
Native round-1/credited-lap9 bound and WASM/sanitized round-3/credited-lap7 bound,
without timeout/contact rejection. Closed 0050 corrects late-race movement and verifies naturally finished NPC
control in the actual three-target first Stockcar season. The independent Arena-A angular comparison was exactly unchanged before/after
0051; closed 0052 subsequently corrects its standalone serial contact path.
Receipts: /tmp/wasm-dd2/rewrite-next-round2-rejection-0049/component-report.json
and /tmp/wasm-dd2/rewrite-season-passing-return-0016/canonical-report.json.

Finished NPCs now continue steering after their natural completion. The original
circuit-5 eight-lap Stockcar race checks 1570 subsequent opponent steps and
21688.791 travel units on Native/WASM/O1 ASan/UBSan, with latched places/timing
and independently checked final points. Prior driving.c fails that new check at
the first opponent finish. All 45 selected race scenarios per target pass
(29 short physical, 14 ordered-rule, two long circuit-5 runs), along with strict
LLVM19/169-file and 38/36 CTest gates. Long Total Destruction completions remain
outside this scope. Native and WASM now naturally complete all four first-season rounds
(three finishes and one engine retirement each), consume correct actual points
and open season 2 at d7dfdd4. Wall times are 406.58/445.36 seconds with unchanged
bounds; the originally blocked round-1/round-3 approaches both complete.
Fresh 34-unit O3 ASan/UBSan now also completes all four rounds and opens season 2
within 1330.16 seconds, with unchanged 300000-tick/3600-second limits and source
identity. Closed 0050 proves these blocked finish approaches; broader modes,
leagues and seasons remain open. Closed 0052 restores the independent body-ground
comparison by using the existing simultaneous one-car joint solver. No renderer/menu/save or full-game/parity claim.
Receipts: /tmp/wasm-dd2/rewrite-finish-movement-0050/ and
/tmp/wasm-dd2/rewrite-season-finisher-movement-0050/.


Closed 0052 passes all eleven unchanged ground levels (79100 queries/26400
steps per target), all 220 controlled original-grid recovery cases per target,
strict LLVM19/169-file checks and 38/36 CTests. All 45 previously proved race
contracts remain passing on Native/WASM/sanitized builds. The full natural arena
suite fails separately: Native rejects Arena-8 owner step 10574 in position
correction, WASM exceeds the unchanged 1800-second limit and sanitized reaches
natural engine retirement. The exact Native capture records zero calls to the
changed world API. Active 0053 isolates this twelve-contact position problem;
velocity already converges. Receipts: /tmp/wasm-dd2/rewrite-ground-stability-0052/
{component-report,owner-report}.json and
/tmp/wasm-dd2/rewrite-arena8-rejection-0053/capture-report.json.


Active 0053 now has a verified component correction: one warm multiplier boundary
is released per refit, and released-row pressure is restored to its exact zero
identity equation. Independent enumeration checks the actual twelve-row position
root; 48 geometry reorder/remap cases, prior-source negatives and all 742 earlier
physical cases pass per target, including fresh O1 ASan/UBSan. Native Valgrind has
zero errors/leaks. Strict LLVM19/170-file and 38/36 CTest gates pass. All eleven original ground levels, 220 recovery cases and 45 prior race scenarios
pass again per target. On 6efaa44, Native Arena 8 reaches engine-retirement/coasting
results at tick 81219 (401.09 seconds survival), within the unchanged deadline;
fresh O1 ASan/UBSan completes at tick 13083 (60.41 seconds survival). WASM exceeds
the unchanged 1800-second deadline after 77251 complete ticks. Every complete
prefix row passes the independent state/damage/clock/pursuit oracle and advances
beyond the former rejection; natural WASM results remain unproved. Active 0053
therefore stays open while actual simulation cost is measured and addressed.
Receipt: /tmp/wasm-dd2/rewrite-arena8-rejection-0053/terminal-owner-report.json.

Closed 0054 removes redundant internal contact scratch zero writes while fully
initializing live counted entries and degenerate-axis selection flags. Eight
actual 6000-tick prefixes preserve every per-target field. Two reversed-order
comparisons reduce child CPU by 1.48..1.92 percent on Native and 5.50..6.01
percent on the private named WASM link; these are prefix costs only. Nine fresh
patterned O1 ASan/UBSan corpora (24 shared units), six Native Memcheck corpora,
all 742 physical checks, strict LLVM19/170-file and 38/36 gates pass. All eleven
original ground levels, 220 recovery cases and 45 prior race scenarios pass again
per target. At source 0323ba9, the selected actual Native and fresh O1 sanitized
owners again reach natural results at ticks 81219/13083, with 401.09/60.41 seconds
survival. WASM times out after 86042 complete ticks (428.21 seconds survival,
twenty available engines). Every complete prefix row passes the independent
oracle; the full public field at tick 77251 matches the prior source exactly.
These are prefix/checkpoint preservation, not natural WASM results or an isolated
whole-case speed comparison. Active 0053 remains open. Completed raw output and
private binaries were removed after retaining hashes, receipts and typed fields.
Receipts: /tmp/wasm-dd2/rewrite-arena8-profile-0053/
{terminal-owner-report,late-checkpoint-report,terminal-cleanup-report}.json.
Receipt: /tmp/wasm-dd2/rewrite-arena8-profile-0053/verification-report.json.

A further 63-key original run saves distinct configurations A/B, deletes A, loads
B from the first visible slot and saves C into the next visible empty slot. Ten
actual X11 windows and six bounded card checkpoints are reviewed. Logical
entries compact occupied physical headers in ascending order: B remains in
physical block 1 after A is deleted but appears at logical slot 0. Saving C uses
first-free physical block 0, producing visible C/B order. Deletion retains both
payload blocks; loading preserves every byte, and C leaves B unchanged. The
provisioned save remains unchanged. Receipt:
/tmp/wasm-dd2/rewrite-frontend-multiple-card-0008-3/report.json.
This proves these original configuration paths and mapping, not game/replay
codecs, corrupt/full/failing storage or accepted rewrite persistence.

Closed 0055 now owns the original Windows save-card container. Sixty-six
production Native/WASM/fresh O1 sanitized comparisons prove exact image/entry
reads, complete physical payload selection, actual put/delete transitions and
compacted duplicate rejection. An actual 68-key original run reproduces B/B
creation after compaction and loading the wrong physical B; existing duplicate
images remain readable in the new container without creating new ambiguity.
Strict LLVM19/174-file, 39/37 gates and Native Memcheck pass. Typed payloads,
durable adapters and frontend integration remain open under 0008/0003. The
frozen Arena-8 binary separately collects late motion/contact evidence under
0053. Receipt: /tmp/wasm-dd2/rewrite-save-card-0055/verification-report.json.

The bounded late Arena-8 WASM observer stops as planned at tick 83591 without
natural results. Its full public tick-77251 checkpoint matches production exactly.
Across ticks 70000..83591, the field averages 58.411 response events per step;
the player travels 24557.623 units with nearly all wheel steps supported and only
five damage-eligible contact records. Late V8 samples identify fleet resolution,
static car SAT and source-surface traversal, including inlined callees. This is
scoped moving-field/cost evidence, not retirement or an isolated speed comparison.
Active 0053 now needs one actual late fleet step with previous/proposed typed
bodies and contact times/rows to distinguish repeated supports from legitimate
dense pursuit. Budgets/material/health/controls remain unchanged. Completed raw
profiling output is removed; three required typed checkpoints and receipts remain.
Receipt: /tmp/wasm-dd2/rewrite-arena8-late-0053/diagnosis-report.json.


Closed 0056 now owns the complete source configuration/profile payload: typed
settings, five seasons, twenty current drivers, ten player names, seven lap
tables and bindings, with retained reserved bytes. Three hundred independent
Native/WASM/fresh O1 sanitized comparisons check 50 original packs/cards,
306300 typed values and complete encoded blocks, including predicted edits.
Strict LLVM19/178-file and 40/38 gates, all truncated extents, 320 invalid fixed
texts, overlap/rollback and zero-error/leak Memcheck pass. Controller type is
explicit; source configuration does not contain a separate Redbook gain.
Complete playable-state translation, durable Native/browser publication and
actual frontend save/load remain next under 0008/0003. Broader Arena-8 natural
WASM results remain active under 0053. Receipt:
/tmp/wasm-dd2/rewrite-save-profile-0056/verification-report.json.


Closed 0057 adds the main-thread polling save-store owner and actual Native/
IndexedDB persistence. Ninety-two Native/fresh O1 sanitized cases and 25 actual
Chromium checks compare complete independently predicted card images, including
physical compaction, full replacement, corrupt storage, short/interrupted writes,
real killed Native writers, stale owners, transaction abort/exception and full
browser-process restart. Pending operations keep accepted memory and refuse
close/destroy. Post-rename sync failure explicitly requires reload; it cannot
report durable success. Strict LLVM19/185-file and 41/39 gates pass, including
the active WASM backend branch and zero-error/leak Native Memcheck. Original data
and pinned SoftGL remain unchanged; completed raw output is removed after reports.
Actual settings application, playable-session validation, replays, rewrite music
gain and frontend save/load remain under 0008/0003. Arena-8 natural WASM results
remain active under 0053. Receipt:
/tmp/wasm-dd2/rewrite-save-store-0057-final/verification-report.json.
