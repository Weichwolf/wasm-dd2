/* Reference-only DirectDraw1 observation. The original game remains unchanged.
 * COM wrappers forward the supported game's calls to unchanged Wine objects.
 * Only the observer's allocations are written. Full indexed uploads and both
 * engine/device palettes are saved after successful primary Flip returns.
 * File I/O and API observation change timing; this is comparison evidence,
 * not physical-display or whole-game parity. */
#include "winmm_timer_observer.h"
typedef unsigned char U8;
typedef struct Object {void **table;} Object;
typedef struct Wrapper {
    void **table; Object *real; struct Wrapper *next,*back;
    U8 *pixels,*locked; U32 width,height,pitch,bits,ready,alive;
} Wrapper;
typedef int(API *Call1)(Object*,void*);
typedef int(API *Call2)(Object*,void*,void*);
typedef int(API *Call3)(Object*,void*,void*,void*);
typedef int(API *Call4)(Object*,void*,void*,void*,void*);
static void *draw_table[23],*surface_table[36];
static Wrapper *objects;
static HANDLE backend,output;
static U32 serial,attempts;
static U8 palette[1024];
static int initialized;
static U32 observe_rng;
static U32 chunk_frames,chunk_number,chunk_count,maximum_frames=4096;
static char capture_path[1024],part_path[1100],closed_path[1100];
IMPORT int API CloseHandle(HANDLE);
IMPORT int API MoveFileA(const char*,const char*);
static void fail(U32 code){ExitProcess(code);}
static void write_bytes(const void *data,U32 count){
    U32 written;if(!WriteFile(output,data,count,&written,0) || written!=count)fail(141);
}
static char *decimal(char *p,U32 n){
    char digits[10];U32 count=0;
    do{digits[count++]=(char)('0'+n%10);n/=10;}while(n);
    while(count)*p++=digits[--count];return p;
}
static U32 option(const char *name,U32 fallback,U32 limit){
    char value[16];U32 i,n=0,length=GetEnvironmentVariableA(name,value,sizeof(value));
    if(!length)return fallback;
    if(length>=sizeof(value))fail(155);
    for(i=0;i<length;i++){
        if(value[i]<'0' || value[i]>'9' || n>limit/10)fail(155);
        n=n*10+(U32)(value[i]-'0');if(n>limit)fail(155);
    }
    if(!n)fail(155);return n;
}
static U32 random_seed(void){
    static const U8 getter[6]={0xa1,0x7c,0x09,0x94,0x00,0xc3};
    U32 i,base;
    /* Verified original Watcom getter: mov eax,[0x94097c]; ret.
     * rand adds 12 to this pointer. Never call the getter or advance rand. */
    if(*(volatile U32*)0x46c32c!=0x4571fb)fail(158);
    for(i=0;i<6;i++)if(((volatile U8*)0x4571fb)[i]!=getter[i])fail(158);
    base=*(volatile U32*)0x94097c;
    if(!base || (base&3) || base>0xfffffff0u)fail(158);
    return *(volatile U32*)(base+12);
}
static void open_chunk(void){
    char *p=part_path,*q=closed_path;const char *s=capture_path;
    U32 divisor=100000,n=chunk_number;
    if(chunk_number>999999)fail(155);
    while(*s){*p++=*s;*q++=*s++;}*p++='.';*q++='.';
    while(divisor){*p++=*q++=(char)('0'+n/divisor);n%=divisor;divisor/=10;}
    s=".part";while(*s)*p++=*s++;*p=0;
    s=".raw";while(*s)*q++=*s++;*q=0;
    output=CreateFileA(part_path,0x40000000u,3,0,1,0x80,0);
    if(output==(HANDLE)-1)fail(153);
}
static void close_chunk(void){
    if(!output)return;
    if(!CloseHandle(output))fail(156);output=0;
    /* Rename only after closing: the host may then archive and remove it. */
    if(!MoveFileA(part_path,closed_path))fail(157);
    chunk_number++;chunk_count=0;
}
static Wrapper *wrap(Object *real,void **table){
    Wrapper *w;
    for(w=objects;w;w=w->next)if(w->alive && w->real==real && w->table==table)return w;
    w=HeapAlloc(GetProcessHeap(),8,sizeof(*w));if(!w)fail(142);
    w->table=table;w->real=real;w->alive=1;w->next=objects;objects=w;return w;
}
static Object *unwrap(void *object){
    Wrapper *w;
    if(!object)return 0;
    for(w=objects;w;w=w->next)if(w==(Wrapper*)object){if(!w->alive)fail(143);return w->real;}
    return (Object*)object;
}
static U32 API release(Wrapper *w){
    typedef U32(API *Release)(Object*);
    U32 result=((Release)w->real->table[2])(w->real);
    if(!result)w->alive=0;
    return result;
}
static int API create_surface(Wrapper *w,void *desc,Object **result,void *outer){
    int hr=((Call3)w->real->table[6])(w->real,desc,result,outer);
    if(!hr && result && *result)*result=(Object*)wrap(*result,surface_table);
    return hr;
}
static int API attached(Wrapper *w,void *caps,Object **result){
    int hr=((Call2)w->real->table[12])(w->real,caps,result);
    if(!hr && result && *result){
        Wrapper *back=wrap(*result,surface_table);*result=(Object*)back;
        if(*(U32*)caps==4)w->back=back;
    }
    return hr;
}
static int API lock_surface(Wrapper *w,void *rect,U32 *desc,void *flags,void *event){
    int hr=((Call4)w->real->table[25])(w->real,rect,desc,flags,event);
    if(!hr){
        if(rect || desc[0]!=108 || desc[2]!=480 || desc[3]!=640 || desc[4]<640 || desc[21]!=8)fail(144);
        w->height=desc[2];w->width=desc[3];w->pitch=desc[4];w->bits=desc[21];w->locked=(U8*)desc[9];
        if(!w->pixels)w->pixels=HeapAlloc(GetProcessHeap(),0,307200);
        if(!w->pixels || !w->locked)fail(145);
    }
    return hr;
}
static int API unlock_surface(Wrapper *w,void *pointer){
    U32 x,y;int hr;
    if(!w->locked || pointer!=w->locked)fail(146);
    for(y=0;y<480;y++)for(x=0;x<640;x++)w->pixels[y*640+x]=w->locked[y*w->pitch+x];
    hr=((Call1)w->real->table[32])(w->real,pointer);
    w->locked=0;w->ready=!hr;
    return hr;
}
static int API flip(Wrapper *w,void *target,void *flags){
    U32 record[32],i,number=++attempts;U64 qpc;int hr,palette_hr,entries_hr=0;
    Object *p=0;
    hr=((Call2)w->real->table[11])(w->real,unwrap(target),flags);
    if(hr)return hr;
    if(!w->back || !w->back->ready)fail(147);
    /* GetPalette is read-only, with its temporary returned reference released. */
    for(i=0;i<1024;i++)palette[i]=0;
    palette_hr=((Call1)w->real->table[20])(w->real,&p);
    if(!palette_hr){
        if(!p)fail(148);
        entries_hr=((Call4)p->table[4])(p,0,0,(void*)256,palette);
        if(entries_hr)fail(149);
        {typedef U32(API *Release)(Object*);((Release)p->table[2])(p);}
    }
    QueryPerformanceCounter(&qpc);
    for(i=0;i<32;i++)record[i]=0;
    record[0]=0x32564444;record[1]=1;record[2]=++serial;record[3]=number;
    record[4]=(U32)__builtin_return_address(0);record[5]=GetCurrentThreadId();
    record[6]=(U32)qpc;record[7]=(U32)(qpc>>32);
    record[8]=*(U32*)0x936ff4;record[9]=*(U32*)0x462ff0;record[10]=*(U32*)0x462cd4;
    record[11]=*(U32*)0x940010;record[12]=*(U32*)0x467420;record[13]=*(U32*)0x7746c0;
    record[14]=*(U32*)0x467074;record[15]=*(U32*)0x7746ac;record[16]=*(U32*)0x9392b4;
    record[17]=w->back->width;record[18]=w->back->height;record[19]=w->back->pitch;record[20]=w->back->bits;
    /* The real successful upload must be identical to the engine buffer. */
    for(i=0;i<307200;i++)if(w->back->pixels[i]!=((volatile U8*)0x700450)[i])fail(150);
    record[21]=307200;record[22]=1024;record[23]=1024;
    record[24]=(U32)palette_hr;record[25]=(U32)entries_hr;
    if(observe_rng){
        record[1]=3;record[26]=random_seed();
        record[27]=*(volatile U32*)0x462d74;record[28]=*(volatile U32*)0x462d70;
        record[29]=*(volatile U32*)0x74f174;record[30]=*(volatile U32*)0x74f178;
    }
    if(serial>maximum_frames)fail(154);
    if(chunk_frames && !output)open_chunk();
    write_bytes(record,sizeof(record));write_bytes(w->back->pixels,307200);
    write_bytes((void*)0x700050,1024);write_bytes(palette,1024);
    if(chunk_frames && ++chunk_count==chunk_frames)close_chunk();
    {char marker[64],*p=marker;const char *prefix="DD2_VIDEO record=";
     while(*prefix)*p++=*prefix++;p=decimal(p,serial);*p=0;OutputDebugStringA(marker);}
    return hr;
}
/* Any untouched slot replaces only the wrapper self argument and tail-calls
 * the real vtable, preserving all remaining arguments and the stdcall ABI.
 * Generated forwarders are compiled as static code, never runtime patches. */
#include "ddraw_video_forwarders.h"
int API DirectDrawCreate(void *guid,Object **result,void *outer){
    typedef int(API *Create)(void*,Object**,void*);
    char path[1024];U32 length;int hr;
    if(!initialized){
        initialized=1;backend=LoadLibraryA("_ddraw_real.dll");if(!backend)fail(151);
        initialize_tables();draw_table[2]=(void*)release;draw_table[6]=(void*)create_surface;
        surface_table[2]=(void*)release;surface_table[11]=(void*)flip;surface_table[12]=(void*)attached;
        surface_table[25]=(void*)lock_surface;surface_table[32]=(void*)unlock_surface;
        length=GetEnvironmentVariableA("DD2_VIDEO_CAPTURE",path,sizeof(path));
        if(!length || length>=sizeof(path))fail(152);
        {U32 i;for(i=0;i<=length;i++)capture_path[i]=path[i];}
        chunk_frames=option("DD2_VIDEO_CHUNK_FRAMES",0,128);
        maximum_frames=option("DD2_VIDEO_MAX_FRAMES",4096,60000);
        observe_rng=option("DD2_VIDEO_RNG",0,1);
        if(!chunk_frames && maximum_frames>4096)fail(155);
        if(!chunk_frames){
            output=CreateFileA(path,0x40000000u,3,0,1,0x80,0);if(output==(HANDLE)-1)fail(153);
        }
    }
    hr=((Create)GetProcAddress(backend,"DirectDrawCreate"))(guid,result,outer);
    if(!hr && result && *result)*result=(Object*)wrap(*result,draw_table);
    return hr;
}
int API DllMain(HANDLE instance,U32 reason,void *reserved){
    (void)reserved;if(reason==1)DisableThreadLibraryCalls(instance);
    /* Partial blocks remain .part; the host verifies closure after process exit. */
    if(reason==0 && output){CloseHandle(output);output=0;}return 1;
}
