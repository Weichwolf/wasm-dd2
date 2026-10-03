/* DirectDraw COM interface shim: real vtable objects so the decompiled DDraw init + render path runs.
   The decompile calls (**(code**)(*iface + off))(iface, args). We give each interface a vtable of
   matching-signature stubs (DD_OK=0); creation methods emit sub-interfaces; Lock yields a pixel buffer.
   (Real WebGL present is layered on later — this gets Init_Application past DDraw -> Play_Game's loop.) */

#include <stdio.h>
#include <stdlib.h>
#include "dd2_native.h"
/* Optional diagnostic output filter: calculate preceding demos normally but
 * save only the selected racing loop. Init_Game/fades retain quit=1 between
 * demos; accepted race frames begin after Play_Game resets it to zero. */
static unsigned dd2_race_capture_level(void){
    static int initialized;
    static unsigned selected;
    if(!initialized){
        const char* value=getenv("DD2_RACE_CAPTURE_LEVEL");initialized=1;
        if(value){
            char* end;unsigned long level=strtoul(value,&end,10);
            if(!*value || *end || level<1 || level>10){
                fprintf(stderr,"[race-stream] invalid capture level\n");exit(1);
            }
            selected=(unsigned)level;
        }
    }
    return selected;
}
static int dd2_race_capture_frame(void){
    unsigned level=dd2_race_capture_level();
    extern unsigned dd2_tick_replay_calls(void);
    return !level || (*(unsigned*)(unsigned long)0x936ff4u==level &&
        dd2_tick_replay_calls() && !*(int*)(unsigned long)0x7746acu);
}
static unsigned char g_pixels[640*512];   /* 8-bit indexed surface store (PSX-style) */

/* interface objects: a single word holding the vtable pointer (the decompile derefs *iface = vtable) */
static void* g_ddraw_vtbl[48];
static void* g_surf_vtbl[48];
static void* g_back_vtbl[48];
static void* g_pal_vtbl[48];
static void* g_ddraw_obj = g_ddraw_vtbl;
static void* g_surf_obj  = g_surf_vtbl;
static void* g_back_obj  = g_back_vtbl;
static void* g_pal_obj   = g_pal_vtbl;

/* generic DD_OK stubs by arg count (incl. `this`) */
static int ok1(int a){return 0;}
static int ok2(int a,int b){return 0;}
static int ok3(int a,int b,int c){return 0;}
static int ok4(int a,int b,int c,int d){return 0;}
static int ok5(int a,int b,int c,int d,int e){return 0;}

/* IDirectDraw::CreatePalette(this,caps,colortable,ppPalette) @0x14 — decompile calls with 4 args (no outer) */
extern unsigned char g_palette[256*4];
static int idd_createpal(int t,int caps,int ct,void** pp){
    if(ct){ int i; const unsigned char* e=(const unsigned char*)(unsigned long)ct;
        for(i=0;i<256*4;i++) g_palette[i]=e[i];
        const char* dir=getenv("DD2_FRAMEDIR");
        if(dir){ char nm[256]; sprintf(nm,"%s/palette.bin",dir);
            FILE* f=fopen(nm,"wb"); if(f){ fwrite(g_palette,1,256*4,f); fclose(f); } } }
    if(pp)*pp=&g_pal_obj; return 0; }
/* IDirectDraw::CreateSurface(this,desc,ppSurface,outer) @0x18 */
static int idd_createsurf(int t,int desc,void** pp,int o){ if(pp)*pp=&g_surf_obj; return 0; }
/* IDirectDrawSurface::GetAttachedSurface(this,caps,ppSurface) @0x30 */
static int ids_getattached(int t,int caps,void** pp){ if(pp)*pp=&g_back_obj; return 0; }
/* IDirectDrawSurface::GetSurfaceDesc(this,descPtr) @0x58 — fill height/width/pitch/lpSurface */
static int ids_getdesc(int t,int* desc){
    if(desc){ desc[2]=480; desc[3]=640; desc[4]=640; desc[9]=(int)(long)g_pixels; } return 0; }
/* IDirectDrawSurface::Lock(this,rect,descPtr,flags,event) @0x64 — fill desc->lpSurface(+0x24)+pitch */
static int ids_lock(int t,int rect,int* desc,int flags,int ev){
    if(desc){ desc[9]=(int)(long)g_pixels; desc[4]=640; } return 0; }

/* IDirectDrawSurface::Flip (@0x2c) = present. Hook it to capture the 8-bit indexed g_pixels frame
   (the bit-exactness comparison surface). Shared by native + WASM (NODERAWFS). */
#include <stdio.h>
#include <stdlib.h>
static int g_frameno = 0;
extern unsigned char g_palette[256*4];  /* defined below; captured DDraw palette (RGBA-ish per entry) */
#ifdef DD2_BROWSER
#include <emscripten.h>
/* Blit the engine's 640x480 8-bit indexed framebuffer (dd2h) (@0x700450) to the page <canvas> via the
   captured palette, then yield to the browser event loop (ASYNCIFY) so it can paint + deliver input.
   Guarded by DD2_BROWSER so the headless node build (build.sh) is completely unaffected. */
EM_JS(void, dd2_present, (const unsigned char* fb, const unsigned char* pal), {
    var c = Module.canvas || document.getElementById('canvas');
    if (!c) return;
    if (c.width !== 640) { c.width = 640; c.height = 480; }  /* dd2h framebuffer is 640x480 */
    var ctx = c.getContext('2d');
    if (!Module._dd2img) Module._dd2img = ctx.createImageData(640, 480);
    var img = Module._dd2img.data;
    for (var i = 0, p = 0; i < 640*480; i++, p += 4) {
        var idx = HEAPU8[fb + i] * 4;
        /* DDraw PALETTEENTRY is R,G,B,flags -> map to canvas RGBA */
        img[p]   = HEAPU8[pal + idx];
        img[p+1] = HEAPU8[pal + idx + 1];
        img[p+2] = HEAPU8[pal + idx + 2];
        img[p+3] = 255;
    }
    ctx.putImageData(Module._dd2img, 0, 0);
});
#endif
static int ids_flip(int t,int a,int b){
    /* Advance the shared audio clock before CD playback/timer status checks. */
    { extern void dd2_snd_mix_flip(void); dd2_snd_mix_flip(); }
    { extern void dd2_cd_pump(void); extern void dd2_mmtimer_poll(void);
      dd2_cd_pump(); dd2_mmtimer_poll(); }
    /* DD2_SCRIPT=<file>: deterministic scripted input. Lines "<cf> <vk> <down>" (decimal/0x..),
       sorted by cf; when the ENGINE frame counter @0x462ff0 reaches that value the key event is
       injected through the same dd2_key_event -> Translate_Keypress path a real key takes.
       cf-based (not flip-based): the presentation flip count differs across targets/paths, the
       engine counter is target-invariant -> scripted play is bit-reproducible native<->wasm. */
    { static FILE* sf; static int sinit, snext=-1, sflip; static unsigned svk; static int sdown;
      int _cf = *(int*)(unsigned long)0x462ff0u;
      /* line forms: "<cf> <vk> <down>" (engine-frame keyed, bit-reproducible across targets)
         or "f<flip> <vk> <down>" (presented-frame keyed -- for the front end, where the engine
         frame counter stays 0; flip counts are only per-target-reproducible). */
      if(!sinit){ sinit=1; const char* p=getenv("DD2_SCRIPT");
          if(p){ sf=fopen(p,"r"); } }
      if(sf && snext<0){ char pfx=0; long v;
          if(fscanf(sf," %c",&pfx)==1){
              if(pfx=='f'){ sflip=1; } else { sflip=0; ungetc(pfx,sf); }
              if(fscanf(sf,"%li %i %i",&v,&svk,&sdown)==3) snext=(int)v; } }
      while(sf && snext>=0 && (sflip ? g_frameno : _cf) >= snext){
          extern void dd2_key_event(unsigned int, int);
          dd2_key_event(svk, sdown);
          snext=-1;
          { char pfx=0; long v;
            if(fscanf(sf," %c",&pfx)==1){
                if(pfx=='f'){ sflip=1; } else { sflip=0; ungetc(pfx,sf); }
                if(fscanf(sf,"%li %i %i",&v,&svk,&sdown)==3) snext=(int)v; } } }
    /* DD2_PADSCRIPT=<file>: deterministic gamepad input, lines "<cf> <x> <y> <buttons>"
       (x/y 0..65535, center 32768) -> dd2_pad_update, polled by the engine's joyGetPos. */
      { static FILE* pfs; static int pinit, pnext=-1; static unsigned ppx,ppy,ppb;
        if(!pinit){ pinit=1; const char* p=getenv("DD2_PADSCRIPT");
            if(p){ pfs=fopen(p,"r");
                if(pfs && fscanf(pfs,"%i %i %i %i",&pnext,&ppx,&ppy,&ppb)!=4) pnext=-1; } }
        while(pfs && pnext>=0 && _cf>=pnext){
            extern void dd2_pad_update(int,unsigned,unsigned,unsigned);
            dd2_pad_update(1, ppx, ppy, ppb);
            if(fscanf(pfs,"%i %i %i %i",&pnext,&ppx,&ppy,&ppb)!=4) pnext=-1; } } }
#ifdef DD2_BROWSER
    dd2_present((const unsigned char*)(unsigned long)0x700450u, g_palette);
    emscripten_sleep(0);   /* yield each presented frame so the browser paints + processes key events */
#endif
#ifdef DD2_NATIVE_SDL
    dd2_native_present((const unsigned char*)(unsigned long)0x700450u,g_palette);
#endif
    /* Complete race-loop comparison against the real original, with its
       observed GetTickCount inputs replayed in the headless port. Init_Game
       presentations precede the first clock read; post-race fades have quit=1.
       Each accepted record identifies an actual pending presentation. */
    {
        const char* logpath=getenv("DD2_RACE_STREAM");
        if(logpath && (!dd2_race_capture_level() ||
                *(unsigned*)(unsigned long)0x936ff4u==dd2_race_capture_level())){
            extern unsigned dd2_tick_replay_calls(void),dd2_random_replay_calls(void);
            static int race_started;
            unsigned calls=dd2_tick_replay_calls();
            if(calls && *(int*)(unsigned long)0x7746acu && getenv("DD2_RACE_STOP_AFTER_CAPTURE") &&
                    (!dd2_race_capture_level() || race_started)){
                fprintf(stderr,"[race-stream] target racing loop finished\n");exit(0);
            }
            if(calls && !*(int*)(unsigned long)0x7746acu){
                race_started=1;
                static FILE* log;
                if(!log){log=fopen(logpath,"w");if(!log){fprintf(stderr,"Cannot open race stream log\n");exit(1);}}
                fprintf(log,"{\"flip\":%d,\"level\":%d,\"cf\":%d,\"ticks\":%d,\"countdown\":%d,\"frame_skip\":%d,\"quit\":%d,\"clock_calls\":%u,\"rng_calls\":%u,\"demo_flash\":%d}\n",
                    g_frameno,*(int*)(unsigned long)0x936ff4u,*(int*)(unsigned long)0x462ff0u,
                    *(int*)(unsigned long)0x7746c0u,*(int*)(unsigned long)0x784298u,
                    *(int*)(unsigned long)0x7746b8u,*(int*)(unsigned long)0x7746acu,calls,dd2_random_replay_calls(),*(int*)(unsigned long)0x4652a0u);fflush(log);
            }
        }
    }
    const char* dir = getenv("DD2_FRAMEDIR");
    if(dir && dd2_race_capture_frame()){
        /* PRIMARY frame = _screenbuffer @0x700450, the engine's real 640x480 8-bit framebuffer (dd2h)
           (where ALL decompiled rasterizers draw; this is the faithful bit-exact comparison
           surface — identical buffer in reference dd2h.exe). */
        /* DD2_CFONLY=1: suppress the per-flip f%05d dumps (27GB+ for a full FE cycle) and keep
           only the cf-keyed overwriting dumps below -- compact (demo,cf) spot compares. */
        char nm[256];
        if(!getenv("DD2_CFONLY")){
            sprintf(nm,"%s/f%05d.bin", dir, g_frameno);
            FILE* f=fopen(nm,"wb"); if(f){ fwrite((void*)(unsigned long)0x700450u,1,640*480,f); fclose(f); }
            /* A changing palette is part of the presented video frame. A single
               final palette.bin cannot verify fades or menu color changes. */
            if(getenv("DD2_PALDUMP")){
                sprintf(nm,"%s/f%05d.pal", dir, g_frameno);
                f=fopen(nm,"wb"); if(f){ fwrite(g_palette,1,256*4,f); fclose(f); } }
        }
        /* current_frame-keyed dump for ref alignment: name by engine frame counter @0x462ff0
           (same counter tools/refcap.c polls in the reference) so frames line up across builds. */
        if(getenv("DD2_CFDUMP")){ int cf=*(int*)(unsigned long)0x462ff0u;
            char nm2[256]; sprintf(nm2,"%s/cf%05d.bin", dir, cf);
            FILE* g2=fopen(nm2,"wb"); if(g2){ fwrite((void*)(unsigned long)0x700450u,1,640*480,g2); fclose(g2); } }
        /* DD2_FLIPLOG: append the engine frame counter @0x462ff0 once per flip -- flip-per-cf
           phase comparison across targets (cf-keyed fb dumps only line up if BOTH targets flip
           the same number of times before each cf increment). */
        if(getenv("DD2_FLIPLOG")){ static FILE* fl; char nm4[256];
            if(!fl){ sprintf(nm4,"%s/fliplog.txt", dir); fl=fopen(nm4,"w"); }
            if(fl){ extern unsigned g_rand_calls;
                fprintf(fl,"%d %u\n",*(int*)(unsigned long)0x462ff0u,g_rand_calls); fflush(fl); } }
        /* DD2_IMGDUMP=<cf-list "a,b,c">: full memory image @0x400000 (0x580400 bytes) per listed cf,
           written once per cf at flip time -- platform-neutral state-divergence bisect (nat vs wasm). */
        { const char* il=getenv("DD2_IMGDUMP");
          if(il){ int cf=*(int*)(unsigned long)0x462ff0u; static int lastcf=-1, flipno=0;
            if(cf!=lastcf){ lastcf=cf; flipno=0; } else flipno++;
            { int m=0; const char* p=il;
              while(*p){ int v=0,seen=0; while(*p>='0'&&*p<='9'){v=v*10+(*p-'0');p++;seen=1;}
                if(seen&&v==cf) m=1; while(*p&&*p!=','&&(*p<'0'||*p>'9')) p++; if(*p==',')p++; }
              if(m){ char nm3[256];
                /* cf resets at the green light. Per-presentation names retain
                   both occurrences for complete original race diagnostics. */
                if(getenv("DD2_IMGDUMP_FLIP"))sprintf(nm3,"%s/imagef%05d.bin",dir,g_frameno);
                else sprintf(nm3,"%s/img%05d_%d.bin", dir, cf, flipno);
                FILE* h=fopen(nm3,"wb"); if(h){ fwrite((void*)(unsigned long)0x400000u,1,0x580400,h); fclose(h);} } } } }
        /* secondary: the DDraw primary (HUD-only until the _screenbuffer->primary Blt is wired) */
        if(getenv("DD2_GPDUMP")){ sprintf(nm,"%s/gp%05d.bin", dir, g_frameno);
            FILE* g=fopen(nm,"wb"); if(g){ fwrite(g_pixels,1,640*512,g); fclose(g); } }
    }
    if(getenv("DD2_INPROBE") && g_frameno<25){
        unsigned short c04a=*(unsigned short*)(unsigned long)0x71c04au;
        unsigned short c048=*(unsigned short*)(unsigned long)0x71c048u;
        unsigned short prev=*(unsigned short*)(unsigned long)0x463050u;
        unsigned char mode=*(unsigned char*)(unsigned long)0x46303eu;
        int dmode=*(int*)(unsigned long)0x46385cu;
        int ncars=*(int*)(unsigned long)0x46765cu;
        int curfr=*(int*)(unsigned long)0x462ff0u;
        /* dual-symbol fix: image slots */
#define _quit_flag (*(int*)0x7746ac)
#define _race_finished (*(int*)0x795df4)
#define _Replay_Script_Ptr (*(void**)0x9392b4)
        unsigned rsp=(unsigned)(unsigned long)_Replay_Script_Ptr;
        unsigned rs=0x8ff2b0u, ft=0x900eb0u;
        unsigned ec4=*(unsigned*)(unsigned long)0x900ec4u;
        int r74=*(int*)(unsigned long)0x467074u;
        unsigned char* pf=(unsigned char*)(unsigned long)0x46304bu; /* rup,rdown,rleft,DAT4e */
        fprintf(stderr,"[inprobe f%d] demo=%d quit=%d yq=%d rf=%d curfr=%d ticks=%d c048=0x%04x pad444a=0x%04x mask3050=0x%04x\n",
                g_frameno,dmode,_quit_flag,*(int*)0x9376acu,_race_finished,curfr,*(int*)0x7746c0u,c048,*(unsigned short*)0x75444au,prev);
    }
    if(getenv("DD2_OTCHECK")){
        /* OT integrity scan: walk both OTs; any entry/link outside image = the wild-splice bug */
        int _cdbs[2]={0x754264,0x7542f2}; int _b;
        int otsz=*(int*)(unsigned long)0x754260u; int i;
        for(_b=0;_b<2;_b++){
        unsigned ot=*(unsigned*)(unsigned long)(_cdbs[_b]+0x8a);
        if(ot<0x400000u||ot>=0x980400u) continue;
        for(i=0;i<otsz;i++){
            unsigned v=*(unsigned*)(unsigned long)(ot+i*4u);
            if(v && v!=0xffffffffu && (v<0x400000u||v>=0x980400u)){
                fprintf(stderr,"[otchk f%d] BAD slot %d @0x%x = 0x%x\n",g_frameno,i,ot+i*4u,v);
                break;
            }
            { unsigned p=v; int d=0; unsigned prev=ot+i*4u;
              while(p && p!=0xffffffffu && d<4096){
                if(p<0x400000u||p>=0x980400u){
                    fprintf(stderr,"[otchk f%d] BAD LINK depth %d slot %d: 0x%x -> 0x%x\n",g_frameno,d,i,prev,p);
                    i=otsz; break; }
                prev=p; p=*(unsigned*)(unsigned long)p; d++; }
            }
        }
        }
    }
    if(getenv("DD2_RANDTRACE") && g_frameno<20){
        extern unsigned g_rand_calls;
        fprintf(stderr,"[frame %d] rand_calls=%u\n", g_frameno, g_rand_calls);
    }
    if(getenv("DD2_STATECF")){   /* one-shot full-state dump keyed by ENGINE frame 0x462ff0 (aligns native/wasm/ref) */
        static int done=0; int want=atoi(getenv("DD2_STATECF"));
        if(!done && *(int*)(unsigned long)0x462ff0u==want){ const char* dir=getenv("DD2_FRAMEDIR");
            if(dir){ char nm[256]; sprintf(nm,"%s/statecf%05d.bin",dir,want);
                FILE* f=fopen(nm,"wb"); if(f){ fwrite((void*)(unsigned long)0x400000u,1,0x500000,f); fclose(f); done=1; } } } }
    if(getenv("DD2_STATEDUMP")){
        int want = atoi(getenv("DD2_STATEDUMP"));
        if(g_frameno==want){ const char* dir=getenv("DD2_FRAMEDIR");
            if(dir){ char nm[256]; sprintf(nm,"%s/state%05d.bin",dir,g_frameno);
                FILE* f=fopen(nm,"wb"); if(f){ fwrite((void*)(unsigned long)0x400000u,1,0x500000,f); fclose(f); } } }
    }
    if(getenv("DD2_RFLOG")){
        int rf=*(int*)(unsigned long)0x75d9f4u;      /* race_finished */
        int ac=*(int*)(unsigned long)0x466e38u;      /* DAT_00466e38 active-car count */
        int qf=*(int*)(unsigned long)0x73c2acu;      /* quit_flag */
        static int last_rf=-99;
        if(rf!=last_rf){ fprintf(stderr,"[RFLOG] frame=%d race_finished=%d active_cars=%d quit=%d\n", g_frameno, rf, ac, qf); last_rf=rf; }
    }
    g_frameno++;
    return 0;
}
int dd2_frame_count(void){ return g_frameno; }

/* IDirectDrawPalette::SetEntries(this,flags,start,count,lpEntries) — capture the active 256-color
   palette so frames can be rendered/compared in true color. lpEntries = PALETTEENTRY[count] (RGBA bytes). */
unsigned char g_palette[256*4];
static int ids_setentries(int t,int flags,int start,int count,const unsigned char* ent){
    int i; if(ent && start>=0 && start+count<=256)
        for(i=0;i<count*4;i++) g_palette[start*4+i]=ent[i];
    if(getenv("DD2_FRAMEDIR")){ char nm[256]; sprintf(nm,"%s/palette.bin",getenv("DD2_FRAMEDIR"));
        FILE* f=fopen(nm,"wb"); if(f){ fwrite(g_palette,1,256*4,f); fclose(f); } }
    return 0;
}

void dd2_com_init(void){
    int i;
    for(i=0;i<48;i++){ g_ddraw_vtbl[i]=(void*)&ok1; g_surf_vtbl[i]=(void*)&ok1; g_back_vtbl[i]=(void*)&ok1; g_pal_vtbl[i]=(void*)&ok1; }
    g_ddraw_vtbl[0x14/4]=(void*)&idd_createpal;
    g_ddraw_vtbl[0x18/4]=(void*)&idd_createsurf;
    g_ddraw_vtbl[0x50/4]=(void*)&ok3;            /* SetCooperativeLevel(this,hwnd,flags) */
    g_ddraw_vtbl[0x54/4]=(void*)&ok4;            /* SetDisplayMode(this,w,h,bpp) */
    g_surf_vtbl[0x2c/4]=(void*)&ids_flip; g_surf_vtbl[0x30/4]=(void*)&ids_getattached;
    g_surf_vtbl[0x6c/4]=(void*)&ok1; g_surf_vtbl[0x7c/4]=(void*)&ok2;
    g_back_vtbl[0x58/4]=(void*)&ids_getdesc; g_back_vtbl[0x64/4]=(void*)&ids_lock; g_back_vtbl[0x6c/4]=(void*)&ok1; g_back_vtbl[0x80/4]=(void*)&ok2;
    g_surf_vtbl[0x58/4]=(void*)&ids_getdesc; g_surf_vtbl[0x64/4]=(void*)&ids_lock;
    g_pal_vtbl[0x08/4]=(void*)&ok1;  g_pal_vtbl[0x18/4]=(void*)&ids_setentries;   /* SetEntries(this,flags,start,count,entries) */
}

/* DirectDrawCreate(guid, ppDD, outer) -> emit our IDirectDraw */
int DirectDrawCreate(int guid, void** ppDD, int outer){ if(ppDD)*ppDD=&g_ddraw_obj; return 0; }

#include <stdarg.h>
#include <stdio.h>
/* MSVC sprintf wrapper, reimplemented with real varargs (decompiled version read x86-stack args) */
int FUN_0045672e(char* buf, const char* fmt, ...){
    va_list ap; va_start(ap, fmt); int n = vsprintf(buf, fmt, ap); va_end(ap); return n;
}
