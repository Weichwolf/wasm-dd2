# Game

Own menus, race/season transitions, results, settings and replays as typed state.
Depend on explicit input, timing and subsystem interfaces. Original behavior is
the functional reference; executable addresses and register globals belong to
the reference reconstruction.

`application.c` currently owns an interactive track/car inspection viewer,
sharing loading, selection, camera, drawing and input logic between native and
browser entry points. A track switch is transactional: failure leaves the
previous track/materials/camera active. Destruction releases cached textures
before the renderer and all borrowed archive views before the owned file bytes.
The single active browser bridge is cleared before destruction; exported
selectors reject invalid requests without changing active state.

This is not a driving game. Menus, race rules, seasons, results, replay/settings
state, simulation and audio still need their gameplay implementations.
