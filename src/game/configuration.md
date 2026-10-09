# Audio preferences and retained configuration

`configuration.h` owns the complete original profile and an independent rewrite
music gain. CONFIG and STARTUP imports validate the complete codec, the consumed
effects volume and any recognized music extension before publishing a value.
GAME and REPLAY are separate formats and cannot restore audio preferences.
Dormant mode, car, track, input, identity, records and season fields are retained;
they are not accepted or applied as playable configuration by these actions.

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
**Audio preferences** controls use IndexedDB database `wasm-dd2-saves-v1`.
Save, delete and reload acceptance must be followed by polling a terminal result.
Selected logical entries resolve directly to their physical payload, including
legacy duplicate names. Imported preferences apply both mixer gains under one
device lock and preserve the active race/championship. Missing audio hardware
still permits owning and saving preferences. Application close refuses pending
storage, and terminal completion remains observable after close until a new
application starts.

`make rewrite-preferences-verify` checks original factory bytes, all source
effects values, malformed extension rollback, complete independently predicted
Native/browser cards, process restart and sanitized ownership. Actual Native
application actions prove restored engine-voice gain and retained navigation/
championship state. Actual browser controls prove cancellation, failed writes,
stale owners, pending close and restored Redbook gain in WebAudio PCM. Memcheck
checks the Native application. Native human save menus, full configuration
consumption and playable saved championships remain under active 0058.
