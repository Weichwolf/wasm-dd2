/* Storage-only DirectDraw probe. Synthetic allocated engine-shaped memory is
 * used solely by this executable; this is never injected into dd2h.exe. */
#define COBJMACROS
#include <windows.h>
#include <ddraw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Reserve these addresses as this probe's own zero-initialized PE data.
 * Reserving them in main would race Wine/CRT heap allocations during startup. */
volatile unsigned char engine_space[0x600000] __attribute__((used));

static void check(HRESULT result,const char *where){
    if(FAILED(result)){fprintf(stderr,"%s: %08lx\n",where,(unsigned long)result);exit(2);}
}
static void region(unsigned address,unsigned bytes){
    unsigned first=(unsigned)engine_space,last=first+sizeof(engine_space);
    if(address<first || address+bytes>last){fprintf(stderr,"data region outside probe BSS\n");exit(3);}
}
int main(int argc,char **argv){
    IDirectDraw *draw;IDirectDrawSurface *primary,*back;IDirectDrawPalette *pal;
    DDSURFACEDESC desc;DDSCAPS caps;WNDCLASSA klass={0};HWND window;
    PALETTEENTRY *entries=(PALETTEENTRY*)0x700050;
    unsigned char *pixels=(unsigned char*)0x700450;
    unsigned frame,x,y,frames=argc>1?(unsigned)atoi(argv[1]):65;
    region(0x460000,0x10000);region(0x700000,0x50000);
    region(0x770000,0x10000);region(0x930000,0x20000);
    klass.lpfnWndProc=DefWindowProcA;klass.hInstance=GetModuleHandleA(0);klass.lpszClassName="DD2VideoStorage";
    if(!RegisterClassA(&klass))return 4;
    window=CreateWindowA(klass.lpszClassName,"Video storage probe",WS_POPUP,0,0,640,480,0,0,klass.hInstance,0);
    if(!window)return 5;
    ShowWindow(window,SW_SHOW);SetForegroundWindow(window);
    check(DirectDrawCreate(0,&draw,0),"DirectDrawCreate");
    check(IDirectDraw_SetCooperativeLevel(draw,window,DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN),"SetCooperativeLevel");
    check(IDirectDraw_SetDisplayMode(draw,640,480,8),"SetDisplayMode");
    memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);desc.dwFlags=DDSD_CAPS|DDSD_BACKBUFFERCOUNT;
    desc.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE|DDSCAPS_COMPLEX|DDSCAPS_FLIP;desc.dwBackBufferCount=1;
    check(IDirectDraw_CreateSurface(draw,&desc,&primary,0),"CreateSurface");
    caps.dwCaps=DDSCAPS_BACKBUFFER;
    check(IDirectDrawSurface_GetAttachedSurface(primary,&caps,&back),"GetAttachedSurface");
    memset(entries,0,1024);
    check(IDirectDraw_CreatePalette(draw,DDPCAPS_8BIT|DDPCAPS_ALLOW256,entries,&pal,0),"CreatePalette");
    check(IDirectDrawSurface_SetPalette(primary,pal),"SetPalette");
    for(frame=0;frame<frames;frame++){
        MSG message;while(PeekMessageA(&message,0,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageA(&message);}
        for(x=0;x<256;x++){
            entries[x].peRed=(BYTE)(x+frame);entries[x].peGreen=(BYTE)(x*3+frame);
            entries[x].peBlue=(BYTE)(x*7+frame);entries[x].peFlags=0;
        }
        check(IDirectDrawPalette_SetEntries(pal,0,0,256,entries),"SetEntries");
        for(y=0;y<480;y++)for(x=0;x<640;x++)pixels[y*640+x]=(BYTE)(x/16+y/16+frame);
        *(unsigned*)0x462ff0=frame;*(unsigned*)0x7746c0=frame*2;
        memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);
        check(IDirectDrawSurface_Lock(back,0,&desc,DDLOCK_WAIT,0),"Lock");
        for(y=0;y<480;y++)memcpy((char*)desc.lpSurface+y*desc.lPitch,pixels+y*640,640);
        check(IDirectDrawSurface_Unlock(back,desc.lpSurface),"Unlock");
        check(IDirectDrawSurface_Flip(primary,0,DDFLIP_WAIT),"Flip");
    }
    IDirectDrawPalette_Release(pal);IDirectDrawSurface_Release(back);
    IDirectDrawSurface_Release(primary);IDirectDraw_Release(draw);DestroyWindow(window);
    printf("{\"frames\":%u}\n",frames);return 0;
}
