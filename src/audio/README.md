# Audio mixing and transport

`mixer.h` owns four effect voices and a separate Redbook music voice. Immutable
PCM views are borrowed: the caller owns their storage and keeps it alive until
replacement, stop or mixer destruction, including during pause. Control and
render calls must be serialized by the platform owner. Rendering allocates no
memory and writes interleaved signed 16-bit stereo into caller-owned storage.

Effects use explicit source-frame frequencies in Hz, independent of WAVE rate
or SBK defaults. A rational cursor stores a source frame and a remainder with
the output rate as denominator. Linear interpolation and signed rounding use
integer arithmetic; splitting a render into callbacks preserves every sample
and cursor. Rates 8,000–192,000 Hz and source frequencies up to 384,000 Hz are
bounded. One-frame loops and source jumps spanning multiple loops are supported.
Non-looping voices hold the final sample only through its remaining duration,
then stop. Linear gain is 0–256; pan attenuates the opposite stereo side. Wide
accumulation precedes final signed-16 saturation, so voices do not wrap or clip
individually before summation.

Automatic channel selection rotates over unlocked channels and replaces the
selected voice, following `FUN_004164e4`/`Play_Sound` in the reference. Locking
does not prevent explicit replacement, and stopping preserves the lock. An
all-locked request fails after checking four channels. It cannot enter the
original's unbounded search. Invalid playback/control requests leave state
unchanged; failed snapshots clear their outputs. Frequency/gain/pan changes keep
the exact playback cursor. Muted voices continue to advance; global pause writes
silence while preserving all effect/music cursors.

Music selection accepts complete stereo 16-bit 44,100-Hz CDDA with physical
track numbers 2–19. Ready, playing, paused and ended states are explicit. Start
rewinds; pause/resume retains the cursor; repeating wraps without a silent gap.
Stop clears the selection and releases its PCM view. Gain persists across track
changes and stop. Global game pause is separate from an explicit music pause.
The original track-selection, repeat and pause/restart functions establish the
functional reference; this runtime does not claim MCI timing or PCM parity.

`make rewrite-original-music-selection-verify` executes the unmodified original
transport with an imported MCI observer and write-protected machine code. Its
48 checks cover all eighteen physical track selections, explicit transport,
repeat guards, failed start, disabled audio and actual countdown/GO routines.
Unmodified caller disassembly and an independent PE data read establish the
context mapping below. This is original component evidence; automatic rewrite
selection remains under active 0060.

| Original context | Physical Redbook track | Start |
| --- | --- | --- |
| Main menu and practice results | 13 | On entry |
| Race | Loaded asset level + 1 (2..12) | At GO |
| Championship results and intermediate season standings | 14 | On entry |
| End of season | 15 | On entry |

Each context repeats its selected track. Menu track positions map to asset
levels `[1,2,7,5,3,6,4,10,8,9,11]`; music follows the loaded asset level, not the
menu position. Selection seeks the title and clears the playing flag; the race
countdown starts it only at its green boundary. Explicit pause suppresses the
repeat poll. These checks do not establish live frontend navigation, original
PCM output or accepted Native/browser automatic-selection behavior.

`rewrite_audio_mixer` covers exact interpolation samples, mono duplication,
fractional cursor/frame partitioning, gain/pan, final positive/negative clipping,
replacement/locks, invalid requests, unsigned-8 PCM, high-frequency one-frame
loops, music endpoints/repeat and pause/resume. The same tests run on Native and
Node/WASM; standalone ASan/UBSan runs check ownership/bounds/arithmetic.

The application now owns actual SDL2 output through `platform/audio_device.c`.
`game/audio.c` owns one CDDA file and sound-bank metadata, and resolves Native tracks relative to the
archive; the browser stages one user-selected file. Selection replaces the
borrowed view under the device lock before releasing old bytes. Failure leaves
the old title and cursor intact. Close joins callbacks before freeing PCM.
Track loading repeats automatically; explicit music pause and game/focus
suspension preserve the cursor independently. Gain zero keeps playback moving.
Native starts track 2 when provisioned, F10 pauses/resumes, and F11/F12 moves
through physical tracks 2..19. Browser file selection, music and volume controls
use the same C transport. Audio-device failure permits visual gameplay.

`make rewrite-music-output-verify` compares real Native SDL disk callback output
and browser WebAudio node samples with an independent integer CDDA oracle. It
also checks Native ASan/UBSan lifetime and browser repeat/control/rollback/reopen.
This establishes this rewrite's output, without claiming original audio engine
or speaker hardware parity.

`game/sound_events.c` collects countdown and nearby collision events at the 5 ms
simulation clock. A frame batch retains events from every fixed step and publishes
with the driving transaction. Countdown cues occur at ticks 36/156/276/400,
independent of render frame partitioning. Spatial impacts use contact closing
speed, distance and the player's orientation, with a 150 ms pair/world cooldown.
Impulse-free repairs and soft ground support remain silent.

`effects.c` borrows the original bank, reserves channel 0 for the looping motor
and channel 1 for countdown, and rotates impacts over channels 2/3. Call reset
after creation to establish these channel reservations. Bank samples 0, 3 and
11/10/9/8 supply motor, impact and THREE/TWO/ONE/GO. One-shot playback uses the
WAVE sample rate, matching the source's default-frequency request. Motor pitch,
levels and spatial attenuation are rewrite tuning. Music and effects have
independent volume controls; reset stops effects and clears cue counters while
preserving music and master gain. The audio owner joins device callbacks before
freeing bank metadata or archive-backed PCM.

`make rewrite-effects-output-verify` checks actual Native idle-motor output and
browser four-voice mixtures at 44,100/48,000 Hz against independent original-WAVE
resampling. Real controls exercise countdown, engine pitch, impacts, pause,
mute, reset and close; ASan/UBSan checks event/effect units and application
lifetime. These are scoped rewrite checks. Skid, crowd, commentary and menu
sounds, automatic race/menu music selection and saved audio settings remain open.
