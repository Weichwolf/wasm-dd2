Type: Work item
Title: Restore the browser canvas after viewport changes in a modal
Depends: 0003, 0010, 0058

## Contract

Keep the actual browser scene and profile dialog visible when the viewport or
canvas presentation changes. Preserve modal draft, game state, paused clocks,
audio and cached world ownership. Native resize behavior must remain correct.

## Evidence

Player-facing review inspects actual Native X11 windows and Chromium canvas
captures after real keyboard/selector input: car preview, moving Time Trial,
withdrawn results, Arena-8 countdown and the F2 name dialog. HUD and dialog text
are readable in these sampled scenes. The black sky, absent contact shadows,
coarse textures and browser controls above the playfield need separate visual
improvement under 0010; these captures do not establish complete visual quality.

Chromium's full-page capture while F2 is open leaves the actual 640x480 canvas
black. Its own getImageData read falls from 139459 nonblack pixels to zero;
subsequent viewport and element captures stay black. The application still
reports asset level 8 and profile phase 1. This is an actual lost canvas buffer,
not merely a different screenshot crop. Captures and bounded reproduction
scripts remain under /tmp/wasm-dd2/rewrite-redbook-preparation-0060/visual-review/
until this diagnosis finishes. No fix or complete visual acceptance is claimed.

## Next

Reproduce with ordinary viewport resizing and inspect resize/canvas events.
Restore presentation from owned state without resuming simulation or discarding
the modal. Check frozen and moving views, focus, modal edits and close/reopen,
then inspect actual both-target captures and preserve the window regressions.

## Accept

Real browser viewport changes retain the scene and readable modal, with unchanged
state/draft/clocks. Native remains correct. Reproduction, focused browser checks,
actual inspected captures and required strict Native/WASM gates prove the fix.
