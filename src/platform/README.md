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

`web/` contains the local-file browser UI and C application bridge. Its canvas
has keyboard focus; canvas/window blur and hidden visibility release input.
Selectors reflect C state, and closing permits reopening the archive. The local
server supplies COOP/COEP for the eight SoftGL workers and serves only the viewer
files. Original assets are neither served nor bundled.

Run `make rewrite-play` or `make rewrite-web`; `make rewrite-window-verify`
exercises actual X11 pixels and Chromium canvas/input. Gamepad, persistent
storage remain to be implemented.

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
