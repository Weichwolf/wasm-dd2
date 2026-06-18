#!/usr/bin/env python3
import os, struct, sys, re
import numpy as np
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(HERE, 'assets', 'raw')

def offtab(d, base, N):
    ptrs=[]; first=None; i=base
    while i+4<=N:
        v=struct.unpack_from('<I',d,i)[0]
        if first is None:
            if v==0 or v>N-base: break
            ptrs.append(v); first=v
        else:
            if (i-base)>=first: break
            ptrs.append(v)
        i+=4
    return ptrs

def secs(d,N):
    p=offtab(d,0,N); u=sorted(set(p+[N])); return [(u[j],u[j+1]) for j in range(len(u)-1)]

def main(level):
    d=open(os.path.join(RAW,level,'LEVEL.DAT'),'rb').read(); N=len(d)
    S=secs(d,N); nv=(S[2][1]-S[2][0])//12
    print(f"{level} nverts={nv}")
    # ---- section 3 strings ----
    o,e=S[3]
    strs=re.findall(rb'[A-Z0-9_]{2,}', d[o:e])
    print("Section3 names:", [s.decode() for s in strs][:40])
    # ---- section 1 face record stride search ----
    o,e=S[1]; blob=d[o:e]; L=e-o
    print(f"\nSection1 size={L}")
    print("first 96 bytes u16:", list(struct.unpack_from('<48H', blob, 0)))
    # mark which of first 64 u16 are valid vertex indices
    u16=np.frombuffer(blob[:L-(L%2)],np.uint16)
    valid=(u16<nv)
    print("valid-index mask (first 64):", ''.join('1' if v else '.' for v in valid[:64]))
    # try strides 8,10,12,16,20,24 : count records whose first 4 u16 are all valid
    for stride in (8,10,12,14,16,20,24,28,32):
        if L<stride*4: continue
        nrec=L//stride
        recs=np.frombuffer(blob[:nrec*stride],np.uint8).reshape(nrec,stride)
        # interpret first 8 bytes as 4 u16
        first4=recs[:, :8].view('<u2') if stride>=8 else None
        good=np.mean(np.all(first4<nv,axis=1)) if first4 is not None else 0
        print(f"  stride {stride:2}: {nrec:5} recs, first-4-u16-all-valid: {good*100:4.0f}%")

if __name__=='__main__':
    main(sys.argv[1] if len(sys.argv)>1 else 'LEV1')
