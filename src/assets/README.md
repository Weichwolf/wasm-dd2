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

## Font glyph data

`font.h` copies the original `LEV0\FONT.BNK` tables into owned glyph storage.
The bank owns no pointers into the input, so the caller can release its bytes
immediately. Each font supplies 96 four-byte UV/width/height records for character
codes 32..127, including valid unavailable zero-sized entries. Invalid lookup
clears the output; destruction accepts null. Font texture pixels and sprite
bindings are separate assets and are not decoded by this module.

The file header has a `0x464f4e54` marker at byte 4, declared size at byte 8 and
font count at byte 12. Records begin at byte 16. Each starts with its byte stride;
the glyph table starts sixteen bytes into the record. `FUN_00415508` establishes
this iteration/copy layout. All reads and UV extents are checked. The shipped
last stride includes a four-byte trailing word absent from the logical asset;
the original increments its final pointer without reading it. The decoder
accepts that exact unused difference, keeping every glyph read inside the file.
It also accepts explicit final padding with an updated file size.

`make rewrite-font-verify` runs mandatory Native/WASM gates, compares all 288
original glyphs on Native, Node/WASM and ASan/UBSan, and rejects twelve corrupted
banks. A normal original Wine frontend startup additionally supplies six small
read-only glyph tables: three base fonts and three copied color variants. Every
loaded glyph byte matches, and both save files remain unchanged. This is font
asset/lifetime evidence; text rendering and working menus remain under 0003.

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
Add `--scene-origins --output /tmp/wasm-dd2/rewrite-original-origins` to check
every vertex origin in the original's loaded scene blocks as well. It captures
only their small position tables and compares them with decoded source centers
and shape flags; no full memory image is saved.

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
bounding center. Static shape vertices are relative to a 32768-unit raster-cell
center, not to that bounding center. `origin` is computed per axis as
`floor(coordinate / 32768) * 32768 + 16384`, matching `Setup_Object_Block`.
Header flag bit 7 instead selects local vertices and uses the source center as
the origin. Both positions are retained; the renderer and its bounds use
`origin`. Negative coordinates and signed limits are handled without signed
right shifts or overflow. Mesh extents end at the next distinct mesh offset or
the block end.
Repeated mesh offsets are valid. Each instance owns a decoded mesh; destruction
also handles partial failures. An empty scene section is valid. Input bytes can
be released after decoding; scene objects and meshes are owned by the scene.

`mesh.h` retains the shape flag byte at offset 4 and decodes the 44-byte shape
header, vertex/normal counts at offsets 10/12 and relative vector/normal/polygon
offsets at 32/36/40. Vectors are signed
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

An independent Python decoder compares every exported bounding center, vertex
origin, shape flag, vector, normal
and decoded face field for all eleven playable original levels, including known
standalone wheel, sky and vehicle shapes (sections 5–21). This covers 6,328 meshes
and 76,856 faces on native, Node/WASM and ASan/UBSan. Six corrupted original
scene/mesh inputs must fail without crashes or sanitizer findings. Synthetic CI
checks overlapping decompression, signed extremes, textured/lit/sprite records,
invalid references, extents and scene ownership. Reports remain under `/tmp`;
raw successful exports are deleted. These checks establish decoded fields, not
complete rendering behavior or game correctness.

## Car liveries

`car.h` copies material bindings for twenty stable driver identities and the
three human paint families into owned values. Section 3 starts with a 32-bit
count of 24-byte sprites: seven 16-bit words followed by a ten-byte terminated
name. Horizontal UV is word zero; word one encodes page and vertical UV in a
256-pixel atlas. Word five selects the CLUT bank. Selected coordinates, page,
palette bank, table extent and names are validated before publishing a livery;
failed decoding clears the output. `track.h` owns all twenty-two distinct skins.

The high-detail body retains one immutable geometry. Typed opcode-group regions
choose B/A/D/C/E, small B/A and number palettes, masking the original opcode's
additional 0x02 marker on levels 3/4/6 before selecting the paint family.
Door UVs move from the baked
DR88A template to each driver's DR sprite with byte wrapping; unused triangle
UVs and unrelated materials stay intact. Driver 18 keeps the original baked
number-88 body. Human Amateur/Pro use CLT01*3/CLT01*2 and P1D1T3/P1D1T2; opponent
paint stays independent of the human class. The driving renderer binds stable
driver identity separately from physical start position, and applies regional
deformation without mutating the mesh, definitions or livery. Shifted numbers
receive their own opacity scan rather than reusing another driver's UV cache.

`make rewrite-car-livery-verify` executes unmodified original class/palette,
high-detail paint and door functions against bounded copied data for every
driver/class on eleven levels. It compares all bindings and 99 painted faces
with Native, Node/WASM and fresh ASan/UBSan. Cross-target SoftGL images cover
twenty-two distinct skins, damaged Rookie and restored Rookie on each level.
The fixture deliberately uses high detail for every opponent; it proves this
material component, not original runtime LOD or pixel parity. Class physics,
selectable frontend classes and detached-part effects remain under active 0005.

## Sound effects and Redbook PCM

`audio.h` exposes bounded, immutable RIFF/WAVE and raw CDDA sample views.
WAVE PCM supports unsigned 8-bit and signed little-endian 16-bit mono/stereo.
The decoder checks the complete RIFF extent, format/rate/block alignment,
every chunk extent and odd-byte padding. Unknown chunks are skipped; duplicate
format/data chunks fail. Format and data may appear in either order. The sample
accessor returns signed 16-bit values and validates frame/channel bounds, even
for caller-constructed views. Failed decodes clear their outputs; an empty PCM
stream is valid and has no accessible samples.

SBK banks retain their 16-byte header and 28-byte sound-record layout without
rewriting original bytes with runtime handles. A bank owns up to 64 decoded
metadata records, borrowing its immutable WAVE samples from the supplied bytes.
Those bytes must outlive the bank and every playback user. Repeated sample
extents are permitted. Partial failure frees the bank; destruction accepts NULL.
Loop flags, playback frequency and channel flags are retained separately.
The bank frequency often differs from the WAVE sample rate; the playback owner
must select its actual frequency explicitly.

The format evidence is `FUN_00416404`, `DSLoadSoundBuffer` and `Play_Sound` in
the reference reconstruction. `Redbook/track02.cdda` through `track19.cdda`
are provisioned lossless little-endian 16-bit stereo PCM at 44,100 Hz. CDDA
views validate four-byte frame alignment. Disc selection, looping, pause and
device output belong to the audio runtime rather than the asset decoder.

```sh
make rewrite-audio-assets-verify
```

The verifier checks every decoded sample and metadata field of all 45 original
effects against Python's independent WAVE reader, then every PCM byte and format
field of all 18 provisioned Redbook tracks on Native, Node/WASM and ASan/UBSan.
It checks the provisioned track hashes and sector/frame lengths. Exports are
bounded to one track, compared in chunks and deleted immediately on success;
reports retain source/binary/input hashes under `/tmp/wasm-dd2/`. Synthetic CI
also covers signed extremes, both sample widths/channel counts, all WAVE
truncations, duplicate/reordered chunks, padding, malformed formats/banks,
shared samples and immutable input. These establish audio asset compatibility;
the separate typed audio runtime provides mixing and transport state (see
`src/audio/README.md`). Device output, audible playback, game sound cues and
application CD controls remain pending.

## Road contact geometry

`road.c` owns decoded road vertices, directed links, strips and lane cells.
Racing section 1 begins with a 32-bit record count. Links address records relative
to the byte after that count; headers are 36 bytes, followed by 14 bytes per lane
and four-byte alignment. The decoder follows next, previous and split/merge links,
including gaps in the source layout (Alpine has a 16-byte gap). It validates the
complete declared inventory, record extents, nonoverlap, vertex references and
forward/backward paths returning to the start; invalid cycles fail without an
unbounded traversal. Arena section 1 contains 1,024 14-byte grid records with
1,024 vertices. The 31×31 full cells use adjacent rows of the 32×32 grid.

Strip offsets become bounded indices. Source number, first vertex, flags,
lane-start byte and heading are retained separately from main-loop order.
Split/merge links can address another branch or the start; they are not assumed
to be a single optional shortcut. Cell corners use the original signed row
offset table, including kind 10 in Chalk Canyon. Cells retain separate surface
flags and heading bytes; these are not texture/material identifiers. The first
and last lanes suppress the missing triangles selected by `Map_Height` for
kinds 2–7. Original generated normal bytes are replaced by geometry-based planes.

The original evidence is `Generate_Surface_Normals`, `FUN_00426be4`,
`Track_Follow`, `Map_Height`, `FUN_00428678` and `Init_Track_Strip_Numbers` in
`re_out/dd2.c`. The original image's signed row-offset table is read independently
by the verifier; game modules never read executable addresses.

```sh
make rewrite-road-verify
```

This compares every playable level's vertices, graph, source attributes and
lane geometry on native, Node/WASM and ASan/UBSan, plus both triangle centroid
contacts against independent integral edge tests and rational plane heights.
It rejects corrupted original links, extents, types, counts, lane widths,
vertex references and branch cycles. Synthetic CI checks slopes, shared edges,
outside/degenerate contact, source ownership, branches, holes and arena limits.
This covers data and vertical contact geometry, not original suspension,
off-road recovery, lap/checkpoint equivalence or vehicle motion.

`track.c` assembles the level, texture pages, scene, road, high-detail car and
two reusable wheel meshes (sections 5/6) into one owned runtime container for levels 1–11 (source codes 1–9, A and B). Racing scene blocks 1–7 use the original compressed path; arena blocks use their stored path.
The source archive bytes are borrowed and must outlive the container. Partial
loads release their owned structures, and the application swaps a successfully
loaded container only after its materials, camera and driving state are ready.

`barriers.c` copies the source collision boundaries into an owned container,
independent of road/archive lifetime. Each racing strip contributes first/last
lane boundaries using the collision offsets from `Barrier_Collision` and
`Barrier_Corner_Collision`; triangular widening/narrowing cells use their active
diagonal edge. All main and alternate branch strips participate. Arena boundaries
retain the original radii: 14,990, 14,600, 14,400 and 14,400 units. They use an
analytic circle rather than the square height grid. Source executable tables
are used only by the independent verifier; runtime constants/data are typed C.

## Windows save-card container

`assets/save_card.h` owns an exact 128 KiB `SaveGames` image. Its fifteen physical
headers are 512 bytes apart; complete 8 KiB payloads start at offset 8 KiB.
Occupied headers contain little-endian marker one and a filename terminated
within nine bytes. Empty headers, unused name tails, reserved bytes and opaque
payloads are retained. Unsupported occupancy, unterminated names and any other
image extent are rejected without replacing the caller's existing owner.

Logical entries compact occupied physical headers in ascending order. Creating
an entry uses the first free physical header. Replacing an occupied logical
entry first releases its old physical header in the candidate and then uses the
first free header; unrelated occupied entries retain their names and payloads.
New duplicate names are rejected using physical identity, independent of display
compaction. Existing duplicate-name images remain readable as distinct physical
entries with their complete original payloads; opening does not rewrite them. Deletion clears only occupancy and the first filename byte, retaining
the old payload. Names have the original eight-byte limit, including its empty
name case. Complete borrowed payload/name inputs are staged before any mutation.
Invalid, full and duplicate operations preserve every image byte.

`dd2_save_card_image` borrows immutable bytes for a platform adapter to stage and
persist. Container mutation alone does not establish durable save success.
Typed replay payloads, playable-state translation, atomic Native/browser storage
and frontend actions remain separate work under 0008/0003.

`make rewrite-save-card-verify` checks ownership/bounds/rollback on Native, WASM
and fresh O1 ASan/UBSan plus an independent inventory and exact round trip of
the provisioned image. For actual-original mutation comparisons, provide six
isolated reference checkpoints to `tools/rewrite/verify_save_card.py` using
`--original-cards /tmp/wasm-dd2/<run>/original-cards`. Reference commands remain
under the frozen `/tmp` reference. The actual 63-key original run and full-image
comparisons are scoped container evidence; they do not implement persistence UI.


## Original configuration and profile payload

`assets/save_profile.h` decodes a selected complete 8192-byte physical block into
an owned value. Configuration `0x1010`, startup `0x1020` and saved-game `0x3030`
share this packed schema; replay `0x2020` has a separate layout and is rejected.
Each multibyte field is decoded explicitly as little endian, without assuming
C struct layout or alignment.

| Byte offset | Stored fields | Extent |
| --- | --- | --- |
| 0 | Kind and 23 signed settings/session words | 48 bytes |
| 48 | Five seasons, oldest first: eleven track winner/destruction/retirement records, twenty driver win/destruction/retirement records and twenty final standing names | 5 × 940 bytes |
| 4748 | Twenty current drivers: name, total points, division, rank, pending points, finish place, race place, round points and retained reserved bytes | 20 × 54 bytes |
| 5828 | Ten player names | 10 × 12 bytes |
| 5948 | Seven circuit tables, each with five name/minute/second/fraction records | 7 × 5 × 16 bytes |
| 6508 | Controller bindings and their retained suffix | 18 bytes |
| 6526 | Unused physical block suffix | 1666 bytes |

Header word order is documented by `dd2_save_profile_header`. Word nine, at
byte 18, is the controller type, not Redbook volume. The original input poll
sets keyboard type 1 and joystick type 2; the packer has no separate CD gain.
Statistics tracks and lap tables retain source/UI order; mapping them to runtime
level numbers is part of gameplay translation. Lap fractions retain the original
unsigned 16-bit fractional second; conversion
to displayed centiseconds is separate. Track and driver destruction counters
retain their original unsigned 16-bit/8-bit widths respectively.

Fixed text must have a terminator within its field. Bytes after that terminator,
reserved driver data and the unused suffix survive round trips and edits.
Unsupported magic, wrong extents and unterminated text leave the existing owner
or output unchanged. Reads and writes stage overlapping caller storage before
publication. Numeric fields remain lossless, including signed values; parsing a
profile does not validate a playable race, division permutation or unlock range.
The application must translate source IDs and validate the complete candidate
before accepting saved gameplay. Durable adapters and actual frontend actions
remain open under 0008/0003.

`make rewrite-save-profile-verify` compares every typed field and encoded byte
with an independent reader of 48 patterned immutable original-x86 packs,
covering all three supported kinds. Independently predicted edits touch settings,
statistics, standings, names, lap tables, bindings and retained data. Bounds,
320 malformed fixed text fields, rollback and aliases are checked on Native,
Node/WASM and fresh O1 ASan/UBSan, with Native Memcheck. Optional
`--original-cards /tmp/wasm-dd2/<observed-run>` adds the two physically observed
A/B configurations from `saved-A-B.card` and its identified original receipt.
These are codec comparisons, not proof of saved-game continuation or durability.
