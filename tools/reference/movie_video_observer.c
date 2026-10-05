/* Read-only observation of Wine 10 MCIAVI's ICDecompress/StretchDIBits route.
 * Original API arguments go unchanged to unchanged Wine backend sections.
 * Only private allocations and a readback DC are written. Observation I/O
 * changes timing; no physical presentation clock parity is claimed. */
#include "winmm_timer_observer.h"
typedef unsigned char U8;
typedef struct {U32 size;int width,height;unsigned short planes,bits;
    U32 compression,image_size;int xppm,yppm;U32 used,important;} Bitmap;
typedef struct {int left,top,right,bottom;} Rect;
IMPORT int API CloseHandle(HANDLE);
IMPORT int API MoveFileA(const char*,const char*);
IMPORT HANDLE API CreateCompatibleDC(HANDLE);
IMPORT HANDLE API CreateDIBSection(HANDLE,const Bitmap*,U32,void**,HANDLE,U32);
IMPORT HANDLE API SelectObject(HANDLE,HANDLE);
IMPORT int API BitBlt(HANDLE,int,int,int,int,HANDLE,int,int,U32);
IMPORT int API GdiFlush(void);
IMPORT HANDLE API WindowFromDC(HANDLE);
IMPORT int API GetClientRect(HANDLE,Rect*);
static HANDLE backend,mutex;
static U32 decode_serial,packet_size;
static U8 packet[32768];
static void fail(U32 code){ExitProcess(code);}
static void enter(void){if(WaitForSingleObject(mutex,0xffffffffu))fail(234);}
static void leave(void){if(!ReleaseMutex(mutex))fail(234);}
#ifndef DD2_MOVIE_GDI_OBSERVER
static void initialize(void){
    if(backend)return;
    backend=LoadLibraryA("_msvfw32_real.dll");if(!backend)fail(231);
    mutex=CreateMutexA(0,0,0);if(!mutex)fail(234);
}
/* Private observer-to-observer copy API; never imported by the game.
 * cdecl, as is VFWAPIV ICDecompress (unlike WINAPI StretchDIBits). */
void DD2MoviePacketCopy(U32 *serial,U32 *size,U8 *bytes){
    U32 i;initialize();enter();*serial=decode_serial;*size=packet_size;
    for(i=0;i<sizeof(packet);i++)bytes[i]=packet[i];leave();
}
int ICDecompress(HANDLE codec,U32 flags,Bitmap *input,const void *compressed,
                 Bitmap *output,void *decoded){
    typedef int(*Call)(HANDLE,U32,Bitmap*,const void*,Bitmap*,void*);
    int result;U32 i;
    initialize();enter();
    result=((Call)GetProcAddress(backend,"ICDecompress"))(codec,flags,input,compressed,output,decoded);
    if(!result){
        if(!input||!output||input->width!=320||input->height!=192||
           input->compression!=0x64697663u||output->width!=320||output->height!=192||
           output->bits!=32||output->compression||!input->image_size||input->image_size>sizeof(packet))fail(236);
        packet_size=input->image_size;
        for(i=0;i<sizeof(packet);i++)packet[i]=i<packet_size?((const U8*)compressed)[i]:0;
        decode_serial++;
    }
    leave();return result;
}
#else
static HANDLE dc,bitmap;
static U32 *pixels,serial,maximum=60000,private_draws;
static U8 source[320*192*3];
static char root[1024],part[1100],closed[1100];
static void initialize(void){
    if(backend)return;
    backend=LoadLibraryA("_gdi32_real.dll");if(!backend)fail(231);
    mutex=CreateMutexA(0,0,0);if(!mutex)fail(234);
}
static void write_bytes(HANDLE file,const void *data,U32 bytes){
    U32 written;if(!WriteFile(file,data,bytes,&written,0)||written!=bytes)fail(235);
}
static void filename(void){
    char *p=part,*q=closed;const char *s=root;U32 n=serial,d=100000;
    while(*s){*p++=*s;*q++=*s++;}*p++='.';*q++='.';
    while(d){*p++=*q++=(char)('0'+n/d);n%=d;d/=10;}
    s=".part";while(*s)*p++=*s++;*p=0;
    s=".raw";while(*s)*q++=*s++;*q=0;
}
int API StretchDIBits(HANDLE target,int x,int y,int width,int height,
                     int sx,int sy,int sw,int sh,const void *bits,const Bitmap *format,
                     U32 usage,U32 operation){
    typedef int(API *Call)(HANDLE,int,int,int,int,int,int,int,int,const void*,const Bitmap*,U32,U32);
    typedef void(*Copy)(U32*,U32*,U8*);
    int result;U32 header[32]={0},i,j;HANDLE window,file,peer;Rect rect;Bitmap info={0};
    initialize();
    result=((Call)GetProcAddress(backend,"StretchDIBits"))(target,x,y,width,height,sx,sy,sw,sh,bits,format,usage,operation);
    /* Wine setup processes forward unchanged and never read game memory. */
    if(GetModuleHandleA("dd2h.exe")!=(HANDLE)0x400000||result<=0)return result;
    /* DirectDraw setup also uses GDI. Only the supported AVI output format
     * belongs to this observer; unrelated setup blits remain unobserved. */
    if(!format||format->width!=320||format->height!=192||format->bits!=32||format->compression)return result;
    window=WindowFromDC(target);
    /* MCI_OPEN's internal window is separate from PC-DD2. */
    if(!window||window!=(HANDLE)*(volatile U32*)0x46047c){private_draws++;return result;}
    if(serial>=maximum)fail(233);
    enter();
    if(!root[0]&&!GetEnvironmentVariableA("DD2_MOVIE_VIDEO_CAPTURE",root,sizeof(root)))fail(232);
    if(!window||!GetClientRect(window,&rect)||rect.left||rect.top||rect.right!=640||rect.bottom!=480)fail(237);
    if(!format||format->width!=320||format->height!=192||format->bits!=32||format->compression||
       sx||sy||sw!=320||sh!=192||usage||operation!=0x00cc0020u)fail(238);
    peer=GetModuleHandleA("msvfw32.dll");
    {
        Copy copy=peer?(Copy)GetProcAddress(peer,"DD2MoviePacketCopy"):0;
        if(!copy)fail(243);copy(&decode_serial,&packet_size,packet);
    }
    if(!decode_serial||!packet_size)fail(244);
    if(!dc){
        dc=CreateCompatibleDC(target);info.size=40;info.width=640;info.height=-480;
        info.planes=1;info.bits=32;
        bitmap=CreateDIBSection(target,&info,0,(void**)&pixels,0,0);
        if(!dc||!bitmap||!pixels||!SelectObject(dc,bitmap))fail(239);
    }
    if(!BitBlt(dc,0,0,640,480,target,0,0,0x00cc0020u)||!GdiFlush())fail(240);
    /* BI_RGB's fourth byte is reserved, not alpha. Normalize only that byte
     * to the ports' opaque convention; every displayed RGB byte is retained. */
    for(i=0;i<640*480;i++)pixels[i]|=0xff000000u;
    for(i=0;i<192;i++)for(j=0;j<320;j++){
        const U8 *p=(const U8*)bits+((191-i)*320+j)*4;
        U8 *q=source+(i*320+j)*3;q[0]=p[2];q[1]=p[1];q[2]=p[0];
    }
    header[0]=0x4d324444u;header[1]=1;header[2]=serial;header[3]=decode_serial;
    header[4]=packet_size;header[5]=(U32)x;header[6]=(U32)y;header[7]=(U32)width;header[8]=(U32)height;
    header[9]=320;header[10]=192;header[11]=32;header[12]=640;header[13]=480;
    header[14]=usage;header[15]=(U32)result;header[16]=GetCurrentThreadId();
    header[17]=GetTickCount();header[18]=(U32)window;header[19]=private_draws;
    filename();file=CreateFileA(part,0x40000000u,3,0,1,0x80,0);
    if(file==(HANDLE)-1)fail(241);
    write_bytes(file,header,sizeof(header));write_bytes(file,packet,sizeof(packet));
    write_bytes(file,source,sizeof(source));write_bytes(file,pixels,640*480*4);
    if(!CloseHandle(file)||!MoveFileA(part,closed))fail(242);
    serial++;leave();return result;
}
#endif
int API DllMain(HANDLE module,U32 reason,void *reserved){
    (void)reserved;if(reason==1)DisableThreadLibraryCalls(module);return 1;
}
