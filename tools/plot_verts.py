#!/usr/bin/env python3
"""Plot candidate vertex section (int32 x,y,z triples) top-down to verify it's a track."""
import os, struct, sys
import numpy as np
from PIL import Image
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(HERE, 'assets', 'raw'); OUT = os.path.join(HERE, 'out'); os.makedirs(OUT, exist_ok=True)

def sections(d, N):
    ptrs = []
    first = None; i = 0
    while i + 4 <= N:
        v = struct.unpack_from('<I', d, i)[0]
        if first is None:
            if v == 0 or v > N: break
            ptrs.append(v); first = v
        else:
            if i >= first: break
            ptrs.append(v)
        i += 4
    uniq = sorted(set(ptrs + [N]))
    return [(uniq[j], uniq[j+1]) for j in range(len(uniq)-1)], ptrs

def plot(level, sec_index=2):
    d = open(os.path.join(RAW, level, 'LEVEL.DAT'), 'rb').read(); N = len(d)
    secs, ptrs = sections(d, N)
    off, end = secs[sec_index]
    size = end - off
    nv = size // 12
    v = np.frombuffer(d[off:off+nv*12], np.int32).reshape(nv, 3).astype(np.float64)
    x, y, z = v[:,0], v[:,1], v[:,2]
    print(f"{level} sec{sec_index} @0x{off:x} size={size} -> {nv} verts; "
          f"x[{x.min():.0f},{x.max():.0f}] y[{y.min():.0f},{y.max():.0f}] z[{z.min():.0f},{z.max():.0f}]")
    # top-down scatter x vs z
    S = 512
    img = np.zeros((S, S, 3), np.uint8)
    def norm(a):
        lo, hi = a.min(), a.max()
        if hi - lo < 1: hi = lo + 1
        return ((a - lo) / (hi - lo) * (S - 8) + 4).astype(int)
    px, pz = norm(x), norm(z)
    # color by height
    hh = y - y.min(); hh = (hh / (hh.max() + 1) * 255).astype(np.uint8)
    for i in range(nv):
        img[S-1-pz[i], px[i]] = (hh[i], 255 - hh[i], 80)
    Image.fromarray(img).save(os.path.join(OUT, f'{level}_verts_top.png'))

if __name__ == '__main__':
    for lv in (sys.argv[1:] or ['LEV1', 'LEV2', 'LEV5']):
        plot(lv)
