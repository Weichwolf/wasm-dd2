Type: Work item
Title: All vehicle classes, liveries and complete damage behavior

## Contract

Support every original car choice/class/livery and its functional handling and damage behavior with owned C state and correct render selection.

## Evidence

Source meshes/wheels and six crush zones are decoded; damage affects power,
retirement and rendered deformation. Original class palettes and all twenty
driver liveries are now decoded into owned bindings. The actual fleet renderer
selects each stable driver's paint and number sprite; static human preview uses
Rookie. Class handling/selection and remaining damage effects are incomplete.

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

## Next

Connect the recovered three
class traction/handling parameters to owned physical state and actual selection.
Consume validated saved car choices through 0058 without partial live-state
publication. Complete damage-dependent effects and behavior.

## Accept

All selections load, drive, collide, deform, retire and reset correctly on Native/browser. Functional comparisons cover class differences and damage boundaries, not only mesh previews.
