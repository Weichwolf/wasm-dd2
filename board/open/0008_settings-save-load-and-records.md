Type: Work item
Title: Settings, save/load and original data compatibility

## Contract

Persist and restore profiles, names, championships, unlocks, settings and track records with original format compatibility and atomic failure handling.

## Evidence

Session Time Trial clocks work. Persistent records/settings and the full save owner are absent. Original SaveGames card layout and reference fixtures exist, including fifteen slots and source championship/statistics regions.

An unmodified-original 38-key run saves Configuration as A, stages and commits
Sound Effects from 4090 to 3681, and loads A to restore 4090. The first 0x197e
payload bytes match the original packed configuration, beginning with magic
0x1010; the card has fifteen 0x200-byte headers and 0x2000-byte payload slots
within 0x20000 bytes. These are observed configuration bytes, not a complete
typed codec. Occupied overwrite/delete cancellation and load preserve every
card byte; accepted deletion clears the occupied marker/name prefix and retains
the old payload. Nine X11 windows are reviewed and the provisioned save remains
unchanged. Receipt:
/tmp/wasm-dd2/rewrite-frontend-occupied-card-0003-5/report.json.
Game/championship/replay data, accepted overwrite, multiple occupied slots,
corrupt/full/failed storage and rewrite adapters remain unproved.

A subsequent 58-key original run proves accepted same-name configuration
replacement and filename cancellation after accepting Overwrite. Cancellation
preserves all bytes; accepted replacement preserves all headers and writes the
new packed configuration exactly. Another committed volume edit followed by load
restores the overwritten value (3272 -> 3681 -> 3272). Twelve actual windows and
eight bounded card checkpoints are reviewed; the provisioned save is unchanged.
The original deletes the old entry before SaveCardFile writes its replacement;
the driver waits for completed occupancy/payload. This is observed original
behavior, not atomic rewrite storage acceptance. Stage atomic replacement in
the new adapter. Receipt:
/tmp/wasm-dd2/rewrite-frontend-overwrite-card-0003-5/report.json.
Renamed replacement, multiple slots, game/replay data, restart, storage failures
and Native/browser codecs/adapters remain unproved.

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

Closed 0056 adds assets/save_profile.h: owned, complete typed configuration/
profile blocks, including five season records, current drivers, player names,
seven lap tables and bindings. Three hundred independent original-x86/actual
card comparisons check 306300 values and full blocks on Native/WASM/fresh O1
sanitized C, including predicted edits; strict LLVM19/178-file, 40/38 gates and
zero-error/leak Memcheck pass. Truncation, all 320 unterminated text fields,
unsupported kinds and alias/rollback checks pass. The source controller word at
byte 18 is not Redbook gain; the original configuration packer has no separate
CD gain. Parsing preserves numeric fields but does not validate a playable race
or championship. No durable write, restart or frontend persistence claim.
Receipt: /tmp/wasm-dd2/rewrite-save-profile-0056/verification-report.json.

## Next

Translate and validate complete typed source configuration/game state, stage durable Native/browser publication and connect actual frontend actions. Replays need a separate codec. Keep selected physical identity through loading, preserve prior application state on invalid gameplay/failed writes, and prevent new ambiguous names after display compaction. Persist rewrite music gain explicitly because the source configuration does not store it.

## Accept

Independent original-file reads and write/read round trips cover valid files, full slots, corrupt/truncated input, failed writes and repeated reloads on both targets without partial state mutation.
