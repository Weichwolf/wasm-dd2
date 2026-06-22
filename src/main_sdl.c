// SDL3 + WebGL2 (Emscripten) front-end for the faithful DD2 port.
// Renders the real game presentation at the original 320x240 internal resolution into a
// low-res FBO, crunches it through H.264/WebCodecs and bilinearly upscales (the wasm-dvd-gl
// look), then presents to the canvas. Front-end state machine starts on the real title screen.
// (3D in-race view is being built; the race is held behind the menus until it's ready.)
#include "render/ui.h"
#include "render/vram.h"
#include "render/render.h"   // dmath (mat/vec) via track.h include chain
#include <SDL3/SDL.h>
#include <GLES3/gl3.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CANVAS_W 960
#define CANVAS_H 720
#define RENDER_W 320     // original DD2 internal resolution (4:3)
#define RENDER_H 240

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
    try { S.encoder.configure({ codec:'avc1.42001f', width:w, height:h, bitrate:1100000,
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

// ---------------- present pass (fullscreen textured quad, upscale) ----------------
static const char* PVS =
    "attribute vec2 a_pos; attribute vec2 a_uv; varying vec2 v_uv;\n"
    "void main(){ v_uv=a_uv; gl_Position=vec4(a_pos,0.0,1.0); }\n";
static const char* PFS =
    "precision mediump float; varying vec2 v_uv; uniform sampler2D u_tex;\n"
    "void main(){ gl_FragColor = texture2D(u_tex, v_uv); }\n";

// ---------------- front-end state ----------------
typedef enum { ST_TITLE, ST_MENU, ST_RACE } AppState;

static SDL_Window* g_win;
static GLuint g_fbo, g_fbo_color, g_fbo_depth, g_codec_tex, g_present_prog, g_quad_vbo;
static GLint  g_u_tex;
static unsigned char* g_readback;
static int g_codec_ready = 0, g_frameno = 0;
static AppState g_state = ST_TITLE;
static GLuint g_title_tex;
// main menu (real DD2 front-end strings), rendered in the bitmap font
static const char* MENU[] = { "SELECT TRACK", "VIEW TRACK STATS", "SAVE GAME" };
#define MENU_N ((int)(sizeof(MENU)/sizeof(MENU[0])))
static GLuint g_mt_norm[MENU_N], g_mt_sel[MENU_N];
static int    g_mw[MENU_N], g_mh[MENU_N];
static int    g_sel = 0;

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

// draw the current front-end screen into the low-res FBO
static void render_scene(void){
    glBindFramebuffer(GL_FRAMEBUFFER,g_fbo); glViewport(0,0,RENDER_W,RENDER_H);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    switch(g_state){
        case ST_TITLE: ui_blit_fullscreen(g_title_tex); break;
        case ST_MENU:
            ui_blit_fullscreen(g_title_tex);
            for (int i=0;i<MENU_N;i++){
                GLuint t = (i==g_sel)?g_mt_sel[i]:g_mt_norm[i];
                if(t) ui_blit_rect(t, (RENDER_W-g_mw[i])/2.f, 150.f+i*18.f, g_mw[i], g_mh[i], RENDER_W, RENDER_H);
            }
            break;
        case ST_RACE:  /* held until 3D view ready */ break;
    }
}

static void present(void){
    int use_codec=0;
    if(g_codec_ready){
        glBindFramebuffer(GL_FRAMEBUFFER,g_fbo);
        glReadPixels(0,0,RENDER_W,RENDER_H,GL_RGBA,GL_UNSIGNED_BYTE,g_readback);
        dvd_codec_push((int)(intptr_t)g_readback,RENDER_W,RENDER_H,(double)g_frameno*16666.0);
        if(dvd_codec_upload((int)g_codec_tex)) use_codec=1;
    }
    g_frameno++;
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

static int g_ticks=0;
static void frame(void){
    SDL_Event e;
    while(SDL_PollEvent(&e)){
        if(e.type==SDL_EVENT_KEY_DOWN){
            SDL_Scancode sc=e.key.scancode;
            if(g_state==ST_TITLE && (sc==SDL_SCANCODE_RETURN||sc==SDL_SCANCODE_SPACE)) g_state=ST_MENU;
            else if(g_state==ST_MENU){
                if(sc==SDL_SCANCODE_DOWN) g_sel=(g_sel+1)%MENU_N;
                else if(sc==SDL_SCANCODE_UP) g_sel=(g_sel+MENU_N-1)%MENU_N;
            }
        }
    }
    if(g_state==ST_TITLE && ++g_ticks>120) g_state=ST_MENU;   // attract-mode auto-advance
    render_scene();
    present();
}

int main(void){
    if(!SDL_Init(SDL_INIT_VIDEO)){SDL_Log("SDL_Init: %s",SDL_GetError());return 1;}
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3); SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
    g_win=SDL_CreateWindow("Destruction Derby 2",CANVAS_W,CANVAS_H,SDL_WINDOW_OPENGL);
    if(!g_win){SDL_Log("CreateWindow: %s",SDL_GetError());return 1;}
    if(!SDL_GL_CreateContext(g_win)){SDL_Log("GL ctx: %s",SDL_GetError());return 1;}
    SDL_Log("GL_VERSION: %s",(const char*)glGetString(GL_VERSION));
    ui_init(); present_init();
    int tw,th; g_title_tex = ui_load_bmp("assets/raw/LEV0/COPYRIGH.BMP",&tw,&th);
    SDL_Log("title %dx%d tex=%u", tw, th, g_title_tex);
    if (vram_init("LEV0")) {
        for(int i=0;i<MENU_N;i++){
            g_mt_norm[i]=vram_text_tex(MENU[i],230,170,40,&g_mw[i],&g_mh[i]);  // ink: DD2 amber
            g_mt_sel[i] =vram_text_tex(MENU[i],255,255,255,NULL,NULL);          // selected: white
        }
        SDL_Log("menu ready: %d items", MENU_N);
    }
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame,0,1);
#else
    for(;;){frame();SDL_Delay(16);}
#endif
    return 0;
}
