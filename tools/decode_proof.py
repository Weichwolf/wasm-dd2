#!/usr/bin/env python3
"""M1 proof: decode LEVEL.PAL + a LEVEL.TX texture to PNG for visual verification."""
import os, struct, sys
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(HERE, 'assets', 'raw')
OUT = os.path.join(HERE, 'out'); os.makedirs(OUT, exist_ok=True)

def load_pal(path):
    d = open(path, 'rb').read()
    n = len(d) // 4
    arr = np.frombuffer(d, np.uint8).reshape(n, 4)
    rgb = arr[:, :3].copy()                 # [R G B] pad
    bgr = arr[:, :3][:, ::-1].copy()        # [B G R] pad
    return rgb, bgr

def swatch(pal, path, cell=16):
    n = len(pal)
    side = 16
    img = np.zeros((side*cell, side*cell, 3), np.uint8)
    for i in range(n):
        r, c = divmod(i, side)
        img[r*cell:(r+1)*cell, c*cell:(c+1)*cell] = pal[i]
    Image.fromarray(img).save(path)

def index_image(raw, pal, off, w, h):
    idx = np.frombuffer(raw[off:off+w*h], np.uint8)
    if len(idx) < w*h:
        idx = np.pad(idx, (0, w*h-len(idx)))
    return pal[idx.reshape(h, w)]

if __name__ == '__main__':
    rgb, bgr = load_pal(os.path.join(RAW, 'LEV0/LEVEL.PAL'))
    swatch(rgb, os.path.join(OUT, 'pal_rgb.png'))
    swatch(bgr, os.path.join(OUT, 'pal_bgr.png'))
    print("palette swatches written. palette[1..4] RGB:", rgb[1:5].tolist())

    tx = open(os.path.join(RAW, 'LEV0/LEVEL.TX0'), 'rb').read()
    count = struct.unpack_from('<I', tx, 0)[0]
    print(f"TX0 directory count = {count} (0x{count:x})")
    # directory = count * 16 bytes after the u32 count
    dir_end = 4 + count * 16
    print(f"directory ends at 0x{dir_end:x}; file size 0x{len(tx):x}; pixel bytes ≈ {len(tx)-dir_end}")
    # dump first few directory records
    for i in range(6):
        rec = tx[4+i*16:4+i*16+16]
        vals = struct.unpack('<8H', rec)
        print(f"  rec{i}: {rec.hex(' ')}  u16={vals}")
    # render the pixel region as a 256-wide indexed strip (both palette orders)
    pix = dir_end
    h = (len(tx) - pix) // 256
    Image.fromarray(index_image(tx, rgb, pix, 256, h)).save(os.path.join(OUT, 'tx0_rgb.png'))
    Image.fromarray(index_image(tx, bgr, pix, 256, h)).save(os.path.join(OUT, 'tx0_bgr.png'))
    print(f"tx0 strip 256x{h} written (rgb + bgr)")
