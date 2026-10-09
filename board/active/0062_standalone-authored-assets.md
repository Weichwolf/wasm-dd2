Type: Work item
Title: Standalone rebuilt assets, procedural textures and new audio
Depends: 0005, 0009, 0010

## Contract

Native/WASM builds and the complete game run without any original DD2 files.
Rebuild all content in the repository with editable Blender sources, procedural
texture generation and new audio. Include every circuit/arena, vehicle/class/
livery, cockpit/interior, scenery/prop, font, UI, visual effect, sound effect,
engine, ambience, commentary and composed music. Generated runtime assets are
versioned alongside their reproducible sources. Substantially surpass the
original's visual/acoustic quality in every area while retaining full gameplay.

The user now explicitly permits intermediate reference-derived content:
convert source geometry and colors once into our asset format, apply two
linear midpoint splits offline (16x), version the prepared exports and later
replace them with original-informed Blender models/procedural maps. The game
must not subdivide geometry or require source DD2 files at runtime.

## Evidence

The scope is explicitly expanded by the user. Current runtime startup still
requires original Dirinfo and BANK1.SBK; music currently uses original CDDA.
Existing geometry, source banks, palettes and font decoding are reference/migration
evidence and do not satisfy authored replacement content. No standalone launch
or complete higher-quality game content collection is accepted yet.

The first owned offline pipeline now rebuilds an editable fictional Racer R1
with wheel/brake assemblies, cage, seat/harness, steering, pedals, shifter,
instruments, headliner and firewall. Its full/exterior/NPC exports contain
31716/10880/4362 triangles and 11/11/10 materials. The project packs eighteen
1024x1024 procedural PNGs (six albedo/roughness/normal sets). Twelve new 48-kHz
stereo PCM clips cover engine, wind, skid, impacts, UI/countdown and the original
32-second, sixteen-bar Foundry Run music arrangement. All sources and exports
are under assets/ and tools/assets/, with an explicit remaining-content inventory.

Two independent builds produce all 38 runtime files byte-for-byte identically.
One starts with only copied recipes in a fresh source tree/empty working directory,
without any original files. An independent reader checks byte/table/index bounds,
unit normals and winding, four wheel pivots, required cockpit components and
referenced maps; all sixteen damaged-container cases are rejected for each LOD.
Texture dimensions, normal lengths and tile seams pass. PCM format, peak/RMS,
DC offset, loop steps and silent one-shot boundaries pass. Validation caught and
fixed combustion-saturation DC bias and one-shot stereo wraparound. Reopening
the .blend proves eighteen packed procedural images and no leftover LOD modifiers.

Actual Cycles exterior/cockpit images were inspected. The review found and
corrected inverted shell/glass/deck winding, missing cockpit firewall, excessive
material scale/bump and a reflective preview road. A road-oriented camera and
proper asphalt improve the diagnostic scene. Prototype body seams, unlabeled
instruments, prominent glass reflections and noisy studio lighting remain.
These are Blender previews; they do not prove actual SoftGL quality, audible
game output or 60-FPS throughput. Game startup still uses original files.

Commands: make assets-generate, make assets-check, make assets-preview.
Receipts: /tmp/wasm-dd2/authored-assets-0062/{export-report,
reproducibility-report,project-report,visual-review-report}.json.
Completed preview renders, raw logs and duplicate generated outputs are removed
after retaining findings and hashes. The standalone content contract stays active.

Owned handwritten C11 model and PNG loaders now read all three LODs and eighteen
material maps. The model validates printable names/resource paths, exact table
extents/caps, material references/ranges, role/pivot/index partitions, bounded
finite geometry/UVs, unit normals, triangle areas and winding. IEEE-bit checks
reject NaN/infinity despite -ffast-math. Typed arrays own their data; a caller
can release even misaligned input bytes immediately. PNG decoding owns top-first
RGBA8 and checks CRCs, chunk ordering/extents, 8-bit noninterlaced color modes,
all five row filters and exact zlib input/output extents. Unsupported color-key,
indexed/depth/interlaced input fails explicitly rather than losing transparency.

The component corpus compares every re-encoded typed mesh field/index and every
Pillow RGBA sample on Native, Node/WASM and fresh O1 ASan/UBSan, with no original
files as inputs. It checks all 21 committed model/map files, 32 damaged meshes
per LOD, 35 malformed/unsupported PNGs and four valid PNG chunk arrangements.
Focused CTests cover source-independent lifetime, misalignment, null handling,
all model truncations and twenty independent PNG color/filter fixtures.
Strict LLVM19 covers 216 C/header files; 49 Native and 47 WASM CTests pass.
The offline authored geometry/map/PCM corpus remains valid after tightening its
reader to the same printable-resource and finite-scalar bounds.

Commands: make assets-content-verify; make assets-check.
Receipt: /tmp/wasm-dd2/authored-content-0062/report.json.
This is loading/ownership evidence, not authored rendering, game integration,
audible output or 60-FPS acceptance. No default game behavior changes yet.

The first original-free authored vehicle preview now renders all three LODs and
the cockpit at 640x360 with four real MSAA samples. It loads committed models/
maps directly, packages them into a 13,894,468-byte browser data bundle and needs
no original file picker. Opaque parts batch by material/role/pivot; transparent
parts remain separate and sort far-to-near. Full/exterior/NPC use 41/39/24 batches
instead of 335/122/49 part submissions. Albedo PNGs upload with corrected row/UV
orientation, complete mip chains, repeat/trilinear filtering and owned GL resources.
Basic lights and scalar metallic/roughness parameters illuminate the geometry;
normal/roughness maps and complete linear-light/display shading remain pending.

Twenty-four rest/steer captures cover all LODs and the cockpit on Native,
Node/WASM and fresh O1 ASan/UBSan, including reached SoftGL code. Sixteen
cross-target comparisons differ by at most 23 pixels and mean RGB error
0.0000811; these are diagnostics, not original parity. Actual Native and sanitized
windows pass fifteen exact own-target comparisons each, including camera,
detail/cockpit/pose/reset and normal Escape exit. Chromium passes seventeen
pixel-exact own-WASM comparisons through real controls/keyboard, viewport
changes, invalid requests and owner close. Strict LLVM19 covers 226 C/header
files; 50 Native and 48 WASM CTests pass. Focused pixel checks cover upload
orientation, alpha, mip minification and failure cleanup; Memcheck frees all
allocations with zero errors. MSAA sample queries and partial edge colors prove
the 2x/4x renderer options; the reference viewer retains single sampling.

The real checks caught and corrected duplicate batch initialization/heap writes,
ignored Escape actions, and missing browser icon requests. The canvas verifier
now requires exact same-target pixels and logical selection state: the initial
cross-target tolerance could mistakenly accept a previous wheel-pose frame.

Native and Chromium exterior/cockpit images were directly inspected. The body,
wheels, pillars/cage, mirror, steering and three gauges are present. Cockpit
darkness obscures dashboard/controls; flat blue/checker scenery, missing contact
shadows, unlabeled instruments and body seams remain clear defects. Fixed
calipers and the steering column require separate motion roles from rolling/
steering-wheel parts. Current lighting is not the required higher-quality shader.
These are actual content-preview windows, not gameplay/audio/60-FPS acceptance.

Commands: make assets-play; make assets-web; make assets-render-verify.
Receipts: /tmp/wasm-dd2/authored-render-0062/{report,browser-report,
visual-review-report}.json and /tmp/wasm-dd2/authored-draw-0062/quality-report.json.
Completed raw captures/logs are removed after retaining hashes and findings.

A bounded warmed preview measurement now records thirty calls per view at
640x360/4x MSAA. Native full/exterior/NPC/cockpit median costs are
17.79/10.70/7.97/21.17 ms; Chromium costs are
22.17/14.34/10.28/30.92 ms. Cockpit nearest-rank p95 is
24.65 ms Native and 47.51 ms Chromium. The measured calls include
rendering, MSAA resolve/readback and presentation submission, excluding ordinary
simulation/event/compositor costs. Native uses a strictly checked temporary
source-copy driver with current Release flags/libraries; browser uses the actual
packaged module. Automatic pinned SoftGL pools remain in use; seven total Native
process threads are observed, which is not a four-render-thread configuration
proof. Full/cockpit component costs already exceed the 16.67-ms budget; the
100,000-200,000-triangle planning budget and complete-game 60 FPS remain unproved.
Command: make assets-preview-measure.
Receipt: /tmp/wasm-dd2/authored-preview-timing-0062/report.json.

Authored scene color transfer is now corrected without modifying SoftGL.
Albedo PNG RGB decodes from sRGB before linear mip filtering/upload; material
base colors, fixed-function illumination, transparency and MSAA share linear
RGBA8 staging. The renderer optionally encodes resolved RGB into an owned display
copy, retaining alpha and the original linear framebuffer. Raw reference output
remains the default. Immutable lookup tables avoid per-pixel powers and mutable
initialization; an independent double-precision oracle checks all 256 input bytes.
Focused GL tests prove alpha, source-independent/in-place transforms, invalid
requests, repeated non-destructive readback and 0x/2x/4x sample output. The existing
upload/alpha/minification fixtures now independently expect decoded linear values.

Strict LLVM19 covers 229 C/header files; 51 Native and 49 WASM CTests pass.
The unchanged 24-image/15-Native/15-sanitized/17-Chromium corpus passes again,
with fresh O1 ASan/UBSan including reached SoftGL sources and zero-error/no-leak
Memcheck for both color and material tests. Cross-target diagnostic differences
are at most 34 pixels and mean channel error 0.000266204.
Four newly captured actual Native/browser baseline images agree exactly with
the published b0b4f7e pixel proof. Direct before/after window/canvas inspection
shows brighter paint/alloy and more legible steering spokes, cage, switches and
instrument rims/faces. In a fixed dashboard/control rectangle, mean display RGB
rises from 3.62 to 16.56 on both targets; pixels with all channels below 12 fall
from 29188 to 1110 of 32500. This region measurement supplements direct review,
not a qualitative-superiority verdict.

The current thirty-call warmed preview baseline records full/exterior/NPC/cockpit
medians 13.23/7.34/4.91/15.91 ms Native and
18.11/11.01/8.05/24.52 ms Chromium. Cockpit p95 is 18.01/26.64 ms.
This is rendering/resolve/readback/presentation submission with automatic pools,
excluding normal simulation/events/compositor costs; differing host load means
it is not an isolated speed comparison with the earlier run. Explicit Native
four-thread selection and complete-game 60 FPS remain unproved. The display copy
adds 921600 owned bytes at 640x360. RGBA8 intermediate rounding still loses dark
material precision and exposes coarse carbon texture patterns; lower controls,
material response, normal/roughness evaluation, lighting/environment and shadows
remain incomplete. No default gameplay/audio behavior or authored road is supplied.

Commands: make assets-render-verify; make assets-preview-measure.
Receipts: /tmp/wasm-dd2/authored-color-0062/after/report.json,
visual-review-report.json, baseline-capture-report.json, timing/report.json
and quality-report.json. Completed raw captures/logs are removed after review.

The offline intermediate conversion is now verified for all eleven playable
levels. Its 6,141 unique meshes and 693 PNGs occupy 129,500,317 binary bytes;
6,144 retained static instances contain 2,278,128 stored triangles. Explicit
omission records account for 120 static and 440 template zero-area triangles,
including six empty static instances. All retained source triangles become
exactly sixteen prepared triangles. Source sprites remain static geometry until
camera-facing policy is migrated; source lighting, livery remaps, road/AI/grid,
UI and audio are not claimed complete. The fictional Racer R1 remains a renderer
prototype and does not count as a DD2 vehicle remake.

Independent mesh decode and full C field/index re-encoding after source release,
and every Pillow PNG pixel, agree on Native, Node/WASM and fresh O1 ASan/UBSan.
Prepared scene references, finite placements, local bounds, resource digests and
16x/omission accounting pass. Source-free subdivision fixtures verify retained
surface/area/winding, UV interpolation and seams. A directly inspected SoftGL
car-body image uses filtered textures at 640x360/4x MSAA; Native/WASM differ at
11 pixels by at most two channel values. Separate wheel placement and actual
game/original screenshots remain pending. Receipts are under
`/tmp/wasm-dd2/prepared-reference-0062/`; commands are `make assets-convert-reference`
and `make assets-reference-verify`. This proves offline conversion and owned
loading, not standalone full-game, original parity or final qualitative acceptance.

Two independent conversions and the committed copy agree for all 6,846 files
(146,191,670 bytes including scene/inventory JSON). The strict gate covers 230
C/header files; all 51 Native and 49 WASM CTests pass. Batch input rejection also
proves failed partial output cleanup. Quality, reproducibility, complete loader
and static visual receipts remain under the run directory; completed captures
and raw comparisons are removed after recording them.
The unchanged authored loader corpus also passes for all 21 prior assets,
four positive PNG layouts and 131 rejection variants on all three targets.

The next component is now implemented: an owned DD2SCN1 scene factory and shared
world renderer. Eleven compact scenes compile from committed JSON without
opening original files or changing/subdividing their meshes. Each distinct mesh
loads once, and its actual vertex bounds must exactly match metadata before
culling can trust it. Resource/name/index/finite/range checks precede loading;
bad/missing resources release the complete partial owner. All eleven complete
typed scene re-encodings pass on Native, Node/WASM and fresh O1 ASan/UBSan after
source release; 21 malformed variants reject, including wrong finite bounds.

The renderer culls world AABBs against all six planes before creating a draw
cache or submitting triangles. Albedo uploads are shared across model caches;
a hidden missing texture does not block visible objects, and failed uploads can
retry cleanly. Prepared alpha masks use alpha testing with depth writes; a new
pixel fixture proves discarded texels reveal a later background while retained
texels occlude it. All 66 whole/cropped scene captures and 44 cross-target checks
pass, with at most 232 changed pixels and maximum mean channel error 0.002203415.
Circuit 1's crop culls 301/767 objects and submits 127,072 triangles in 829 batches
with 34 uploads, compared with 190,048/1,278/40 for the overview. This is static
visibility accounting, not a complete four-thread/60-FPS result.

Direct circuit/arena inspection finds plausible layout and texture orientation,
but dark arena-floor contrast and incomplete sky/terrain need improvement. An
optional original-decoder overview has the same coarse layout and central gaps;
it is not original-executable parity. Fresh O1 sanitizer rendering includes
reached SoftGL sources. World-cache Memcheck reports zero errors, 152 matched
allocations/frees and no leaks. The previous authored preview retains all
24 captures, 15 Native/15 sanitized window checks and 17 Chromium comparisons.
Strict LLVM19 covers 243 C/header files; 54 Native/52 WASM CTests pass. Evidence
is under `/tmp/wasm-dd2/prepared-world-0062/`, including owned-verification,
asset-regression, world-render, authored-render and visual-review reports.

The shared model renderer now builds bounds from each batch's actual indexed
vertices, rejects exterior/wheel roles for player cockpit draws and tests the
remaining bounds after their pivot/steer/roll pose, before material binding or
triangle submission. Statistics reset per frame and count actual submitted
batches/triangles; the prepared world renderer sums these submissions. Opaque
and transparent fixtures cover all seven roles, filtered pixels, re-selection,
translated/behind/crossing/touching bounds, steering into/out of view, wheel poses
and unchanged modelview matrices. No runtime subdivision or source conversion
was added; scene/opponent options remain independently unrestricted.

The prototype's rest/steer cockpit submits 7,592 triangles in nine batches, with
32 of 41 batches rejected by role. All remaining broad material bounds intersect
the straight-ahead frustum; no additional batch culls are claimed there. Strict
243-file LLVM19 gates and 54 Native/52 WASM CTests pass. The 24-capture authored
corpus, 15 Native/15 sanitized window checks and 17 Chromium comparisons pass
with fresh O1 instrumentation including reached SoftGL and zero-error/no-leak
material Memcheck. All 66 prepared-world images remain byte-identical to the
previous verified captures across Native/WASM/sanitized. The circuit-1 crop now
submits 122,224 triangles in 791 batches, omitting an additional 4,848 triangles
and 38 batches after coarse object culling.

Direct actual Native/browser review retains instrument/steering layout but
exposes missing inner door/window surrounds previously hidden by exterior body
geometry. Those remain authored-content defects, not completed cockpit acceptance.
Exterior browser pixels are unchanged; old sanitized/current production Native
exteriors differ at five pixels with mean channel error 0.000054977. A premature
startup-black Native baseline was discarded and reacquired only after matching
the old rendered frame.

Thirty warmed current render/resolve/readback/presentation calls measure cockpit
medians 18.49 ms Native and 25.79 ms Chromium under other projects' build load.
A same-session O3 Native before/current ABBA comparison with 60 samples per view
records cockpit 34.35 -> 14.17 ms median and 55.23 -> 20.30 ms p95. Host load still
varies; full/exterior/NPC medians are 21.47/17.47/12.88 ms before and
24.05/16.19/11.28 ms after. These are scoped component observations with automatic
pools, excluding normal simulation/events/compositor work, not isolated throughput
or the four-total-thread complete-game 60-FPS contract. Reports remain under
`/tmp/wasm-dd2/cockpit-submission-0062/`: quality, visual-review, after, world-render,
timing and paired-native. Completed raw captures/diagnostic drivers/logs are
removed after recording their hashes and findings.

The one-time gameplay-road import now commits eleven owned DD2ROAD1 containers,
eleven editable topology JSON files and a manifest under assets/runtime/roads/.
Their 23,336 vertices, 3,246 indexed strips and 19,775 cells occupy 924,972 binary
bytes. Geometry keeps the declared 160-unit/meter physics convention; the visual
meshes/textures are unchanged. Raw original records, offsets and unused provenance
fields are absent. Source-free JSON compilation reproduces all eleven binaries.
The owned C factory validates counts, extent, coordinates, cell ownership and
main/branch topology, copies the data and allows immediate input release.

Native, Node/WASM and fresh O1 ASan/UBSan checks cover every field/index and
39,550 rational triangle probes per target, with 39,302 valid contacts and maximum
height error 3.086402244889541e-10. Existing consumers preserve 220 starting poses,
6,492 barrier segments, course rules and 15,931 valid circuit AI queries; an
explicit optional comparison matches the original-decoded Native consumer output
exactly on all eleven levels. This is a typed-component comparison, not original
executable or complete gameplay parity. Thirty malformed variants reject through
both diagnostics on all three targets; unaligned/truncated/late-failure ownership
fixtures and zero-error/no-leak Memcheck pass. Strict LLVM19 checks cover 245
C/header files; 55 Native and 53 WASM CTests pass.

Additional legacy scene, livery and driving image regressions pass after restoring
the missing color.c unit in four manual sanitizer build lists. Actual original-
dependent Native scene review retains the coarse oval/stands layout. Actual
Chromium driving/player-contact views retain vehicle placement and readable lap
and damage HUDs, but black sky, coarse/aliasing materials and the old 4:3 layout
remain visible defects. These checks supply no prepared-provider or new-quality
acceptance.
The legacy window corpus passes with 31 Native and 31 sanitized comparisons,
plus 145 actual Chromium comparisons and no browser errors. The first optional
sanitizer window attempt failed its focus-return assertion; four focused repeats
and a complete final sanitized window run pass without changing game input code.
An intervening launcher termination has no acceptance receipt. Keep this scoped
retry history in the reports; no focus fix or standalone-window claim is made.
Evidence and reproducibility reports are under
/tmp/wasm-dd2/prepared-roads-0062/. The normal application still needs its prepared
track/provider, UI and audio migration. This broad work item remains active.

The shared track owner now loads prepared DD2SCN1 worlds/models and DD2ROAD1 roads
through a directory provider. It requires close/medium/distant bodies, both wheel
templates and sky, validates number/layout agreement and rejects wrong-number
provider results. Input files and model-reader state are released before publishing
the immutable owner. Prepared legacy archive/scene/mesh/palette getters return
NULL, so old material code cannot silently masquerade as prepared presentation.
The application and championship session now share this provider boundary for
track changes, candidate rounds and restarts. Normal startup still selects the
reference provider; prepared rendering/UI/audio integration remains pending.

All eleven prepared tracks pass 120 fixed moving steps after the existing 200
settling steps for twenty real vehicles on Native, Node/WASM and fresh O1
ASan/UBSan. This covers 26,400 controlled vehicle steps per target; Native state
snapshots exactly match the explicitly selected original-decoded provider.
Cross-target comparisons retain the existing driving tolerances. Initial
championship fields and candidate restarts preserve Amateur identity, league
standings and old track/driving ownership on load failure or wrong-number results;
stale candidates reject after commit. These checks do not complete a physical
round, result continuation or a championship. Seven filesystem/content failures
reject on all three targets without fallback or partial publication. Unaligned
source-release/required-template/partial-loader fixtures and zero-error Memcheck
pass. Strict LLVM19 covers 249 C/header files; 56 Native and 54 WASM CTests pass.

All 66 full/crop provider scene captures and 44 cross-target image comparisons
pass with maximum mean channel error 0.002203414351851852. Direct Native scene
loading has exactly the same images and counters. Circuit 1's crop retains
122,224 submitted triangles in 791 batches and 34 uploads. Direct scene review
retains terrain gaps and a dark, low-contrast arena floor; no new graphical
superiority is claimed. The real reference-backed application passes 31 Native,
31 sanitized and 145 Chromium comparisons, including input/focus, track and mode
changes, restarts, results and menus. Manual sanitizer lists now include the
new track owner's model/world dependencies. Additional directly inspected Native
and Chromium paused-driving captures agree exactly and retain the player/track
and legible lap/damage HUD; the black sky, coarse materials and old 4:3 layout
remain. These real windows still use the reference provider, not prepared default
startup. Evidence is under
/tmp/wasm-dd2/content-provider-0062/; command: make assets-tracks-verify.
This broad item remains active.

The prepared provider now reaches the normal shared application renderer and
becomes the default Native/browser startup. The application copies its content
root, owns a track presentation handle and destroys that handle before its
track/context. World, body LODs and suspended/steered/rolling wheels share one
context-local texture cache; actual posed AABBs are culled before lazy model
creation/submission. Simulation retains 160 units/meter while prepared drawing
uses meters. Player exterior stays close; opponent body thresholds are 30/90 m.
Default output is 640x360/4x MSAA with filtered maps and sRGB display encoding.
An explicit Native archive path or browser `?reference=1` preserves optional
reference rendering; prepared missing files fail without original fallback.

Strict LLVM19 format/tidy covers 255 C/header files; 58 Native and 55 WASM CTests
pass. Focused camera-scale, visible/hidden body LOD, borrowed shared-cache lifetime
and full GL cleanup checks pass on both targets and fresh O1 ASan/UBSan, with
zero Native Memcheck errors. Prepared application lifecycle checks render all
eleven tracks, vehicle views and short controlled drives, preserve pause, copy
caller path storage and enter/restart/exit both championship modes. Forty-four
world/car/start/drive cases produce 132 Native/WASM/sanitized images and 88
cross-target comparisons (maximum mean channel error 0.006624711). State and
submission/culling/LOD/cache counts agree; raster visibility counts belong to
the image oracle rather than a bitidentity assertion. Starting frame geometry
ranges from 35,440 to 142,304 actual submitted world/vehicle triangles; these
short snapshots do not establish full-frame timing or material budgets.

Actual prepared windows pass 35 Native and 35 fresh sanitized comparisons, plus
trusted throttle, pause, track wrap, deterministic reset and close. Default
no-argument Native launch independently matches its prepared world/driving
images. Chromium starts with eleven packaged roads and no Dirinfo, passes all
34 prepared canvas comparisons exactly, drives with trusted keys and verifies
both championship start/restart/exit paths, the player-name modal and missing
scene rollback. The optional reference corpus still passes 31 Native/31
sanitized/145 Chromium comparisons. Browser extent/aspect is now established
before SDL resize initialization, preserving 16:9 prepared and 4:3 reference
presentation. The prepared browser verifier uses a controlled 640x360 canvas;
a wider CSS window scales presentation and is not a larger SoftGL render profile.

Direct before/after and actual Native/browser review finds plausible preserved
layout/vehicle pose, wider horizontal coverage, less texture/edge aliasing and
readable HUD, championship and player-name text. The fleet still shares the
cyan base livery, the sky is black and the arena floor remains low contrast.
Liveries, visual deformation, cockpit, sky/billboard policy and the new audio
owner are not complete; prepared gameplay is currently silent. No whole-game,
quality-superiority, original-binary or four-thread/60-FPS acceptance is claimed.
The verification driver was corrected to include the car-class HUD, retain the
application's pause across track changes and compare pixel counts through the
existing image oracle. Completed raw captures/logs are removed after review.
Evidence: /tmp/wasm-dd2/prepared-presentation-0062/{quality-report,
visual-review-report,native-default-report}.json and verification/report.json.
Reusable command: python3 tools/assets/verify_game.py --sanitized-build
/tmp/wasm-dd2/prepared-presentation-0062/sanitized --output
/tmp/wasm-dd2/prepared-game-verification. This broad item remains active.

## Next

Convert driver/class livery remaps offline to owned runtime variants and verify
actual moving fleet appearance. Add prepared visual deformation, sky/billboard
policy and a player cockpit; keep scene/opponent options unrestricted while
selecting only player cockpit roles. Author inner door/window surrounds and use
finer visibility groups where useful. The prototype must not rely on its
exterior shell to cover interior gaps. Bind the new PCM content to an audio
owner and complete game-context music/effect behavior without original input.
Replace intermediate meshes with original-informed Blender models gradually.

Then evaluate authored normal/roughness maps with an owned tangent/material path,
improve limited linear working precision and add environmental/interior lighting. Separate fixed caliper/column roles,
add contact shadows and improve the actual inspected vehicle/interior images.
Implement production LOD/visibility and explicit Native thread selection against
the complete-frame budget. Inspect actual windows/canvases with real input and
measure complete frame costs before claiming the four-thread/60-FPS profile. Supply
the first authored road/contact/progress graph and Blender track/scenery, then
complete the new audio-owner input. Connect authored replacements to the
prepared default Native/browser launch and replace the whole content inventory. Retain
optional original functional comparisons separately from normal acceptance.

Inspect actual both-target scenes/audio after every content step. Record visible
improvements, remaining defects, memory/size/frame budgets and performance;
use the authored 640x360 / 4x MSAA / four-total-thread, 60-FPS profile and initial
100,000-200,000-triangle / 20-30-material per-frame planning budget under 0010.
passing numerical component checks alone cannot prove qualitative superiority.
Complete the remaining game surfaces and playback/context rules under their
existing work items, rather than closing them from a replacement asset export.

## Accept

All editable inputs and runtime game content are in Git and reproducible.
Default Native/WASM distributions build, launch and complete every game flow
with original assets absent and no original-data fallback. Direct visual/acoustic
review demonstrates substantially higher quality across the complete content
inventory, including working cockpit views. Functional, lifecycle, performance
and strict LLVM19 evidence passes on both targets. This broad item remains open
until the whole standalone content contract is proved.
