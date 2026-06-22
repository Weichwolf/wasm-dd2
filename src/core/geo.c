#include "geo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WORLD_SCALE (1.0f/500.0f)

static unsigned u32(const unsigned char* p){return p[0]|(p[1]<<8)|(p[2]<<16)|((unsigned)p[3]<<24);}
static unsigned u16(const unsigned char* p){return p[0]|(p[1]<<8);}
static int      i16(const unsigned char* p){short s=(short)u16(p);return s;}
static int      i32(const unsigned char* p){return (int)u32(p);}

// LZSS (Decompress @00415550): [u32 size][stream]; ctrl byte LSB-first, flag1=literal,
// flag0=2 bytes -> back-offset (b1|(b2&0xf0)<<4)-0x1000, length (b2&0xf)+3, copy overlapping.
static unsigned char* lzss(const unsigned char* d, long n, long off, long* outlen){
    if(off+4>n) return NULL;
    unsigned size=u32(d+off); long src=off+4;
    unsigned char* out=malloc(size?size:1); long o=0; unsigned ctrl=0; int bits=0;
    while(o<(long)size && src<n){
        if(bits==0){ ctrl=d[src++]; bits=8; }
        if(ctrl&1){ if(src<n) out[o++]=d[src++]; }
        else{
            if(src+1>=n) break;
            unsigned b1=d[src],b2=d[src+1]; src+=2;
            int L=(b2&0xf)+3, off2=(int)((b1|((b2&0xf0)<<4)))-0x1000;
            for(int k=0;k<L && o<(long)size;k++){ long p=o+off2; out[o++]=(p>=0&&p<o)?out[p]:0; }
        }
        ctrl>>=1; bits--;
    }
    *outlen=size; return out;
}

static int cmp_f(const void* a, const void* b){ float x=*(const float*)a, y=*(const float*)b; return x<y?-1:(x>y?1:0); }
// grow buffer of floats
typedef struct { float* b; long n, cap; } Buf;
static void push6(Buf* b, float x,float y,float z,float r,float g,float bl){
    if(b->n+6>b->cap){ b->cap=b->cap?b->cap*2:4096; b->b=realloc(b->b,b->cap*sizeof(float)); }
    float* o=b->b+b->n; o[0]=x;o[1]=y;o[2]=z;o[3]=r;o[4]=g;o[5]=bl; b->n+=6;
}

// face-record sizes (gpoly stride) by type, from the draw_face_* dispatch table (docs/REVERSING.md).
// Most poly types are 20-byte records sharing the layout RGB@+4, vtx indices(u16) @+12 (3 for 3pt
// types, 4 for 4pt). Textured types carry uv/tpage/clut too but the index block is at the same offset.
static int face_size(int t){
    switch(t){
        case 2: case 46: return 16;
        case 18: case 27: return 24;
        case 26: case 31: return 28;
        case 30: return 32;
        default: return 20;   // the common case (flat/textured 3pt&4pt all 20B here)
    }
}

int geo_load(const char* path, Geo* g){
    FILE* f=fopen(path,"rb"); if(!f) return 0;
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    unsigned char* d=malloc(n); if(!d||fread(d,1,n,f)!=(size_t)n){free(d);fclose(f);return 0;} fclose(f);

    unsigned tblsz=u32(d); int subn=tblsz/4;            // sec0 sub-table count (==section table size/4? no: chunk table)
    // section-0 starts at offset 116 (its own sub-table); reuse the file-level table[0] convention:
    unsigned sec0=u32(d);                                // first section offset == sec0 base == table size
    unsigned subcount=u32(d+sec0)/4;                     // sec0's sub-table size/4
    Buf buf={0};
    for(unsigned si=0; si<subcount; si++){
        unsigned suboff=u32(d+sec0+si*4);
        long ol; unsigned char* out=lzss(d, n, sec0+suboff, &ol);
        if(!out){ continue; }
        if(ol<8){ free(out); continue; }
        unsigned cnt=u32(out);
        if(cnt==0||cnt>4000){ free(out); continue; }
        for(unsigned r=0;r<cnt;r++){
            long rec=4+r*16;
            if(rec+16>ol) break;
            int off=i32(out+rec), px=i32(out+rec+4), py=i32(out+rec+8), pz=i32(out+rec+12);
            if(off<0||off>ol-0x2c) continue;
            int nv=(u32(out+off+8)>>16)&0xff;
            unsigned r20=u32(out+off+0x20), r28=u32(out+off+0x28);
            if(nv<=0||nv>2000) continue;
            long vb=off+r20, fb=off+r28;
            if(vb<0||fb<0||vb>ol) continue;
            long gp=fb;
            for(int it=0; it<128; it++){
                if(gp+4>ol) break;
                unsigned fc=u16(out+gp); int ft=out[gp+2], term=out[gp+3]; gp+=4;
                if(term==0||fc==0||fc>3000||ft==0xff) break;     // 0xff type = end-of-list marker
                int sz=face_size(ft);
                if(sz==20){                                       // flat/textured poly: rgb@+4, idx@+12
                    for(unsigned fi=0;fi<fc;fi++){
                        long rr=gp+fi*20;
                        if(rr+20>ol) break;
                        int id[4]; for(int k=0;k<4;k++) id[k]=u16(out+rr+12+k*2);
                        float cr=out[rr+4]/255.f, cg=out[rr+5]/255.f, cb=out[rr+6]/255.f;
                        if(id[0]>=nv||id[1]>=nv||id[2]>=nv) continue;          // tri must be valid
                        int quad = (id[3]<nv && id[3]!=id[0]);                 // 4th valid -> quad, else tri
                        float P[4][3];
                        int cn=quad?4:3;
                        for(int k=0;k<cn;k++){ const unsigned char* vp=out+vb+id[k]*8;
                            P[k][0]=(px+i16(vp))*WORLD_SCALE; P[k][1]=(py+i16(vp+2))*WORLD_SCALE; P[k][2]=(pz+i16(vp+4))*WORLD_SCALE; }
                        push6(&buf,P[0][0],P[0][1],P[0][2],cr,cg,cb);
                        push6(&buf,P[1][0],P[1][1],P[1][2],cr,cg,cb);
                        push6(&buf,P[2][0],P[2][1],P[2][2],cr,cg,cb);
                        if(quad){ push6(&buf,P[0][0],P[0][1],P[0][2],cr,cg,cb);
                            push6(&buf,P[2][0],P[2][1],P[2][2],cr,cg,cb);
                            push6(&buf,P[3][0],P[3][1],P[3][2],cr,cg,cb); }
                    }
                }
                gp+=fc*sz;
            }
        }
        free(out);
    }
    free(d);
    g->v=buf.b; g->nverts=(int)(buf.n/6);
    // Align to the sim plane: the sim's track path is flat at Y=0, but the real geo has true
    // elevation centered well below. Recenter geo Y to its median so the ground sits near Y=0
    // (where the cars are). XZ already shares origin with the sim.
    if(g->nverts>0){
        float* ys=malloc(sizeof(float)*g->nverts);
        for(int i=0;i<g->nverts;i++) ys[i]=g->v[i*6+1];
        qsort(ys,g->nverts,sizeof(float),cmp_f);
        float medy=ys[g->nverts/2]; free(ys);
        for(int i=0;i<g->nverts;i++) g->v[i*6+1]-=medy;       // ground -> ~0
    }
    fprintf(stderr,"geo_load %s: %d verts (%d tris)\n",path,g->nverts,g->nverts/3);
    return g->nverts>0;
}

void geo_free(Geo* g){ free(g->v); g->v=NULL; g->nverts=0; }
