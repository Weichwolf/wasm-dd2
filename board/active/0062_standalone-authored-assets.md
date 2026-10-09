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
evidence and do not satisfy authored replacement content. No standalone launch,
Blender asset collection or improved cockpit/audio collection is accepted yet.

## Next

Inventory every consumed asset, gameplay metadata dependency and missing cockpit
feature. Define the owned runtime content manifest and renderer/audio interfaces.
Establish `assets/` Blender/procedural/audio sources and exports, generation tools
and a reproducible first track/vehicle/cockpit with entirely new textures/audio.
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
