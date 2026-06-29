#!/usr/bin/env python3
# PIL-free frame comparator (numpy + pure-python PNG decode). Replaces compare_frames.py
# where Pillow is unavailable. Decodes RGB PNG (8 or 16 bit), resizes to 320x240 (nearest),
# reports MAE/255, % pixels close (<24), and luma correlation. Usage: compare_frames_np.py ref.png wasm.png
import sys, zlib, struct, numpy as np

def decode_png(path):
    d = open(path, 'rb').read()
    assert d[:8] == b'\x89PNG\r\n\x1a\n', "not a PNG"
    i = 8; w = h = bitdepth = colortype = None; idat = b''
    while i < len(d):
        ln = struct.unpack('>I', d[i:i+4])[0]; typ = d[i+4:i+8]; data = d[i+8:i+8+ln]; i += 12 + ln
        if typ == b'IHDR':
            w, h, bitdepth, colortype = struct.unpack('>IIBB', data[:10])
        elif typ == b'IDAT':
            idat += data
        elif typ == b'IEND':
            break
    raw = zlib.decompress(idat)
    ch = {0:1, 2:3, 3:1, 4:2, 6:4}[colortype]
    bpp = ch * (bitdepth // 8)
    stride = w * bpp
    out = np.zeros((h, stride), dtype=np.uint8)
    prev = np.zeros(stride, dtype=np.uint8)
    pos = 0
    for y in range(h):
        ft = raw[pos]; pos += 1
        line = np.frombuffer(raw[pos:pos+stride], dtype=np.uint8).astype(np.int32); pos += stride
        if ft == 1:   # Sub
            for x in range(bpp, stride): line[x] = (line[x] + line[x-bpp]) & 255
        elif ft == 2: # Up
            line = (line + prev) & 255
        elif ft == 3: # Average
            for x in range(stride):
                a = line[x-bpp] if x >= bpp else 0
                line[x] = (line[x] + ((a + prev[x]) >> 1)) & 255
        elif ft == 4: # Paeth
            for x in range(stride):
                a = line[x-bpp] if x >= bpp else 0
                c = prev[x-bpp] if x >= bpp else 0
                b = prev[x]
                p = a + b - c; pa = abs(p-a); pb = abs(p-b); pc = abs(p-c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[x] = (line[x] + pr) & 255
        out[y] = line.astype(np.uint8); prev = out[y]
    px = out.reshape(h, w, bpp)
    if bitdepth == 16:
        px = px[:, :, 0::2]          # take high byte of each channel
    rgb = px[:, :, :3].astype(np.float64)
    return rgb

def resize_nn(a, W=320, H=240):
    h, w = a.shape[:2]
    yi = (np.arange(H) * h // H); xi = (np.arange(W) * w // W)
    return a[yi][:, xi]

a = resize_nn(decode_png(sys.argv[1])); b = resize_nn(decode_png(sys.argv[2]))
mae = np.mean(np.abs(a-b)); close = np.mean(np.all(np.abs(a-b) < 24, axis=2)) * 100
la = a@[.299,.587,.114]; lb = b@[.299,.587,.114]
la -= la.mean(); lb -= lb.mean()
corr = float((la*lb).sum() / (np.sqrt((la*la).sum()*(lb*lb).sum())+1e-9))
print(f"MAE={mae:.1f}/255  pixels_close={close:.1f}%  luma_corr={corr:.3f}")
