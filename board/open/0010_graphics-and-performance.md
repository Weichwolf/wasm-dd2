Type: Work item
Title: Continuous graphics improvement

## Contract

Improve resolution, textures, lighting, shadows and effects while keeping all cars/tracks correct, readable and performant in SoftGL on Native/browser.

## Evidence

The user now requests an original-informed intermediate visual baseline rather
than further independent fictional prototype detail. Offline conversion exports
all eleven levels and dynamic templates to owned DD2MESH2/PNG assets with two
linear midpoint splits. Every retained source triangle becomes sixteen triangles;
the complete static collection contains 2,278,128 triangles across all levels,
not per frame. Original stored static counts range from 2,305 to 24,680 triangle
equivalents per level; original high/medium/distant car bodies have 198/172/36
triangle equivalents before zero-area removal, and a wheel has 12. These source
counts do not establish submitted visibility or include all effects/overlays.

Native/WASM/sanitized owned loaders verify all 6,141 prepared meshes and 693 PNGs
without source archives. A prepared circuit-1 body image is directly inspected
in SoftGL at 640x360/4x MSAA with filtering; silhouette, paint and UV orientation
are plausible. Native/WASM differ at eleven pixels by at most two channel values.
This static geometry check does not prove original-frame similarity, a full
track view, cockpit-only submission or complete-frame throughput. The next
renderer step is the prepared scene owner, pre-submission frustum culling and
cockpit-only geometry, followed by actual gameplay/original screenshots and
complete-frame measurements. See active 0062 and
`/tmp/wasm-dd2/prepared-reference-0062/visual-report.json`.

Shared textured meshes, chase camera, wheels, damage and HUD exist. Lighting, shadows, detached parts, smoke, billboard orientation and blending remain incomplete. SoftGL is our library in deps/softgl.

The player explicitly prioritizes visual experience. Actual Native/browser
captures now receive direct inspection alongside numerical and pixel checks.
Sampled car preview, driven Time Trial, results, arena countdown and profile
dialogs show readable HUD/text, but a black sky, absent contact shadows and
coarse textures. The browser's long controls section puts the canvas below much
of the initial viewport. These are concrete visual follow-ups, not accepted
complete graphics or an original parity claim. The full-page browser capture
also reproduced an actually cleared modal canvas; closed 0061 now preserves
its visible scene/dialog through viewport and same-size canvas changes.
Review evidence: /tmp/wasm-dd2/rewrite-redbook-preparation-0060/visual-review/.

Active 0062 now has an owned Blender vehicle/cockpit, three mesh LODs and
six procedural material sets, with direct exterior/interior Cycles review.
That review corrected winding, firewall, texture density/bump and preview-road
problems. Body seams, unlabeled instruments, glass reflections and preview
noise still need work. The new assets have not reached the actual SoftGL game
renderer; studio lights/shadows and offline triangle counts prove neither
both-target visual acceptance nor 60-FPS throughput.

The 2026-10-09 host probe confirms linux-perf 6.12.111, Valgrind 3.24.0 and
Intel PCM 202502-1 are installed. Unprivileged perf software/hardware probes
are denied with perf_event_paranoid=3; `/usr/sbin/pcm` cannot access MSRs/PCI
configuration. No host security settings were changed. Memcheck and Callgrind
complete the existing Native car component test; Memcheck finds zero errors
and the component allocates no heap memory. This is tool-access evidence,
not full-game memory acceptance or a performance baseline. Callgrind raw output
is removed after summarizing it in
`/tmp/wasm-dd2/profiling-tools-20261009/access-report.json`.

## Next

The first authored SoftGL vehicle preview now draws all three LODs and cockpit
at 640x360/4x MSAA with batched indices, filtered albedo mips, basic lighting and
transparency. It passes 24 Native/WASM/O1 sanitized image captures, 15 Native/15
sanitized exact window comparisons and 17 exact Chromium control/canvas checks;
226-file strict LLVM19 and 50/48 Native/WASM CTests pass. Direct actual exterior/
cockpit review finds an excessively dark dashboard, flat display/shading, absent
normal/roughness evaluation, missing contact shadows/environment and unlabeled
gauges/body seams. Source caliper/column motion roles also need correction.
The default game remains reference-backed; this diagnostic is not standalone
game, qualitative superiority or measured full-frame 60-FPS acceptance.
Receipts: /tmp/wasm-dd2/authored-render-0062/{report,visual-review-report}.json.

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

Prioritize authored normal/roughness evaluation, better working precision and
environmental/interior light to preserve dark detail and make all controls readable. Add contact shadows,
separate fixed/moving parts and connect the first authored track/sky/scenery.

Use 640x360, 4x MSAA and four total render threads (including the caller) as the
authored renderer's starting profile, targeting 60 FPS / 16.67 ms per complete
frame on this machine. Plan 100,000-200,000 submitted triangles and 20-30
materials per frame with headroom for overdraw, alpha tests and complex materials.
These are planning values; throughput is unproved. Measure production Native
and browser median/tail frame times including simulation, resolve/readback and
presentation. The reference-backed 640x480 single-sample viewer has not yet
adopted this profile; Native explicit thread selection and aspect/HUD work remain.

Establish current real window/canvas images and performance budgets, then implement reviewable visual steps with before/after evidence.
Use the owned Blender/procedural content pipeline under 0062. Prioritize the
sky and
improve vehicle/road separation with lighting/contact shadows; reduce the
browser controls' intrusion into the playfield as the frontend is implemented.

Use bounded captured contact cases for Native Callgrind diagnosis when hardware
counters are unavailable. Continue separate WASM/V8 sampling for browser cost;
do not substitute instrumented Native timings for either production benchmark.

## Accept

All original tracks/cars and complete game flows render correctly. Every visual change includes actual both-target image/lifecycle checks and measured performance. Original pixel identity is not required.
