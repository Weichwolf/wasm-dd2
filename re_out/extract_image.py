#!/usr/bin/env python3
"""Extract dd2.exe's loaded image (initialized data + layout) to dd2_image.bin.
RVA-indexed: memcpy'd to wasm addr 0x400000 so RVA+0x400000 == original VA, making the
decompile's raw-VA pointers valid. NOTE: this PE has VirtualSize=0 in every section header,
so the copy MUST use SizeOfRawData (using min(rsz,vsz) yields an all-zero image — the bug
that made the game run on no data)."""
import struct, sys
exe = sys.argv[1] if len(sys.argv) > 1 else 'DestructionDerby2/dd2.exe'
out = sys.argv[2] if len(sys.argv) > 2 else 're_out/dd2_image.bin'
d = open(exe, 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]
opt = struct.unpack_from('<H', d, pe + 20)[0]
imgbase = struct.unpack_from('<I', d, pe + 0x34)[0]
secoff = pe + 24 + opt
secs = []
for i in range(nsec):
    o = secoff + i * 40
    vsz = struct.unpack_from('<I', d, o + 8)[0]
    va = struct.unpack_from('<I', d, o + 12)[0]   # RVA
    rsz = struct.unpack_from('<I', d, o + 16)[0]  # SizeOfRawData
    raw = struct.unpack_from('<I', d, o + 20)[0]  # PointerToRawData
    secs.append((va, vsz, raw, rsz))
hi = max(va + max(rsz, vsz) for va, vsz, raw, rsz in secs)
blob = bytearray(hi)
for va, vsz, raw, rsz in secs:
    if raw and rsz:
        n = min(rsz, len(d) - raw)
        blob[va:va + n] = d[raw:raw + n]
open(out, 'wb').write(blob)
print(f"wrote {out}: {len(blob)} bytes (base {imgbase:#x}, RVA-indexed)")
