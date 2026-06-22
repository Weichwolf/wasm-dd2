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
