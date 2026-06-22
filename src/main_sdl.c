// SDL3 + WebGL2 (Emscripten) front-end for the faithful DD2 port.
// Renders the real game presentation at the original 320x240 internal resolution into a
// low-res FBO, crunches it through H.264/WebCodecs and bilinearly upscales (the wasm-dvd-gl
// look), then presents to the canvas. Front-end state machine starts on the real title screen.
// (3D in-race view is being built; the race is held behind the menus until it's ready.)
#include "render/ui.h"
#include "render/vram.h"
#include "render/render.h"
#include "core/race.h"
#include "core/geo.h"
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
EM_JS(int, em_raw_param, (), { return (location.search.indexOf("raw")>=0)?1:0; });
// ---- WebAudio from BANK1.SBK (8-bit unsigned PCM) ----
EM_JS(int, audio_init, (), {
  try{ const C=new (window.AudioContext||window.webkitAudioContext)();
    Module.__au={ctx:C,buf:[],eng:null,eg:null};
    const r=()=>{ if(C.state!=='running')C.resume(); };
    window.addEventListener('pointerdown',r); window.addEventListener('keydown',r);
    console.log('[audio] WebAudio ready'); return 1;
  }catch(e){ console.warn('[audio] unavailable',e); return 0; }
});
EM_JS(void, audio_add, (int idx,int ptr,int n,int rate), {
  const A=Module.__au; if(!A)return;
  const b=A.ctx.createBuffer(1,n,rate||11025); const ch=b.getChannelData(0);
  for(let i=0;i<n;i++) ch[i]=(HEAPU8[ptr+i]-128)/128.0; A.buf[idx]=b;
});
EM_JS(void, audio_engine, (int idx,float rate,float gain), {
  const A=Module.__au; if(!A||!A.buf[idx])return;
  if(!A.eng){ const s=A.ctx.createBufferSource(); s.buffer=A.buf[idx]; s.loop=true;
    const g=A.ctx.createGain(); g.gain.value=0; s.connect(g); g.connect(A.ctx.destination); s.start(); A.eng=s; A.eg=g; }
  try{ A.eng.playbackRate.value=rate; A.eg.gain.value=gain; }catch(e){}
});
EM_JS(void, audio_oneshot, (int idx,float gain), {
  const A=Module.__au; if(!A||!A.buf[idx]||A.ctx.state!=='running')return;
  const s=A.ctx.createBufferSource(); s.buffer=A.buf[idx];
  const g=A.ctx.createGain(); g.gain.value=gain; s.connect(g); g.connect(A.ctx.destination); s.start();
});
#else
static int audio_init(void){return 0;}
static void audio_add(int i,int p,int n,int r){(void)i;(void)p;(void)n;(void)r;}
static void audio_engine(int i,float r,float g){(void)i;(void)r;(void)g;}
static void audio_oneshot(int i,float g){(void)i;(void)g;}
#endif
#ifndef __EMSCRIPTEN__
static int dvd_codec_init(int w,int h){(void)w;(void)h;return 0;}
static void dvd_codec_push(int p,int w,int h,double t){(void)p;(void)w;(void)h;(void)t;}
static int dvd_codec_upload(int t){(void)t;return 0;}
static int canvas_px_w(void){return CANVAS_W;} static int canvas_px_h(void){return CANVAS_H;}
static int em_raw_param(void){return 0;}
#endif

// ---------------- present pass (fullscreen textured quad, upscale) ----------------
static const char* PVS =
    "attribute vec2 a_pos; attribute vec2 a_uv; varying vec2 v_uv;\n"
    "void main(){ v_uv=a_uv; gl_Position=vec4(a_pos,0.0,1.0); }\n";
static const char* PFS =
    "precision mediump float; varying vec2 v_uv; uniform sampler2D u_tex;\n"
    "void main(){ gl_FragColor = texture2D(u_tex, v_uv); }\n";

// ---------------- front-end state ----------------
typedef enum { ST_TITLE, ST_MENU, ST_TRACKSEL, ST_STATS, ST_RACE, ST_RESULTS } AppState;

static SDL_Window* g_win;
static GLuint g_fbo, g_fbo_color, g_fbo_depth, g_codec_tex, g_present_prog, g_quad_vbo;
static GLint  g_u_tex;
static unsigned char* g_readback;
static int g_codec_ready = 0, g_frameno = 0;
static AppState g_state = ST_TITLE;
static GLuint g_title_tex, g_dim_tex, g_car_sw[8], g_metal_tex;
// main menu (real DD2 front-end strings), rendered in the bitmap font
static const char* MENU[] = { "SELECT TRACK", "VIEW TRACK STATS", "SAVE GAME" };
#define MENU_N ((int)(sizeof(MENU)/sizeof(MENU[0])))
static GLuint g_mt_norm[MENU_N], g_mt_sel[MENU_N];
static int    g_mw[MENU_N], g_mh[MENU_N];
static int    g_sel = 0;
// real DD2 track names in the game's menu order (racing-name array @0x4682f8). 11 tracks.
// (Destruction Derby / Total Destruction are race MODES, not tracks; Black Sail Valley was missing.)
static const char* TRACKS[] = {
    "PINE HILLS RACEWAY","CHALK CANYON","S.C.A. MOTORPLEX","CAPRIO COUNTY RACEWAY","BLACK SAIL VALLEY",
    "LIBERTY CITY","ULTIMATE DESTRUCTION","RED PIKE ARENA","THE COLOSSEUM","THE PIT","DEATH BOWL"
};
#define TRACK_N ((int)(sizeof(TRACKS)/sizeof(TRACKS[0])))
static GLuint g_tt_norm[TRACK_N], g_tt_sel[TRACK_N];
static int    g_tw[TRACK_N], g_th[TRACK_N], g_tsel=0;
// preview-image sprite name per track (LEVEL.SPR), shown on track-select (CLUT-coloured)
static const char* PREVIEW_SPR[TRACK_N] = {
    "PINEHILL","CHALKCAN","MOTRPLEX","CAPRIO","BLAKSAIL","LIBCITY","ULTDEST","REDPIKE","COLOSEUM","THEPIT","DETHBOWL"
};
static GLuint g_prev_tex[TRACK_N]; static int g_prev_w[TRACK_N], g_prev_h[TRACK_N], g_prev_try[TRACK_N];
// radial main menu: icon sprite + screen slot (matches the dd2h metal-button layout)
static struct { const char* icon; float x,y; } MBTN[] = {
    {"IWRECKIN",58,98},{"IRACMODE",112,98},{"ITRACK",166,98},{"ICAR",220,98},
    {"IVIEWSTA",86,150},{"ICONFIG",140,150},{"ICREDITS",194,150},
};
#define MBTN_N ((int)(sizeof(MBTN)/sizeof(MBTN[0])))
static GLuint g_icon_tex[MBTN_N]; static int g_iw[MBTN_N], g_ih[MBTN_N];
static GLuint g_ring_tex, g_logo_tex, g_go_tex; static int g_ringw,g_ringh,g_logow,g_logoh,g_gow,g_goh;
static int    g_best_lap[TRACK_N]={0}, g_races_played[TRACK_N]={0};  // per-track stats (centiseconds)
// Championship game-flow: season 0 race sequence (exe table @0x46758c levels 1,2,5,7,10 -> our track
// indices) + points by finish position. The "16 levels" = these championship EVENTS over the 11 tracks.
static const int CHAMP_SEQ[5] = {1,8,3,6,7};          // track indices for levels 1,2,5,7,10
static const int CHAMP_PTS[8] = {10,8,6,5,4,3,2,1};   // points by finishing position
static int g_champ_active=0, g_champ_idx=0, g_champ_awarded=0, g_champ_points[RACE_MAX_CARS]={0};
// track-name index -> LEVEL.DAT level dir, menu order (LEV5=Caprio/LEV6=Pine Hills confirmed by content;
// LEV9=Black Sail Valley = the one track with no other name; each of LEV1-B used once. Verify per-level.)
static const char* TRACK_LEV[TRACK_N] = {
    "LEV6","LEV1","LEV4","LEV5","LEV9","LEV3","LEV7","LEVA","LEV2","LEVB","LEV8"
};
static Race  g_race; static int g_racing=0; static int g_last_hits=0;
static Geo   g_geo;
#define SND_ENGINE 0
#define SND_CRASH  1
static const float CAR_COLS[8][3]={{.9f,.2f,.15f},{.2f,.45f,.95f},{.95f,.85f,.15f},{.2f,.8f,.3f},
    {.95f,.55f,.1f},{.75f,.25f,.85f},{.15f,.85f,.85f},{.85f,.85f,.85f}};

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

// transient HUD text: build glyph texture, blit, free (small strings, per-frame is fine)
static void hud_text(const char* s, float x, float y, float scale, int hi){
    int w,h; GLuint t=vram_text_tex(s, hi?255:235, hi?255:185, hi?255:55, &w,&h);
    if(t){ ui_blit_rect(t, x, y, w*scale, h*scale, RENDER_W, RENDER_H); glDeleteTextures(1,&t); }
}

// a recognizable stock-car: low wide body + rear cabin + 4 dark wheels (composed from boxes).
// Battle damage: body darkens + crumples (lower/narrower) with hit count.
static void draw_car(const float* view, const float* proj, vec3 p, float yaw, const float* col, int hits){
    float cs=cosf(yaw), sn=sinf(yaw);
    float dmg = hits>24?1.0f:(float)hits/24.f;            // 0..1 damage
    float k = 1.0f - dmg*0.55f;                           // darken (soot/dents)
    float sq = 1.0f - dmg*0.28f;                          // crumple
    float br=col[0]*k, bg=col[1]*k, bb=col[2]*k;          // body colour
    // local-offset box: oriented by yaw (z=forward, x=side, y=up)
    #define PART(ox,oy,oz, hx,hy,hz, r,g,b) render_box(view,proj, \
        v3(p.x+cs*(ox)-sn*(oz), p.y+(oy), p.z+sn*(ox)+cs*(oz)), v3(hx,hy,hz), yaw, r,g,b)
    PART(0,0.34f,0,        0.92f,0.27f,1.95f, br,bg,bb);                 // chassis
    PART(0,0.48f*sq,1.0f,  0.84f,0.11f,0.75f, br,bg,bb);                 // hood (front, low)
    PART(0,0.86f*sq,-0.45f,0.76f,0.27f*sq,0.78f, br*0.82f,bg*0.82f,bb*0.82f); // cabin/roof (rear, raised)
    PART(0,0.74f*sq,0.32f, 0.70f,0.20f*sq,0.12f, 0.10f,0.12f,0.16f);     // windshield (dark glass)
    PART(0,0.66f,-1.28f,   0.86f,0.05f,0.16f, 0.10f,0.10f,0.11f);        // rear deck/spoiler
    const float wx=0.88f, wz=1.26f;
    for(int s=0;s<4;s++){ float lx=(s&1)?wx:-wx, lz=(s&2)?wz:-wz;
        PART(lx,0.20f,lz, 0.18f,0.22f,0.34f, 0.07f,0.07f,0.08f); }       // wheels
    #undef PART
}

// draw the current front-end screen into the low-res FBO
static void render_scene(void){
    glBindFramebuffer(GL_FRAMEBUFFER,g_fbo); glViewport(0,0,RENDER_W,RENDER_H);
    glDisable(GL_DEPTH_TEST);
    glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    switch(g_state){
        case ST_TITLE: ui_blit_fullscreen(g_title_tex); break;
        case ST_MENU: {
            ui_blit_rect(g_metal_tex,0,0,RENDER_W,RENDER_H,RENDER_W,RENDER_H);   // metal backdrop
            if(g_logo_tex && g_logow>0){ float lw=180.f, lh=lw*g_logoh/(float)g_logow;
                ui_blit_rect_tint(g_logo_tex,(RENDER_W-lw)/2.f,12,lw,lh,RENDER_W,RENDER_H, 1.18f,0.55f,0.16f); }  // dd2h orange logo
            for(int i=0;i<MBTN_N;i++){
                float r=(g_sel==i)?27.f:22.f;
                if(g_ring_tex) ui_blit_rect(g_ring_tex,MBTN[i].x-r,MBTN[i].y-r,r*2,r*2,RENDER_W,RENDER_H);
                if(g_icon_tex[i]&&g_iw[i]>0){ float iw=r*1.4f, ih=iw*g_ih[i]/(float)g_iw[i];
                    ui_blit_rect(g_icon_tex[i],MBTN[i].x-iw/2,MBTN[i].y-ih/2,iw,ih,RENDER_W,RENDER_H); }
            }
            { float gx=252,gy=178,r=(g_sel==MBTN_N)?26.f:21.f;        // GO/forward button
              if(g_ring_tex) ui_blit_rect(g_ring_tex,gx-r,gy-r,r*2,r*2,RENDER_W,RENDER_H);
              if(g_go_tex&&g_gow>0){ float iw=r*1.2f, ih=iw*g_goh/(float)g_gow;
                  ui_blit_rect(g_go_tex,gx-iw/2,gy-ih/2,iw,ih,RENDER_W,RENDER_H); } }
            hud_text("WRECKING   RACING   PRACTICE", 60, 214, 0.55f, 0);
            break; }
        case ST_TRACKSEL: {
            // dd2h shows ONE cycling track preview + name (not a list). Match that layout.
            ui_blit_fullscreen(g_title_tex);
            ui_blit_rect(g_dim_tex,0,0,RENDER_W,RENDER_H,RENDER_W,RENDER_H);
            int hw=vram_text_measure("SELECT TRACK");
            hud_text("SELECT TRACK",(RENDER_W-hw*0.8f)/2.f,18,0.8f,1);
            // large centred preview of the selected track (lazily loaded)
            if(!g_prev_try[g_tsel]){ g_prev_try[g_tsel]=1;
                g_prev_tex[g_tsel]=vram_sprite_tex(PREVIEW_SPR[g_tsel],&g_prev_w[g_tsel],&g_prev_h[g_tsel]); }
            if(g_prev_tex[g_tsel]&&g_prev_w[g_tsel]>0){
                float pw=176.f, ph=pw*g_prev_h[g_tsel]/(float)g_prev_w[g_tsel];
                if(ph>118.f){ ph=118.f; pw=ph*g_prev_w[g_tsel]/(float)g_prev_h[g_tsel]; }
                ui_blit_rect(g_prev_tex[g_tsel],(RENDER_W-pw)/2.f, 52, pw, ph, RENDER_W, RENDER_H); }
            // selected track name (prominent) under the preview
            GLuint t=g_tt_sel[g_tsel]; float w=g_tw[g_tsel]*0.85f, h=g_th[g_tsel]*0.85f;
            if(t) ui_blit_rect(t,(RENDER_W-w)/2.f, 182, w, h, RENDER_W, RENDER_H);
            hud_text("UP/DOWN  SELECT      GO  RACE", 58, 214, 0.5f, 0);
            break; }
        case ST_STATS: {
            ui_blit_fullscreen(g_title_tex);
            ui_blit_rect(g_dim_tex,0,0,RENDER_W,RENDER_H,RENDER_W,RENDER_H);
            int hw=vram_text_measure("TRACK STATISTICS");
            hud_text("TRACK STATISTICS",(RENDER_W-hw*0.85f)/2.f,26,0.85f,1);
            int nw=vram_text_measure(TRACKS[g_tsel]);
            hud_text(TRACKS[g_tsel],(RENDER_W-nw*0.7f)/2.f,62,0.7f,0);
            char b[64];
            int bl=g_best_lap[g_tsel];
            if(bl>0){ snprintf(b,sizeof(b),"FASTEST LAP   %d:%02d.%02d",bl/6000,(bl/100)%60,bl%100); }
            else     snprintf(b,sizeof(b),"FASTEST LAP   --:--.--");
            hud_text(b,70,98,0.6f,0);
            snprintf(b,sizeof(b),"RACES PLAYED   %d",g_races_played[g_tsel]); hud_text(b,70,118,0.6f,0);
            hud_text("UP/DOWN  TRACK      ESC  BACK",70,150,0.5f,0);
            break; }
        case ST_RESULTS: {
            ui_blit_fullscreen(g_title_tex);
            ui_blit_rect(g_dim_tex,0,0,RENDER_W,RENDER_H,RENDER_W,RENDER_H);
            char b[64];
            const char* ttl = g_champ_active ? "CHAMPIONSHIP" : "RACE RESULTS";
            int hw=vram_text_measure(ttl); hud_text(ttl,(RENDER_W-hw*0.85f)/2.f,12,0.85f,1);
            if(g_champ_active){ snprintf(b,sizeof(b),"%s   RACE %d/5",TRACKS[g_tsel],g_champ_idx+1); }
            else snprintf(b,sizeof(b),"%s",TRACKS[g_tsel]);
            int hw2=vram_text_measure(b); hud_text(b,(RENDER_W-hw2*0.55f)/2.f,34,0.55f,0);
            int order[RACE_MAX_CARS]; int nc=race_rank(&g_race,order);
            static const char* ORD[]={"1ST","2ND","3RD","4TH","5TH","6TH","7TH","8TH"};
            if(g_champ_active){
                // championship standings: drivers sorted by accumulated points
                int so[RACE_MAX_CARS]; for(int i=0;i<nc;i++)so[i]=i;
                for(int i=0;i<nc;i++)for(int j=i+1;j<nc;j++) if(g_champ_points[so[j]]>g_champ_points[so[i]]){int t=so[i];so[i]=so[j];so[j]=t;}
                hud_text("STANDINGS         PTS",70,50,0.5f,1);
                for(int i=0;i<nc&&i<8;i++){ int ci=so[i];
                    ui_blit_rect(g_car_sw[ci%8],70,64+i*15,9,9,RENDER_W,RENDER_H);
                    snprintf(b,sizeof(b),"%d. CAR %d%s",i+1,ci+1, ci==0?" (YOU)":"");
                    hud_text(b,84,62+i*15,0.5f,ci==0);
                    snprintf(b,sizeof(b),"%d",g_champ_points[ci]); hud_text(b,238,62+i*15,0.5f,ci==0); }
            } else {
                for(int i=0;i<nc;i++){ int ci=order[i];
                    ui_blit_rect(g_car_sw[ci%8], 96, 60+i*15, 9, 9, RENDER_W, RENDER_H);
                    snprintf(b,sizeof(b),"%s   CAR %d%s",ORD[i],ci+1, ci==0?"  (YOU)":"");
                    hud_text(b,110,58+i*15,0.55f, ci==0); }
            }
            break; }
        case ST_RACE: if(g_racing){
            glEnable(GL_DEPTH_TEST);
            Car* c=&g_race.cars[0];
            float sy=sinf(c->yaw), cyy=cosf(c->yaw);
            vec3 eye=v3(c->pos.x-sy*9.f, c->pos.y+5.f, c->pos.z-cyy*9.f);
            vec3 at =v3(c->pos.x+sy*6.f, c->pos.y+1.5f, c->pos.z+cyy*6.f);
            float view[16],proj[16];
            mat4_lookat(view,eye,at,v3(0,1,0));
            mat4_perspective(proj,1.0f,(float)RENDER_W/RENDER_H,1.0f,800.0f);
            render_sky();                       // gradient sky/horizon
            render_track(view,proj);            // drivable surface: procedural brown (real VRAM/CLUT ground
                                                //   exists/renders but per-texture colours read multicolored vs
                                                //   ref's cohesive brown until CLUT-exact; scenery below IS textured)
            render_geo(view,proj);              // flat-shaded authentic faces
#ifdef DD2_GTE
            gte_render(view,proj);              // software-GTE affine (near-clipped) textured faces
#else
            render_geo_tex(view,proj);          // VRAM/CLUT-textured faces (trees/walls/scenery)
#endif
            for(int i=0;i<g_race.ncars;i++){ Car* cc=&g_race.cars[i];
                draw_car(view,proj,cc->pos,cc->yaw,CAR_COLS[i%8],cc->hits); }
            // HUD (real bitmap font over the 3D view)
            glDisable(GL_DEPTH_TEST);
            char buf[64]; int order[RACE_MAX_CARS];
            int nc=race_rank(&g_race,order), pos=1; for(int i=0;i<nc;i++) if(order[i]==0){pos=i+1;break;}
            int lap=c->lap+1; if(lap>g_race.target_laps)lap=g_race.target_laps;
            snprintf(buf,sizeof(buf),"LAP %d/%d",lap,g_race.target_laps);    hud_text(buf,6,5,0.6f,0);
            snprintf(buf,sizeof(buf),"POS %d/%d",pos,nc);                    hud_text(buf,6,19,0.6f,0);
            snprintf(buf,sizeof(buf),"%d MPH",(int)(c->speed*2.237f+0.5f));  hud_text(buf,6,33,0.6f,0);
            { int dp=c->hits>24?100:c->hits*100/24; snprintf(buf,sizeof(buf),"DMG %d%%",dp); hud_text(buf,6,47,0.6f,dp>=70); }
            { int tt=(int)g_race.time; snprintf(buf,sizeof(buf),"%d:%02d",tt/60,tt%60);
              int tw2=vram_text_measure(buf); hud_text(buf,RENDER_W-tw2*0.6f-6,5,0.6f,0); }
        } break;
    }
}

static int g_raw=0;   // 1 = present the clean 320x240 FBO (skip DVD crunch) for reference comparison
static void present(void){
    int use_codec=0;
    if(g_codec_ready && !g_raw){
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

static void audio_load(void){
    FILE* f=fopen("assets/raw/VAGS/BANK1.SBK","rb"); if(!f){SDL_Log("no SBK");return;}
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    unsigned char* d=(unsigned char*)malloc(n);
    if(!d||fread(d,1,n,f)!=(size_t)n){free(d);fclose(f);return;} fclose(f);
    if(!audio_init()){free(d);return;}
    #define RD32(p) ((unsigned)(d[p]|(d[p+1]<<8)|(d[p+2]<<16)|((unsigned)d[p+3]<<24)))
    unsigned count=RD32(12); int loaded=0;
    for(unsigned i=0;i<count;i++){ int b=16+i*28; unsigned off=RD32(b),size=RD32(b+4),rate=RD32(b+12);
        if(off+size<=(unsigned)n){ audio_add(i,(int)(intptr_t)(d+off),(int)size,(int)rate); loaded++; } }
    SDL_Log("audio: %d/%u samples loaded from BANK1.SBK",loaded,count);
    free(d);   // audio_add copies into AudioBuffers synchronously
    #undef RD32
}

static void start_race(int idx){
    char dat[160]; snprintf(dat,sizeof(dat),"assets/raw/%s/LEVEL.DAT",TRACK_LEV[idx]);
    if(race_init(&g_race,dat,RACE_MAX_CARS,2,1234u)){
        render_set_track(&g_race.track);
        geo_free(&g_geo); if(geo_load(dat,&g_geo)){ render_geo_set(g_geo.v,g_geo.nverts);
            render_geo_set_tex(g_geo.tv,g_geo.ntverts,g_geo.vram,g_geo.vram_w,g_geo.vram_h,g_geo.clut,g_geo.nclut);
            gte_render_set(g_geo.tv,g_geo.ntverts,g_geo.vram,g_geo.vram_w,g_geo.vram_h,g_geo.clut,g_geo.nclut); }
        g_racing=1; g_state=ST_RACE;
        if(idx>=0&&idx<TRACK_N) g_races_played[idx]++;
        SDL_Log("race init %s ncars=%d",dat,g_race.ncars);
    } else SDL_Log("race_init FAILED %s",dat);
}

// begin a championship season: zero points, start the first event in the sequence
static void champ_start(void){
    g_champ_active=1; g_champ_idx=0; g_champ_awarded=0;
    for(int i=0;i<RACE_MAX_CARS;i++) g_champ_points[i]=0;
    g_tsel=CHAMP_SEQ[0]; start_race(CHAMP_SEQ[0]);
}
// award points by finish order for the just-finished race (once)
static void champ_award(void){
    if(g_champ_awarded) return;
    int order[RACE_MAX_CARS]; int nc=race_rank(&g_race,order);
    for(int p=0;p<nc&&p<8;p++){ int car=order[p]; if(car>=0&&car<RACE_MAX_CARS) g_champ_points[car]+=CHAMP_PTS[p]; }
    g_champ_awarded=1;
}
// advance to the next championship event, or end the season
static void champ_next(void){
    if(g_champ_idx+1 < (int)(sizeof(CHAMP_SEQ)/sizeof(CHAMP_SEQ[0]))){
        g_champ_idx++; g_champ_awarded=0; g_tsel=CHAMP_SEQ[g_champ_idx]; start_race(CHAMP_SEQ[g_champ_idx]);
    } else { g_champ_active=0; g_racing=0; g_state=ST_MENU; }   // season complete
}

static int g_ticks=0, g_res_ticks=0;
static void frame(void){
    SDL_Event e;
    while(SDL_PollEvent(&e)){
        if(e.type==SDL_EVENT_KEY_DOWN){
            SDL_Scancode sc=e.key.scancode;
            if(g_state==ST_TITLE && (sc==SDL_SCANCODE_RETURN||sc==SDL_SCANCODE_SPACE)) g_state=ST_MENU;
            else if(g_state==ST_MENU){
                int N=MBTN_N+1;
                if(sc==SDL_SCANCODE_RIGHT||sc==SDL_SCANCODE_DOWN) g_sel=(g_sel+1)%N;
                else if(sc==SDL_SCANCODE_LEFT||sc==SDL_SCANCODE_UP) g_sel=(g_sel+N-1)%N;
                else if(sc==SDL_SCANCODE_RETURN||sc==SDL_SCANCODE_SPACE) g_state=ST_TRACKSEL;
            }
            else if(g_state==ST_STATS){
                if(sc==SDL_SCANCODE_DOWN) g_tsel=(g_tsel+1)%TRACK_N;
                else if(sc==SDL_SCANCODE_UP) g_tsel=(g_tsel+TRACK_N-1)%TRACK_N;
                else if(sc==SDL_SCANCODE_ESCAPE) g_state=ST_MENU;
            }
            else if(g_state==ST_TRACKSEL){
                if(sc==SDL_SCANCODE_DOWN) g_tsel=(g_tsel+1)%TRACK_N;
                else if(sc==SDL_SCANCODE_UP) g_tsel=(g_tsel+TRACK_N-1)%TRACK_N;
                else if(sc==SDL_SCANCODE_ESCAPE) g_state=ST_MENU;
                else if(sc==SDL_SCANCODE_RETURN||sc==SDL_SCANCODE_SPACE) start_race(g_tsel);
            }
            if(g_state==ST_RESULTS && (sc==SDL_SCANCODE_RETURN||sc==SDL_SCANCODE_SPACE||sc==SDL_SCANCODE_ESCAPE)){ g_racing=0; g_state=ST_MENU; }
            else if(g_state==ST_RACE && sc==SDL_SCANCODE_R){ g_state=ST_RESULTS; }   // show results
            else if(sc==SDL_SCANCODE_C){ g_raw^=1; }                                       // toggle raw (no DVD crunch) for comparison
            else if(sc==SDL_SCANCODE_N){ g_tsel=(g_tsel+1)%TRACK_N; start_race(g_tsel); }  // next track
        }
    }
    ++g_ticks;                                                // attract-mode auto-advance (for demo/screenshot)
    if(g_state==ST_TITLE && g_ticks>90) g_state=ST_MENU;
    else if(g_state==ST_MENU && g_ticks>240) g_state=ST_TRACKSEL;
    else if(g_state==ST_TRACKSEL && g_ticks>420) champ_start();   // demo runs the championship season
    if(g_state==ST_RACE && g_racing){
        race_step(&g_race, 1.f/60.f);
        Car* p=&g_race.cars[0]; float sp=fabsf(p->speed);
        audio_engine(SND_ENGINE, 0.55f + sp*0.018f, 0.22f);          // engine pitch by speed
        if(p->hits>g_last_hits) audio_oneshot(SND_CRASH, 0.7f);      // collision
        g_last_hits=p->hits;
        if(race_done(&g_race)){ if(g_champ_active) champ_award(); g_state=ST_RESULTS; g_res_ticks=0; }  // race over -> results
    } else { audio_engine(SND_ENGINE, 0.5f, 0.0f); g_last_hits=0; }  // silence engine in menus
    if(g_state==ST_RESULTS && g_champ_active && ++g_res_ticks>300) champ_next();   // demo: auto-advance season
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
    ui_init(); present_init(); render_init(); g_raw=em_raw_param();
    { unsigned char d[4]={0,0,0,170}; glGenTextures(1,&g_dim_tex); glBindTexture(GL_TEXTURE_2D,g_dim_tex);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,d);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); }
    { const int MW=128,MH=128; unsigned char* m=malloc((size_t)MW*MH*4); unsigned seed=2463534242u;  // scratched dark brown-metal backdrop (matches dd2h)
      for(int y=0;y<MH;y++){ seed=seed*1103515245u+12345u; float scr=((seed>>16)&0xff)/255.f;        // per-row horizontal scratch
        for(int x=0;x<MW;x++){ seed=seed*1103515245u+12345u; float n=((seed>>16)&0xff)/255.f;
          float base=0.21f+0.06f*scr+0.05f*(n-0.5f); base*=1.0f-0.22f*(y/(float)MH);                 // metal + scratch + grain, darker low
          unsigned char* px=m+((size_t)y*MW+x)*4;
          px[0]=(unsigned char)(base*255*1.08f<255?base*255*1.08f:255);   // warm/brown bias
          px[1]=(unsigned char)(base*255*0.96f); px[2]=(unsigned char)(base*255*0.82f); px[3]=255; } }
      glGenTextures(1,&g_metal_tex); glBindTexture(GL_TEXTURE_2D,g_metal_tex);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,MW,MH,0,GL_RGBA,GL_UNSIGNED_BYTE,m);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); free(m); }
    for(int i=0;i<8;i++){ unsigned char c[4]={(unsigned char)(CAR_COLS[i][0]*255),(unsigned char)(CAR_COLS[i][1]*255),(unsigned char)(CAR_COLS[i][2]*255),255};
      glGenTextures(1,&g_car_sw[i]); glBindTexture(GL_TEXTURE_2D,g_car_sw[i]);
      glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,c);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); }
    int tw,th; g_title_tex = ui_load_bmp("assets/raw/LEV0/COPYRIGH.BMP",&tw,&th);
    SDL_Log("title %dx%d tex=%u", tw, th, g_title_tex);
    if (vram_init("LEV0")) {
        for(int i=0;i<MENU_N;i++){
            g_mt_norm[i]=vram_text_tex(MENU[i],230,170,40,&g_mw[i],&g_mh[i]);  // ink: DD2 amber
            g_mt_sel[i] =vram_text_tex(MENU[i],255,255,255,NULL,NULL);          // selected: white
        }
        for(int i=0;i<TRACK_N;i++){
            g_tt_norm[i]=vram_text_tex(TRACKS[i],230,170,40,&g_tw[i],&g_th[i]);
            g_tt_sel[i] =vram_text_tex(TRACKS[i],255,255,255,NULL,NULL);
        }
        // radial menu sprites (real dd2h layout)
        g_logo_tex=vram_sprite_tex_pal("DD2L1",&g_logow,&g_logoh,57);   // pal57 = chrome wordmark (cy*4=32 renders washed)
        g_ring_tex=vram_sprite_tex("RING",&g_ringw,&g_ringh);
        g_go_tex  =vram_sprite_tex("GO",&g_gow,&g_goh);
        for(int i=0;i<MBTN_N;i++) g_icon_tex[i]=vram_sprite_tex(MBTN[i].icon,&g_iw[i],&g_ih[i]);
        SDL_Log("menu ready: %d items, %d tracks, radial=%d btns (ring=%u logo=%u)", MENU_N, TRACK_N, MBTN_N, g_ring_tex, g_logo_tex);
    }
    audio_load();
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame,0,1);
#else
    for(;;){frame();SDL_Delay(16);}
#endif
    return 0;
}
