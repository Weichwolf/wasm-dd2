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

This is an offline content foundation. The game still needs its original
archive and sound bank at startup; these new exports are not integrated into
Native/WASM gameplay. Checked C mesh/PNG loaders now read every committed model
and material map on both targets, with fresh sanitizer and corruption evidence;
this is a loading component rather than default game integration.
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

Use LOD and visibility against the 640x360, 4x MSAA, four-total-thread,
60-FPS planning profile. The triangle/material budget applies to the whole
visible frame. Offline mesh counts do not prove game-renderer throughput or
player-visible quality; both require actual Native/browser integration and review.
