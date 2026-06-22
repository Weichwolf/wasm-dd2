#!/usr/bin/env python3
# WIP: parse DD2 authentic geometry (section-0 chunks -> objects -> faces). Vertices parse cleanly;
# face records are per-type (~40 types, varying size+index offset). Type 12 = 20B quad, idx@+4..+10.
# Needs per-type size/index map (from each draw_face_* fn) + GTE + VRAM/CLUT to render faithfully.
import struct, importlib.util
spec=importlib.util.spec_from_file_location("lzss","tools/lzss.py"); lz=importlib.util.module_from_spec(spec); spec.loader.exec_module(lz)
def objects(dat):
    d=open(dat,'rb').read(); subn=struct.unpack_from('<I',d,116)[0]//4
    sub=[struct.unpack_from('<I',d,116+i*4)[0] for i in range(subn)]
    for si in range(subn):
        try: out,_=lz.decompress(d,116+sub[si])
        except: continue
        if len(out)<8: continue
        cnt=struct.unpack_from('<I',out,0)[0]
        if not (0<cnt<4000): continue
        for r in range(cnt):
            rec=4+r*16
            if rec+16>len(out): break
            off,px,py,pz=struct.unpack_from('<4i',out,rec)
            if not (0<=off<len(out)-0x2c): continue
            nv=(struct.unpack_from('<I',out,off+8)[0]>>16)&0xff
            rel20=struct.unpack_from('<I',out,off+0x20)[0]
            if not(0<nv<2000): continue
            verts=[struct.unpack_from('<3h',out,off+rel20+i*8) for i in range(nv) if off+rel20+i*8+6<=len(out)]
            yield (px,py,pz),verts
if __name__=='__main__':
    import sys; n=sum(1 for _ in objects(sys.argv[1])); print(n,"objects")
