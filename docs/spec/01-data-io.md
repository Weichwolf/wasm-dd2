# 01 — Data I/O & formats (file.C, compress.C, object.C, font.C) — spec

Our-words spec of the on-disk formats + loaders, grounded in decompiled Decompress/Create_Object/
File_Load/Load_Textures/Load_Sprite_Info/Setup_Font and verified empirically. Detailed byte layouts also
in docs/REVERSING.md; this is the authoritative summary.

## Dirinfo archive
- Single packed file (22MB), 2048-byte sectors. TOC entries: {name "LEVx\\FILE.EXT", offset, size, flag}.
- Dirs: LEV0 (front-end), LEV1..LEVB (11 tracks), LEVF (results/special), VAGS (audio). Loaders open a
  file by name → seek offset → read size. flag hints content/compression class.
- File_Load @0x415354 reads a TOC file into a buffer; Add_Buffer_Load_ queues async loads (level boot);
  Load_Completion_Status polls. Set_Load_Textures/Load_Textures assemble VRAM from the level's TX pages.

## LZSS (Decompress @0x415550) — CONFIRMED
- Streaming decompressor with a context {src, dst, remaining_u32, flag}. Block begins with a u32
  uncompressed size (read once when flag==0). Then a bit-stream:
  - control byte read LSB-first (sentinel 0x100 marks "8 bits consumed, fetch next").
  - bit==1 → literal: copy 1 byte src→dst.
  - bit==0 → back-reference: read 2 bytes b1,b2; offset = `(b1 | (b2&0xf0)<<4) - 0x1000` (negative,
    distance back into dst), length = `(b2&0xf)+3`; copy length bytes from dst+offset.
- Used for LEVEL.DAT section-0 object chunks (each chunk = [u32 size][stream]).

## LEVEL.DAT (per track)
- Header: [u32 table_size]; section table of u32 offsets (29 sections for LEV5). Sections:
  - sec0 = LZSS-compressed object chunks (the static scene geometry placements).
  - sec1 = strip/topology. sec2 = int32 vertices (track path). sec3 = embedded SPRITE table (LEVEL.SPR
    24B format). sec4 = embedded TDF (texture table). sec5..28 = AI/camera/collision/sound tables.
- Object chunk: [u32 count][placements {u32 objdef_offset, i32 x,y,z}]. objdef (relocated by Create_Object)
  has **verts@+0x20 (int16 x,y,z,pad)**, **normals@+0x24**, **faces@+0x28**, vert count (u32@+8)>>16,
  rotation@+0x0a. Face list batched: per batch [u16 count][u8 type][u8 term≠0]; ~40 face types dispatched
  via table @0x462d94 (see 06-scene). Flat types (20B record): rgb@+4, vtx indices@+12 (3 or 4 → tri/quad).
  Textured types: texture-index@+8 → TDF entry. (Empirically validated count×size math per level.)

## Create_Object @0x41fc00
- Relocates an objdef in place: turns the stored relative offsets at +0x20/+0x24/+0x28 into absolute
  pointers (base + offset) so the renderer can walk verts/normals/faces directly. Links object into the
  scene/draw list with its order index. (Car objects: vertex ptr set to &car_vertices + carIdx*0x330.)

## VRAM / textures (Load_Textures, LEVEL.TX0..TX4)
- 256-wide 8-bit index VRAM assembled by concatenating the TX-page pool and copying each tile (each has a
  +4 header) to its (destX,destY) from the TX directory ([u32 count][count × 16B dir entries
  {type,w,h,destX,destY,...}]). Height grows to ~256×N.
- LEVEL.TDF (texture table): [u32 count][count × 12B {u16 clut, u16 flags, u16 uv0..uv3}]; UVs are packed
  u8 (u=low,v=high) VRAM coords (page-0; flags=tpage). texture-index from a textured face → TDF entry →
  VRAM UV rect + CLUT id.
- LEVEL.CLT (palettes): N × (256 × RGBA) — 1024B each. Draw palette row = `clut_field * 4` (the engine's
  __clutspace + dth_clut*0x1000 addressing; dth_clut = sprite.cy or TDF.clut). 4th byte = ALPHA/key
  (0 = transparent). Verified: tree foliage green @clut7*4, RING grey metal.

## Sprites / Font
- LEVEL.SPR (Load_Sprite_Info/Setup_Sprite): 24B entries {u16 u,v,w,h,cx,cy,mode + 10B name}. cy → CLUT
  palette (cy*4). Named sprites (CAPRIO, RING, IWRECKIN, DD2L1, BLAKSAIL, …) blitted from VRAM via CLUT.
- FONT.BNK (Setup_Font): glyph metrics (4B x,y,w,h from offset 32, index=ascii-32); glyphs in VRAM v=128;
  indices 16=bg/17=body/18=edge, ink-tinted. Multiple fonts (CFONT/FONT2/FONT3) duplicated w/ recolour.

## Port note
All loaders are deterministic transforms of the user's own Dirinfo data → keep as data-driven loaders.
The WASM build reads the same files (identical native↔wasm). No format is approximated; geo.c/vram.c
already implement these and are validated against the data.
