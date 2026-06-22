#!/usr/bin/env python3
# DD2 LZSS decompressor (from Decompress @00415550, "C:\\PcMpe\\compress\\compress.C").
# Block = [u32 uncompressed_size][stream]. Stream: control byte (8 flags, LSB-first);
# flag=1 -> literal byte; flag=0 -> 2 bytes (b1,b2): 12-bit back-offset=(b1|(b2&0xf0)<<4)-0x1000,
# length=(b2&0xf)+3, copy from output[pos+offset] (overlapping/RLE ok).
import struct, sys

def decompress(data, off):
    size = struct.unpack_from('<I', data, off)[0]; src = off + 4
    out = bytearray(); ctrl = 0; bits = 0
    while len(out) < size and src < len(data):
        if bits == 0: ctrl = data[src]; src += 1; bits = 8
        if ctrl & 1:
            out.append(data[src]); src += 1
        else:
            b1 = data[src]; b2 = data[src+1]; src += 2
            L = (b2 & 0xf) + 3; o = (b1 | ((b2 & 0xf0) << 4)) - 0x1000
            for _ in range(L):
                p = len(out); out.append(out[p+o] if 0 <= p+o < len(out) else 0)
        ctrl >>= 1; bits -= 1
    return bytes(out[:size]), src - off   # (decompressed, compressed_bytes_consumed)

def block_table(dat):
    n = struct.unpack_from('<I', dat, 0)[0] // 4    # first offset == table size in bytes
    return [struct.unpack_from('<I', dat, i*4)[0] for i in range(n)]

if __name__ == '__main__':
    d = open(sys.argv[1], 'rb').read(); tbl = block_table(d)
    print(f"{len(tbl)} blocks; file {len(d)}B")
    for i, off in enumerate(tbl):
        if off+4 > len(d): continue
        sz = struct.unpack_from('<I', d, off)[0]
        if 0 < sz < 2_000_000:
            out, comp = decompress(d, off)
            print(f" block{i:2d} off={off} comp={comp} -> {len(out)}B")
