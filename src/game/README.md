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

`driving.c` owns a surface index and vehicle state, borrowing immutable road
geometry until destruction. Original first-grid positions use runtime strip IDs
(main loop first, then alternate branches), selected lanes and byte headings;
arenas use their original positions with decoded ground heights. No executable
image or original addresses enter runtime game code. Reset aligns the body to the local road normal, then settles suspension
for one second with the brake held and restores that deterministic start.

Validated frame input accumulates into fixed 5 ms steps; each frame is bounded
at 250 ms and commits state only after all steps succeed. Pause/focus loss drops
partial elapsed time. The application maps real keyboard input to free driving,
with independent inspection camera modes. Enter toggles driving; W/Up gives gas,
S/Down reverses, A/D or arrows steer, Space brakes, P pauses, R resets and
Page Up/Down changes track. Browser controls call the same main-thread C bridge.

`make rewrite-driving-verify` covers independent original start positions,
frame partition equivalence, pause/reset/rejection and actual native/browser
input/presentation. This is free driving: opponents, car-pair collision, damage, checkpoints,
race outcomes, menus, championships, replay, persistence and audio remain pending.

The driving owner also owns copied source barriers and their collision index.
Each accepted vehicle step resolves body/road and barrier contacts together before committing frame
state. Frame failure preserves both the vehicle and collision counter; reset
clears the counter. Rendering borrows the corrected pose. A main-thread read-only
collision-count bridge lets the browser verification observe a real impact;
original addresses or synthetic game input are not used. Damage and collision
sound/effects will consume richer impact events in subsequent gameplay work.

Reset settling also uses the combined collision solver. Body support uses the
original eight-corner contact box, so inverted and sideways bodies remain on the
road instead of falling through it. Ground impacts are inelastic and use tuned
friction. `make rewrite-ground-verify` checks source geometry, swept selection,
body drops and existing free-driving presentation on all three targets.
