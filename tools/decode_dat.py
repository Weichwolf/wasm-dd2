#!/usr/bin/env python3
"""Analyze LEVEL.DAT: header = u32 section-offset table, then sections."""
import os, struct, sys
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(HERE, 'assets', 'raw')

def analyze(level):
    path = os.path.join(RAW, level, 'LEVEL.DAT')
    d = open(path, 'rb').read(); N = len(d)
    print(f"\n===== {level}/LEVEL.DAT  ({N} bytes) =====")
    # Read the offset table: u32s that are non-decreasing and <= N. First data at table[k].
    ptrs = []
    i = 0
    first_data = None
    while i + 4 <= N:
        v = struct.unpack_from('<I', d, i)[0]
        if first_data is None:
            if v == 0 or v > N:
                break
            ptrs.append(v); first_data = min(p for p in ptrs)
        else:
            if i >= first_data:
                break
            ptrs.append(v)
        i += 4
    print(f"header pointers: {len(ptrs)} (header size {len(ptrs)*4} = 0x{len(ptrs)*4:x}); first data @0x{first_data:x}")
    # sections = sorted unique offsets, sizes from gaps
    secs = []
    uniq = sorted(set(ptrs + [N]))
    for j in range(len(uniq)-1):
        off, end = uniq[j], uniq[j+1]
        idxs = [k for k,p in enumerate(ptrs) if p == off]
        secs.append((off, end-off, idxs))
    print("sections (offset, size, which header slots point here):")
    for off, sz, idxs in secs:
        head = d[off:off+16].hex(' ')
        print(f"  @0x{off:06x} size={sz:7d} slots={idxs}  head={head}")
    return d, ptrs, secs

if __name__ == '__main__':
    levels = sys.argv[1:] or ['LEV0', 'LEV1']
    for lv in levels:
        analyze(lv)
