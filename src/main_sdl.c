// SDL3 + WebGL2 (Emscripten) frontend with the wasm-dvd-gl cinematic look:
// the self-playing AI race renders into a low-res FBO, that frame is encoded to H.264 at
// low bitrate via WebCodecs and immediately decoded (real DVD/PSX-era compression artifacts),
// then the decoded VideoFrame is uploaded and bilinearly upscaled to the canvas. Shares the
// same deterministic core + GLES3 renderer as the headless build.
#include "core/race.h"
#include "render/render.h"
#include <SDL3/SDL.h>
#include <GLES3/gl3.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CANVAS_W 960
#define CANVAS_H 720
#define RENDER_W 512     // low internal res (4:3) -> codec crunch -> upscale
#define RENDER_H 384

static const char* PLAYLIST[] = {
    "assets/raw/LEV5/LEVEL.DAT","assets/raw/LEV1/LEVEL.DAT","assets/raw/LEV3/LEVEL.DAT",
    "assets/raw/LEV4/LEVEL.DAT","assets/raw/LEV6/LEVEL.DAT","assets/raw/LEV2/LEVEL.DAT",
    "assets/raw/LEV7/LEVEL.DAT","assets/raw/LEV8/LEVEL.DAT","assets/raw/LEV9/LEVEL.DAT",
    "assets/raw/LEVA/LEVEL.DAT","assets/raw/LEVB/LEVEL.DAT",
};
static const int PLAYLIST_N = sizeof(PLAYLIST)/sizeof(PLAYLIST[0]);
static const float CAR_COLS[8][3] = {
    {0.90f,0.20f,0.15f},{0.20f,0.45f,0.95f},{0.95f,0.85f,0.15f},{0.20f,0.80f,0.30f},
    {0.95f,0.55f,0.10f},{0.75f,0.25f,0.85f},{0.15f,0.85f,0.85f},{0.85f,0.85f,0.85f},
};

// ---------------- WebCodecs H.264 crunch (browser only) ----------------
#ifdef __EMSCRIPTEN__
EM_JS(int, dvd_codec_init, (int w, int h), {
    if (typeof VideoEncoder === 'undefined' || typeof VideoDecoder === 'undefined') {
        console.warn('[dvd] WebCodecs unavailable - plain soft upscale.'); return 0; }
    const S = { ready:false, frame:null, frameCount:0 }; Module.__dvd = S;
    S.decoder = new VideoDecoder({ output:(f)=>{ if(S.frame)S.frame.close(); S.frame=f; },
                                   error:(e)=>console.error('[dvd] dec',e) });
    S.encoder = new VideoEncoder({ output:(chunk,meta)=>{
        if (meta && meta.decoderConfig && S.decoder.state==='unconfigured') S.decoder.configure(meta.decoderConfig);
        if (S.decoder.state==='configured') S.decoder.decode(chunk); },
        error:(e)=>console.error('[dvd] enc',e) });
    try { S.encoder.configure({ codec:'avc1.42001f', width:w, height:h, bitrate:1200000,
        framerate:60, latencyMode:'realtime', avc:{format:'avc'} }); }
    catch(e){ console.error('[dvd] configure failed',e); return 0; }
    S.ready = true; console.log('[dvd] WebCodecs ready', w+'x'+h); return 1;
});
EM_JS(void, dvd_codec_push, (int ptr, int w, int h, double ts), {
    const S = Module.__dvd; if (!S || !S.ready) return;
    if (S.encoder.encodeQueueSize > 2) return;
    let vf; try { vf = new VideoFrame(HEAPU8.subarray(ptr, ptr+w*h*4),
        { format:'RGBA', codedWidth:w, codedHeight:h, timestamp:ts }); } catch(e){ return; }
    const key = (S.frameCount % 60) === 0; S.frameCount++;
    try { S.encoder.encode(vf, { keyFrame:key }); } catch(e){} vf.close();
});
EM_JS(int, dvd_codec_upload, (int texId), {
    const S = Module.__dvd; if (!S || !S.frame) return 0;
    const tex = GL.textures[texId]; if (!tex) return 0;
    GLctx.bindTexture(GLctx.TEXTURE_2D, tex);
    try { GLctx.texImage2D(GLctx.TEXTURE_2D,0,GLctx.RGBA,GLctx.RGBA,GLctx.UNSIGNED_BYTE,S.frame); }
    catch(e){ return 0; } return 1;
});
EM_JS(int, canvas_px_w, (), { return GLctx.drawingBufferWidth; });
EM_JS(int, canvas_px_h, (), { return GLctx.drawingBufferHeight; });
#else
static int dvd_codec_init(int w,int h){(void)w;(void)h;return 0;}
static void dvd_codec_push(int p,int w,int h,double t){(void)p;(void)w;(void)h;(void)t;}
static int dvd_codec_upload(int t){(void)t;return 0;}
static int canvas_px_w(void){return CANVAS_W;} static int canvas_px_h(void){return CANVAS_H;}
#endif

// ---------------- present pass (fullscreen textured quad) ----------------
static const char* PVS =
    "attribute vec2 a_pos; attribute vec2 a_uv; varying vec2 v_uv;\n"
    "void main(){ v_uv=a_uv; gl_Position=vec4(a_pos,0.0,1.0); }\n";
static const char* PFS =
    "precision mediump float; varying vec2 v_uv; uniform sampler2D u_tex;\n"
    "void main(){ gl_FragColor = texture2D(u_tex, v_uv); }\n";

static SDL_Window* g_win;
static Race  g_race;
static int   g_track = 0; static unsigned g_seed = 1;
static float g_view[16], g_proj[16];
static GLuint g_fbo, g_fbo_color, g_fbo_depth, g_codec_tex, g_present_prog, g_quad_vbo;
static GLint  g_u_tex;
static unsigned char* g_readback;
static int g_codec_ready = 0, g_frameno = 0;

static GLuint psh(GLenum t,const char*s){GLuint h=glCreateShader(t);glShaderSource(h,1,&s,0);glCompileShader(h);return h;}
static GLuint make_tex(int w,int h){
    GLuint t; glGenTextures(1,&t); glBindTexture(GL_TEXTURE_2D,t);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,0);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    return t;
}
static void present_init(void){
    g_fbo_color = make_tex(RENDER_W, RENDER_H);
    glGenRenderbuffers(1,&g_fbo_depth); glBindRenderbuffer(GL_RENDERBUFFER,g_fbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT16,RENDER_W,RENDER_H);
    glGenFramebuffers(1,&g_fbo); glBindFramebuffer(GL_FRAMEBUFFER,g_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,g_fbo_color,0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,g_fbo_depth);
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    g_codec_tex = make_tex(RENDER_W, RENDER_H);
    g_readback = (unsigned char*)malloc((size_t)RENDER_W*RENDER_H*4);
    g_present_prog = glCreateProgram();
    glAttachShader(g_present_prog,psh(GL_VERTEX_SHADER,PVS));
    glAttachShader(g_present_prog,psh(GL_FRAGMENT_SHADER,PFS));
    glBindAttribLocation(g_present_prog,0,"a_pos"); glBindAttribLocation(g_present_prog,1,"a_uv");
    glLinkProgram(g_present_prog); g_u_tex=glGetUniformLocation(g_present_prog,"u_tex");
    static const float quad[]={-1,-1,0,0, 1,-1,1,0, 1,1,1,1, -1,-1,0,0, 1,1,1,1, -1,1,0,1};
    glGenBuffers(1,&g_quad_vbo); glBindBuffer(GL_ARRAY_BUFFER,g_quad_vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(quad),quad,GL_STATIC_DRAW);
    g_codec_ready = dvd_codec_init(RENDER_W, RENDER_H);
}

static void setup_cam(const Track* t){
    vec3 ctr=v3scale(v3add(t->bbmin,t->bbmax),0.5f);
    vec3 size=v3sub(t->bbmax,t->bbmin); float radius=v3len(v3(size.x,0,size.z))*0.5f; if(radius<1)radius=1;
    vec3 eye=v3(ctr.x, ctr.y+radius*1.35f, ctr.z-radius*1.0f);
    mat4_lookat(g_view,eye,ctr,v3(0,1,0));
    mat4_perspective(g_proj,1.0f,(float)RENDER_W/RENDER_H,1.0f,radius*8.0f);
}
static void load_track(int idx){
    g_track=idx%PLAYLIST_N;
    if(!race_init(&g_race,PLAYLIST[g_track],6,2,g_seed)){fprintf(stderr,"load fail %s\n",PLAYLIST[g_track]);return;}
    render_set_track(&g_race.track); setup_cam(&g_race.track);
    SDL_Log("track %d: %s (%d ribs, %s)",g_track,PLAYLIST[g_track],g_race.track.nribs,g_race.arena?"arena":"circuit");
}

static void frame(void){
    SDL_Event e;
    while(SDL_PollEvent(&e))
        if(e.type==SDL_EVENT_KEY_DOWN && e.key.scancode==SDL_SCANCODE_N) load_track(g_track+1);
    if(!race_done(&g_race)) race_step(&g_race,1.0f/60.0f);
    else { g_seed++; load_track(g_track+1); }

    // pass 1: scene -> low-res FBO
    glBindFramebuffer(GL_FRAMEBUFFER,g_fbo); glViewport(0,0,RENDER_W,RENDER_H);
    render_begin(0.45f,0.6f,0.8f); render_track(g_view,g_proj);
    for(int i=0;i<g_race.ncars;i++){Car*c=&g_race.cars[i];const float*col=CAR_COLS[i%8];
        render_box(g_view,g_proj,v3(c->pos.x,c->pos.y+0.6f,c->pos.z),v3(1.0f,0.6f,2.0f),c->yaw,col[0],col[1],col[2]);}

    // codec crunch: readback -> encode -> decode -> upload
    int use_codec=0;
    if(g_codec_ready){
        glReadPixels(0,0,RENDER_W,RENDER_H,GL_RGBA,GL_UNSIGNED_BYTE,g_readback);
        dvd_codec_push((int)(intptr_t)g_readback,RENDER_W,RENDER_H,(double)g_frameno*16666.0);
        if(dvd_codec_upload((int)g_codec_tex)) use_codec=1;
    }
    g_frameno++;

    // pass 2: present upscaled to canvas (aspect-correct)
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    int dw=canvas_px_w(), dh=canvas_px_h();
    glDisable(GL_DEPTH_TEST); glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
    float target=(float)RENDER_W/RENDER_H; int vw=dw,vh=dh;
    if((float)dw/dh>target) vw=(int)(dh*target+0.5f); else vh=(int)(dw/target+0.5f);
    glViewport((dw-vw)/2,(dh-vh)/2,vw,vh);
    glUseProgram(g_present_prog); glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, use_codec?g_codec_tex:g_fbo_color); glUniform1i(g_u_tex,0);
    glBindBuffer(GL_ARRAY_BUFFER,g_quad_vbo);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glDrawArrays(GL_TRIANGLES,0,6);
    SDL_GL_SwapWindow(g_win);
}

int main(void){
    if(!SDL_Init(SDL_INIT_VIDEO)){SDL_Log("SDL_Init: %s",SDL_GetError());return 1;}
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
    g_win=SDL_CreateWindow("Destruction Derby 2 — WASM",CANVAS_W,CANVAS_H,SDL_WINDOW_OPENGL);
    if(!g_win){SDL_Log("CreateWindow: %s",SDL_GetError());return 1;}
    if(!SDL_GL_CreateContext(g_win)){SDL_Log("GL ctx: %s",SDL_GetError());return 1;}
    SDL_Log("GL_VERSION: %s",(const char*)glGetString(GL_VERSION));
    render_init(); present_init(); load_track(0);
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame,0,1);
#else
    for(;;){frame();SDL_Delay(16);}
#endif
    return 0;
}
