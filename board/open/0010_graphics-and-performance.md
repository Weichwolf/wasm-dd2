Type: Work item
Title: Continuous graphics improvement

## Contract

Improve resolution, textures, lighting, shadows and effects while keeping all cars/tracks correct, readable and performant in SoftGL on Native/browser.

## Evidence

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
