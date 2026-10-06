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
game address, original memory image or register emulation is involved. Game data
still needs typed level, mesh, texture and audio decoders.

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
