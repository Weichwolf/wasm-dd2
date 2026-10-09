Type: Work item
Title: Update pinned SoftGL and record the 60-FPS content budget
Depends: 0010, 0062

## Contract

Update the untouched SoftGL submodule to the published upstream master tip and
verify the current Native/WASM renderer and actual window/canvas integration.
Record the user's 640x360, 4x MSAA, four-total-thread, 60-FPS target and initial
100,000-200,000-triangle / 20-30-material content budget as planning assumptions.

## Evidence

Upstream fetched on 2026-10-09: a95534e8ac9f061a2e86286ffd4d80aa69c10bfa.
The previous pin is 7963be1d5b5e1bebbe97ece2c655228c8bc0a838. The new library
adds optional scene visibility/material paths and genuine MSAA optimizations;
the game does not yet use those optional paths. WASM automatic rendering uses
up to three helpers plus caller. Native still uses the automatic core-based pool.
Eight prestarted WASM pthread slots describe capacity rather than active usage.
No local SoftGL source changes are made.

Strict clang-format/clang-tidy 19 passes for all 208 rewrite C/header files;
47 Native and 45 WASM CTests pass. The permanent actual window/canvas corpus
passes all eleven levels: 31 Native, 31 fresh O1 ASan/UBSan and 145 Chromium
comparisons, including input/focus, moving gameplay, results, owner close/reopen
and modal viewport/same-size/full-page redraw checks. Four sampled scene/car
RGB hashes are unchanged from the previous published dependency's evidence.
Direct inspection of real Native X11 and Chromium car/profile-modal captures
confirms visible geometry and readable text. Coarse reference textures, aliasing,
black background and missing lighting/shadows remain under 0010/0062.

Receipts: /tmp/wasm-dd2/softgl-update-0063/{quality-report,
previous-pixel-report,visual-review-report}.json and window/report.json.
Completed raw comparison/capture output and logs are removed after receipts.
This proves dependency integration, not the 60-FPS profile or standalone game.

## Next

Continue the authored profile, explicit Native
thread selection and complete-frame performance measurements under 0010/0062.

## Accept

The exact published dependency pin passes required Native/WASM quality gates
and actual presentation/lifecycle regressions. The requested 60-FPS planning
budget is documented without claiming measured frame rate, MSAA adoption or
standalone-content acceptance.
