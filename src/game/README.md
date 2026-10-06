# Game

Own menus, race/season transitions, results, settings and replays as typed state.
Depend on explicit input, timing and subsystem interfaces. Original behavior is
the functional reference; executable addresses and register globals belong to
the reference reconstruction.

`application.c` currently owns an interactive track/car inspection and free-driving application,
sharing loading, selection, camera, drawing and input logic between native and
browser entry points. A track switch is transactional: failure leaves the
previous track/materials/camera active. Destruction releases cached textures
before the renderer and all borrowed archive views before the owned file bytes.
The single active browser bridge is cleared before destruction; exported
selectors reject invalid requests without changing active state.

Menus, race phases/mode rules, seasons, results, replay/settings state and audio still need
their gameplay implementations.

`driving.c` owns a surface index and twenty vehicle states, borrowing immutable road
geometry until destruction. `starting_grid.c` generates all original grid slots using runtime strip IDs
(main loop first, then alternate branches), selected lanes and byte headings;
arenas use full rings or the B arena's half ring with decoded ground heights.
The special SCA grid is a typed twenty-entry source table. Arena angles use
continuous trigonometry; original integer rounding is not required. No executable
image or original addresses enter runtime game code. Reset aligns every body to
its local road normal, then settles all twenty suspensions for one second with
the brakes held and restores that deterministic start.

Validated frame input accumulates into fixed 5 ms steps; each frame is bounded
at 250 ms and commits state only after all steps succeed. Pause/focus loss drops
partial elapsed time. The application maps real keyboard input to free driving,
with independent inspection camera modes. Enter toggles driving; W/Up gives gas,
S/Down reverses, A/D or arrows steer, Space brakes, P pauses, R resets and
Page Up/Down changes track. Browser controls call the same main-thread C bridge.

`make rewrite-driving-verify` covers independent original start positions,
frame partition equivalence, pause/reset/rejection and actual native/browser
input/presentation. Other nineteen cars drive by default, are rendered
with the shared car/wheel models and react to collisions. Their decisions borrow
one simultaneous field before physics advances any body. Controller state commits
with the frame and resets with every vehicle; pause also freezes all decisions.
`dd2_driving_set_opponents` can disable decisions for stationary collision probes.
This is free driving with checkpoint/lap tracking: distinct liveries/vehicle classes,
race outcomes, menus, championships, replay, persistence and audio remain pending.

The driving owner also owns copied source barriers and their collision index.
Each accepted vehicle step resolves body/road, barrier and car-pair contacts on a
shared event clock before committing frame state. Frame failure preserves every
vehicle, wheel roll and collision counter; reset clears both contact and player
car-pair counters. Rendering borrows the corrected poses. A main-thread read-only
collision-count bridge lets the browser verification observe a real impact;
original addresses or synthetic game input are not used. Regional damage now
consumes every step report before frame state commits; collision sound and
additional effects remain pending.

Reset settling also uses the combined collision solver. Body support uses the
original eight-corner contact box, so inverted and sideways bodies remain on the
road instead of falling through it. Ground impacts are inelastic and use tuned
friction. `make rewrite-ground-verify` checks source geometry, swept selection,
body drops and existing free-driving presentation on all three targets.

`make rewrite-fleet-verify` independently checks all 220 source grid slots and
three-second reverse/turning drives on Native, Node/WASM and ASan/UBSan. Every
body is sampled, reset restores all twenty, and a level 1 impact must move the
other car. Actual browser input also exercises a car-pair impact and reset.
Collision counts describe solver responses, not unique accident events or damage
credits. Championship driver-to-slot ordering remains separate race work.

`make rewrite-ai-verify` checks the shared driving controller and source-linked
guidance, including sixty-second full-field scenarios on every level. Frame
partition tests compare every vehicle, wheel roll and AI state. Driving tactics,
overturned/off-road recovery and race rules remain pending. The long AI
diagnostics disable damage to isolate controller behavior.

The driving owner now receives the complete contact report inside every fixed
step, alongside the aggregate counters. `dd2_driving_contact_report` borrows the
last fixed step of the last successful frame for diagnosis. A reset or successful
frame with no fixed steps clears that snapshot; a rejected frame preserves it.
Frame-partition tests compare contacts as well as vehicles/controllers. Gameplay
modules must consume each report inside the fixed-step transaction, rather than
reuse the diagnostic snapshot as an event queue or lose intermediate impacts.

Damage state commits with the complete vehicle/controller frame. Reset restores
all six regions and engine health for every car; pause leaves damage unchanged.
The renderer borrows player/opponent damage for body deformation and the player
HUD. Read-only application bridges expose player engine health and each region
for browser checks with real keyboard impacts and reset. Retired vehicles remain
collidable wrecks. Detached parts, smoke and repairs are pending.

## Accident points and attribution

`accidents.h` owns per-driver race accident points, destruction counts and the
currently attributed victim window. The original `CheckPointScoring @00433d34`
credits 90/180/360-degree spins with 10/25/50 points, destruction with 25 points
plus one destruction, and caps points at 999. Source pair collision handlers
`@0043a096`/`@0043a230` arm both untracked, living victims for 75 game ticks.
The source main loop processes those ticks every 20 ms; the rewrite represents
this 1.5-second interval with 300 fixed 5 ms steps.

Attribution latches to the first responding partner and does not refresh on
repeated solver events or transfer to later contacts. The signed shortest heading
changes accumulate within the window; the largest 90/180-degree threshold reached
is retained even if the victim turns back. A complete spin scores immediately,
other thresholds score on expiration. Retirement during the window instead gives
destruction credit; a retired instigator cancels the credit. Wrecks cannot arm new
windows or resurrect without reset. Standing contacts/repair chatter are ignored
using the same tuned speed >500 and impulse-per-mass >250 gates as regional damage.
Near-vertical forward axes retain the last horizontal heading. These sampling and
collision gates are rewrite tuning; this is not original fixed-point parity.

The driving owner consumes each full contact report after damage in the same
transaction. Scores, windows and their tick counters commit with all twenty
vehicles/controllers, remain unchanged on frame failure/pause and clear on reset.
The diagnostic contact snapshot is not used as a delayed event queue. PTS/KO are
rendered from borrowed player state; read-only application bridges expose points,
destructions and the number of active windows for actual browser checks.

`rewrite_accidents` tests both spin directions, wrap, peak retention, no stolen or
refreshed attribution, expiration, destruction/cancellation, repeated wreck hits,
score saturation, harmless contacts and transactional rejection. Driving tests
compare every scoring field for equivalent frame partitions.
`make rewrite-accidents-verify` replays original-track physical observations and
actual contacts through an independent angle-history/deadline oracle on Native,
WASM and ASan/UBSan. Collision encounters may vary between targets; each target's
actual observations must satisfy the same scoring rules. Lap tracking is described
below; race outcomes, driver/class mapping and championship points remain separate work.

Two controlled side-impact cases on original arena 8 verify actual 180/360-degree
awards and credited engine retirement from the shared physical/damage pipeline.
These assigned high-speed stress inputs are independent owned fields; they do
not mutate game data or demonstrate normal driving/original handling parity.
The ordinary six-second game-owner profiles exercise attribution/expiration
and may finish without point awards. Synthetic rules cover the 90-degree tier
and 999 cap separately.

## Course progress and laps

`course.c` owns one finish-equivalent progress number per racing strip and borrows
the immutable road. It follows `Init_Track_Strip_Numbers` separately from the
starting-grid IDs generated by `FUN_00426be4`. At each main-route split, the
shorter path determines the progress count; consecutive strips on the longer arm
share units through the source 16.16 rounding. Nested split nodes inside an arm
are numbered as ordinary nodes. Endpoint rounding is bounded at the merge for
small synthetic paths; the original courses do not reach that rounding edge.
Missing merges, repeated numbering, uncovered nodes, fewer than three progress
units (ambiguous finish neighbors) and invalid rules fail with
bounded traversal. Source road bytes, raw numbers and main-loop IDs do not change.

The seven finish equivalents are 267, 560, 340, 319, 23, 243 and 449; the default
lap counts are 10, 5, 5, 5, 8, 7 and 5. These are typed format constants checked
against the original executable by the independent verifier. Course lengths
must be computed at runtime: 280, 604, 384, 326, 234, 247 and 450. The static
executable's nominal lengths for levels 3, 4, 6 and 7 differ. Runtime game code
never accesses the executable image. Arenas have no lap course/state.

`laps.c` gives each driver the sequential checkpoint and two lap counters used
by `Calc_Track_Positions`. Reset seeds the grid cell's finish-relative progress.
The first forward finish crossing starts lap 1 without crediting the partial
grid approach. Each subsequent lap needs every equivalent in order. Missing
units block the next lap until they are collected. Reverse finish crossings
remove credited progress, and re-crossing restores it without another lap or
timer reset. The required full laps latch an individual finish flag and tick.
Lap durations/records exclude the grid approach and use fixed 5 ms ticks.
Retirement freezes progress; finished drivers retain their progress/timing.
Overflow, malformed observations/state and resurrection without reset fail
transactionally. Ordered contacts are borrowed only during a call.

The driving owner samples actual center motion at intervals no larger than 32
world units inside each 5 ms step. Each sample uses the shared surface index with
a support window from center Y minus 380 to center Y, preserving bridge height
separation and shared-edge preference. Unsupported samples cannot grant progress;
AI targets or global nearest-road guesses are never substituted. Exceptional
motion needing more than 64 samples earns no progress. These geometry/sample
bounds are rewrite tuning. All twenty lap states commit with vehicles, damage,
AI and accident points; rejected frames preserve them, pause freezes them and
reset restores the grid approach. The LAP current/required and FIN overlay
borrows player state. Read-only browser bridges expose the current/required lap,
completed laps, active-lap ticks and individual completion.

`make rewrite-laps-verify` checks every original progress number and all 32
combinations of main-route branch choices through complete ordered rule traces.
Six-second physical starter fields supply 168,000 independently checked vehicle
steps per target on Native, Node/WASM and ASan/UBSan; another field checks frame
partition equivalence. An independent Python geometry oracle locates the actual
motion contacts and replays sequential checkpoints/timers. Synthetic tests check
reverse/re-crossing, missing checkpoints, off-road observations, retirement,
completion freeze, malformed state and overflow. Actual window/browser checks
exercise start-line crossing, pause/reset and the overlay; pixel checks cover
both LAP and FIN. Ordered full-lap traces establish rules/data coverage, not a
physically driven full race. Countdown, finishing order, race endings/results,
mode-specific rules, championships and persistent track records remain pending.
