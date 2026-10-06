# SoftGL rendering

`renderer.h` owns a CPU framebuffer/context. Make it current on one render
thread; destroy all context-owned material caches before destroying it. RGBA
pixels are borrowed until the next draw/destruction and use bottom-first rows.

`mesh_draw.h` renders decoded mesh and scene objects without original memory
addresses or register emulation. The caller supplies viewport, clear/depth state
and matrices. Each scene object uses its decoded vertex origin: the center of
its 32768-unit raster cell for static shapes, or the source bounding center
when mesh flag bit 7 selects local coordinates. The source bounding center is
retained separately for future culling. Adding static vertices directly to
that center displaces adjacent track surfaces and leaves gaps. PSX quad order is
two rows (0/1, 2/3), so the renderer emits triangles
0/1/2 and 2/1/3, matching the original diagonal instead of a crossed GL quad.
Untextured faces use their retained RGB words; textured faces use their UV
definition, page (`page_flags & 31`) and palette bank. UVs address texel centers.
Texture rows use source V order. Per-context material caches borrow the level
and texture set and own lazily uploaded pages keyed by page/palette bank; both
borrowed sources must outlive the cache. Page ownership is independent of the
conversion scratch buffer and all GL textures are deleted with the cache.
Sources remain immutable for the cache lifetime; animation/remapping will need
explicit cache updates.

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
upward; the diagnostic camera preserves it instead of mirroring the car/track.
The diagnostic camera also intentionally uses orthographic bounds fitting
alongside the perspective driving camera.

`camera.c` now shares that bounds fitting between the headless preview and the
interactive native/browser application. Its explicit state supports orbit,
tilt, screen-plane pan and bounded exponential zoom; reset fits the currently
selected scene or mesh. The separate `driving_draw.c` implements a perspective chase camera.

The `rewrite_mesh_render` CTest checks every pixel of decoded textured quads,
correct corner/UV order, transparent texels, depth occlusion and switching
palette banks between queued draws, and opaque/cutout variants sharing the same
page/bank. Colored palette index zero and a nonzero mapping of source index zero
ensure this checks palette indices rather than inferred RGB brightness. It runs
without original assets on native and Node/WASM. A scene pixel check distinguishes
raster-cell and local origins;
using the source bounding center for static vertices leaves the entire expected
quad outside the frame and fails the check. The original-asset diagnostic can
be run directly after building:

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

`driving_draw.c` uses a world-up camera behind the current body heading, with a
fallback for near-vertical orientation. It applies the full quaternion pose to
the original high-detail car mesh and draws four independent wheel models from
level sections 5/6, with front steering and signed travel-based roll. Source
visual wheel XZ placement differs from the contact rig; rendering uses the visual
placements (X ±152, front Z 291, rear Z -232), tuned body height and wheel radius,
and simulated suspension compression. This is presentation tuning, not original
wheel-transform parity. Lighting, sky policy, animated vehicle materials and
shadows are still pending. The shared headless driving snapshot tool and actual
native/browser window checks compare all eleven deterministic starts.

`dd2_mesh_draw_damaged` deforms copied vertex coordinates using each car's six
crush zones, without changing owned meshes or wheel geometry. Full local crush
compresses body X/Y/Z by 20/35/45%; these are visual tuning values. Intact bodies
use the existing mesh path. A six-cell green/yellow/red HUD points forward upward
and shows engine health in the lower bar. It uses the shared SoftGL renderer on
Native and WASM; detached parts and smoke remain pending.

`score_draw.c` overlays player accident points (PTS, three digits) and credited
destructions (KO, two digits) beside the damage icon. The HUD uses compact bitmap
glyphs and a black backdrop, scales with the viewport and restores matrices/depth
testing. Native/WASM pixel tests independently check the 123 glyph pattern. These
are current-race accident values; championship standings remain game-rule work.

The same overlay module draws LAP current/required at the upper left on racing
tracks, showing lap 1 during the partial grid approach and FIN on individual lap
completion. Arenas omit it. Native/WASM pixel charts independently check the
LAP02/10 and FIN patterns and clearing of old digits. This is presentation of
borrowed lap state. `race_draw.c` overlays source-timed countdown lights, GO,
position and scored results. Time Trial replaces the finite LAP/POS display
with current/last/best clocks and an unlimited lap count; its result panel
retains those times and completed laps without scores. Timestamp rendering
uses integer 5 ms ticks, MM:SS.mmm punctuation and a 99:59.995 display cap.
Session records remain owned by game state; persistent records are pending.
