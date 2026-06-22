# 02 — Software GTE + GPU (libgte.c / libgpu.c / draw.C) — functional spec

Our-words description of the PSX-style transform + display-list rasterizer the PC port emulates.
Source: decompiled gte_*, RotTrans/RotTransPers, OuterProduct12, Draw_All, Draw_Tile, Allocate_OT_,
Init_Primitive_Buffer, DrawOTag.

## GTE (geometry transform engine) — fixed point Q12
- Rotation matrix: 3x3 of int16, set by gte_SetRotMatrix (9 shorts @__globmat..). Translation vector
  separate. All math is Q12 (>>0xc).
- Vertex registers vr0..vr3 form a 3-DEEP PIPELINE: each transform shifts vr3<=vr2<=vr1<=vr0<=new.
  This mirrors the PSX GTE's RTPT (transform-3) — batches of 3 verts (triangles) pipe through.
- RotTrans(src,dstXYZ,flag): rotate+translate a vertex (GTERT), no perspective → camera-space XYZ.
- RotTransPers(src,dstSXSY,dstZ,flag): rotate+translate+PERSPECTIVE divide (GTERPS) → packed screen
  coords (sx in low 16, sy in high 16 of *param2), z=0 slot, and a clip/overflow FLAG (param4).
  Screen XY are the projected 2D coords used to build GPU packets.
- OuterProduct12(a,b,dst): cross product in Q12 (>>0xc) → face normal for lighting.
- gte_dpcs (depth-cue colour) / gte_ncds (normal-colour-depth-single): compute a vertex/face colour by
  applying ambient light + the depth-cue (fog) ramp set by Set_Depth_Cue / Set_Ambient_Light. This is
  where flat/gouraud face colours get their final shaded+fogged RGB. (read in full for 06-scene.)

## GPU / ordering table (the painter's-algorithm display list — NO z-buffer)
- Ordering table (OT): array of `_otsize` 32-bit list-heads, allocated by Allocate_OT_ (double-buffered:
  two OTs). Index = depth bucket (z). Larger index = nearer or farther depending on walk direction.
- Primitive buffer `_prim_buf`: a per-frame BUMP allocator (_free_mem counts 8-byte units); reset each
  frame (Reset_Primitive_Buffer). Every primitive (poly/sprite/tile/font) is allocated here.
- Submit = link into OT bucket: `slot = OT_base + zindex*4; prim->next = *slot; *slot = prim;`
  (prepend). So each bucket is a singly-linked list of primitives at that depth. (Draw_Tile,
  Draw_Font_Poly, and the per-face drawers all do this.)
- Draw_All(flush): VSync (if arg) → PutDrawEnv/PutDispEnv (set draw+display framebuffer rects) →
  DrawOTag(OT_base + (otsize-1)*4): walk the OT from the LAST bucket following each prim's next-link,
  rasterizing in order = BACK-TO-FRONT painter's algorithm. No depth buffer; ordering is by z-bucket.
- Primitive packet format (PSX GPU codes): byte at +7 = command/len tag (e.g. 0x60 flat 4pt tile,
  0x1c/0x2c textured sprite w/ optional semi-transparency bit |2), +1.. = packed RGB (via `rgb_lookup`
  table: RGB->framebuffer pixel), +2.. = screen XY, +0xe.. = UV / size, +0x16 = clut/tpage. The
  per-face drawers (draw_face_*) fill these from the GTE-projected verts + TDF texture entry + CLUT.

## Port implications (WebGL2)
- Depth: replicate OT ordering (assign each primitive a z-bucket from its GTE z, draw back-to-front with
  blending) rather than a GL depth buffer — matters for transparency and coplanar decals/road markings,
  which the original orders explicitly. A GL z-buffer would mis-order semi-transparent polys vs the original.
- Transform: implement the Q12 fixed-point GTE math for determinism/faithfulness (RotTransPers screen
  coords, OuterProduct12 normals, gte_dpcs/ncds shaded colour) — the projected coords + shaded colours
  are what define the look, not GL's float pipeline.
- Primitive types map to GL draws: flat poly (0x60)=colored tri/quad; textured (0x1c/0x2c/pict)=VRAM/CLUT
  textured quad (see 01-data-io textures); sprite=2D billboard; font=2D quad.
