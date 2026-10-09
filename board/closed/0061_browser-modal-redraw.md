Type: Work item
Title: Restore the browser canvas after viewport changes in a modal
Depends: 0003, 0010, 0058

## Contract

Keep the actual browser scene and profile dialog visible when the viewport or
canvas presentation changes. Preserve modal draft, game state, paused clocks,
audio state and cached world ownership. Native resize behavior remains correct.

## Evidence

Direct inspection covers actual Native X11 and Chromium captures after real
input: car preview, driven Time Trial, withdrawn results, Arena-8 countdown and
F2 name entry. Sampled HUD/dialog text is readable. Black sky, missing contact
shadows, coarse textures and browser controls above the playfield remain under
0010/0062; these sampled scenes do not establish complete visual quality.

Before the fix, a full-page capture in F2 clears the actual canvas. Reading its
640x480 region drops from 139459 nonblack pixels to zero and later captures stay
black, while level 8 and profile phase 1 remain. Ordinary viewport resizing can
redraw, but same-size canvas width/height assignments still clear its buffer;
editing the draft restores it. The diagnostic observes those assignments.

The browser frontend now coalesces resize notifications into one animation-frame
presentation after SDK handlers finish. It uses the existing shared presentation
bridge and owned/cached world, without changing simulation or modal state.
Application-generation guards discard queued requests from closed owners.
The same arena reproduction now retains all 139459 nonblack pixels before,
after and following full-page capture. Direct inspection of the resulting
full-page, viewport and element images confirms a visible scene/readable dialog.

The permanent browser regression checks ordinary viewport changes, same-size
resize, full-page capture and viewport restoration. All four retain exact modal
pixels, draft PLAYERx, phase, level, race steps and music gain/cursor. The
existing all-eleven-level window/input lifecycle corpus passes: 31 Native and
31 fresh O1 ASan/UBSan comparisons plus 145 Chromium comparisons, including
focus, moving gameplay, results, profile exit and close/reopen. Strict LLVM19
format/tidy covers 208 C/header files; 47 Native and 45 WASM CTests pass.
Receipts: /tmp/wasm-dd2/rewrite-modal-redraw-0061/
{report,quality-report,visual-review-report}.json.
Completed raw captures are removed after retaining hashes/review evidence.
This closes the presentation defect, not complete graphics or standalone assets.

## Next

Continue qualitative graphics and authored standalone content under 0010/0062.
Keep direct visual inspection alongside the four permanent resize checks.

## Accept

Real browser viewport/canvas changes preserve exact modal pixels and tested
state/draft/clocks; Native window behavior remains correct. Actual inspected
captures, permanent regressions and required strict Native/WASM gates prove
this bounded contract.
