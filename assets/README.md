# Authored game content

This directory contains entirely rebuilt game content. Original DD2 assets,
extracted meshes/textures, source sound banks and CD audio are not inputs.
Editable Blender scenes and generated runtime exports are committed here;
verification images, logs and builds belong under `/tmp/wasm-dd2/`.

The first authored vehicle is the fictional Racer R1, with an exterior, four
independent wheel assemblies and an interior containing a cage, seat/harness,
headliner, firewall, pedals, shifter, steering assembly and three instruments.
It has full, exterior and distant-NPC mesh exports. Six procedural 1024x1024
material sets each provide albedo, roughness and tangent normal maps. Twelve
new 48-kHz stereo PCM clips include engine/road/impact/UI cues and the original
16-bar, 120-BPM music arrangement **Foundry Run**.

The original-free `assets-play` / `assets-web` vehicle preview now draws all
three LODs and the full cockpit with SoftGL at 640x360 and 4x MSAA, without a
file picker. It batches opaque parts, uploads filtered albedo mip chains,
sorts transparent parts and exposes camera/detail/wheel-pose controls. The
browser bundle includes only committed authored models/maps and metadata.

The default game still needs its original archive and sound bank at startup;
these new exports are not integrated into Native/WASM gameplay. Checked C
mesh/PNG loaders read every committed model and material map on both targets,
with fresh sanitizer and corruption evidence.
Blender studio lights/shadows are preview facilities,
not game-renderer evidence. Commentary, all tracks/arenas, further vehicle and
livery variants, new fonts/UI/VFX and full audio behavior remain incomplete.
See [inventory.json](inventory.json) and [work item 0062](../board/active/0062_standalone-authored-assets.md).

## Rebuild

Offline regeneration uses Blender 4.3 (verified with Debian 4.3.2), Python 3,
NumPy and Pillow. Normal game builds will consume the committed exports and
must not require Blender. The source recipes are under `tools/assets/`.

```sh
make assets-generate
make assets-check
make assets-content-verify
make assets-preview
make assets-play
make assets-web
make assets-render-verify
make assets-preview-measure
```

`assets-generate` deliberately rebuilds the generated editable `.blend` from
its recipe. Keep persistent authoring changes in the recipe so they survive
regeneration. Material PNGs are packed into the `.blend`; no external image
location is required when editing the checked-in scene. Object names and
individual components remain editable. Python errors make the Blender step
fail rather than silently accepting partial output.

`assets-preview` puts exterior/cockpit images under
`/tmp/wasm-dd2/authored-assets-preview/`. Inspect them, record findings/hashes and
remove finished captures. The preview road/floor, lights and cameras are
excluded from game mesh exports. Review uses real Cycles samples without an
optional denoiser, so Debian builds without OpenImageDenoise can regenerate it.

Mesh format version 2 is described in [mesh-format.md](mesh-format.md).
`assets-check` checks byte bounds, material/part/index ownership, finite unit
normals, triangle orientation, complete wheel/cockpit components, referenced
maps, tile seams, PCM format/headroom/DC/boundaries and sixteen corruptions of
each mesh. Runtime meshes, PNGs, WAVs and metadata are reproducible; Blender
project byte identity is not promised because project/session metadata can vary.

`assets-content-verify` runs strict LLVM19/build/CTest gates and compares all
three models and eighteen PNGs through the C loaders on Native, Node/WASM and
fresh O1 ASan/UBSan. Every model field/index and every top-first RGBA sample
must agree. Thirty-two corrupted meshes per LOD and thirty-five malformed or
unsupported PNG variants must fail cleanly; four positive PNG chunk layouts
must preserve pixels. The report retains hashes under `/tmp/wasm-dd2/` and
removes completed raw output. No original files are inputs to this check.

`assets-play` opens the actual native vehicle window. `assets-web` serves the
packaged browser preview at the printed `/content.html` URL. Tab switches
cockpit/exterior; Page Up/Down changes detail; arrows orbit/look; +/- zooms;
Space toggles a wheel/steering pose; R resets; Escape closes. The browser also
provides camera/detail/pose/reset controls. Cockpit selection requires the full
mesh. This is a content preview, not driving or a playable authored course.

`assets-render-verify` checks decoded upload orientation, mip minification,
alpha, state and failure cleanup; all three LODs and cockpit in rest/steer poses
on Native, Node/WASM and a fresh sanitizer build including reached SoftGL code;
then actual Native/sanitized windows and Chromium input/canvas/resize/close.
Same-target window/canvas comparisons are exact; cross-target diagnostics allow
the separately recorded small raster/lighting differences. Raw frame images are
removed after the report; review the remaining window/canvas PNGs, record findings
and remove them when the diagnosis is complete.

`assets-preview-measure` records thirty warmed render/resolve/readback/
presentation-submission calls per view on actual Native SDL and Chromium.
Native uses a temporary source-copy driver checked with the repository's strict
LLVM19 configuration and linked to current Release libraries; WASM uses the
packaged browser application. Reports retain source/build hashes and raw timing
samples beneath `/tmp/wasm-dd2/`, then remove temporary drivers/logs. The method
excludes normal simulation/event/compositor work and retains automatic SoftGL
pools. It measures a component baseline, not complete-game FPS or the explicit
four-thread acceptance profile.

The authored preview now decodes sRGB albedo to linear RGBA8 before filtering
and illumination, then encodes resolved RGB into an owned display copy. Alpha
and the linear framebuffer stay unchanged; raw reference rendering remains the
default. Actual Native/browser before/after review shows more legible cockpit
controls, paint and alloy detail. Eight-bit working precision loses dark detail
and exposes coarse carbon patterns; improved material/environment light,
unlabeled instruments, body seams and missing contact shadows remain. Normal and
roughness maps are exported but not yet evaluated by the initial scalar-material
lighting path. Fixed calipers and the steering column still need separate motion
roles from rolling wheels/steering-wheel geometry. The checker floor/blue clear
are diagnostic surfaces, not authored game scenery. These captures do not prove
qualitative superiority, complete gameplay/audio or the full 60-FPS profile.

Use LOD and visibility against the 640x360, 4x MSAA, four-total-thread,
60-FPS planning profile. The triangle/material budget applies to the whole
visible frame. Offline mesh counts do not prove game-renderer throughput or
player-visible quality; both require actual Native/browser integration and review.
