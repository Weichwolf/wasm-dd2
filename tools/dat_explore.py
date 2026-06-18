#!/usr/bin/env python3
"""Explore LEVEL.DAT sections 0/1 for face/polygon index lists."""
import os, struct, sys
import numpy as np
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(HERE, 'assets', 'raw')

def offset_table(d, base, N):
    """Parse a u32 offset-table header at `base` (offsets relative to base)."""
    ptrs = []; first = None; i = base
    while i + 4 <= N:
        v = struct.unpack_from('<I', d, i)[0]
        if first is None:
            if v == 0 or v > N - base: break
            ptrs.append(v); first = v
        else:
            if (i - base) >= first: break
            ptrs.append(v)
        i += 4
    return ptrs

def top_sections(d, N):
    ptrs = offset_table(d, 0, N)
    uniq = sorted(set(ptrs + [N]))
    return [(uniq[j], uniq[j+1]) for j in range(len(uniq)-1)], ptrs

def u16_stats(d, off, end, nverts):
    a = np.frombuffer(d[off:end - ((end-off) % 2)], np.uint16)
    if len(a) == 0: return 0, 0
    frac = float((a < nverts).mean())
    return len(a), frac

def main(level):
    d = open(os.path.join(RAW, level, 'LEVEL.DAT'), 'rb').read(); N = len(d)
    secs, ptrs = top_sections(d, N)
    nverts = (secs[2][1] - secs[2][0]) // 12
    print(f"{level}: N={N} nverts={nverts}")
    # Section 0 nested sub-table
    s0 = secs[0][0]
    sub = offset_table(d, s0, N)
    print(f"\nSection0 @0x{s0:x}: {len(sub)} sub-pointers; subheader={len(sub)*4} bytes")
    subuniq = sorted(set([s0+p for p in sub] + [secs[0][1]]))
    for j in range(len(subuniq)-1):
        o, e = subuniq[j], subuniq[j+1]
        cnt, frac = u16_stats(d, o, e, nverts)
        slots = [k for k,p in enumerate(sub) if s0+p == o]
        print(f"  sub@0x{o:06x} size={e-o:6d} slots={slots} u16<{nverts}:{frac*100:4.0f}%  head={d[o:o+20].hex(' ')}")
    # Section 1
    o, e = secs[1]
    cnt, frac = u16_stats(d, o, e, nverts)
    print(f"\nSection1 @0x{o:x} size={e-o} u16<nverts:{frac*100:.0f}%  head={d[o:o+32].hex(' ')}")
    # Sections 3,4
    for si in (3, 4):
        o, e = secs[si]
        cnt, frac = u16_stats(d, o, e, nverts)
        print(f"Section{si} @0x{o:x} size={e-o} u16<nverts:{frac*100:.0f}%  head={d[o:o+32].hex(' ')}")

if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else 'LEV1')
