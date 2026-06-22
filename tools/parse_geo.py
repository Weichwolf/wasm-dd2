#!/usr/bin/env python3
# Parse DD2 authentic geometry: section-0 LZSS chunks -> objects -> vertices + type-12 flat quads.
# Type 12 (≈90% of faces) = 20B record: RGB@+4, four u16 vtx indices @+12. Other types (8/37/41/textured) TODO.
import struct, importlib.util
spec=importlib.util.spec_from_file_location("lzss","tools/lzss.py"); lz=importlib.util.module_from_spec(spec); spec.loader.exec_module(lz)
def parse(dat):
    d=open(dat,'rb').read(); subn=struct.unpack_from('<I',d,116)[0]//4
    sub=[struct.unpack_from('<I',d,116+i*4)[0] for i in range(subn)]; quads=[]
    for si in range(subn):
        try: out,_=lz.decompress(d,116+sub[si])
        except: continue
        if len(out)<8: continue
        cnt=struct.unpack_from('<I',out,0)[0]
        if not(0<cnt<4000): continue
        for r in range(cnt):
            rec=4+r*16
            if rec+16>len(out): break
            off,px,py,pz=struct.unpack_from('<4i',out,rec)
            if not(0<=off<len(out)-0x2c): continue
            nv=(struct.unpack_from('<I',out,off+8)[0]>>16)&0xff
            r20=struct.unpack_from('<I',out,off+0x20)[0]; r28=struct.unpack_from('<I',out,off+0x28)[0]
            if not(0<nv<2000): continue
            V=[struct.unpack_from('<3h',out,off+r20+i*8) for i in range(nv) if off+r20+i*8+6<=len(out)]
            g=off+r28
            for _ in range(64):
                if g+4>len(out): break
                fc=struct.unpack_from('<H',out,g)[0]; ft=out[g+2]; term=out[g+3]; g+=4
                if term==0 or not(0<fc<3000): break
                if ft!=12: break
                for fi in range(fc):
                    rr=g+fi*20
                    if rr+20>len(out): break
                    idx=struct.unpack_from('<4H',out,rr+12); col=(out[rr+4],out[rr+5],out[rr+6])
                    if all(i<len(V) for i in idx):
                        quads.append(([(px+V[i][0],py+V[i][1],pz+V[i][2]) for i in idx],col))
                g+=fc*20
    return quads
if __name__=='__main__':
    import sys; q=parse(sys.argv[1]); print(len(q),"type-12 quads")
