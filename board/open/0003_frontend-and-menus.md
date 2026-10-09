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

A further actual original run opens/cancels Information and Configuration.
The initial visible actions are View Lap Times and Select Control Method. Seven
real X11 actions preserve both selected main entries and the isolated/provisioned
save files. Source excerpts separately establish the statistics-availability
navigation guard and initial captions. Completed raw images/logs are removed;
interpretations, state transitions and hashes remain at
/tmp/wasm-dd2/rewrite-frontend-info-config/report.json.
Entry/cancel evidence does not prove subordinate actions or rewrite menus.

A subsequent 19-key original run opens/cancels the lap-times table, Credits,
Control Method/Keyboard remapping prompt and Audio Volume. Escape preserves
parents and the original main selections; both save files remain unchanged.
The bounded screen observations are interpreted and identified at
/tmp/wasm-dd2/rewrite-frontend-submenus/report.json. Remapping submission,
volume changes, record updates, joystick actions and persistence remain unproved.

Closed 0034 supplies owned original font glyph data. Every byte of the three
96-character tables matches Native/WASM/ASan and all six loaded original
base/duplicate tables. Source lifetime and twelve corrupted banks are checked.
Receipt: /tmp/wasm-dd2/rewrite-font-verification-complete/report.json.
Sprite/font texture bindings and original shared menu rendering remain open.

A seventeen-key original File Manager run now observes the 0/1/4/3/0 slot
navigation sequence, empty-load error and empty-delete return without confirmation.
Both save copies remain unchanged. A separate eight-key actual X11 client-window
run visibly confirms the red selected slot, fifteen-slot grid, ERROR / Not a DD2
file dialog and retained main-menu selection after Escape. Internal framebuffer
reads in the first run are torn; visible claims use the separate window capture.
Receipts: /tmp/wasm-dd2/rewrite-frontend-file-manager-errors-confirmed/report.json
and /tmp/wasm-dd2/rewrite-frontend-file-manager-window/report.json.
This establishes the empty-slot routes; it does not implement rewrite menus.

A subsequent 38-key original run saves Configuration as A, cancels an occupied
overwrite, commits a staged Sound Effects edit from 4090 to 3681, reloads A to
restore 4090, cancels deletion and confirms deletion. Nine actual X11 windows
are reviewed. Six bounded card checkpoints match the original's in-memory card;
load and both cancellations preserve all bytes. Confirmed deletion clears the
occupied header/name prefix but retains the old payload. The provisioned save
is unchanged. Receipt:
/tmp/wasm-dd2/rewrite-frontend-occupied-card-0003-5/report.json.
This is original configuration-route evidence, not implemented rewrite menus or
accepted occupied overwrite, game/replay data, restart or storage-failure proof.

A further 58-key original run proves same-name occupied overwrite, plus cancel
at the retained-A filename prompt after Yes. Cancellation preserves every old
card byte; accepted replacement preserves headers and writes the updated packed
configuration. Committed Sound Effects are saved at 3272, edited to 3681 and
restored to 3272 by load. Twelve actual X11 windows and eight bounded card
checkpoints are reviewed; the provisioned save remains unchanged. The driver
waits for completed replacement, including the original delete-before-save
intermediate state. Receipt:
/tmp/wasm-dd2/rewrite-frontend-overwrite-card-0003-5/report.json.
Renamed overwrite, multiple occupied slots, game/replay data, restart, storage
failures and rewrite menus/storage remain open.

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

A 68-key actual-original follow-up exposes a filename ambiguity after display
compaction. With only B in physical block 1 and logical slot 0, saving another B
from logical empty slot 1 succeeds and places the new B in physical block 0.
The two configurations contain distinct effects volumes (3272/3681). Selecting
the second visible B for load restores 3272 from the first physical B, rather
than 3681 from the selected block. Ten actual X11 windows and seven bounded card
checkpoints are reviewed; provisioned data remains unchanged. Receipt:
/tmp/wasm-dd2/rewrite-frontend-duplicate-card-0008-1/report.json.
The frozen duplicate check excludes a logical index as though it were physical;
load searches by filename. Rewrite uniqueness must use physical identity, and
loading must consume the selected entry's payload. Preserve existing duplicate
images for recovery while preventing new ambiguous names. This is a reproduced
original bug, not accepted rewrite frontend behavior.

Closed 0055 now supplies assets/save_card.h: an owned opaque Windows container,
compact logical-to-physical entries, complete payload views and staged borrowed
put/delete inputs. Production Native/WASM/fresh O1 sanitizer pass 66 independent
original image/payload/mutation/duplicate comparisons and synthetic bounds,
ownership, reserved-byte preservation and rollback. Strict LLVM19/174-file,
39/37 gates and Native Memcheck pass. The reproduced original duplicate-name bug
is prevented for new entries while existing duplicate payloads remain distinct.
This does not complete typed settings/game/replay, durable adapters or frontend
persistence. Receipt: /tmp/wasm-dd2/rewrite-save-card-0055/verification-report.json.

Active 0058 now provides Native/browser-canvas F2 name entry, F3 save selection/
filename/occupied confirmation and F4 reload/restore, plus HTML player/audio
controls. Thirty-two actual Chromium checks and 22 Native/sanitized X11 dialog
checks prove typing, all fifteen positions, cancel, completion, failure/conflict
and restart. Completed-save Escape dismisses without undoing data. Accepted HTML
name/restore actions synchronize the field before another edit; otherwise drafts
survive reflection. Modal panes reuse the paused world's image rather than
redrawing the entire circuit per character. Named championship standings have
a synthetic GL check; naturally completed named results remain unproved.
This is a partial front end, not every original menu or saved-game action.
Receipt: /tmp/wasm-dd2/rewrite-player-profile-0058-release/verification-report.json.

## Next

Continue original navigation to resolve remaining submenus, File Manager
renamed overwrite, multiple occupied slots, game/replay data and storage-failure paths,
CD transport and multiplayer input/turn topology.
Complete the route inventory,
decode its assets and implement typed navigation/actions using docs/frontend.md.
Connect real championship, persistence, replay, input and audio owners.

## Accept

Every menu entry is exercised through actual native/browser input with visible output and correct game-state changes, including back/cancel, failure and reload paths. No inactive placeholder actions remain.
