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

## Next

Implement an owned, checked C loader for DD2MESH2 and material maps, with actual
Native/WASM rendering of the full/exterior/NPC exports and cockpit view. Supply
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
