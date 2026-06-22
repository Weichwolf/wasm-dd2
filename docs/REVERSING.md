# DD2 Reverse-Engineering Notes

Inputs: `DestructionDerby2/dd2.exe`, `dd2h.exe` (Win32 PE32, software renderer,
imports KERNEL32/USER32/GDI32/DDRAW/DSOUND/WINMM — no Glide/D3D), and `Dirinfo` (22 MB asset archive).
Built Nov 1996. Source tree was `C:\pcdd2\...` (C). The EXE embeds **function names**
(assert/debug macro) — grep `strings dd2.exe` for `_Track_`, `Init_`, `Calc_`, etc. to map the disassembly.

## Dirinfo archive (DONE)
- TOC: 114 records starting at offset 0, contiguous. Record = `NAME\0` + binary field.
- 9-byte field: `[flag u8][reserved u16=0][sector u16 LE][size u32 LE]`; **byte offset = sector × 2048**.
  - `flag` usually 0; occasionally non-zero (e.g. LEV0/LEVEL.DAT flag=0x4e) — meaning TBD (type/compression?).
- Special short records (header files):
  - `COPYRIGH.BMP`: 6B `[u16 sector][u32 size]`
  - `LOADING.BMP`:  7B `[u8 flag][u16 sector][u32 size]`
  - `FONT.BNK`:     10B `"RAW\0" + [u16 sector][u32 size]`
- Proof: consecutive files tile end-to-end (sector-aligned), last file ends ≈ EOF, 99.5% coverage, 0 overlaps.
- Per-track files: `LEVEL.DAT` (geometry), `.CLT` (collision; ~80–140 KB), `.PAL` (1024 B = 256 colors),
  `.SPR` (sprites), `.TX0..TXn` (8-bit textures, ~380 KB each = texture pages), some `.ECL` (always 86016 B),
  `.TDF` (small, track def?). Globals: `COPYRIGH.BMP`, `LOADING.BMP`, `FONT.BNK`, `VAGS\BANK1.SBK` (PS1 ADPCM sound bank).
- Note `.ECL` is exactly 86016 = 0x15000 bytes for every track that has it → fixed-size table.

## Track list (from EXE strings)
Track type names: Speedway, Stunt, Liberty, Forest, SCA, Caravan, Ultimate. 16 dirs LEV0..LEVF
(LEVC/D/E exist per strings; LEV0 looks like the front-end/menu level: it has COPYRIGH/LOADING/FONT
and no ECL/TDF). `TRACK%02d` / `TRACK%02dL` naming in stats.

## Decompilation (Ghidra) — dd2.exe ships WITH CodeView debug symbols
- Tooling (userspace, no sudo): JDK21 `/home/cosmo/tools/jdk-21.0.11+10`, Ghidra 12.1.2
  `/home/cosmo/tools/ghidra_12.1.2_PUBLIC` (set `JAVA_HOME_OVERRIDE` in support/launch.properties).
  Analyzed project saved at `/home/cosmo/tools/dd2_ghidra_proj` (also /tmp/re/ghidra_proj).
- Regenerate decomp:
  `JAVA_HOME=/home/cosmo/tools/jdk-21.0.11+10 PATH=$JAVA_HOME/bin:$PATH \
   /home/cosmo/tools/ghidra_12.1.2_PUBLIC/support/analyzeHeadless /tmp/re/ghidra_proj dd2 \
   -process dd2.exe -noanalysis -scriptPath /tmp/re -postScript ExportDecomp.java`
- Outputs: `re_out/functions.txt` (837 funcs: addr, size, name), `re_out/dd2_decomp.c` (44k lines, real names).
- Extract one function: `tools/refunc.sh 'Car_Drive_Motion @'`.
- `Read_Directory`: fread 0x2808 bytes into `dirbuf` => TOC = first 0x2800 bytes (matches our format).

## Physics model (from decompiled C) — to port faithfully
- **Global car array**, stride **0x1b2 (434) bytes/car**, indexed `car * 0x1b2` off base ~`0x75a5dc`.
  Known fields (offset within car): +0x34 `0x75a610` & +0x3c `0x75a618` = velocity components (x,z);
  +0x624 area angles; +0x67e speed-ish; +0x6a6..6b2 = 4 wheel values (susp); +0x6c6 flag; +0x72a param.
- **Fixed-point**: `>>0xc` = /4096 (12-bit frac). Thrust const `0xccb0` (52400). Decay e.g. `x - x/256`.
- Per-car **control input** ptr: `*(byte**)(car*0x2c + 0x75a2a0)`; steer/accel as 16.16 (`>>0x10`).
- `Car_Movement` = state machine: 0 normal drive (`Car_Drive_Motion`+`_3D`+`Barrier_Collision`),
  1 2pt, 2 2pt_3D, 3 1pt_3D, 4 fly, 5 grounded(rolled). AI/track cars use case 0.
- Key fns to port: `Car_Drive_Motion`, `Car_Drive_Motion_3D`, `Calc_Suspension_*`, `Car_Friction`,
  `Do_Car_Collisions`/`Check_*_Car_Collision`, `Barrier_Collision`; AI: `Track_Follow`, `AI_CommandList*`.
- Suspension (`Calc_Suspension_Right_Wheels`): per-wheel spring-damper in fixed-point. Wheel-local state
  at car+0x6...: pos `+0xce`, vel `+0xde`; spring stiffness const `0x280`, damping `0x200`, travel clamp
  `±0x1f000`. Ground contact from car orientation rows (`0x75a640/0x75a648`) · wheel pos array
  (`0x75d5b8`, stride 0x20/car). `Car_Friction` is a global var (friction applied inline), not a function.
  NOTE: some Ghidra names have a stray trailing `"` (e.g. `TransformWheels"`).

## Real driving model (Car_Drive_Motion @004413ac) — extracted, for the faithful port
DD2 uses a slip-angle tire-force model, fixed-point (0x1000 = 1.0):
- **Surface tables** (12-bit): `surface_friction_coeff @0x466d90 = {4096,2048,...}` (1.0 grippy / 0.5 slippery),
  `surface_traction_coeff @0x466d98`. Indexed by `(*input & 7) >> 1` = surface type 0..3 under the car.
- **Handling types** (field car+0x75a75a = 0/1/2): per-type, per-slip-direction grip split
  (`local_24` long, `local_1c` lat), baseline 0x800 (=0.5): type0 {0x862/0x79e}, type1 {0x6be/0x942 or 0x894/0x76c},
  type2 {0x592/0xa6e or 0x8e4/0x71c}. Encodes understeer/oversteer per car class.
- **Thrust** added to velocity = `(input>>16) * 0xccb0 >> 12` (0xccb0 = 52400) along facing axes.
- **Steering authority** falls with speed: `factor = 0xc000/(speed+0x10) + 0x400`.
- `Car_Friction @0x465a20 = {44000,54000,52000,...}` per-track wheel-lock speeds (Caravan/Forest/etc).
- Velocity stored at car+0x34 (`0x75a610`) & +0x3c (`0x75a618`); decay e.g. x - x/256. `rcos/rsin` = fixed trig.
- Plan for faithful port: build `vehicle_dd2.c` implementing this exact model + constants behind a build
  flag, validate it still produces plausible racing on all tracks (vs the current arcade model = fallback),
  then swap in. (Full bit-exact transliteration incl. collision/AI command lists is a larger multi-pass effort.)

## Real track topology — LEVEL.DAT section 1 = strip graph (decode in progress)
The engine's `_strip_data` = `*(int*)(_level_data + 4)` → **LEVEL.DAT section 1** (the section with
86% valid vertex indices; header `[340, ?, 1537, ...]`). It's a **linked-list/graph of strip nodes**
(from `Init_Track_Strip_Numbers @443da4`):
- strip record (>=0x20 bytes): `+0x00` type byte (8 = fork/junction, 9 = '\t' terminator),
  `+0x0e` u16 strip number (assigned by traversal), `+0x14` int "next" offset (relative to `_strip_data+4`),
  `+0x1c` int alternate/branch "next" offset (forks), `+0x29` a byte (surface/flags?).
- There's a strip lookup table at stride `0xe` (`_strip_data + n*0xe`).
- Traversal follows `+0x14` (and `+0x1c` at forks) to order strips along the track.
**Why this matters**: walking this graph gives the TRUE strip ordering + each strip's geometry refs,
which replaces the cross-section heuristic and eliminates the infield-chord artifacts (LEV4) and any
mis-ordering. Next: map each strip's vertex/poly references (into section 2 verts), prototype the real
road mesh in Python, then integrate into `track.c` behind a flag and validate vs the current model.

Progress (`tools/decode_strips.py`): the list walks cleanly — **LEV1 = 276 strips** (≈283 cross-sections).
Record (variable len, 92–120 B): `+0x00` type (1 normal, 3/6 corner-ish, 9 end), `+0x01` count byte (4–8),
`+0x04`/`+0x06` = constant flags on straights (255/49), `+0x14` next, `+0x1c` branch (0 on simple loops).
The per-strip **geometry** is in the variable tail (likely PSX poly primitives + vertex refs) — NOT yet
decoded; `+0x04` is not a turn/heading delta (ruled out). **DECISION**: deep decode paused — the
cross-section reconstruction already yields clean drivable tracks (10/11 clean; LEV4 cosmetic chord only).
Confirm with user whether full real-geometry is worth the effort/risk before resuming. The strip ORDER
alone could later be used surgically to fix mis-ordering if a strip→vertex-range map is found.

CORRECTION (later analysis): section-1 strip record **tails are mostly zeros** — the earlier "86% valid
u16 indices" was inflated by those zeros, NOT real vertex references. So **section 1 = pure topology**
(strip graph: type/links/params, used for AI track-following), and the actual **render geometry is in
section 0** (the 28 sub-section "strips" with int16 coord/display-list data, ~9% index density) — the
genuinely hard PSX-primitive decode. This reinforces the pause: the real-geometry fix means decoding
section-0 display lists, high effort/uncertain payoff vs the already-clean cross-section reconstruction.
If revisited, the lever is section 0, not section 1. Section-1 topology could still order strips, but
strip records carry no obvious position (mostly zeros), so it's not a quick ordering fix either.

FURTHER (read `Draw_Screen_Polys @450ed4`): the PC port **emulates the PlayStation GTE** — the draw
path uses GTE ops (`GTERPT`/`GTERPS`) and the GTE register block at `0x714xxx` (`__vr0..3`, etc.).
So the real geometry pipeline = display lists transformed through an emulated GTE (PSX-hardware-level).
Faithfully reproducing it ≈ reimplementing PSX GTE geometry — a very deep effort for exact polygons/
walls/scenery, low marginal value over the working cross-section reconstruction (clean, drivable, all
tracks pass). **Conclusion: geometry-fidelity is genuinely a large PSX-RE project; do it only if the
user wants pixel-faithful tracks. Otherwise the current reconstruction is the pragmatic answer.**

## Front-end / sprite / VRAM model (for the faithful WASM port)
- `COPYRIGH.BMP`/`LOADING.BMP` = standard 320x240 8-bit Windows BMPs (title/loading), **top-down**.
- `LEVEL.SPR` = `[u32 count][count x 24B]`; entry = `[u16 u][u16 v][u16 w][u16 h][u16 cx][u16 cy][u16 mode=4]` + 10-byte name @0x0e.
  `Load_Sprite_Info`: count first; `Search_For_Sprite` strcmp at +0xe; stride 0x18. CAPRIO=(0,0,256,128) = TX top-left billboard.
  v ranges 0..6256 => sprites address a **tall virtual VRAM** = the TX pages stacked; (cx,cy)=CLUT loc; mode 4 = PSX 4-bit CLUT.
- Font: `Setup_Font`->`Setup_Sprite` (glyphs are sprites); `Allocate_Font_Buffers` (src path `C:\PcMpe\graphics\font.C`),
  `Print`/`Print_Locate`/`Print_Ink` render strings. `MPE_malloc` = engine allocator ("MPE" = the Psygnosis/Reflections lib).
- Renderer = PSX-style: load TX pages into a VRAM atlas, address via (u,v)+CLUT, geometry via emulated GTE (`Draw_Screen_Polys`).
  => faithful 3D view = software GTE + VRAM/CLUT texturing + the section-0 display lists.

## TODO formats
- [ ] LEVEL.PAL / .CLT exact color encoding (RGBA? BGRA? 5551? VGA 6-bit?).
- [ ] LEVEL.TX* texture page layout (dimensions, header?, palette association).
- [ ] LEVEL.DAT geometry (vertices/faces; saw u16 index lists like `04 00 05 00 07 00 06 00`).
- [ ] LEVEL.SPR sprite atlas.
- [ ] FONT.BNK bitmap font.
- [ ] VAGS\BANK1.SBK — PS1 VAG ADPCM bank (header + entries).
- [ ] CLT collision structure; ECL (86016 fixed) purpose; TDF.

## TX page directory (refined)
TX = [u32 count][count x 16B: tag=4, w, h, destX, destY, f5(srcCol,0..240 step16), f6(srcPage 0..19), 0][pixels].
Sum(tile w*h) >> pixel bytes => tiles SHARE a compact source pool; VRAM built by copying source
region (selected by f5/f6) to dest (destX,destY) in the tall virtual VRAM. CAPRIO/(first tile) verified.
TODO: exact source addressing from f5/f6 (+ srcY?) — read the TX-load/Decrunch_Object_Block path.

## CLUT system (LEVEL.CLT)
- `LEVEL.CLT` = RAW 16-bit CLUT data (no header), 81920 B = 160 x 256-colour CLUTs. Loaded into
  `__clutspace` (PSX CLUT VRAM, row stride 0x100=256 words) by `Load_Cluts` (count+entries built elsewhere).
- 16-bit colour = PSX 1555 (R low 5, G next 5, B next 5, bit15=STP). Textures are **8-bit** indices
  (confirmed: 22 TX0 tiles consume exactly Σ(w*h) bytes); a sprite/poly selects a CLUT via (cx,cy):
  word offset ≈ `cy*256 + cx*16`. DRIVER1 (cx48,cy0 -> off 768) gives the right SHAPE but wrong colours
  => exact (cx,cy)->CLUT mapping / index base still to pin down. CAPRIO renders fine with LEVEL.PAL.
- Font glyphs are sprites (Setup_Font->Setup_Sprite); text colour set by `Print_Ink` — so menu text may be
  ink-tinted intensity rather than CLUT, check `Print`.

## VRAM multi-page assembly (key)
- TX0 = [u32 count=316][316 x 16B dir entries][page-0 pixel pool]. TX1..TXn = RAW source pixel pools
  (no directory; first bytes are pixels). Dir entry = (tag=4, w, h, destX, destY, f5=srcCol, f6=srcPage, 0).
- Each entry copies a w*h tile from source page f6 (0=TX0 pool, 1=TX1, ...) at column f5 -> VRAM (destX,destY).
  First ~22 entries (srcPage 0) read TX0's pool sequentially (each tile preceded by 4-byte header);
  later entries reference pages 1..n. The FONT atlas is a dir entry at destY=128,h=125 (sources another page).
- So full VRAM = process ALL 316 entries copying from the page pools. srcY within a page still TBD
  (f5=srcCol 0..240 step16). This fills font/sprites/textures; gates correct front-end + textured 3D.

## FULL VRAM + FONT (SOLVED)
- Full VRAM = concatenate source pools [TX0 pool after its 316-entry dir] + TX1 + TX2 + TX3 + TX4,
  then read all 316 dir entries SEQUENTIALLY, each [4-byte header][w*h 8-bit pixels], place at (destX,destY).
  Σ(4+w*h)=1,638,736 = exact total pool. Fills entire 256-wide VRAM (sprites, font, textures).
- FONT: sprite "FONT" at VRAM (0,128,256,125); FONT.BNK = [16B hdr][3 font metric tables].
  Glyph metric = 4 bytes (gx,gy,gw,gh) from offset 32; **glyph index = ascii-32**. Glyph pixels use
  indices {16=background, 17=body, 18=edge}; tinted by Print_Ink colour. "DESTRUCTION DERBY 2" verified.
- FONT2/FONT3 = sprites at (0,6256)/(0,1506); LETTERS sprite at (0,1664,256,92).

## CLUT addressing (sprite colours)
- Draw CLUT pointer = `__clutspace + (dth_clut*0x10 + _dth_shade)*0x100` bytes (__clutspace=0x6c0100 fixed,
  stride 0x100=256B/row=128 16-bit colours/row; shade=row offset for palette-anim).
- `dth_clut = *(u16*)(drawstruct+0xe)`, built from the sprite entry's cx/cy in FUN_00416714 (Setup_Sprite).
- __clutspace is filled by Load_Cluts from a (count+entries+data) struct; LEVEL.CLT is the raw colour data
  but the (cx,cy)->dth_clut->row mapping needs FUN_00416714 fully traced. Font is ink-tinted (no CLUT) so
  menus render correctly already. Image sprites (track previews/drivers/cars) need this mapping for colour.

## CLUT mapping (PC port) — partial, parked
- Draw-prim CLUT id `dth_clut = sprite.cy` (FUN_00416714: param_1+0xe = piVar6[2]>>16 = cy field);
  clut ptr = `__clutspace + (dth_clut*0x10 + shade)*0x100`.
- `LEVEL.CLT` (PC port) = 80 x (256 x RGBA) palettes — SAME format as LEVEL.PAL (NOT 16-bit PSX VAG-style).
- BUT brute-forcing CHALKCAN's region vs all 80 palettes: logo/text resolve, background stays speckled in
  every palette => the preview BACKGROUND indices in the assembled VRAM look wrong (tile overlap/compositing
  or the preview is composited from multiple elements), not purely a palette pick. Font uses ink (no CLUT) so
  menus are correct. Image-sprite colour needs a runnable reference to pin precisely; PARKED.

## LZSS + LEVEL.DAT object blocks (Decompress @00415550)
- Compression = LZSS ("C:\PcMpe\compress\compress.C"). Block=[u32 uncompressed_size][stream].
  Control byte (8 flags LSB-first): flag1=literal byte; flag0=2 bytes (b1,b2) -> back-offset
  (b1|(b2&0xf0)<<4)-0x1000, length (b2&0xf)+3, copy output[pos+off] (overlap ok). See tools/lzss.py (verified).
- LEVEL.DAT: first u32 = table size in bytes (e.g. 116 => 29 entries); table[i]=byte offset of object block i.
  These compressed blocks are SMALL (decoded 84/234/413/754B) = scenery/prop/object defs+transforms,
  NOT the track. The bulk TRACK geometry is the large uncompressed region (section-2 int32 vertices etc.)
  that the reconstruction already uses. Faithful track render needs section-1 strip/poly connectivity +
  per-poly tpage/CLUT, processed via the software GTE — the remaining deep-fidelity task.

## Section-0 = LZSS-compressed geometry chunks (the authentic track geometry)
- LEVEL.DAT 29-entry table = SECTION table (track.c reads it). sec0=ptr[0]..ptr[1] (~114KB bulk),
  sec2=vertices (int32 x,y,z; verified: vert0 LEV5 = (413,0,-14917)). The earlier "object blocks"
  reading was a misread (413 = a vertex x, not an LZSS size).
- sec0 begins with a sub-table of ~21 u32 offsets (84,4304,9512,...); each points to a geometry CHUNK
  that is LZSS-compressed: [u32 uncompressed_size][LZSS]. chunk0 (LEV5) -> 12105B, decoded head u32
  = [31, 500,413,127,-8818, 832,413,619,-8817, ...] i.e. a count + coordinate records (413 == vertex x).
- => authentic per-chunk geometry is now DECOMPRESSABLE (tools/lzss.py). NEXT PHASE: parse the chunk's
  vertex/face record layout + per-poly tpage/CLUT, build meshes, render via software GTE (the exact
  "in-race view as the original draws it"). Large multi-step task; data layer unlocked.

## Authentic geometry data path (traced) + object format (partial)
Full path: LEVEL.DAT -> section table -> sec0 -> sub-table of ~21 LZSS chunks ->
  chunk = [u32 count][count x {u32 offset, i32 x,y,z}]  (Setup_Object_Block: scene-object PLACEMENTS;
  pos snapped (&0xffff8000)+0x4000, rot from (u16&0x7fff)+0xc000).
  Each placement.offset -> object def in same chunk (Set_Object relocates ptrs @+0x20/+0x24/+0x28).
Object def header (~44B): u32[2]&0xff = vertex count (obj0=15); verts @ +0x2c = int16 x,y,z,pad (8B);
  faces @ +0x24 ptr (PSX poly prims: vtx idx + uv + tpage + clut + rgb — exact layout via Draw fn TBD).
CAVEAT: parsing all 536 objects in LEV5 with the obj0 heuristic gives a NOISY point cloud (74745 pts) =>
  the header/vertex-count/face layout VARIES per object type and needs the draw/parse fn + validation
  (ideally a reference). track.c's section-2 reconstruction remains the working in-race geometry.
=> Decompression + data path: DONE. Correct universal mesh parse + textured GTE render: large remaining phase.

## Object mesh format — CORRECTED from Draw_Subdiv_Object @0041fdbc
Object def (relocated by Set_Object, +base added to ptrs @0x20/0x24/0x28):
  +0x08: u32 with vertex count = (val>>16)&0xff   (obj0 LEV5 = 15)
  +0x0a: u16 rotation (Pre_Rotate)
  +0x20: ptr -> VERTICES  (int16 x,y,z,pad = 8B each)
  +0x24: ptr -> NORMALS
  +0x28: ptr -> POLYGON/FACE list
Face loop (Draw_Subdiv_Object): while (((char*)gpoly)[3]!=0): sVar=*gpoly; type=((byte*)(gpoly+1))[0];
  gpoly+=2; call draw_face_TABLE[type](sVar). Table = PTR_draw_face_3pt_flat_00462d94 -> the
  draw_face_3pt/4pt_{flat,text,pict,gour,sprite}_dpq fns (each reads its own vtx-idx/uv/clut/tpage and
  advances gpoly). So face record size is type-dependent.
Re-parse with these fixes: 599 objs / 10574 verts (vs 74745 garbage before) -> structured scene
  (grandstand grids + track features) but full validation needs the per-type draw fns + a reference.
=> Vertices: parseable. Faces (per-type layout) + textured GTE render: the remaining phase.

## COMPLETE authentic-render pipeline spec (for faithful GTE render)
1. LEVEL.DAT[0]=table-size; section table -> sec0 (geometry), sec2 (int32 verts).
2. sec0: sub-table of ~21 u32 -> each a CHUNK [u32 size][LZSS] (tools/lzss.py).
3. chunk: [u32 count][count x {u32 objdef_off, i32 x,y,z}] = scene-object placements (Setup_Object_Block).
4. objdef (Set_Object relocates +0x20/24/28): vcount=(u32@+8)>>16; +0x20 verts(int16 x,y,z,pad);
   +0x24 normals; +0x28 face-list; +0x0a rotation (Pre_Rotate).
5. face-list (Draw_Subdiv_Object): batched -> [u16 count][u8 type][u8] then count records;
   dispatch table @0x462d94 has ~40 types -> draw_face_* fns. Record sizes (gpoly stride) vary by type:
   3pt_flat=16, 3pt_text/sprite=20, 3pt_gour/4pt_lit/3pt_pict=24/28, 4pt_pict/pict_lit=32. 3pt-vtx idx
   = u16 @ rec+8,+10,+12 (index into per-object vert array). Textured types add uv/clut/tpage in the record.
6. each vertex GTE-transformed (RotTransPers/GTERPS), backface-culled (cross-product sign), depth-sorted
   into an ordering table (OT) by otz, then drawn as VRAM/CLUT-textured prims.
STATUS: pipeline fully MAPPED + decompressable + vertices parseable. Faithful render = implement all ~40
face-type record parsers + software GTE + OT + VRAM/CLUT texturing. Large bounded phase; validation needs
a reference runner (unavailable here). Current shipping in-race view = section-2 reconstruction.

## Face record specifics (sampled) — per-type, varies
- Header (Draw_Subdiv): [u16 count][u8 type][u8 term!=0]; then count records, advance by type-size.
- Type 12 (LEV5 obj0): 20-byte QUAD record: +0 u16 id, +2 u16(0x0d), +4..+10 four u16 vtx indices
  (0,1,2,3), +12 u16(0x0c), +14 u16(0xffff), +16 rgb+byte. => index offset & record size DIFFER per
  type (3pt_flat had idx@+8..+12/16B). ~40 types each need individual size+index RE.
- Real face data confirmed present (clean quad indices). Faithful render still = per-type parsers (~40)
  + software GTE + VRAM/CLUT + OT, validated vs a reference. Characterized wall for autonomous completion.

## BREAKTHROUGH: authentic geometry decodes + renders (type 12 = ~90% of faces)
Face-type histogram (LEV5/1/2): type 12 = 18522 faces (dominant), then 41(1000), 8(724), 37(428).
Type 12 record = 20 bytes: +0 u16, +4 RGB(3)+flag, +8 u16 id +u16, +12..+18 FOUR u16 vertex indices
(quad). Flat-colored (no texture). Parsing all type-12 quads (4219 in LEV5) renders COHERENT scene
geometry (grandstand grids, walls, panels) — verified visually (out/lev5_type12.png). This is the
ORIGINAL geometry, not the reconstruction. Remaining common types 8/37/41 + textured types for full
fidelity; integrate into the C renderer to replace the cross-section reconstruction.

## Geo integration status (geo-only test)
- Rendering real geo ALONE: cars (sim on reconstruction coords) float ABOVE the real-geo scenery, and
  no drivable road surface appears in the flat types (8/12/37/41). So: (a) the road is in OTHER face
  types (textured/skipped), and (b) the real geo's coord origin/Y differs from the section-2-based
  reconstruction the SIM uses. Current build = hybrid: reconstruction road (cars drive on it) + real-geo
  scenery overlay. Full fidelity needs sim+render unified on the real geometry (align coords, find the
  road type, collide vs real polys) — a larger architectural change. Committed render keeps the hybrid.

## Geometry coverage validated across all 11 levels (circuit vs arena)
Object counts (section-0 placements): LEV1 ChalkCanyon 767, LEV2 Colosseum 930, LEV3 Liberty 905,
LEV4 Motorplex 1016, LEV5 Caprio 599, LEV6 PineHills 662, LEV7 TotalDest 753 = the 7 CIRCUITS (full
authentic scene geometry, 12k-24k tris). LEV8 DeathBowl, LEV9 DestrDerby, LEVA RedPike, LEVB ThePit =
0 objects = the 4 demolition ARENAS (open bowls; arena surface is the section-2 reconstruction). So the
geo parser is correct for all 11 levels: circuits get real object geometry, arenas correctly have none.
(DD2's "16 levels" = championship events over these 11 distinct tracks.)

## Texturing reality (PC port draws mostly FLAT)
- draw_face_3pt_flat (type 12, 46%) and the 20B types set per-vertex POSITION + a base RGB only — no UV.
  draw_face_3pt_text (type 10) sets positions + rgb (gte_dpcs) but does NOT read per-vertex UV either:
  textured prims use FIXED UVs mapping a single per-object texture (_gtexture, set in Draw_Subdiv_Object),
  not per-poly UV in the record. So the in-race scene is LARGELY FLAT-SHADED polygons.
- => our flat-colored render of the real geometry (using each poly's real RGB@+4) is faithful for ~99% of
  faces. "VRAM/CLUT-textured geometry" applies to the textured minority (per-object texture page) + road/sky.
  Full texturing = assemble per-level VRAM (like vram.c for LEV0) + per-object tpage + fixed-UV sample.
  Diminishing returns vs the flat majority already rendered.

## Geo-only retest (post Y-align): real geometry HAS a ground/drivable surface
After Y-alignment + broad face-type coverage, rendering geo-only puts the cars ON the real geometry
(ground surface present, structures around) — no float. So the real geo includes the ground, not just
scenery. Path exists to drop the procedural reconstruction road and render the level fully from real geo
(cars still follow the section-2 path, now over real ground). Kept the hybrid (procedural road + real
scenery) for a clearer track read; full real-geo road is a follow-on.

## In-race textures (race levels DO have TX pages)
- Circuits have LEVEL.TX0-2 (~390KB). Decoding LEV5 VRAM (357 tiles): content = TREES (scenery
  billboards), HUD digits 0-9, gauge faces, signs/logos. NOT the road (road/ground = flat geo).
- Colors render off under LEVEL.PAL => need per-tile CLUT (LEVEL.CLT = 80x256-RGBA; dth_clut=cy) — the
  same parked CLUT mapping (needs a reference to validate the (cx,cy)->palette pick).
- So full in-race texturing = per-level VRAM assembly (have it) + per-object texture page (_gtexture in
  Draw_Subdiv_Object) + correct CLUT. Adds textured trees/HUD/signs; road stays flat geo (faithful).
  Gated on the CLUT pick; geometry (flat scene) already faithful.

## In-race CLUT UN-PARKED (self-validated, no reference needed)
- LEV5 LEVEL.CLT = 92 x 256-RGBA palettes. Brute-forcing a tree tile by "greenness" -> palette 84 wins.
- Palette 84 renders LEV5 textures RECOGNIZABLY: tree shapes w/ foliage, HUD digits "0123456789", gauges
  (vs bluish noise under LEVEL.PAL). Self-validating (digits/trees unmistakable) => the level's primary
  texture CLUT is palette 84, found WITHOUT a reference. (Foliage reads brown — desert/autumn or minor
  channel nuance; content is clearly correct.)
- => in-race texturing is now feasible: assemble per-level VRAM + use the level CLUT (e.g. pal 84) to
  color textured polys/sprites + HUD. Next: pick per-level CLUT (greenness/heuristic) + apply to geo textures.
