Type: Work item
Title: All vehicle classes, liveries and complete damage behavior

## Contract

Support every original car choice/class/livery and its functional handling and damage behavior with owned C state and correct render selection.

## Evidence

Source meshes/wheels and six crush zones are decoded; damage affects power,
retirement and rendered deformation. Original class palettes and all twenty
driver liveries are now decoded into owned bindings. The actual fleet renderer
selects each stable driver's paint and number sprite. The earlier livery increment
used Rookie for static preview; the class integration below replaces that default.
Full class race/damage coverage and remaining damage effects are incomplete.

Unmodified original Init_Car_Cluts, high-detail paint and door functions match
all 660 driver/class cases on eleven levels for Native, WASM and fresh O1
ASan/UBSan. Each case compares eight palettes, both number descriptors and all
99 painted faces. The 0x02 opcode marker on levels 3/4/6 selects the same typed
paint regions; ignoring it reproduced identical template skins on those levels
and was corrected before release.

All 264 body images per target match exactly across the rewrite targets.
Twenty-two distinct skins per level, damaged Rookie and restored Rookie prove
material/geometry lifetime separately; five copied corrupt sprite tables are
safely rejected on all three targets. Level-1/8 montages were visually reviewed
and removed. Strict LLVM19 checks all 201 C/header files; 45 Native/43 WASM
CTests pass. Native full-export Memcheck reports zero errors, zero retained
heap bytes and only the three standard descriptors, without suppressions.
The eleven-level scene/car regression also passes all 22 comparisons per
target. Actual X11 Native/sanitized application checks pass 31 comparisons each,
and Chromium passes all 144 checks without browser errors. Fleet/collision views
were visually reviewed with distinct human/opponent paint and source numbers.

The strict free-driving regression passes 21 of 22 level/mode cases. LEVB after
one settling second and three accelerating seconds fails position and image
comparison on WASM/sanitized. Fresh builds of prior commit d81e289 reproduce
each target's complete final state exactly, and its old renderer also fails the
image comparison. All physics, AI and reached game C units are unchanged.
The final bounded sanitized case completes the 66-state inventory. This is an
existing 0002 contact/movement issue, not a passed full driving regression; no
tolerance or scenario was relaxed.

Receipts:
/tmp/wasm-dd2/rewrite-car-liveries-0005-release/verification-report.json,
/tmp/wasm-dd2/rewrite-car-livery-memcheck-0005/report.json and
/tmp/wasm-dd2/rewrite-car-scenes-0005-release/report.json,
/tmp/wasm-dd2/rewrite-car-window-0005-release/report.json and
/tmp/wasm-dd2/rewrite-car-driving-0005-release/diagnosis-report.json.
These prove the high-detail livery component, not original runtime LOD,
original pixel parity, class handling/selection or complete vehicle acceptance.

## Verified class handling and selection

Rookie/Amateur/Pro now have an immutable typed catalog, separate from motion
snapshots. Original drive scaling, rear grip and drive-dependent front/rear
lateral weights affect actual tire forces; displayed ratings are not physical
multipliers. Human selection is copied into the owning field; all NPCs use Pro
independently of physical slots. Reset/mode changes and championship candidate
restarts preserve class identity. Native F1 and the browser selector prepare a
complete field before publication; playing, championship and dialog changes
are rejected. Car preview exposes class name/ratings and selected paint.

Thirty unmodified original rating/isolated force cases match all three targets.
Focused class tests prove acceleration/grip/axle differences, braking/reverse
and invalid-input rollback. All 120 original level/mode/class short prefixes
per Native/WASM/fresh O1 ASan/UBSan target validate finite fields, class ownership,
mode/reset and intact damage restoration. Six session creation/restart cases
per target retain the selected human and Pro NPC classes. Real Native/sanitized
F1 cycling and modal/playing locks pass; Chromium checks all three selections,
ratings, track/reset/session retention, lock rejection and invalid identities.
The three actual class previews were visually reviewed. Memcheck reports zero
errors and no retained heap blocks for the new physical class corpus.
LLVM19 checks all 205 C/header files; 46 Native and 44 WASM CTests pass.
The existing original livery component and all 22 scene/car datasets pass again.
The complete existing window suite also passes 31 Native/31 sanitized X11 and
144 Chromium comparisons across all views, input, modes and lifecycle.
Receipts: /tmp/wasm-dd2/rewrite-car-classes-0005-final/verification-report.json,
/tmp/wasm-dd2/rewrite-car-liveries-classes-0005/verification-report.json and
/tmp/wasm-dd2/rewrite-car-class-scenes-0005/report.json.
Window receipt: /tmp/wasm-dd2/rewrite-car-class-window-0005/report.json.

The unchanged strict 800-step free-drive comparison still passes 21/22 cases.
LEVB now has Native X -5866.5017820426301, WASM -5864.8079891853395 and sanitized
-5873.595608575949, with four supported wheels and zero player collisions.
This does not prove cross-target convergence or fix the existing 0002 issue.
Receipt: /tmp/wasm-dd2/rewrite-car-class-driving-0005/diagnosis-report.json.
No tolerance/scenario was relaxed. These checks do not close full vehicle,
original force parity or completed class campaigns. Saved class restoration has
separate evidence below.

0058 now persists the live class with player/audio profiles and restores a
validated complete field transactionally. Native/sanitized allocation failure,
moving-field retention, all three fresh-process classes and original-compatible
complete images pass. Actual F1/F3/F4 and Chromium dialogs show restored class
paint/ratings before closing; exact unoccluded pixels match the restored closed
world. LLVM19/205-file and 46/44 gates pass. These are saved-class checks, not
completed class campaigns or full damage acceptance. Receipt:
/tmp/wasm-dd2/rewrite-saved-car-0058-release/verification-report.json

## Next

Extend class comparisons through ordinary races, damage boundaries
and body-supported tires. Diagnose the LEVB accepted-step difference under 0002.
Complete damage-dependent effects and behavior.

## Accept

All selections load, drive, collide, deform, retire and reset correctly on Native/browser. Functional comparisons cover class differences and damage boundaries, not only mesh previews.
