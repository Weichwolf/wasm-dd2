Type: Work item
Title: Complete front end and menu actions
Depends: 0004, 0008

## Contract

Provide every original menu flow, real actions, scores, navigation, car/track/mode selection and pause/results behavior in the shared Native/browser application.

## Evidence

The current application exposes inspection, free driving and practice modes, not the complete front end. The user reported graphic defects, buttons doing nothing and broken championship scores in the reference. Original LEVF menu/font assets are available, but full decoding and menu ownership remain open.

Static original-image/frozen-source inventory now records all eight main slots,
their bounded directional graph and seventeen related source functions.
The race selectors establish circuit/arena/car-count constraints, championship
track locking and one-to-ten multiplayer name entry. Track cancel restores the
entry selection; confirm-release gating prevents successive held confirmations.
See docs/frontend.md and /tmp/wasm-dd2/rewrite-frontend-inventory/report.json.
The subsequent unmodified-original Wine observation opens CARD's File Manager
Load view (fifteen slots), returns through Escape and opens LINK's CD Audio Player
with title/group and previous/play/stop/next controls. Ten actual X11 key actions,
read-only observations and an isolated save are recorded; the provisioned save
is unchanged. Receipt: /tmp/wasm-dd2/rewrite-frontend-card-link/report.json.
This resolves those entry routes, not save/delete or accepted audio playback.

## Next

Continue original navigation to resolve remaining labels, submenus, File Manager
save/delete/failure paths, CD transport and multiplayer input/turn topology.
Complete the route inventory,
decode its assets and implement typed navigation/actions using docs/frontend.md.
Connect real championship, persistence, replay, input and audio owners.

## Accept

Every menu entry is exercised through actual native/browser input with visible output and correct game-state changes, including back/cancel, failure and reload paths. No inactive placeholder actions remain.
