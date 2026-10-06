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

Menus, race rules, seasons, results, replay/settings state and audio still need
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
This is free driving: distinct liveries/vehicle classes, damage, checkpoints,
race outcomes, menus, championships, replay, persistence and audio remain pending.

The driving owner also owns copied source barriers and their collision index.
Each accepted vehicle step resolves body/road, barrier and car-pair contacts on a
shared event clock before committing frame state. Frame failure preserves every
vehicle, wheel roll and collision counter; reset clears both contact and player
car-pair counters. Rendering borrows the corrected poses. A main-thread read-only
collision-count bridge lets the browser verification observe a real impact;
original addresses or synthetic game input are not used. Damage and collision
sound/effects will consume richer impact events in subsequent gameplay work.

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
overturned/off-road recovery, race rules and damage integration remain pending.
