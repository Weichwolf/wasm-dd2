#include "geo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WORLD_SCALE (1.0f/500.0f)

static unsigned u32(const unsigned char* p){return p[0]|(p[1]<<8)|(p[2]<<16)|((unsigned)p[3]<<24);}
static unsigned u16(const unsigned char* p){return p[0]|(p[1]<<8);}
static int      i16(const unsigned char* p){return (short)u16(p);}
static int      i32(const unsigned char* p){return (int)u32(p);}
static int cmp_f(const void* a,const void* b){float x=*(const float*)a,y=*(const float*)b;return x<y?-1:(x>y?1:0);}

static unsigned char* rd(const char* path, long* n){
    FILE* f=fopen(path,"rb"); if(!f) return NULL;
    fseek(f,0,SEEK_END); long s=ftell(f); fseek(f,0,SEEK_SET);
    unsigned char* b=malloc(s?s:1); if(b&&fread(b,1,s,f)!=(size_t)s){free(b);b=NULL;} fclose(f);
    if(n)*n=s; return b;
}

static unsigned char* lzss(const unsigned char* d, long n, long off, long* outlen){
    if(off+4>n) return NULL;
    unsigned size=u32(d+off); long src=off+4;
    unsigned char* out=malloc(size?size:1); long o=0; unsigned ctrl=0; int bits=0;
    while(o<(long)size && src<n){
        if(bits==0){ ctrl=d[src++]; bits=8; }
        if(ctrl&1){ if(src<n) out[o++]=d[src++]; }
        else{ if(src+1>=n) break; unsigned b1=d[src],b2=d[src+1]; src+=2;
            int L=(b2&0xf)+3, off2=(int)((b1|((b2&0xf0)<<4)))-0x1000;
            for(int k=0;k<L&&o<(long)size;k++){ long p=o+off2; out[o++]=(p>=0&&p<o)?out[p]:0; } }
        ctrl>>=1; bits--;
    }
    *outlen=size; return out;
}

typedef struct { float* b; long n, cap; } Buf;
static void pushf(Buf* b, int k, ...){ (void)k; }
static void push(Buf* b, const float* vals, int k){
    if(b->n+k>b->cap){ b->cap=b->cap?b->cap*2:8192; b->b=realloc(b->b,b->cap*sizeof(float)); }
    memcpy(b->b+b->n, vals, k*sizeof(float)); b->n+=k;
}

// per-level VRAM (8-bit) from the level's TX pages (same assembly as the sprite VRAM)
static void load_vram(const char* lev, Geo* g){
    char path[256]; long n0;
    snprintf(path,sizeof(path),"assets/raw/%s/LEVEL.TX0",lev);
    unsigned char* tx0=rd(path,&n0); if(!tx0) return;
    unsigned count=u32(tx0), diroff=4, pooloff=4+count*16;
    long cap=n0-pooloff; long sz[5]={0}; unsigned char* ex[5]={0};
    for(int k=1;k<=4;k++){ snprintf(path,sizeof(path),"assets/raw/%s/LEVEL.TX%d",lev,k); ex[k]=rd(path,&sz[k]); if(sz[k]>0)cap+=sz[k]; }
    unsigned char* pool=malloc(cap?cap:1); long pp=n0-pooloff; memcpy(pool,tx0+pooloff,pp);
    for(int k=1;k<=4;k++) if(ex[k]){ memcpy(pool+pp,ex[k],sz[k]); pp+=sz[k]; free(ex[k]); }
    int maxy=0; for(unsigned i=0;i<count;i++){ const unsigned char* r=tx0+diroff+i*16; int h=u16(r+4),dy=u16(r+8); if(dy+h>maxy)maxy=dy+h; }
    g->vram_w=256; g->vram_h=maxy; g->vram=calloc((size_t)256*maxy,1);
    long p=0;
    for(unsigned i=0;i<count;i++){ const unsigned char* r=tx0+diroff+i*16;
        int w=u16(r+2),h=u16(r+4),dx=u16(r+6),dy=u16(r+8);
        if(p+4+(long)w*h>pp) break; p+=4;
        for(int y=0;y<h;y++) if(dy+y<maxy&&dx+w<=256) memcpy(g->vram+(size_t)(dy+y)*256+dx,pool+p+(size_t)y*w,w);
        p+=(long)w*h;
    }
    free(tx0); free(pool);
}

static int face_size(int t){ switch(t){case 2:case 46:return 16;case 18:case 27:return 24;case 26:case 31:return 28;case 30:return 32;default:return 20;} }
// type 14 (and other 20B textured) carry a texture-index @ +8 into TDF; flat types use rgb@+4.
static int is_textured(int t){ return t==12 || t==14 || t==30; }

int geo_load(const char* path, Geo* g){
    memset(g,0,sizeof(*g));
    // derive level dir "LEVx" from path .../LEVx/LEVEL.DAT
    char lev[16]={0}; const char* p=strstr(path,"LEV"); if(p){ int i=0; while(i<15&&p[i]&&p[i]!='/'){lev[i]=p[i];i++;} lev[i]=0; }
    long n; unsigned char* d=rd(path,&n); if(!d) return 0;
    load_vram(lev, g);
    // CLUT (RGB palettes)
    char cp[256]; snprintf(cp,sizeof(cp),"assets/raw/%s/LEVEL.CLT",lev); long nclt; unsigned char* clt=rd(cp,&nclt);
    if(clt){ g->nclut=nclt/1024; g->clut=malloc((size_t)g->nclut*256*3);
        for(int pi=0;pi<g->nclut;pi++)for(int i=0;i<256;i++){const unsigned char* s=clt+pi*1024+i*4;unsigned char* o=g->clut+(pi*256+i)*3;o[0]=s[0];o[1]=s[1];o[2]=s[2];}
        free(clt); }
    // TDF (texture table)
    char tp[256]; snprintf(tp,sizeof(tp),"assets/raw/%s/LEVEL.TDF",lev); long ntdf_b; unsigned char* tdf=rd(tp,&ntdf_b);
    unsigned ntdf = tdf? u32(tdf):0;

    unsigned sec0=u32(d); unsigned subcount=u32(d+sec0)/4;
    Buf flat={0}, tex={0};
    for(unsigned si=0; si<subcount; si++){
        unsigned suboff=u32(d+sec0+si*4); long ol; unsigned char* out=lzss(d,n,sec0+suboff,&ol);
        if(!out){continue;} if(ol<8){free(out);continue;}
        unsigned cnt=u32(out); if(cnt==0||cnt>4000){free(out);continue;}
        for(unsigned r=0;r<cnt;r++){ long rec=4+r*16; if(rec+16>ol)break;
            int off=i32(out+rec),px=i32(out+rec+4),py=i32(out+rec+8),pz=i32(out+rec+12);
            if(off<0||off>ol-0x2c) continue;
            int nv=(u32(out+off+8)>>16)&0xff; unsigned r20=u32(out+off+0x20),r28=u32(out+off+0x28);
            if(nv<=0||nv>2000) continue; long vb=off+r20,fb=off+r28; if(vb<0||fb<0||vb>ol) continue;
            long gp=fb;
            for(int it=0;it<128;it++){ if(gp+4>ol)break;
                unsigned fc=u16(out+gp); int ft=out[gp+2],term=out[gp+3]; gp+=4;
                if(term==0||fc==0||fc>3000||ft==0xff) break;
                int sz=face_size(ft);
                for(unsigned fi=0;fi<fc;fi++){ long rr=gp+fi*sz; if(rr+sz>ol)break;
                    // vertex indices (4) at +12 for 20B types; (4) at +0x18 for 32B
                    int vo = (sz==32)?0x18:12;
                    int id[4]; for(int k=0;k<4;k++) id[k]=u16(out+rr+vo+k*2);
                    if(id[0]>=nv||id[1]>=nv||id[2]>=nv) continue;
                    int quad=(id[3]<nv&&id[3]!=id[0]); int cn=quad?4:3;
                    float P[4][3];
                    for(int k=0;k<cn;k++){ const unsigned char* vp=out+vb+id[k]*8;
                        P[k][0]=(px+i16(vp))*WORLD_SCALE; P[k][1]=(py+i16(vp+2))*WORLD_SCALE; P[k][2]=(pz+i16(vp+4))*WORLD_SCALE; }
                    int tidx = u16(out+rr+8);
                    if(is_textured(ft) && tdf && tidx<(int)ntdf){
                        const unsigned char* e=tdf+4+tidx*12; int clut=u16(e); int uvr=clut*4;
                        float uv[4][2]; for(int k=0;k<4;k++){ unsigned p2=u16(e+4+k*2); uv[k][0]=p2&0xff; uv[k][1]=p2>>8; }
                        int tri[6]={0,1,2,0,2,3};
                        for(int k=0;k<(quad?6:3);k++){ int vi=tri[k];
                            float row[6]={P[vi][0],P[vi][1],P[vi][2],uv[vi][0],uv[vi][1],(float)uvr}; push(&tex,row,6); }
                    } else {
                        float cr=out[rr+4]/255.f,cg=out[rr+5]/255.f,cb=out[rr+6]/255.f;
                        // light + emit flat
                        float e1x=P[1][0]-P[0][0],e1y=P[1][1]-P[0][1],e1z=P[1][2]-P[0][2];
                        float e2x=P[2][0]-P[0][0],e2y=P[2][1]-P[0][1],e2z=P[2][2]-P[0][2];
                        float nx=e1y*e2z-e1z*e2y,ny=e1z*e2x-e1x*e2z,nz=e1x*e2y-e1y*e2x;
                        float nl=sqrtf(nx*nx+ny*ny+nz*nz); if(nl>1e-6f){ny/=nl;}
                        float sh=0.55f+0.45f*(ny<0?-ny:ny); cr*=sh;cg*=sh;cb*=sh;
                        int tri[6]={0,1,2,0,2,3};
                        for(int k=0;k<(quad?6:3);k++){ int vi=tri[k]; float row[6]={P[vi][0],P[vi][1],P[vi][2],cr,cg,cb}; push(&flat,row,6); }
                    }
                }
                gp+=fc*sz;
            }
        }
        free(out);
    }
    free(d); if(tdf) free(tdf);
    g->v=flat.b; g->nverts=(int)(flat.n/6); g->tv=tex.b; g->ntverts=(int)(tex.n/6);
    // recenter flat+textured Y to median (align to sim plane)
    int total=g->nverts+g->ntverts;
    if(total>0){ float* ys=malloc(sizeof(float)*total); int c=0;
        for(int i=0;i<g->nverts;i++) ys[c++]=g->v[i*6+1];
        for(int i=0;i<g->ntverts;i++) ys[c++]=g->tv[i*6+1];
        qsort(ys,total,sizeof(float),cmp_f); float med=ys[total/2]; free(ys);
        for(int i=0;i<g->nverts;i++) g->v[i*6+1]-=med;
        for(int i=0;i<g->ntverts;i++) g->tv[i*6+1]-=med;
    }
    fprintf(stderr,"geo_load %s: %d flat-tri-v, %d tex-tri-v, vram %dx%d, %d cluts, %u tdf\n",
            lev,g->nverts,g->ntverts,g->vram_w,g->vram_h,g->nclut,ntdf);
    (void)pushf;
    return g->nverts>0 || g->ntverts>0;
}

void geo_free(Geo* g){ free(g->v); free(g->tv); free(g->vram); free(g->clut); memset(g,0,sizeof(*g)); }
