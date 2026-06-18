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

## TODO formats
- [ ] LEVEL.PAL / .CLT exact color encoding (RGBA? BGRA? 5551? VGA 6-bit?).
- [ ] LEVEL.TX* texture page layout (dimensions, header?, palette association).
- [ ] LEVEL.DAT geometry (vertices/faces; saw u16 index lists like `04 00 05 00 07 00 06 00`).
- [ ] LEVEL.SPR sprite atlas.
- [ ] FONT.BNK bitmap font.
- [ ] VAGS\BANK1.SBK — PS1 VAG ADPCM bank (header + entries).
- [ ] CLT collision structure; ECL (86016 fixed) purpose; TDF.
