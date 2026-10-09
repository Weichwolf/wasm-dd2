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
also reproduces an actually cleared modal canvas; active 0061 owns that fix.
Review evidence: /tmp/wasm-dd2/rewrite-redbook-preparation-0060/visual-review/.

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

Establish current real window/canvas images and performance budgets, then implement reviewable visual steps with before/after evidence.
Prioritize visible rendering/UI defects: finish 0061, then render the sky and
improve vehicle/road separation with lighting/contact shadows; reduce the
browser controls' intrusion into the playfield as the frontend is implemented.

Use bounded captured contact cases for Native Callgrind diagnosis when hardware
counters are unavailable. Continue separate WASM/V8 sampling for browser cost;
do not substitute instrumented Native timings for either production benchmark.

## Accept

All original tracks/cars and complete game flows render correctly. Every visual change includes actual both-target image/lifecycle checks and measured performance. Original pixel identity is not required.
