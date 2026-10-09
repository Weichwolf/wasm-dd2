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

## Next

Correct dark cockpit/display shading with a checked linear-light material path
and evaluate authored normal/roughness maps. Separate fixed caliper/column roles,
add contact shadows and improve the actual inspected vehicle/interior images.
Implement production LOD/visibility and explicit Native thread selection against
the complete-frame budget. Inspect actual windows/canvases with real input and
measure complete frame costs before claiming the four-thread/60-FPS profile. Supply
the first authored road/contact/progress graph and Blender track/scenery, then
define the runtime content manifest/provider and new audio-owner input.
Connect that vertical slice to default Native/browser launch without a file
picker or original archive, then replace the whole content inventory. Retain
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
