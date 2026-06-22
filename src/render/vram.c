#include "vram.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VRAM_W 256

static unsigned char* s_vram;     // 8-bit indices, VRAM_W * s_vram_h
static int   s_vram_h;
static unsigned char s_pal[256*3];
static unsigned char* s_clt; static int s_nclt;   // LEVEL.CLT: s_nclt x 256 x RGB (from 256xRGBA)
typedef struct { unsigned short u,v,w,h,cx,cy,mode; char name[11]; } Sprite;
static Sprite* s_spr; static int s_nspr;
// font glyph metrics (ascii-32 indexed): x,y,w,h relative to FONT sprite origin (v=128)
static unsigned char s_glyph[128][4]; static int s_font_v=128;

static unsigned char* rd(const char* path, long* n){
    FILE* f=fopen(path,"rb"); if(!f) return NULL;
    fseek(f,0,SEEK_END); long s=ftell(f); fseek(f,0,SEEK_SET);
    unsigned char* b=malloc(s); if(b&&fread(b,1,s,f)!=(size_t)s){free(b);b=NULL;} fclose(f);
    if(n)*n=s; return b;
}
static unsigned u32(const unsigned char*p){return p[0]|(p[1]<<8)|(p[2]<<16)|((unsigned)p[3]<<24);}
static unsigned u16(const unsigned char*p){return p[0]|(p[1]<<8);}

int vram_init(const char* level){
    char path[256]; long n;
    snprintf(path,sizeof(path),"assets/raw/%s/LEVEL.PAL",level);
    unsigned char* pal=rd(path,&n); if(!pal) return 0;
    for(int i=0;i<256;i++){ s_pal[i*3]=pal[i*4]; s_pal[i*3+1]=pal[i*4+1]; s_pal[i*3+2]=pal[i*4+2]; }
    free(pal);
    // LEVEL.CLT = N x (256 x RGBA) palettes -> store RGB (per-sprite CLUT, coherence-picked at use)
    snprintf(path,sizeof(path),"assets/raw/%s/LEVEL.CLT",level);
    long nclt; unsigned char* clt=rd(path,&nclt);
    if(clt){ s_nclt=nclt/1024; s_clt=malloc((size_t)s_nclt*256*3);
        for(int p=0;p<s_nclt;p++) for(int i=0;i<256;i++){ const unsigned char* s=clt+p*1024+i*4; unsigned char* o=s_clt+(p*256+i)*3; o[0]=s[0];o[1]=s[1];o[2]=s[2]; }
        free(clt); }

    // TX0 = directory + page-0 pool; concatenate TX0-pool + TX1..TX4 as the full source pool
    snprintf(path,sizeof(path),"assets/raw/%s/LEVEL.TX0",level);
    long n0; unsigned char* tx0=rd(path,&n0); if(!tx0) return 0;
    unsigned count=u32(tx0), diroff=4, pooloff=4+count*16;
    long poolcap=n0-pooloff;
    long sizes[5]={0,0,0,0,0}; unsigned char* extra[5]={0,0,0,0,0};
    for(int k=1;k<=4;k++){ snprintf(path,sizeof(path),"assets/raw/%s/LEVEL.TX%d",level,k);
        extra[k]=rd(path,&sizes[k]); poolcap+=sizes[k]>0?sizes[k]:0; }
    unsigned char* pool=malloc(poolcap); long pp=0;
    memcpy(pool, tx0+pooloff, n0-pooloff); pp=n0-pooloff;
    for(int k=1;k<=4;k++) if(extra[k]){ memcpy(pool+pp,extra[k],sizes[k]); pp+=sizes[k]; free(extra[k]); }

    int maxy=0;
    for(unsigned i=0;i<count;i++){ const unsigned char* r=tx0+diroff+i*16; int h=u16(r+4),dy=u16(r+8); if(dy+h>maxy)maxy=dy+h; }
    s_vram_h=maxy; s_vram=calloc((size_t)VRAM_W*s_vram_h,1);
    long p=0;
    for(unsigned i=0;i<count;i++){
        const unsigned char* r=tx0+diroff+i*16;
        int w=u16(r+2), h=u16(r+4), dx=u16(r+6), dy=u16(r+8);
        if(p+4+(long)w*h > pp) break;
        p+=4;
        for(int y=0;y<h;y++) if(dy+y<s_vram_h && dx+w<=VRAM_W)
            memcpy(s_vram+(size_t)(dy+y)*VRAM_W+dx, pool+p+(size_t)y*w, w);
        p+=(long)w*h;
    }
    free(tx0); free(pool);

    snprintf(path,sizeof(path),"assets/raw/%s/LEVEL.SPR",level);
    unsigned char* sp=rd(path,&n);
    if(sp){ s_nspr=u32(sp); s_spr=calloc(s_nspr,sizeof(Sprite));
        for(int i=0;i<s_nspr;i++){ const unsigned char* e=sp+4+i*24;
            s_spr[i].u=u16(e);s_spr[i].v=u16(e+2);s_spr[i].w=u16(e+4);s_spr[i].h=u16(e+6);
            s_spr[i].cx=u16(e+8);s_spr[i].cy=u16(e+10);s_spr[i].mode=u16(e+12);
            memcpy(s_spr[i].name,e+14,10); s_spr[i].name[10]=0; }
        // FONT sprite gives the glyph atlas origin (v)
        for(int i=0;i<s_nspr;i++) if(strncmp(s_spr[i].name,"FONT",10)==0){ s_font_v=s_spr[i].v; break; }
        free(sp);
    }
    // FONT.BNK glyph metrics (font 0): 4-byte (gx,gy,gw,gh) from offset 32, index = ascii-32
    snprintf(path,sizeof(path),"assets/raw/%s/FONT.BNK",level);
    unsigned char* fb=rd(path,&n);
    if(fb){ for(int i=0;i<95 && 32+i*4+4<=n;i++) memcpy(s_glyph[i], fb+32+i*4, 4); free(fb); }
    fprintf(stderr,"vram_init %s: %dx%d, %d sprites, font_v=%d\n",level,VRAM_W,s_vram_h,s_nspr,s_font_v);
    return 1;
}

static GLuint upload(unsigned char* rgba, int w, int h){
    GLuint t; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    return t;
}

GLuint vram_sprite_tex_pal(const char* name, int* wout, int* hout, int pal_override){
    Sprite* s=NULL;
    for(int i=0;i<s_nspr;i++) if(strncmp(s_spr[i].name,name,10)==0){ s=&s_spr[i]; break; }
    if(!s || !s_vram) return 0;
    int u=s->u,v=s->v,w=s->w,h=s->h;
    if(u+w>VRAM_W)w=VRAM_W-u; if(v+h>s_vram_h)h=s_vram_h-v; if(w<=0||h<=0)return 0;
    // CLUT (engine-faithful): __clutspace = LEVEL.CLT loaded directly; draw clut for a sprite is at
    // __clutspace + dth_clut*0x1000, dth_clut = sprite.cy. LEVEL.CLT is 256xRGBA palettes (PC port),
    // so palette index = cy*4 (byte offset cy*0x1000). Verified: RING -> grey metal, DRIVER0 -> clean.
    // pal_override>=0 forces a palette (e.g. logo DD2L1 -> pal57 chrome, not cy*4=32 washed).
    const unsigned char* pal=s_pal;
    if(s_clt && s_nclt>0){ int pi = pal_override>=0 ? pal_override : s->cy*4; if(pi<s_nclt) pal=s_clt + (size_t)pi*256*3; }
    unsigned char* rgba=malloc((size_t)w*h*4);
    for(int y=0;y<h;y++)for(int x=0;x<w;x++){ unsigned idx=s_vram[(size_t)(v+y)*VRAM_W+(u+x)];
        unsigned char* o=rgba+((size_t)y*w+x)*4; o[0]=pal[idx*3];o[1]=pal[idx*3+1];o[2]=pal[idx*3+2];o[3]=idx?255:0; }
    GLuint t=upload(rgba,w,h); free(rgba); if(wout)*wout=w; if(hout)*hout=h; return t;
}
GLuint vram_sprite_tex(const char* name, int* wout, int* hout){ return vram_sprite_tex_pal(name,wout,hout,-1); }

int vram_text_measure(const char* s){
    int w=0; for(const char* c=s;*c;c++){ int gi=(unsigned char)*c-32; if(gi<0||gi>=95)continue; w+=s_glyph[gi][2]+1; } return w;
}

GLuint vram_text_tex(const char* s, unsigned char ir,unsigned char ig,unsigned char ib, int* wout,int* hout){
    if(!s_vram) return 0;
    int W=vram_text_measure(s)+2, H=26; if(W<3)return 0;
    unsigned char* rgba=calloc((size_t)W*H*4,1);
    int x=1;
    for(const char* c=s;*c;c++){ int gi=(unsigned char)*c-32; if(gi<0||gi>=95)continue;
        int gx=s_glyph[gi][0], gy=s_glyph[gi][1], gw=s_glyph[gi][2], gh=s_glyph[gi][3];
        for(int yy=0;yy<gh;yy++){ int sy=s_font_v+gy+yy; if(sy<0||sy>=s_vram_h)continue;
            for(int xx=0;xx<gw;xx++){ if(gx+xx>=VRAM_W)continue;
                unsigned idx=s_vram[(size_t)sy*VRAM_W+gx+xx];
                if(idx==16||idx==0) continue;                 // background
                unsigned char* o=rgba+(((size_t)(yy+1)*W)+(x+xx))*4;
                if(idx==18){ o[0]=ir*7/10;o[1]=ig*7/10;o[2]=ib*7/10; } else { o[0]=ir;o[1]=ig;o[2]=ib; }
                o[3]=255;
            } }
        x+=gw+1;
    }
    GLuint t=upload(rgba,W,H); free(rgba); if(wout)*wout=W; if(hout)*hout=H; return t;
}
