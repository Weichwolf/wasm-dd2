#!/usr/bin/env python3
"""Walk LEVEL.DAT section-1 strip linked-list (the real track topology) and dump records.
From Init_Track_Strip_Numbers: base = section1 + 4; records at base+off; type@0, num@0x0e,
next@0x14, branch@0x1c, byte@0x29. Follow +0x14 to order strips along the track."""
import os, struct, sys
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def sections(d, N):
    ptrs=[];first=None;i=0
    while i+4<=N:
        v=struct.unpack_from('<I',d,i)[0]
        if first is None:
            if v==0 or v>N: break
            ptrs.append(v);first=v
        else:
            if i>=first: break
            ptrs.append(v)
        i+=4
    u=sorted(set(ptrs+[N])); return [(u[j],u[j+1]) for j in range(len(u)-1)], ptrs

def main(level):
    d=open(os.path.join(HERE,'assets','raw',level,'LEVEL.DAT'),'rb').read(); N=len(d)
    S,ptrs=sections(d,N)
    s1=d[S[1][0]:S[1][1]]; nverts=(S[2][1]-S[2][0])//12
    print(f"{level}: section1 size={len(s1)} nverts={nverts}")
    print(f"  s1 header u32: {struct.unpack_from('<4I', s1, 0)}")
    base = 4
    # walk linked list via +0x14
    off=0; seen=set(); order=[]
    u16=lambda o: struct.unpack_from('<H', s1, base+off+o)[0]
    while base+off+0x20 <= len(s1) and off not in seen and len(order)<5000:
        seen.add(off)
        typ = s1[base+off]
        num = struct.unpack_from('<h', s1, base+off+0x0e)[0]
        nxt = struct.unpack_from('<i', s1, base+off+0x14)[0]
        brn = struct.unpack_from('<i', s1, base+off+0x1c)[0]
        b29 = s1[base+off+0x29] if base+off+0x29 < len(s1) else -1
        order.append((off,typ,num,nxt,brn,b29))
        if typ==9: break
        if nxt==off or nxt<0 or base+4+nxt+0x20>len(s1): break
        off=nxt
    print(f"  walked {len(order)} strips")
    for i,(o,typ,num,nxt,brn,b29) in enumerate(order[:12]):
        raw=s1[base+o:base+o+0x30]
        print(f"  strip[{i}] @0x{o:05x} type={typ} num={num} next=0x{nxt:x} branch=0x{brn:x} b29={b29}")
        print(f"      {raw[:0x18].hex(' ')}")
        print(f"      {raw[0x18:0x30].hex(' ')}")
    # gap between consecutive records (record size) for first few
    offs=[o for o,*_ in order]
    print("  record sizes (next-this):", [order[i+1][0]-order[i][0] for i in range(min(8,len(order)-1))])

if __name__=='__main__':
    main(sys.argv[1] if len(sys.argv)>1 else 'LEV1')
