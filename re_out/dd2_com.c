/* DirectDraw COM interface shim: real vtable objects so the decompiled DDraw init + render path runs.
   The decompile calls (**(code**)(*iface + off))(iface, args). We give each interface a vtable of
   matching-signature stubs (DD_OK=0); creation methods emit sub-interfaces; Lock yields a pixel buffer.
   (Real WebGL present is layered on later — this gets Init_Application past DDraw -> Play_Game's loop.) */

#include <stdio.h>
#include <stdlib.h>
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
static int ids_flip(int t,int a,int b){
    const char* dir = getenv("DD2_FRAMEDIR");
    if(dir){
        /* PRIMARY frame = _screenbuffer @0x700450, the engine's real 320x240 8-bit framebuffer
           (where ALL decompiled rasterizers draw; this is the faithful bit-exact comparison
           surface — identical buffer in reference dd2h.exe). */
        char nm[256]; sprintf(nm,"%s/f%05d.bin", dir, g_frameno);
        FILE* f=fopen(nm,"wb"); if(f){ fwrite((void*)(unsigned long)0x700450u,1,320*240,f); fclose(f); }
        /* secondary: the DDraw primary (HUD-only until the _screenbuffer->primary Blt is wired) */
        if(getenv("DD2_GPDUMP")){ sprintf(nm,"%s/gp%05d.bin", dir, g_frameno);
            FILE* g=fopen(nm,"wb"); if(g){ fwrite(g_pixels,1,640*512,g); fclose(g); } }
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
