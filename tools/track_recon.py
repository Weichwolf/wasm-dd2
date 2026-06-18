#!/usr/bin/env python3
"""Reconstruct a track road ribbon from LEVEL.DAT section-2 vertices.

Vertices are ordered as ~N cross-sections (ribs) spanning the track width.
We split at large consecutive jumps, resample each rib to K points by arc length,
and build a quad strip. Output: validation render + centerline/width stats.
This algorithm will be ported to C (track.c)."""
import os, struct, sys, math
import numpy as np
from PIL import Image, ImageDraw
HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(HERE, 'assets', 'raw'); OUT = os.path.join(HERE, 'out')

def load_verts(level):
    d = open(os.path.join(RAW, level, 'LEVEL.DAT'), 'rb').read(); N = len(d)
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
    u=sorted(set(ptrs+[N])); S=[(u[j],u[j+1]) for j in range(len(u)-1)]
    o,e=S[2]; nv=(e-o)//12
    return np.frombuffer(d[o:o+nv*12],np.int32).reshape(nv,3).astype(float)

def split_ribs(v):
    xz=v[:,[0,2]]
    dd=np.sqrt(((np.diff(xz,axis=0))**2).sum(1))
    thr=np.median(dd)*4
    cuts=[0]+[i+1 for i in np.where(dd>thr)[0]]+[len(v)]
    ribs=[]
    for a,b in zip(cuts[:-1],cuts[1:]):
        if b-a>=2: ribs.append(v[a:b])
    return ribs

def resample(rib,K):
    seg=np.sqrt(((np.diff(rib,axis=0))**2).sum(1)); L=seg.sum()
    if L<1: return np.repeat(rib[:1],K,0)
    cum=np.concatenate([[0],np.cumsum(seg)]); t=np.linspace(0,cum[-1],K)
    out=np.empty((K,3))
    for d in range(3): out[:,d]=np.interp(t,cum,rib[:,d])
    return out

def recon(level,K=9):
    v=load_verts(level); ribs=split_ribs(v)
    R=np.array([resample(r,K) for r in ribs])   # (nrib,K,3)
    center=R.mean(1)                              # centerline
    widths=np.sqrt(((R[:,0]-R[:,-1])**2).sum(1))
    print(f"{level}: {len(v)} verts -> {len(ribs)} ribs; width med={np.median(widths):.0f} "
          f"min={widths.min():.0f} max={widths.max():.0f}")
    # top-down filled render
    Sz=700;img=Image.new('RGB',(Sz,Sz));dr=ImageDraw.Draw(img)
    allp=R.reshape(-1,3);lo=allp[:,[0,2]].min(0);hi=allp[:,[0,2]].max(0);sp=hi-lo;sp[sp<1]=1
    def P(p):
        px=int((p[0]-lo[0])/sp[0]*(Sz-20)+10);py=int((p[2]-lo[1])/sp[1]*(Sz-20)+10);return px,Sz-1-py
    nr=len(R)
    for i in range(nr):
        j=(i+1)%nr
        for k in range(K-1):
            quad=[P(R[i,k]),P(R[i,k+1]),P(R[j,k+1]),P(R[j,k])]
            shade=40+int(160*k/(K-1))
            dr.polygon(quad,fill=(shade,shade,shade))
    # centerline
    for i in range(nr):
        dr.line([P(center[i]),P(center[(i+1)%nr])],fill=(255,80,40),width=2)
    img.save(os.path.join(OUT,f'{level}_road.png'))
    print(f"  -> out/{level}_road.png")
    return R,center,widths

if __name__=='__main__':
    for lv in (sys.argv[1:] or ['LEV1','LEV2','LEV5']):
        recon(lv)
