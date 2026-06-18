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

## TODO formats
- [ ] LEVEL.PAL / .CLT exact color encoding (RGBA? BGRA? 5551? VGA 6-bit?).
- [ ] LEVEL.TX* texture page layout (dimensions, header?, palette association).
- [ ] LEVEL.DAT geometry (vertices/faces; saw u16 index lists like `04 00 05 00 07 00 06 00`).
- [ ] LEVEL.SPR sprite atlas.
- [ ] FONT.BNK bitmap font.
- [ ] VAGS\BANK1.SBK — PS1 VAG ADPCM bank (header + entries).
- [ ] CLT collision structure; ECL (86016 fixed) purpose; TDF.
