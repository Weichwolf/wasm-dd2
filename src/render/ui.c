#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GLuint s_prog, s_vbo, s_dyn_vbo;
static GLint  s_u_tex, s_u_tint;

static const char* VS =
    "attribute vec2 a_pos; attribute vec2 a_uv; varying vec2 v_uv;\n"
    "void main(){ v_uv=a_uv; gl_Position=vec4(a_pos,0.0,1.0); }\n";
static const char* FS =
    "precision mediump float; varying vec2 v_uv; uniform sampler2D u_tex; uniform vec3 u_tint;\n"
    "void main(){ vec4 c=texture2D(u_tex, v_uv); gl_FragColor = vec4(c.rgb*u_tint, c.a); }\n";

static GLuint sh(GLenum t, const char* s){ GLuint h=glCreateShader(t); glShaderSource(h,1,&s,0); glCompileShader(h); return h; }

void ui_init(void){
    s_prog=glCreateProgram();
    glAttachShader(s_prog,sh(GL_VERTEX_SHADER,VS));
    glAttachShader(s_prog,sh(GL_FRAGMENT_SHADER,FS));
    glBindAttribLocation(s_prog,0,"a_pos"); glBindAttribLocation(s_prog,1,"a_uv");
    glLinkProgram(s_prog); s_u_tex=glGetUniformLocation(s_prog,"u_tex"); s_u_tint=glGetUniformLocation(s_prog,"u_tint");
    // fullscreen quad (pos.xy, uv.xy); uv.y flipped so texture row0 = top of image
    static const float q[]={ -1,-1, 0,1,  1,-1, 1,1,  1,1, 1,0,
                             -1,-1, 0,1,  1,1, 1,0,  -1,1, 0,0 };
    glGenBuffers(1,&s_vbo); glBindBuffer(GL_ARRAY_BUFFER,s_vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(q),q,GL_STATIC_DRAW);
    glGenBuffers(1,&s_dyn_vbo);
}

void ui_blit_rect_tint(GLuint tex, float x, float y, float w, float h, float sw, float sh, float tr, float tg, float tb){
    // pixel rect (top-left origin) -> NDC; uv 0..1 with row0=top
    float x0=x/sw*2.f-1.f, x1=(x+w)/sw*2.f-1.f;
    float y0=1.f-y/sh*2.f,  y1=1.f-(y+h)/sh*2.f;
    float q[]={ x0,y0, 0,0,  x1,y0, 1,0,  x1,y1, 1,1,
                x0,y0, 0,0,  x1,y1, 1,1,  x0,y1, 0,1 };
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(s_prog); glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,tex); glUniform1i(s_u_tex,0); glUniform3f(s_u_tint,tr,tg,tb);
    glBindBuffer(GL_ARRAY_BUFFER,s_dyn_vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(q),q,GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glDrawArrays(GL_TRIANGLES,0,6);
    glDisable(GL_BLEND);
}
void ui_blit_rect(GLuint tex, float x, float y, float w, float h, float sw, float sh){
    ui_blit_rect_tint(tex,x,y,w,h,sw,sh,1.f,1.f,1.f);
}

static unsigned rd_u32(const unsigned char* p){ return p[0]|(p[1]<<8)|(p[2]<<16)|((unsigned)p[3]<<24); }
static int rd_i32(const unsigned char* p){ return (int)rd_u32(p); }
static unsigned rd_u16(const unsigned char* p){ return p[0]|(p[1]<<8); }

GLuint ui_load_bmp(const char* path, int* wout, int* hout){
    FILE* f=fopen(path,"rb"); if(!f){ fprintf(stderr,"ui_load_bmp: %s missing\n",path); return 0; }
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    unsigned char* d=(unsigned char*)malloc(n);
    if(fread(d,1,n,f)!=(size_t)n){ fclose(f); free(d); return 0; } fclose(f);
    if(d[0]!='B'||d[1]!='M'){ free(d); fprintf(stderr,"ui_load_bmp: not BMP\n"); return 0; }
    unsigned off=rd_u32(d+10);
    int w=rd_i32(d+18), h=rd_i32(d+22);
    int bpp=rd_u16(d+28);
    int topdown = h<0; int H = topdown? -h : h;
    // RGBA output, row 0 = top of image
    unsigned char* rgba=(unsigned char*)malloc((size_t)w*H*4);
    if(bpp==8){
        const unsigned char* pal=d+54;              // BGRA palette after 40-byte DIB header
        int stride=(w+3)&~3;                        // rows padded to 4 bytes
        for(int y=0;y<H;y++){
            // DD2's BMPs are authored top-down (file row 0 = visible top) despite a
            // positive height field, so read file rows in order; texture row 0 = top.
            int srcrow = topdown? (H-1-y) : y;
            const unsigned char* row=d+off+(size_t)srcrow*stride;
            for(int x=0;x<w;x++){
                const unsigned char* c=pal+row[x]*4;
                unsigned char* o=rgba+((size_t)y*w+x)*4;
                o[0]=c[2]; o[1]=c[1]; o[2]=c[0]; o[3]=255;
            }
        }
    } else if(bpp==24||bpp==32){
        int bs=bpp/8; int stride=((w*bs)+3)&~3;
        for(int y=0;y<H;y++){
            int srcrow = topdown? (H-1-y) : y;
            const unsigned char* row=d+off+(size_t)srcrow*stride;
            for(int x=0;x<w;x++){
                const unsigned char* c=row+x*bs; unsigned char* o=rgba+((size_t)y*w+x)*4;
                o[0]=c[2]; o[1]=c[1]; o[2]=c[0]; o[3]=255;
            }
        }
    } else { free(d); free(rgba); fprintf(stderr,"ui_load_bmp: bpp %d unsupported\n",bpp); return 0; }
    free(d);
    GLuint t; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,H,0,GL_RGBA,GL_UNSIGNED_BYTE,rgba);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    free(rgba);
    if(wout)*wout=w; if(hout)*hout=H;
    return t;
}

void ui_blit_fullscreen(GLuint tex){
    glDisable(GL_DEPTH_TEST);
    glUseProgram(s_prog); glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,tex); glUniform1i(s_u_tex,0);
    glBindBuffer(GL_ARRAY_BUFFER,s_vbo);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glDrawArrays(GL_TRIANGLES,0,6);
}
