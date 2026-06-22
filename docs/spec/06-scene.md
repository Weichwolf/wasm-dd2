# 06 — In-race scene rendering (scene.C, draw.C, drawcar.C, camera.C, sky.C) — spec

Our-words functional spec from decompiled Draw_Subdiv_Object @0x41fdbc, the draw_face_* table, the
projection setup, and the camera/car/sky drawers. Ties 01-data-io (geometry/textures) → 02-gte-gpu (GTE+OT).

## Object structure (track sections, cars, scenery, HUD props)
An "object" passed to the drawer is a small header; the fields used:
- `[0]` → texture table pointer (`_gtexture`, = the LEVEL.TDF entries for this object's textured faces).
- `[1]+8` → u16 order/z index → selects the OT bucket base `_DAT_0071bdc4 = OT_base + index*4`.
- `[4]` → geometry block, which at offsets: **+0x20 vertices**, **+0x24 normals**, **+0x28 face list**,
  **+0x0a rotation** (fed to Pre_Rotate). (Confirms the empirically-decoded object format.)
- `[5]` → byte offset into the per-frame primitive buffer where this object's GPU packets are written
  (`_gprim1`).

## Draw_Subdiv_Object (the per-object renderer)
1. Set _gtexture, OT bucket, vertex/normal/face pointers, _gprim1; `Pre_Rotate(rot)` builds the object's
   Q12 rotation matrix (composed with camera) into the GTE matrix registers.
2. Walk the face list: each face record begins `[u16 count][u8 type][u8 term]`; if `term==0` stop.
   Dispatch `PTR_draw_face_3pt_flat_00462d94[type](count)` — a jump table of per-face-type drawers.
   (A second table @0x462e44 is used by Update_Object/FUN_41fe68 for animated/lit objects — same idea,
   with the light matrix `__lmptr=_light_matrix` enabled when the object flag bit0 is set.)

## Face-type drawers (draw_face_*) — each: GTE-transform verts → build packet → link into OT
- `draw_face_3pt_flat_dpq` (type for flat tris): RotTransPers the 3 verts; colour = face RGB run through
  gte_dpcs (depth-cue/fog); emit a flat-shaded poly packet; link to OT bucket by z.
- `draw_face_3pt_gour_dpq`: gouraud — per-vertex colour via gte_ncds (normal·light + depth cue).
- `draw_face_3pt_text_dpq` / `_squash`: textured tri (UV from texture entry), depth-cued.
- `draw_face_3pt_pict_dpq` / `_lit`, `draw_face_4pt_pict_dpq` / `_lit`: textured tri/quad ("pict"),
  optionally lit; UV + CLUT + tpage from the `_gtexture` (TDF) entry indexed by the face's texture id.
- `draw_face_sprite` / `_sprite_dpq`: billboard sprite faces (scenery sprites, e.g. trees) — a screen-space
  sprite placed at the projected vertex, sized by distance. (This is the separate scenery-sprite path.)
- All drawers allocate their packet from `_gprim1`/the prim buffer and prepend into the OT bucket
  `_DAT_0071bdc4` → back-to-front via DrawOTag (02-gte-gpu). Clipped faces (RotTransPers flag) are skipped.

## Projection setup (FUN_0041ff50 / FUN_0042003c)
- Screen centre = (`_screen_centre_x_`,`__scry`) set to (w/2,h/2). Focal length `DAT_00462fcc`.
- `_h_norm`,`_v_norm` etc. = Q12 normalisation of screen half-extents vs focal length (for the
  perspective + the billboard/sprite sizing and back-face/normal screen mapping). Derived from
  screen_width/height + focal length via SquareRoot0_ (fixed-point sqrt).

## Camera (Car_Camera @0x42968c) — feeds GTE matrix
- Builds the view matrix from the followed car's position/orientation (chase cam). Sets the GTE rotation
  matrix + translation each frame; Pre_Rotate composes each object's local rotation onto it. (Detail TODO:
  read Car_Camera fully for the exact follow spring/offset.)

## Draw_Car (@0x42c02c) and Sky (Draw_Sky @0x431200)
- Draw_Car: sets the car object's vertex pointer to the (denting-modified) car mesh
  (`&car_vertices + carIdx*0x330`, see roadmap) + draws via the same subdiv path; wheels are separate
  objects (TransformWheels / wheel_object). Damage denting modifies the vertex buffer in place.
- Draw_Sky: draws the sky/horizon band primitives (gradient) behind everything (far OT bucket).

## EXACT face-drawer mechanics (from draw_face_3pt_flat_dpq @0x41828c)
Confirms the precise per-face pipeline (all drawers follow this shape):
- Vertices are PRE-TRANSFORMED before the face loop: a `rot_points` table holds, per vertex index,
  {screenX, screenY (@+0x716dc4), Z (@+0x716dc8), w/extra (@+0x716dcc)} stride 0x10, plus a `rot_flags`
  per-vertex clip mask. (So Pre_Rotate + a vertex pass run RotTransPers over all verts into rot_points;
  the drawers then just index it — no per-face transform.)
- The face record (flat = 0x10 bytes): vertex INDICES stored in the high word at +6,+8,+10 (`field>>0x10`),
  ×0x10 = offset into rot_points. (+4 = colour, set by prim setup.)
- Backface/clip cull: signed area `__opz = (x0-x1)(y0-y2) - (x0-x2)(y0-y1)`; draw only if `__opz < 1`
  AND `(cullmask & rot_flags[v]) == 0` (no vertex clipped). (Screen-space winding cull.)
- OT bucket: `__otz = ((z0+z1+z2) * 0x15 / 0x40) >> 6`, then bucket index `= __otz >> 2`. Larger averaged
  Z → farther bucket. Primitive linked into `OT_base + bucket*4` by prepend (back-to-front via DrawOTag).
- Primitive packet (flat = 5 dwords/20 bytes): [0]=next-link, [1]=tag+packed RGB, [2..4]=packed screen
  coords (`X&0xffff | Y<<16`) for the 3 verts. Quad/textured drawers add a 4th coord + UV/CLUT words.
- Loop advances prim by its size, poly by the record size; writes back `_gpoly`,`_gprim1`.

## Port mapping (WebGL2)
Replicate: object geom (verts/normals/faces) → GTE Q12 transform → per-face-type packet (flat/gouraud/
textured/sprite) with gte_dpcs/ncds shaded+fogged colour → OT z-bucket → back-to-front draw. Textured faces
sample VRAM(index)→CLUT(rgba) per 02/01. Sprite faces = billboards (the faithful tree path). This replaces
the procedural shaders with the real display-list look.
