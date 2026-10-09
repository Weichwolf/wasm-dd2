# Prepared reference-derived content

The user requests an intermediate visual baseline made by converting the
original geometry once into our asset format. These committed resources are
development-derived placeholders for later Blender remodeling and procedural
materials. They are not the final authored-content acceptance for 0062.

`runtime/reference/` contains only our `DD2MESH2` float containers, PNG maps,
JSON scene placements and an inventory. Original archives, raw level sections,
mesh command streams, palettes and sound banks are not shipped in this set.
Original DD2 data is an optional offline conversion input. Ordinary builds and
runtime loading must not invoke the converter. The default game still needs its
provider migration before it can start without original files.

## Offline conversion

```sh
make assets-convert-reference
make assets-reference-verify
```

The converter accepts the provisioned `DestructionDerby2/Dirinfo`, checks its
supported SHA-256 and uses the independently verified development decoder.
`--archive` selects the offline input; `--output` selects the runtime root.
Run changed recipes into `/tmp/wasm-dd2/`, review and verify their results, then
replace the managed `runtime/reference/` tree. This command is separate from
`assets-generate`, which only rebuilds original-free authored recipes.

Each PSX quad becomes triangles `(0,1,2)` and `(2,1,3)`. Each nondegenerate
triangle receives two standard linear midpoint subdivision steps: 4 then 16
triangles. Position and UV interpolation preserve the reference surface and
seams; there is no smoothing or runtime subdivision. Provisional normals are
flat per source triangle. The initial meter convention is 160 reference units
per meter, with source +Y up preserved; this is a content scale convention,
not a proof of the original game's physical units.

Texture definitions and neutral palette colors are resolved offline into RGBA
PNG pages. Their alpha reproduces the existing reference renderer's
first-zero-texel cutout selection. Untextured flat colors become linear-light
material tints; varying corner colors become 32x32 baked gradient maps.
Normal/roughness maps are neutral placeholders. These resources support the
existing filtered, mipmapped owned renderer, but original lighting and exact
gradient pixels are not claimed. Content hashes deduplicate exported resources.

The initial conversion covers levels `1` through `B`: seven circuits and four
arenas, all static objects and dynamic-template sections 5 through 21. Menu/font,
road/contact/grid/AI metadata, all livery remappings, audio, animation and complete
gameplay integration remain separate migration work. Source sprite geometry is
stored as static triangles; its camera-facing policy remains pending. The
120 static and 440 template zero-area triangles are retained as explicit
omission records, including six entirely zero-area static objects. They do not
become invalid runtime geometry or silently count toward 16x output.

## Scene and inventory

`scenes/level-*.json` uses `format: DD2SCENE1`, a level ID, `units: meters` and
`up: Y`. Each `objects` entry owns a stable ID, a relative model resource path,
three-component world position and the model-local minimum/maximum AABB.
`templates` maps names such as `car-close`, `car-medium`, `car-distant`,
`wheel-primary`, `wheel-secondary`, `flag` and `sky` to resource paths and local
bounds. Unknown props retain neutral template names until their gameplay roles
are migrated. Referenced paths resolve under the supplied runtime root.

`manifest.json` uses `DD2REFERENCE1` and records recipe/source provenance,
subdivision/scaling policy, full resource hashes, independently decoded model
tables, per-level counts and omissions. Provenance does not require the source
archive at runtime. Scene metadata is currently a prepared provider input;
a production scene owner and pre-submission frustum culling still need wiring.

## Evidence

All 6,141 distinct meshes and 693 PNGs pass complete owned C field/index and
pixel comparisons on Native, Node/WASM and fresh O1 ASan/UBSan after releasing
their source bytes. Verification runs only against this prepared directory,
never opens original files and removes completed raw exports. It validates
scene references, finite placements, culling bounds, all 16x counts and omission
accounting. Batch-loader diagnostics also check malformed paths/lists and
cleanup after a failure following a valid entry.

The `dd2_prepared_preview` diagnostic accepts a runtime root, relative model
path and `/tmp/wasm-dd2/` PPM destination. It uses SoftGL at 640x360/4x MSAA with
filtered textures and display encoding. The directly inspected circuit-1 car
body has plausible silhouette and texture orientation; Native/WASM differ at
eleven pixels by at most two channel values. Wheels remain separate templates.
This is a static model check, not original-frame, full-game or 60-FPS acceptance.
