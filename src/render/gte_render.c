// Software-GTE-style affine renderer for the textured geo (the "drawn as the original draws it" path).
// Transforms world tris on the CPU, near-plane CLIPS them (the GTE clip flag the GPU-affine path lacked),
// then AFFINE-projects (emit screen NDC with w=1 so UVs interpolate affinely -> the PSX texture warp).
// Depth via the GL z-buffer using the clipped z/w. Enabled behind render_geo_tex when DD2_GTE is set.
#include "render.h"
#include "../core/geo.h"
#include "../core/dmath.h"   // static inline mat4_mul
#include <GLES3/gl3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static const char* AVS =
    "#version 300 es\n"
    "layout(location=0) in vec2 a_ndc; layout(location=1) in float a_z; layout(location=2) in vec2 a_uv;"
    " layout(location=3) in float a_cl;\n"
    "uniform vec2 u_vsz; out vec2 v_uv; flat out int v_cl; out float v_d;\n"
    // a_ndc already projected to NDC; emit w=1 -> affine varying interpolation (PSX warp)
    "void main(){ v_uv=a_uv/u_vsz; v_cl=int(a_cl+0.5); v_d=clamp(a_z*1.2,0.0,1.0);\n"
    "  gl_Position=vec4(a_ndc, a_z, 1.0); }\n";
static const char* AFS =
    "#version 300 es\n"
    "precision mediump float; precision mediump int;\n"
    "in vec2 v_uv; flat in int v_cl; in float v_d; out vec4 o;\n"
    "uniform sampler2D u_vram; uniform sampler2D u_pal; uniform vec3 u_fog; uniform float u_levbright;\n"
    "void main(){ int idx=int(texture(u_vram,v_uv).r*255.0+0.5);\n"
    "  vec4 p=texelFetch(u_pal, ivec2(idx, v_cl), 0); if(p.a<0.05) discard;\n"
    "  vec3 c=p.rgb; float l=dot(c,vec3(0.299,0.587,0.114)); c=mix(vec3(l),c,1.7);\n"  // toward dd2h's vivid palette (scenery sat ~0.44)
    "  c*=vec3(1.22,1.22,1.16)*u_levbright; c=mix(c, u_fog, pow(clamp(v_d,0.0,1.0),2.0)*0.9);\n"  // per-level brightness (stormy tracks darker) + steep fog ramp
    "  o=vec4(clamp(c,0.0,1.0),1.0); }\n";

static GLuint s_prog, s_vbo, s_vram, s_pal; static GLint uv_vsz, uv_vram, uv_pal, uv_fog, uv_levbright;
static float s_vsz[2]; static int s_inited;
static float s_fog[3]={0.5f,0.5f,0.5f};   // per-level fog/depth-cue colour (from exe fog_col_ @0x46589e)
static float s_levbright=1.0f;            // per-level in-race brightness (stormy/dark tracks < 1)
void gte_set_fog(float r,float g,float b){ s_fog[0]=r; s_fog[1]=g; s_fog[2]=b; }
void gte_set_levbright(float k){ s_levbright=k; }
// source geo (kept for per-frame CPU transform)
static const float* g_tv; static int g_ntv;
static float* g_out;  static int g_outcap;   // emitted screen verts: ndc.x,ndc.y,z,u,v,clut (6 floats)

static GLuint compileA(GLenum t,const char* s){ GLuint h=glCreateShader(t);glShaderSource(h,1,&s,0);glCompileShader(h);
    GLint ok=0;glGetShaderiv(h,GL_COMPILE_STATUS,&ok); if(!ok){char L[512];glGetShaderInfoLog(h,512,0,L);fprintf(stderr,"gte: %s\n",L);} return h; }

void gte_render_set(const float* tv,int ntv,const unsigned char* vram,int vw,int vh,
                    const unsigned char* clut,int nclut){
    if(!s_inited){
        s_prog=glCreateProgram(); glAttachShader(s_prog,compileA(GL_VERTEX_SHADER,AVS));
        glAttachShader(s_prog,compileA(GL_FRAGMENT_SHADER,AFS)); glLinkProgram(s_prog);
        uv_vsz=glGetUniformLocation(s_prog,"u_vsz"); uv_vram=glGetUniformLocation(s_prog,"u_vram"); uv_pal=glGetUniformLocation(s_prog,"u_pal");
        uv_fog=glGetUniformLocation(s_prog,"u_fog"); uv_levbright=glGetUniformLocation(s_prog,"u_levbright");
        glGenBuffers(1,&s_vbo); glGenTextures(1,&s_vram); glGenTextures(1,&s_pal); s_inited=1;
    }
    g_tv=tv; g_ntv=ntv; s_vsz[0]=(float)vw; s_vsz[1]=(float)vh;
    if(vram){ glBindTexture(GL_TEXTURE_2D,s_vram); glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glTexImage2D(GL_TEXTURE_2D,0,GL_R8,vw,vh,0,GL_RED,GL_UNSIGNED_BYTE,vram);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE); }
    if(clut){ glBindTexture(GL_TEXTURE_2D,s_pal); glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,256,nclut,0,GL_RGBA,GL_UNSIGNED_BYTE,clut);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); }
}

// transform a world vertex by mvp -> clip space (x,y,z,w)
static void xf(const float* m,const float* p,float* c){
    c[0]=m[0]*p[0]+m[4]*p[1]+m[8]*p[2]+m[12];
    c[1]=m[1]*p[0]+m[5]*p[1]+m[9]*p[2]+m[13];
    c[2]=m[2]*p[0]+m[6]*p[1]+m[10]*p[2]+m[14];
    c[3]=m[3]*p[0]+m[7]*p[1]+m[11]*p[2]+m[15];
}
// emit a projected vertex (clip xyzw + uv + clut) into the output buffer
static void emit(float** o,long* n,const float* c,float u,float v,float cl){
    float iw=1.0f/c[3];
    float nx=c[0]*iw, ny=c[1]*iw;
    // PSX integer-screen-coord snap to the 320x240 grid (the vertex wobble). Safe here: near-clipped +
    // correct triangle geometry (the old collapse was the spurious-quad faces, since fixed).
    nx=floorf(nx*160.0f+0.5f)/160.0f; ny=floorf(ny*120.0f+0.5f)/120.0f;
    (*o)[(*n)++]=nx; (*o)[(*n)++]=ny; (*o)[(*n)++]=c[2]*iw;
    (*o)[(*n)++]=u; (*o)[(*n)++]=v; (*o)[(*n)++]=cl;
}

void gte_render(const float* view,const float* proj){
    if(!g_tv||g_ntv<=0) return;
    float mvp[16]; mat4_mul(mvp,proj,view);
    const float NEAR=0.05f;                          // near-plane in clip w
    if(!g_out){ g_outcap=g_ntv*8; g_out=malloc((size_t)g_outcap*6*sizeof(float)); }
    long n=0;
    for(int t=0;t+3<=g_ntv;t+=3){
        const float* A=g_tv+(size_t)t*6; const float* B=g_tv+(size_t)(t+1)*6; const float* C=g_tv+(size_t)(t+2)*6;
        float ca[4],cb[4],cc[4]; xf(mvp,A,ca); xf(mvp,B,cb); xf(mvp,C,cc);
        // near-clip against w>=NEAR: classify, output 0/1/2 tris (Sutherland-Hodgman, 1 plane)
        const float* cv[3]={ca,cb,cc}; const float* sv[3]={A,B,C};
        int in[3]; int nin=0; for(int k=0;k<3;k++){ in[k]=cv[k][3]>=NEAR; nin+=in[k]; }
        if(nin==0) continue;
        if(n+12*6>g_outcap*6) break;                  // buffer guard
        if(nin==3){
            emit(&g_out,&n,ca,A[3],A[4],A[5]); emit(&g_out,&n,cb,B[3],B[4],B[5]); emit(&g_out,&n,cc,C[3],C[4],C[5]);
            continue;
        }
        // build clipped polygon (3 or 4 verts) by walking edges, inserting near-plane intersections
        float poly[4][4]; float puv[4][3]; int pc=0;
        for(int k=0;k<3;k++){
            int k2=(k+1)%3; const float* c1=cv[k]; const float* c2=cv[k2];
            const float* s1=sv[k]; const float* s2=sv[k2];
            if(in[k]){ for(int j=0;j<4;j++)poly[pc][j]=c1[j]; puv[pc][0]=s1[3];puv[pc][1]=s1[4];puv[pc][2]=s1[5]; pc++; }
            if(in[k]!=in[k2]){
                float w1=c1[3],w2=c2[3]; float a=(NEAR-w1)/(w2-w1);
                if(pc<4){ for(int j=0;j<4;j++)poly[pc][j]=c1[j]+a*(c2[j]-c1[j]);
                    puv[pc][0]=s1[3]+a*(s2[3]-s1[3]); puv[pc][1]=s1[4]+a*(s2[4]-s1[4]); puv[pc][2]=s1[5]; pc++; }
            }
        }
        // fan-triangulate the clipped polygon
        for(int k=1;k+1<pc;k++){
            if(n+3*6>g_outcap*6) break;
            emit(&g_out,&n,poly[0],puv[0][0],puv[0][1],puv[0][2]);
            emit(&g_out,&n,poly[k],puv[k][0],puv[k][1],puv[k][2]);
            emit(&g_out,&n,poly[k+1],puv[k+1][0],puv[k+1][1],puv[k+1][2]);
        }
    }
    if(n<=0) return;
    glUseProgram(s_prog); glUniform2fv(uv_vsz,1,s_vsz); glUniform3fv(uv_fog,1,s_fog); glUniform1f(uv_levbright,s_levbright);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,s_vram); glUniform1i(uv_vram,0);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,s_pal); glUniform1i(uv_pal,1);
    glBindBuffer(GL_ARRAY_BUFFER,s_vbo); glBufferData(GL_ARRAY_BUFFER,n*sizeof(float),g_out,GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,1,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(2*sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(3); glVertexAttribPointer(3,1,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(5*sizeof(float)));
    glDrawArrays(GL_TRIANGLES,0,(int)(n/6));
    glActiveTexture(GL_TEXTURE0);
}
