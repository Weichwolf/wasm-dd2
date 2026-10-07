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

`rewrite_audio_mixer` covers exact interpolation samples, mono duplication,
fractional cursor/frame partitioning, gain/pan, final positive/negative clipping,
replacement/locks, invalid requests, unsigned-8 PCM, high-frequency one-frame
loops, music endpoints/repeat and pause/resume. The same tests run on Native and
Node/WASM; standalone ASan/UBSan runs check ownership/bounds/arithmetic.

This is the platform-independent mixer and transport. Device/browser audio
output, original sound-bank cue selection, engine/impact/menu sounds, Redbook
loading/selection in the application and player-facing audio settings remain
to be integrated and verified with actual output.
