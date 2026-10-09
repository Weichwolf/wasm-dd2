# Authored mesh container, version 2

`DD2MESH2` is an owned format for float geometry/materials. It does not encode
original absolute addresses, registers, palette banks or original mesh opcodes.
All integers and IEEE-754 binary32 values are little-endian. Tables are contiguous
in the order below, with no padding or trailing bytes. Strings are nonempty,
printable ASCII (32..126), NUL-terminated and zero-padded to their fixed width.

| Table | Record size | Fields |
| --- | --- | --- |
| Header | 28 bytes | magic `DD2MESH2` (8 bytes); texture, material, part, vertex and index counts (5 uint32) |
| Texture set | 192 bytes | relative albedo, roughness and normal PNG paths (3 strings of 64 bytes) |
| Material | 60 bytes | name (32 bytes); base RGBA, metallic and roughness (6 floats); texture-set index (uint32) |
| Part | 92 bytes | name (64 bytes); material index, first index, index count, role (4 uint32); pivot XYZ (3 floats) |
| Vertex | 32 bytes | position XYZ, unit normal XYZ, UV (8 floats) |
| Index | 4 bytes | vertex index (uint32); three consecutive indices form a triangle |

Resource paths resolve below `assets/runtime/`; absolute paths, backslashes,
empty/dot/parent components and external symlinks are forbidden. Path components
use only ASCII letters, digits, underscores, hyphens and dots. The container
loader validates names; the future filesystem provider must enforce symlink
containment when resolving them. Material scalar values
are in [0,1]; material base RGB is linear light and alpha is linear opacity.
Texture index `0xffffffff` means no maps. Albedo is an sRGB factor
multiplied by the material's base tint; roughness and normal maps are linear.
PNG row zero is the top row. UVs use the Blender/OpenGL bottom-left convention;
the image upload adapter must flip PNG rows to match it. Normal-map +Y follows
increasing UV V. Tangents can be derived from the indexed position/UV triangles;
no implemented game tangent/shading path is claimed by this format description.

The typed coordinate system is meters, +Y up, +Z forward, with left at -X.
Export reflects Blender's Z-up coordinates and reverses triangle winding once.
Vertices already contain their reference-pose positions. A moving part transforms
around its retained pivot; positions are not offsets to add to the pivot again.
Part roles are 0 exterior/static, 1 front-left wheel, 2 front-right wheel,
3 rear-left wheel, 4 rear-right wheel, 5 cockpit and 6 steering. All wheel
subparts share their assembly pivot. Names preserve editable component identity
for future damage/animation; material/role batching can reduce draw submission.

Each part owns a nonempty, triangle-aligned index range. In table order, ranges
partition the entire index table without overlaps or gaps. Indices reference
valid vertices; shared vertices are allowed across parts. Normals must be finite
and unit length (absolute length error at most 0.0001); geometry/UVs and pivots
must be finite and have absolute values at most 65536. Degenerate triangles
are removed by export, and retained winding must agree with exported normals.

The verifier caps a single container at 32 texture sets, 64 materials, 4096
parts, 1,000,000 vertices and 3,000,000 indices before reading payload tables.
Those are corruption/allocation bounds, not per-frame performance budgets.
Default scene planning uses 100,000-200,000 submitted triangles and 20-30
materials for the complete 60-FPS frame, with headroom for other work.
`src/assets/model.h` loads the container into owned typed arrays after checking
all bounds, finite values, material references, partitions, names, triangle
areas and winding. It accepts misaligned source bytes and can release them
immediately after loading. Nonfinite checks inspect IEEE bits so that the
required `-ffast-math` cannot optimize them away. Destruction accepts NULL;
getters borrow arrays until destruction and return zero/NULL for a null model.

`make assets-content-verify` checks all three committed LODs on Native,
Node/WASM and fresh O1 ASan/UBSan. A diagnostic re-encodes every typed field
after releasing the source and compares it exactly with the independently
validated authored container. This is format/ownership evidence. The authored
default game integration remains pending. The separate original-free authored
vehicle/cockpit preview now uploads albedo in linear RGBA8, filters its mip chain
in linear light and encodes resolved RGB for display. Normal/roughness-map shading
and the limited 8-bit working precision remain under active 0062/0010.
