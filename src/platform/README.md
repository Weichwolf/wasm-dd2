# Platform

Own native/browser presentation, keyboard/gamepad events, monotonic timing and
persistence. Feed typed events into the game; platform code does not decide
race rules. Browser storage and audio activation require actual browser checks.
`file.c` owns a bounded archive read (up to 64 MiB) from native or Emscripten's
virtual filesystem. Borrowed archive views must be destroyed before these bytes.

`window.c` owns one SDL2 software window and a top-first RGBA staging surface.
It copies SoftGL's borrowed bottom-first pixels, reacquires the destination
surface after resize, and presents with aspect-preserving black letterboxing.
It creates no system OpenGL context. Both targets use the same keyboard state,
fresh key presses, wheel direction and monotonic elapsed time; focus loss clears
held keys and queued keyboard events. Frame time is capped at 50 ms for camera
interaction, independently of future simulation timing.

`src/platform/web/` contains the local-file browser UI and C application bridge. Its canvas
has keyboard focus; canvas/window blur and hidden visibility release input.
Selectors reflect C state, and closing permits reopening the archive. The local
server supplies COOP/COEP for the eight SoftGL workers and serves only the viewer
files. Original assets are neither served nor bundled.

Run `make rewrite-play` or `make rewrite-web`; `make rewrite-window-verify`
exercises actual X11 pixels and Chromium canvas/input. Gamepad remains to be
implemented.

`save_store.h` owns one original Windows save card and a staged replacement.
Open, put, delete and reload accept one request at a time; callers poll to a
terminal result. Acceptance is not save completion. The previous borrowed card
stays available during pending work and ordinary failures; successful completion
publishes the full replacement. Close/destroy refuse pending owners. Conflicts,
invalid current storage or indeterminate publication require an explicit reload
before another mutation. Reload publishes only a complete validated card.

Native callers provide an existing dedicated writable folder, reserving
`SaveGames`, `SaveGames.lock` and `.SaveGames.pending`. A nonblocking exclusive
lease protects cooperating writers. Writes compare the complete previous image,
synchronize a temporary file, close it, atomically rename it and synchronize the
directory. Failure after rename is indeterminate: the new file is visible but
the old memory snapshot stays accepted. Reload recovers the visible image; it
does not turn an earlier ambiguous write into a successful durable save.
Interruption leaves a complete old or new file; only an exclusive new owner may
discard the reserved temporary file. Noncooperating writers must respect this
dedicated directory; comparison is not an atomic file-system CAS against them.

Browser callers provide a dedicated IndexedDB database name. One full card is
one record. Strict-durability read/write transactions compare previous presence
and all bytes before replacing that record. Only transaction completion permits
publication; abort, enqueue exception and stale owners preserve the prior image.
JavaScript owns copied bytes and obtains current WASM heap views during polling.
Missing storage opens empty without creating a card until a mutation succeeds.
No persistence location defaults to provisioned game assets. The application now
consumes this owner for player/car/audio profiles through Native C actions, actual
F2/F3/F4 keyboard dialogs and browser controls. The Delete key reloads the
inventory and opens a separate deletion selection/confirmation dialog on Native
and the focused browser canvas. Empty entries cannot be deleted. Pending deletion
owns the application until storage acknowledges success or failure; cancelling
the preceding confirmation preserves the card. The default Native location is
SDL's per-user `Weichwolf/wasm-dd2` preference folder; explicit C callers may
provide an existing dedicated writable folder. It preserves pending application
owners and terminal completion receipts across close. See
`../game/configuration.md`, `make rewrite-preferences-verify` and
`make rewrite-player-profile-verify`. Full configuration consumption,
playable-state validation and replay encoding remain under
0008/0003; this adapter alone does not save a playable session.
`make rewrite-save-store-verify` checks independent full images, real Native
writer interruption and Chromium process restart, faults/conflicts, sanitized
ownership and Memcheck. Successful runs retain receipts and remove raw output.

`audio_device.c` owns an SDL2 signed-16 stereo device and a mixer at its negotiated
sample rate. It opens paused, serializes control changes with the device lock,
and closes/join-waits before freeing the mixer or borrowed PCM. The callback
allocates nothing. Emscripten's SDL2 port converts the resulting PCM to WebAudio
and resumes its context after a real user gesture. Device pause preserves the
exact source cursor; browser/canvas focus loss and game pause use it.
`make rewrite-music-output-verify` checks actual native callback PCM through SDL's
disk sink and real browser output-node buffers against independent CDDA sampling,
plus transport controls and close/reopen. Native speaker hardware is outside the
automated sink check. Missing devices leave visual gameplay available.

Free driving shares native/browser keyboard mapping with the inspection modes:
Enter toggles driving, P pauses and Space brakes. Input reports focus state;
focus loss releases held keys and suspends simulation, including coasting.
The browser canvas focus bridge also covers focus changes to page controls.
Focus transitions reset the monotonic timestamp so hidden/unfocused elapsed
time cannot advance the resumed vehicle. Frame elapsed time is capped at 250 ms;
the game owner advances bounded 5 ms steps independently of presentation.
