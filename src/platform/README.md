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
storage and audio activation remain to be implemented.
