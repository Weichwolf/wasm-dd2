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
conversion scratch buffer and all GL textures are deleted with the cache.

This initial renderer uses neutral palette shade 8, nearest sampling and source
low-nibble cutout with alpha testing. It retains stored sprite quad geometry.
Lighting, billboard/tilted sprite behavior, fog, blending, animated/vehicle
material remapping, sky policy and original road strips remain pending. Static
previews show the scene object layer and unmodified high-detail car shape;
they are not complete track/race frames. The diagnostic camera also intentionally
uses orthographic bounds fitting rather than the eventual driving camera.

The `rewrite_mesh_render` CTest checks every pixel of decoded textured quads,
correct corner/UV order, transparent texels, depth occlusion and switching
palette banks between queued draws. It runs without original assets on native
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
