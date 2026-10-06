# SoftGL rendering

`renderer.h` owns a CPU framebuffer/context. Make it current on one render
thread; destroy all context-owned material caches before destroying it. RGBA
pixels are borrowed until the next draw/destruction and use bottom-first rows.

`mesh_draw.h` renders decoded mesh and scene objects without original memory
addresses or register emulation. The caller supplies viewport, clear/depth state
and matrices. Each object uses its signed world placement plus local mesh
vertices. PSX quad order is two rows (0/1, 2/3), so the renderer emits triangles
0/1/2 and 2/1/3, matching the original diagonal instead of a crossed GL quad.
Untextured faces use their retained RGB words; textured faces use their UV
definition, page (`page_flags & 31`) and palette bank. UVs address texel centers.
Texture rows use source V order. Per-context material caches borrow the level
and texture set and own lazily uploaded pages keyed by page/palette bank; both
borrowed sources must outlive the cache. Page ownership is independent of the
conversion scratch buffer and all GL textures are deleted with the cache. Sources remain immutable for
the cache lifetime; animation/remapping will need explicit cache updates.

This initial renderer uses neutral palette shade 8 and nearest sampling.
Cutout is selected per UV definition and palette bank, following `Modify_TDF`
and the primitive initializers `FUN_00418f30`/`FUN_0041a3b4`: find the first source
texel with zero low nibble in its UV rectangle, and mask the material only when
that texel's neutral CLUT lookup maps to palette index zero. Index zero is the
criterion even when its RGB color is nonblack. Other materials are opaque,
including nonzero palette mappings of zero-low-nibble texels. Scanning includes
UV endpoints to match this renderer's texel-center sampling and handle degenerate
single-texel regions. Both opacity variants of the same page/bank have separate
GL textures. Choices are cached per definition/bank rather than rescanning per
face/frame. It retains stored sprite quad geometry.
Lighting, billboard/tilted sprite behavior, fog, blending, animated/vehicle
material remapping and sky policy remain pending. The reference draws track
visuals as scene meshes; road-strip metadata belongs to contact/topology rules.
Static previews show stored scene meshes and the unmodified high-detail car
shape; they are not complete race frames. Original geometry uses positive Y
upward; the diagnostic camera preserves it instead of mirroring the car/track. The diagnostic camera also intentionally
uses orthographic bounds fitting rather than the eventual driving camera.

The `rewrite_mesh_render` CTest checks every pixel of decoded textured quads,
correct corner/UV order, transparent texels, depth occlusion and switching
palette banks between queued draws, and opaque/cutout variants sharing the same
page/bank. Colored palette index zero and a nonzero mapping of source index zero
ensure this checks palette indices rather than inferred RGB brightness. It runs without original assets on native
and Node/WASM. The original-asset diagnostic can be run directly after building:

```sh
/tmp/wasm-dd2/rewrite-native/dd2_scene_preview DestructionDerby2/Dirinfo \
  /tmp/wasm-dd2/scene.ppm 1 scene
/tmp/wasm-dd2/rewrite-native/dd2_scene_preview DestructionDerby2/Dirinfo \
  /tmp/wasm-dd2/car.ppm 1 car
```

Select level `1`–`9`, `A` or `B` and mode `scene` or `car`. The diagnostic is
headless and only accepts capture paths beneath `/tmp/wasm-dd2/`. It checks GL
errors and nonempty output, writes bounded 640×480 PPM files and releases all
meshes/textures/context resources. Host filesystem access in the Node variant
belongs to this test executable, not the future browser platform adapter.

```sh
make rewrite-scene-verify
```

This produces both previews for every playable level on native, Node/WASM and
ASan/UBSan-instrumented rewrite modules linked to the pinned release SoftGL.
The report records frame coverage, hashes and exact cross-target RGB differences;
SIMD edge rounding allows at most 1% changed pixels and mean channel error 0.5.
These are consistency bounds for the rewrite renderer, not original image
parity or evidence of complete rendering policies. Successful PPMs, sanitizer
executables and raw logs are removed after the report is written. Inspect a
specific frame only as needed and remove it when its diagnosis is complete.
