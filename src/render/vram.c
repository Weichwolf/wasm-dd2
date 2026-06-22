#include "vram.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VRAM_W 256

static unsigned char* s_vram;     // 8-bit indices, VRAM_W * s_vram_h
static int   s_vram_h;
static unsigned char s_pal[256*3];
// sprite table
typedef struct { unsigned short u,v,w,h,cx,cy,mode; char name[11]; } Sprite;
static Sprite* s_spr; static int s_nspr;

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
    // palette
    snprintf(path,sizeof(path),"assets/raw/%s/LEVEL.PAL",level);
    unsigned char* pal=rd(path,&n); if(!pal) return 0;
    for(int i=0;i<256;i++){ s_pal[i*3]=pal[i*4]; s_pal[i*3+1]=pal[i*4+1]; s_pal[i*3+2]=pal[i*4+2]; }
    free(pal);
    // TX0 -> VRAM (place tiles until source pixel pool is consumed, like Load_Textures)
    snprintf(path,sizeof(path),"assets/raw/%s/LEVEL.TX0",level);
    unsigned char* tx=rd(path,&n); if(!tx) return 0;
    unsigned count=u32(tx);
    unsigned diroff=4, pixoff=4+count*16;
    int maxy=0;
    for(unsigned i=0;i<count;i++){
        const unsigned char* r=tx+diroff+i*16;
        int h=u16(r+4), dy=u16(r+8); if(dy+h>maxy) maxy=dy+h;
    }
    s_vram_h=maxy; s_vram=calloc((size_t)VRAM_W*s_vram_h,1);
    unsigned p=pixoff;
    for(unsigned i=0;i<count;i++){
        const unsigned char* r=tx+diroff+i*16;
        int w=u16(r+2), h=u16(r+4), dx=u16(r+6), dy=u16(r+8);
        if(p+(size_t)w*h > (unsigned)n) break;             // source pool exhausted -> done (22 tiles)
        for(int y=0;y<h;y++) if(dy+y<s_vram_h && dx+w<=VRAM_W)
            memcpy(s_vram+(size_t)(dy+y)*VRAM_W+dx, tx+p+(size_t)y*w, w);
        p+=(size_t)w*h;
    }
    free(tx);
    // sprite table
    snprintf(path,sizeof(path),"assets/raw/%s/LEVEL.SPR",level);
    unsigned char* sp=rd(path,&n);
    if(sp){
        s_nspr=u32(sp); s_spr=calloc(s_nspr,sizeof(Sprite));
        for(int i=0;i<s_nspr;i++){
            const unsigned char* e=sp+4+i*24;
            s_spr[i].u=u16(e); s_spr[i].v=u16(e+2); s_spr[i].w=u16(e+4); s_spr[i].h=u16(e+6);
            s_spr[i].cx=u16(e+8); s_spr[i].cy=u16(e+10); s_spr[i].mode=u16(e+12);
            memcpy(s_spr[i].name,e+14,10); s_spr[i].name[10]=0;
        }
        free(sp);
    }
    fprintf(stderr,"vram_init %s: %dx%d, %d sprites\n",level,VRAM_W,s_vram_h,s_nspr);
    return 1;
}

GLuint vram_sprite_tex(const char* name, int* wout, int* hout){
    Sprite* s=NULL;
    for(int i=0;i<s_nspr;i++) if(strncmp(s_spr[i].name,name,10)==0){ s=&s_spr[i]; break; }
    if(!s || !s_vram) return 0;
    int u=s->u,v=s->v,w=s->w,h=s->h;
    if(u+w>VRAM_W) w=VRAM_W-u; if(v+h>s_vram_h) h=s_vram_h-v; if(w<=0||h<=0) return 0;
    unsigned char* rgba=malloc((size_t)w*h*4);
    for(int y=0;y<h;y++) for(int x=0;x<w;x++){
        unsigned idx=s_vram[(size_t)(v+y)*VRAM_W+(u+x)];
        unsigned char* o=rgba+((size_t)y*w+x)*4;
        o[0]=s_pal[idx*3]; o[1]=s_pal[idx*3+1]; o[2]=s_pal[idx*3+2]; o[3]= idx==0?0:255;
    }
    GLuint t; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    free(rgba);
    if(wout)*wout=w; if(hout)*hout=h;
    return t;
}
