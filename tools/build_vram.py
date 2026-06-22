#!/usr/bin/env python3
"""Assemble a level's PSX-style texture VRAM from its TX pages and render named sprites.
Validates the texture/sprite pipeline used by the faithful renderer (menus/HUD/3D).
TX page = [u32 count][count x 16B dir records: tag=4,w,h,destX,destY,f5,..][packed pixels].
Sprites (LEVEL.SPR) address the assembled VRAM by (u,v,w,h). CAPRIO=(0,0,256,128) -> billboard. OK.
TODO: f5 is the per-tile source offset (records can share pixel data); only leading tiles place now."""
import os, struct, sys
import numpy as np
from PIL import Image
HERE=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW=os.path.join(HERE,'assets','raw'); OUT=os.path.join(HERE,'out'); os.makedirs(OUT,exist_ok=True)

def load_pal(level):
    d=open(os.path.join(RAW,level,'LEVEL.PAL'),'rb').read()
    return np.frombuffer(d,np.uint8).reshape(256,4)[:,:3]

def assemble_vram(level, page='TX0'):
    tx=open(os.path.join(RAW,level,f'LEVEL.{page}'),'rb').read()
    count=struct.unpack_from('<I',tx,0)[0]
    recs=[struct.unpack_from('<8H',tx,4+i*16) for i in range(count)]
    VW=max(256,max(r[3]+r[1] for r in recs)); VH=max(r[4]+r[2] for r in recs)
    vram=np.zeros((VH,VW),np.uint8)
    p=4+count*16
    for (tag,w,h,dx,dy,f5,a,b) in recs:
        need=w*h
        if p+need>len(tx): break
        if dy+h<=VH and dx+w<=VW:
            vram[dy:dy+h,dx:dx+w]=np.frombuffer(tx[p:p+need],np.uint8).reshape(h,w)
        p+=need
    return vram

def sprites(level):
    d=open(os.path.join(RAW,level,'LEVEL.SPR'),'rb').read()
    n=struct.unpack_from('<I',d,0)[0]; out={}
    for i in range(n):
        o=4+i*24; f=struct.unpack_from('<7H',d,o); nm=d[o+14:o+24].split(b'\0')[0].decode('latin1')
        out[nm]=f
    return out

if __name__=='__main__':
    lvl=sys.argv[1] if len(sys.argv)>1 else 'LEV0'
    pal=load_pal(lvl); vram=assemble_vram(lvl); spr=sprites(lvl)
    Image.fromarray(pal[vram]).save(f'{OUT}/vram_{lvl}.png')
    for nm in ('CAPRIO','UNDER','DRIVER1'):
        if nm in spr:
            u,v,w,h,cx,cy,m=spr[nm]
            if v+h<=vram.shape[0] and u+w<=vram.shape[1]:
                Image.fromarray(pal[vram[v:v+h,u:u+w]]).save(f'{OUT}/spr_{nm}.png')
    print(f"{lvl}: vram {vram.shape}, {len(spr)} sprites")
