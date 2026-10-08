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
This does not prove live menu behavior or resolve CARD/LINK action semantics.

## Next

Click through the original front end to resolve rendered labels, CARD/LINK,
submenus and multiplayer input/turn topology. Complete the route inventory,
decode its assets and implement typed navigation/actions using docs/frontend.md.
Connect real championship, persistence, replay, input and audio owners.

## Accept

Every menu entry is exercised through actual native/browser input with visible output and correct game-state changes, including back/cancel, failure and reload paths. No inactive placeholder actions remain.
