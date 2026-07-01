#!/usr/bin/env python3
# Re-base the memory-image snapshot from dd2.exe (320x240) to dd2h.exe (640x480): insert a 0x38400 zero gap
# at VA 0x713050 (file offset 0x313050) -- right after _screenbuffer@0x700450 -- so all DATA >=0x713050 moves
# up by 0x38400 to match dd2h's larger framebuffer .bss. Pairs with transpile's DD2_DD2H +0x38400 literal shift.
#   usage: tools/make_dd2h_image.py <in dd2.exe image> <out dd2h image>
import sys
inp = sys.argv[1] if len(sys.argv) > 1 else "DestructionDerby2/dd2_image.bin"
out = sys.argv[2] if len(sys.argv) > 2 else "build/dd2_image.bin"
GAP = 0x38400
OFF = 0x713050 - 0x400000   # file offset of the framebuffer end / gap insertion point
img = open(inp, "rb").read()
dd2h = img[:OFF] + b"\x00" * GAP + img[OFF:]
open(out, "wb").write(dd2h)
print("dd2h image: %s (0x%x) -> %s (0x%x), inserted 0x%x gap at VA 0x713050" %
      (inp, len(img), out, len(dd2h), GAP))
