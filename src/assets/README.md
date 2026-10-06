# Assets

Decode original tracks, cars, textures and other data into documented structures.
Validate lengths, offsets and ownership at loading boundaries. Original files
remain ignored/provisioned data; do not embed assets or memory dumps in source.

`archive.h` provides a validated, read-only view of the original `Dirinfo`
container. The caller owns the loaded bytes. The archive owns its directory
index; returned `dd2_asset` values borrow the caller's bytes and contain the
exact declared length, excluding sector padding. Close the archive before
releasing its bytes, and stop using asset views when those bytes are released.
Opening failures leave the output archive null. Failed entry/lookups clear the
output asset. Null close is valid.

The original `Read_Directory`, `FUN_00415498` and `File_Load` in `re_out/dd2.c`
establish this on-disk layout:

| Offset in each 24-byte directory row | Field |
| --- | --- |
| 0 | 18-byte field containing a null-terminated ASCII name |
| 18 | Little-endian unsigned 16-bit sector number |
| 20 | Little-endian unsigned 32-bit logical file length |

A row whose first name byte is zero ends the directory. Ignore bytes after the
name's first null: the shipped font entry contains unrelated bytes in this
padding. Sector size is 2048 bytes, and data starts at sector 5. The original
reads `0x2808` directory bytes even though five reserved sectors contain only
`0x2800` bytes; the last eight bytes belong to the first payload. Termination,
rather than scanning unused rows, is essential. File views use the declared
logical size, without the original loader's sector-rounded overread.

The decoder checks the directory read extent, name termination/ASCII, duplicate
names, sector placement and payload bounds before returning an index. Lookup
uses exact original spelling, including backslashes. No OS file access, absolute
game address, original memory image or register emulation is involved. The following modules decode level, mesh and texture data; audio formats remain
to be implemented.

`rewrite_archive` in CTest covers valid views, exact lookup, null/invalid
arguments, truncated headers, duplicate/invalid names, missing termination,
out-of-range sectors/lengths and a zero-length view exactly at EOF. It runs
without proprietary assets in CI. With provisioned data, run:

```sh
make rewrite-archive-verify
```

This checks all 114 original entries against an independent Python directory
reader using exact names, offsets, lengths and full-payload FNV-1a64. Native,
Node/WASM and ASan/UBSan must agree, reject eight corrupted original archives
without crashes or sanitizer diagnostics, and accept an archive with unnecessary
tail padding removed. Only the Node test binary enables host filesystem access.
The bounded report stays under `/tmp/wasm-dd2/`; completed raw output is removed.

## Level data and textures

`level.h` decodes a borrowed `LEVEL.DAT` view into 29 read-only sections, with
explicit vertex and texture-definition accessors. The first 116 bytes are 29
little-endian 32-bit offsets relative to the file start, as established by
`FUN_00445ca8`. Consecutive offsets define section lengths; the last section
ends at the declared file length. Equal offsets are valid empty sections. The
decoder checks every extent before publishing a view, without modifying or
relocating any bytes. Failed decoding/access clears the output value.

The known sections are scene blocks (0), road records (1), signed 32-bit XYZ road
vertices (2), sprites (3), texture definitions (4), and low/medium/high car shapes
(15/16/17). Road vertices occupy 12 bytes each; negative values are decoded
without relying on implementation-defined unsigned-to-signed casts. Section 4
contains a 32-bit count and 12-byte definitions: a 16-bit texture page/flags word,
a reserved 16-bit word and four byte-sized UV pairs. The reserved word is zero
in every shipped definition; the polygon selects its palette bank. Other
sections remain borrowed views until their individual formats are implemented.

`textures.h` assembles the original `TX0` through `TXn` files into an owned
256-by-8192 index atlas (32 pages). PAL, CLT and optional ECL lookup bytes remain
borrowed and must outlive the texture set. The atlas view expires at destruction;
RGBA pages are written into caller-owned buffers. Invalid page/bank/shade/output
arguments leave that buffer unchanged.

`sub_415000`, `Load_Textures` and `LoadImage` establish the texture layout. TX0
starts with a 32-bit image count and 16-byte descriptors. Each descriptor has
16-bit format, byte width, height, X and Y fields followed by six bytes unused
by the loader. Images follow in descriptor order across all TX files. Each
has a `TEXT` marker followed by width times height expanded 8-bit texels; format
4 and 8 both use these expanded bytes. A file boundary separates whole images.
Images may cross page boundaries and overlap earlier images; original loading
order determines the final texels. Bounds, markers, counts and every part's
complete extent are checked before returning a texture set.

CLT contains palette banks of 16 shade rows by 256 index mappings; ECL appends
additional banks. `draw_text_half`/`FUN_0041033a` establish the lookup
`bank * 4096 + shade * 256 + texel`; its byte selects a BGR entry in the
1024-byte PAL table. Convert BGR to RGB for the output page; the source palette's
fourth byte is unused and never supplies output alpha. `SetPalette` swaps the
same channels before publishing DirectDraw palette entries. The neutral
shade is 8. Cutout follows the original low-nibble rule, independent of the
mapped palette index. Material cutout selection and other blending/lighting policies belong to the
renderer; low-nibble zero alone does not imply that every material is masked.
`dd2_texture_palette_index` exposes a checked base/ECL shade lookup so the
renderer can test the actual palette index, independent of its RGB color.
Failed lookup clears the output index.

```sh
make rewrite-level-verify
```

This runs the strict native checks and the synthetic CTests on native and Node/WASM,
then ASan/UBSan checks and an independent complete comparison for all 13 shipped
level containers. Every section offset/length, signed vertex and UV field, all
atlas bytes, all 32 cutout RGBA pages at neutral shade, and dark/bright samples
from the last palette bank are compared byte for byte. Six corrupted original
inputs must be rejected on all three targets without crashes/sanitizer errors.
Successful asset exports are deleted immediately after comparison; reports stay
under `/tmp/wasm-dd2/`. These are asset-format checks, not full-game parity claims.
CI uses synthetic data to cover null/bounds/format cases, cross-page loading,
multiple TX parts, base/ECL palette banks, shade/cutout selection and an actual
SoftGL upload/alpha-test render covering every framebuffer pixel.
The synthetic PAL fixture has distinct red/blue values and a non-alpha reserved
byte, so decoder and mesh/texture pixel tests reject the previous RGB assumption.

With Wine installed, this checks all 256 palette entries and complete CLT/ECL
bytes against a normal player race in the unmodified original:

```sh
make clean-logs
python3 tools/rewrite/verify_original_palette.py
make clean-logs
```

The diagnostic sends real X11 inputs, reads only the loaded palette and CLUT
banks, retains hashes and removes completed raw logs. These are read-only asset
observations rather than full-game or framebuffer parity checks.

## Scene objects and polygon meshes

`lz.h` decodes the original size-prefixed LZ stream into caller-owned storage.
Control bits run least significant first: one copies a literal, zero reads a
12-bit backward distance and a length of 3–18 bytes. Overlapping copies are
intentional. Truncated tokens, backward references before the output start,
output overflow and mismatched declared size fail; the written count is cleared
on failure. Input/output storage must not overlap. Trailing source alignment is
allowed, and a failed output can contain partial data.

`scene.h` decodes section 0. Its first word is the offset-table byte length;
each 32-bit offset starts a block. Racing levels 1–7 use compressed blocks with
a 16 KiB output limit; arenas 8/9/A/B use raw blocks. A decoded block starts with
an object count and 16-byte instance rows: mesh-relative offset and signed XYZ
placement. Mesh extents end at the next distinct mesh offset or the block end.
Repeated mesh offsets are valid. Each instance owns a decoded mesh; destruction
also handles partial failures. An empty scene section is valid. Input bytes can
be released after decoding; scene objects and meshes are owned by the scene.

`mesh.h` decodes the 44-byte shape header, vertex/normal counts at offsets 10/12
and relative vector/normal/polygon offsets at 32/36/40. Vectors are signed
16-bit XYZ plus a retained auxiliary word, eight bytes each. Polygon groups
start with a 16-bit count, opcode byte and flags byte; zero flags terminate.
Opcodes 0–43 encode flat/textured/Gouraud triangles and quads plus sprite quads.
Lit Gouraud records have one common color and separate corner normal references;
unlit Gouraud records have per-corner colors. The decoder retains opcode,
group flags, attribute word, raw color words, material references and corner
indices/normals, checking every reference and record extent. Rendering policies
for billboard orientation, shading, fog and blending belong to the renderer.

The format evidence is `Decompress`, `Decrunch_Object_Block`,
`Setup_Object_Block`, `Set_Object`, `Pre_Rotate` and the polygon dispatch handlers
in `re_out/dd2.c`. The reference patch `030-gpoly-byteoff-…` corrects Ghidra's
short-pointer scaling: handler offsets describe bytes, not scaled short indices.
The new decoder never relocates original bytes or uses game memory addresses.

```sh
make rewrite-mesh-verify
```

An independent Python decoder compares every exported placement, vector, normal
and decoded face field for all eleven playable original levels, including known
standalone wheel, sky and vehicle shapes (sections 5–21). This covers 6,328 meshes
and 76,856 faces on native, Node/WASM and ASan/UBSan. Six corrupted original
scene/mesh inputs must fail without crashes or sanitizer findings. Synthetic CI
checks overlapping decompression, signed extremes, textured/lit/sprite records,
invalid references, extents and scene ownership. Reports remain under `/tmp`;
raw successful exports are deleted. These checks establish decoded fields, not
complete rendering behavior or game correctness.
