# Prepared reference-derived content

The user requests an intermediate visual baseline made by converting the
original geometry once into our asset format. These committed resources are
development-derived placeholders for later Blender remodeling and procedural
materials. They are not the final authored-content acceptance for 0062.

`runtime/reference/` contains only our `DD2MESH2` float containers, PNG maps,
JSON scene placements, compact `DD2SCN1` scenes and an inventory. Original archives, raw level sections,
mesh command streams, palettes and sound banks are not shipped in this set.
Original DD2 data is an optional offline conversion input. Ordinary builds and
runtime loading must not invoke the converter. The default Native/browser game
now selects the prepared provider without original input; complete audio/content
quality remains open.

## Offline conversion

```sh
make assets-convert-reference
make assets-convert-liveries
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
arenas, all static objects and dynamic-template sections 5 through 21. Owned
roads/contact/grid/AI inputs live separately under `runtime/roads/`; vehicle pose
and wheel animation now use the prepared templates. Menu/font replacement, audio, cockpit/sky policy and complete gameplay quality remain
separate migration work. Source sprite geometry is
stored as static triangles; its camera-facing policy remains pending. The
120 static and 440 template zero-area triangles are retained as explicit
omission records, including six entirely zero-area static objects. They do not
become invalid runtime geometry or silently count toward 16x output.

## Prepared paint and numbers

`make assets-convert-liveries` is a separate one-time offline step. It reads the
supported archive and executable, extracts a semantic recipe to
`assets/recipes/reference-liveries.json`, resolves the driver/class palette and
number UVs, then emits owned meshes and RGBA PNGs. It exports twenty stable
drivers plus the two additional human classes at each of three body LODs for
all eleven levels: 726 bindings. Close, medium and distant paint regions have
separate source opcode-group ordinals; number UV shifts preserve unused triangle
corners. The template driver keeps its source paint. Untextured black body
triangles follow the original initialization policy.

The scene binds `car-close-00` through `car-close-21`, with corresponding medium
and distant names. Runtime selects these immutable models by livery index and
LOD; it does not read recipes, original palette/sprite tables or executable
addresses, alter UVs, or subdivide geometry. The track factory requires every
variant and fails transactionally if one is missing. Visible models lazily share
the world's texture cache after posed frustum culling. Human preview/driving
uses the selected class; opponents retain their stable driver paint across LODs.

The complete intermediate inventory now contains 6,812 meshes and 2,280 PNGs.
`tools/assets/verify_liveries.py` checks committed bindings without original input.
Its explicit `--original` option executes the original close/medium/distant paint
and number functions as an optional offline material-field oracle; this does not
establish full-game parity. Replacement Blender/procedural content remains open.

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
the owned scene loader and renderer consume the compiled files during default
gameplay. An explicitly selected reference mode remains optional.

## Owned runtime scene container

`DD2SCN1` is eight bytes including its final NUL. Three little-endian uint32
counts follow: resources, instances and templates. The header occupies 20 bytes.
Tables are contiguous with no padding or suffix:

| Table | Bytes | Fields |
| --- | --- | --- |
| Resource | 88 | relative mesh path (64 bytes), local minimum XYZ and maximum XYZ (six binary32 floats) |
| Instance | 48 | unique name (32 bytes), resource index (uint32), meter-space world XYZ (three floats) |
| Template | 36 | unique name (32 bytes), resource index (uint32) |

Strings are nonempty printable ASCII, NUL-terminated and zero-padded. Mesh paths
use safe relative components and `.dd2mesh`. Counts are capped at 4,096 resources,
65,536 instances and 128 templates before any allocation. Placement/bounds values
must be finite and within one million meters; loaded mesh limits remain stricter.
The scene owner copies metadata, loads each unique mesh once into owned memory,
and requires exact agreement between declared bounds and loaded vertex extents.
Malformed metadata or failed resources discard the complete partial owner.
Arrays/models are borrowed only until scene destruction; input bytes may be
released immediately. There are no original addresses, palette references or
original mesh commands in this format.

`make assets-scenes-prepare` compiles the committed placement JSON without any
original input or subdivision. `make assets-world-verify` checks every scene's
C field re-encoding after source release on Native, Node/WASM and fresh O1
ASan/UBSan. Twenty-one malformed-scene variants must fail cleanly, including
wrong finite bounds which could otherwise hide geometry from the frustum.
`make assets-world-render-verify` adds all full/crop scene images and shared-cache
lifecycle checks with fresh sanitized SoftGL, preserving the existing actual
Native/browser vehicle preview regression first.

The shared world renderer tests each translated model AABB against all six clip
planes before creating its draw cache or submitting triangles. Visible models
share context-local albedo uploads; unchanged images do not upload per object.
Prepared cutout textures use alpha testing and depth writes. Empty/invalid
frustum inputs fail open conservatively; crossing/touching bounds remain visible.
The caller owns the GL context and matrices. World and texture-loader user state
must outlive the renderer; destroy draw caches before their world/context.

The prepared diagnostic also accepts `reference/scenes/level-*.dd2scene`; an
optional final `crop` argument narrows the overview. Circuit 1's crop rejects
301 of 767 objects before submission. Additional posed material/role batch tests
reduce actual submissions to 122,224 triangles in 791 batches versus 190,048
triangles for the whole-map overview. All 66 full/crop Native/WASM/sanitized
images remain byte-identical to the preceding verified renderer despite the
additional rejected geometry. These static counts do not establish complete-frame FPS.

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

## Prepared game presentation

Default `dd2_app` reads `runtime/reference/` plus `runtime/roads/`, draws prepared
world/car/wheel models and advances the same typed gameplay owners. Native
`--assets directory [1..9,A,B]` selects a relocated owned-content root; an
explicit archive filename selects optional reference comparison. The browser
packages prepared resources and starts without a file picker; `?reference=1`
selects the optional original-input comparison UI. Normal builds never convert
or subdivide resources. The application owns its root path, rejects missing
content transactionally and preserves each candidate championship track until
its presentation has been prepared.

Prepared rendering uses meters, with simulation poses/camera explicitly scaled
from the road's 160-unit/meter convention. World/car/wheel instances share texture
uploads and test their posed bounds before submission. Player exterior stays
close; NPC body LOD thresholds are 30 and 90 meters. Wheels retain suspension,
steering and roll. Prepared paint/number variants are selected before submission.
The baseline still needs visual damage,
sky/billboard policy, a complete player cockpit and new audio integration.
Prepared launch is currently silent; optional reference audio is separate.

Run the strict Native/WASM gates first, then use a fresh instrumented build:

```sh
make clean-logs
make rewrite-check rewrite-wasm
ctest --preset rewrite-wasm
cmake -S . -B /tmp/wasm-dd2/prepared-game-sanitized -G Ninja \
  -DCMAKE_C_COMPILER=clang-19 -DCMAKE_BUILD_TYPE=Debug \
  -DDD2_ENABLE_CLANG_TIDY=OFF \
  '-DCMAKE_C_FLAGS=-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer'
cmake --build /tmp/wasm-dd2/prepared-game-sanitized -j4 --target \
  dd2_track_draw_test dd2_world_draw_test dd2_prepared_game_preview \
  dd2_prepared_application_test dd2_app
PYTHONDONTWRITEBYTECODE=1 python3 tools/assets/verify_game.py \
  --sanitized-build /tmp/wasm-dd2/prepared-game-sanitized
make clean-logs
```

This supplemental build instruments reached SoftGL and rewrite units; strict
analysis remains the separate Native gate. The verifier checks all eleven
prepared worlds/car/driving prefixes on Native/Node/WASM/sanitized, shared
cache and application lifetime, Memcheck, real Native windows and the browser
with trusted controls. It records state/geometry separately from image tolerance.
The browser comparison uses a 640x360 presentation canvas; wider CSS sizes scale
presentation. Review the actual moving/menu captures and remove completed raw
output after retaining the report. These short checks do not accept complete
campaigns, final remodeled assets/audio or the full-frame 60-FPS target.
