# Player, car, audio and retained configuration

`configuration.h` owns the complete original profile and an independent rewrite
music gain. CONFIG and STARTUP imports validate the complete codec, the consumed
effects volume and any recognized music extension before publishing a value.
GAME and REPLAY are separate formats and cannot restore audio preferences.
Dormant mode, track, input, other players, records and season fields are retained;
they are not applied as playable configuration by these actions. Full
player/car/audio actions additionally validate and consume the first player name
and the signed source car field: 0 Rookie, 1 Amateur, 2 Pro. Audio-only restore
keeps the active human name and class unchanged, including when the retained
source car field is invalid. The lossless codec does not reject dormant fields.

Profile saves capture the live human class and name, independently of retained
source metadata. A different-class restore prepares a complete physical field
with the current track and practice mode before publishing class, name and audio.
Failed validation or allocation preserves every live owner. During driving,
a different class is rejected; a same-class restore keeps the existing physical
field and motion. An owned championship rejects full profile restores. These
actions do not restore an entire saved race or championship.

New name entry accepts up to eight printable ASCII characters, as the original
name editor does. Empty input resolves to `PLAYER`. Imported names may use all
eleven characters in the original twelve-byte field. A legacy name survives
restore/save and editor cancellation without truncation. Explicit name edits
replace only the bytes through the terminator, retaining subsequent field bytes.
The application owns the human name; NPC stable IDs and names stay immutable.
Championship standings use the active human name, and an owned championship
rejects name changes and player-profile restores. Audio-only restore remains
available during a championship. This does not restore a saved championship.

Factory defaults reproduce the original packed configuration's first 6526 bytes,
including source keyboard bindings and lap records. The independent x86 fixture
calls the immutable original packer with the settings and zero profile regions
observed in the earlier read-only original startup. It proves packing/default
bytes, not that the original executes the rewrite's load action.

Original effects volume has range 0..4090. The mixer and UI use 0..256, rounded
to the nearest integer. Import retains the exact source value: restoring 3681
shows 230 but saving without an effects edit still writes 3681. Editing the
effects slider adopts its quantized value. Original `controller_type` remains
an input setting and is never used for music gain.

An explicit save emits CONFIG and updates sixteen bytes at payload offset 6526.
The remaining 1650 reserved bytes and all unrelated profile fields stay intact.

| Offset within extension | Encoding |
| --- | --- |
| 0 | Four-byte ASCII tag `D2CF` |
| 4 | Little-endian u16 version, currently 1 |
| 6 | Little-endian u16 record length, 16 |
| 8 | Little-endian u16 music gain, 0..256 |
| 10 | Little-endian u16 flags, currently zero |
| 12 | Little-endian u32 FNV-1a checksum of the preceding twelve bytes |

FNV-1a starts at 2166136261 and multiplies by 16777619 with unsigned 32-bit
wrapping after each byte XOR. It covers the extension only, so original edits to
the source prefix do not invalidate music metadata. Unknown legacy suffixes
retain the current music gain on import. A recognized tag with an invalid gain,
version, length, flags or checksum rejects the import without changing live audio.
The original packer leaves this suffix untouched; original playback ignores it.

The actual application lazily opens a dedicated save store. Native C callers use
`dd2_application_saves_open` with an existing writable folder; the browser's
**Saved player, car and audio** controls use IndexedDB database `wasm-dd2-saves-v1`.
Save, delete and reload acceptance must be followed by polling a terminal result.
Selected logical entries resolve directly to their physical payload, including
legacy duplicate names. Imported preferences apply both mixer gains under one
device lock and preserve the active race/championship. Missing audio hardware
still permits owning and saving preferences. Application close refuses pending
storage, and terminal completion remains observable after close until a new
application starts.

Native F2 edits the human name. F3 selects any of fifteen entries and edits a
save filename; replacing an occupied entry requires a separate confirmation.
F4 explicitly reloads the complete file and selects an entry to restore player,
car and audio. Delete explicitly reloads the inventory, selects one of fifteen
logical entries and requires a separate confirmation of its name and position.
It deletes CONFIG/STARTUP/GAME/REPLAY entries without loading their payloads;
duplicate source names still identify the selected physical entry. Completion
appears only after durable storage acknowledges the full candidate image.
Escape cancels drafts and confirmations without changing stored data.
Opening/writing must reach terminal completion before the dialog can dismiss.
Dialogs suspend simulation and music and reuse the frozen world image underneath
their opaque pane, so typing does not repeatedly render the whole circuit.
Publishing a restored class invalidates that retained image once, so the open
completion dialog immediately shows the restored paint and ratings. The
Native store uses SDL's per-user `Weichwolf/wasm-dd2` preference folder unless
a C caller has already opened a dedicated location. It never uses game assets.
The same SDL dialogs work on the focused browser canvas; browser controls are
disabled while a modal owns input. The HTML controls also expose player/car/audio
save and restore separately from audio-only actions.
Accepted HTML name/restore actions immediately synchronize the input and its
reflection cache. A later presentation frame cannot replace a new edit with
the preceding restored name. Unsaved drafts otherwise survive frame reflection.

`make rewrite-preferences-verify` checks original factory bytes, all source
effects values, malformed extension rollback, complete independently predicted
Native/browser cards, process restart and sanitized ownership. Actual Native
application actions prove restored engine-voice gain and retained navigation/
championship state. Actual browser controls prove cancellation, failed writes,
stale owners, pending close and restored Redbook gain in WebAudio PCM. Memcheck
checks the Native application. `make rewrite-player-profile-verify` compares
unmodified original driver rosters, retained complete profiles, actual X11 and
Chromium dialog input, cancellation, overwrite confirmation and process restart.
The checks include all three classes, fresh browser/Native processes, actual
F1/F3/F4 class restoration, consumed-class rollback, an allocation-failed Native
candidate, independent audio-only actions, fresh sanitizers and Native
application Memcheck. A focused
SoftGL test covers all printable ASCII glyphs and custom-name standings; a
synthetic standings test does not prove a naturally completed named season.
Full configuration consumption and playable saved championships remain under
active 0058.
