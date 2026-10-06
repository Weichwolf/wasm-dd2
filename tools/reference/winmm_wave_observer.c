/* Observe original WinMM source buffers, forwarding all arguments/results.
 * MCI submits MS-ADPCM here; decoded PCM is observed separately at ACM.
 * Copies precede waveOutWrite: the backend retains the caller's header/buffer.
 * No callback, header, PCM, clock or backend result is replaced. Logging changes
 * measured timing; these observations do not establish port or DAC parity. */
#include "winmm_timer_observer.h"
IMPORT U32 API GetLastError(void);
IMPORT void API SetLastError(U32);
IMPORT int API CloseHandle(HANDLE);
IMPORT int API HeapFree(HANDLE,U32,void*);
IMPORT U32 API GetModuleFileNameA(HANDLE,char*,U32);
IMPORT U32 API GetCurrentProcessId(void);
typedef struct {char* data;U32 bytes,recorded,user,flags,loops;void* next;U32 reserved;} Header;
typedef U32(API *Open)(HANDLE*,U32,const void*,U32,U32,U32);
typedef U32(API *Buffer)(HANDLE,Header*,U32);
typedef U32(API *Device)(HANDLE);
typedef struct {
    U32 magic,version,serial,kind,pid,thread,caller,handle,result,last_error;
    U32 argument_bytes,bytes,header,data,user,flags_before,flags_after,loops,next,reserved;
    U32 callback,instance,device,unused;
    U64 offset,begin,end,frequency;
    unsigned char format[18],padding[14];
} Record;
static HANDLE backend,mutex,events,pcm;
static Open real_open;
static Buffer real_write,real_prepare,real_unprepare;
static Device real_reset,real_close;
static volatile int initialized;
static U32 serial,pid;
static U64 offset,frequency;
static char* decimal(char* p,U32 value){
    char digits[10];U32 count=0;
    do{digits[count++]=(char)('0'+value%10);value/=10;}while(value);
    while(count)*p++=digits[--count];return p;
}
void* memset(void* destination,int value,unsigned count){
    volatile unsigned char* p=destination;
    while(count--)*p++=(unsigned char)value;return destination;
}
static HANDLE create(const char* root,const char* suffix){
    char path[1200],*p=path;const char* s=root;
    while(*s)*p++=*s++;*p++='\\';s="winmm-";while(*s)*p++=*s++;
    p=decimal(p,pid);while(*suffix)*p++=*suffix++;*p=0;
    HANDLE file=CreateFileA(path,0x40000000u,3,0,1,0x80,0);
    if(file==(HANDLE)-1)ExitProcess(141);return file;
}
static void initialize(void){
    char path[1024],exe[1024];U32 length,i;const char* wanted="dd2h.exe";char* name;
    if(InterlockedCompareExchange(&initialized,1,0)){
        while(InterlockedCompareExchange(&initialized,2,2)!=2)Sleep(0);return;
    }
    backend=LoadLibraryA("_winmm_real.dll");if(!backend)ExitProcess(142);
    real_open=(Open)GetProcAddress(backend,"waveOutOpen");
    real_write=(Buffer)GetProcAddress(backend,"waveOutWrite");
    real_prepare=(Buffer)GetProcAddress(backend,"waveOutPrepareHeader");
    real_unprepare=(Buffer)GetProcAddress(backend,"waveOutUnprepareHeader");
    real_reset=(Device)GetProcAddress(backend,"waveOutReset");
    real_close=(Device)GetProcAddress(backend,"waveOutClose");
    if(!real_open || !real_write || !real_prepare || !real_unprepare || !real_reset || !real_close)ExitProcess(143);
    length=GetEnvironmentVariableA("DD2_WINMM_CAPTURE",path,sizeof(path));
    if(length){
        if(length>=sizeof(path))ExitProcess(144);
        length=GetModuleFileNameA(0,exe,sizeof(exe));
        if(!length || length>=sizeof(exe))ExitProcess(145);
        name=exe;for(i=0;i<length;i++)if(exe[i]=='\\' || exe[i]=='/')name=exe+i+1;
        for(i=0;wanted[i] && name[i];i++){
            char c=name[i];if(c>='A' && c<='Z')c+=(char)('a'-'A');if(c!=wanted[i])break;
        }
        if(!wanted[i] && !name[i]){
            pid=GetCurrentProcessId();mutex=CreateMutexA(0,0,0);
            if(!mutex || !QueryPerformanceFrequency(&frequency) || !frequency)ExitProcess(146);
            events=create(path,".bin");pcm=create(path,".adpcm");
        }
    }
    InterlockedExchange(&initialized,2);
}
static void record(Record* r,const void* copy){
    U32 written;
    if(!events)return;
    WaitForSingleObject(mutex,0xffffffffu);
    r->magic=0x57443244u;r->version=1;r->serial=++serial;r->pid=pid;
    r->thread=GetCurrentThreadId();r->frequency=frequency;r->offset=offset;
    if(r->kind==2 && r->result==0 && r->bytes){
        if(!copy || !WriteFile(pcm,copy,r->bytes,&written,0) || written!=r->bytes)ExitProcess(147);
        offset+=r->bytes;
    }
    if(sizeof(*r)!=160 || sizeof(Header)!=32 || !WriteFile(events,r,sizeof(*r),&written,0) || written!=sizeof(*r))ExitProcess(148);
    ReleaseMutex(mutex);
}
U32 API waveOutOpen(HANDLE* handle,U32 device,const void* format,U32 callback,U32 instance,U32 flags){
    U32 incoming=GetLastError(),result,i;Record r={0};initialize();
    r.kind=1;r.caller=(U32)__builtin_return_address(0);r.device=device;r.callback=callback;r.instance=instance;r.flags_before=flags;
    if(events && format)for(i=0;i<18;i++)r.format[i]=((const unsigned char*)format)[i];
    QueryPerformanceCounter(&r.begin);SetLastError(incoming);
    result=real_open(handle,device,format,callback,instance,flags);r.last_error=GetLastError();
    QueryPerformanceCounter(&r.end);r.result=result;r.handle=result==0 && handle?(U32)*handle:0;
    record(&r,0);SetLastError(r.last_error);return result;
}
static U32 buffer(Buffer next,U32 kind,HANDLE handle,Header* header,U32 size,U32 caller,U32 incoming){
    U32 result,i;void* copy=0;Record r={0};
    r.kind=kind;r.caller=caller;r.handle=(U32)handle;r.header=(U32)header;r.argument_bytes=size;
    if(events && header){
        r.bytes=header->bytes;r.data=(U32)header->data;r.user=header->user;
        r.flags_before=header->flags;r.loops=header->loops;r.next=(U32)header->next;r.reserved=header->reserved;
        if(kind==2 && r.bytes){
            if(r.bytes>1024*1024 || !header->data)ExitProcess(149);
            copy=HeapAlloc(GetProcessHeap(),0,r.bytes);if(!copy)ExitProcess(150);
            for(i=0;i<r.bytes;i++)((unsigned char*)copy)[i]=((const unsigned char*)header->data)[i];
        }
    }
    QueryPerformanceCounter(&r.begin);SetLastError(incoming);
    result=next(handle,header,size);r.last_error=GetLastError();QueryPerformanceCounter(&r.end);
    r.result=result;if(events && header)r.flags_after=header->flags;record(&r,copy);
    if(copy)HeapFree(GetProcessHeap(),0,copy);SetLastError(r.last_error);return result;
}
#define BUFFER(name,real,kind) \
U32 API name(HANDLE h,Header* header,U32 size){U32 incoming=GetLastError();initialize(); \
    return buffer(real,kind,h,header,size,(U32)__builtin_return_address(0),incoming);}
BUFFER(waveOutWrite,real_write,2)
BUFFER(waveOutPrepareHeader,real_prepare,5)
BUFFER(waveOutUnprepareHeader,real_unprepare,6)
static U32 device(Device next,U32 kind,HANDLE handle,U32 caller,U32 incoming){
    U32 result;Record r={0};r.kind=kind;r.caller=caller;r.handle=(U32)handle;
    QueryPerformanceCounter(&r.begin);SetLastError(incoming);
    result=next(handle);r.last_error=GetLastError();QueryPerformanceCounter(&r.end);r.result=result;
    record(&r,0);SetLastError(r.last_error);return result;
}
#define DEVICE(name,real,kind) \
U32 API name(HANDLE h){U32 incoming=GetLastError();initialize(); \
    return device(real,kind,h,(U32)__builtin_return_address(0),incoming);}
DEVICE(waveOutReset,real_reset,3)
DEVICE(waveOutClose,real_close,4)
int API DllMain(HANDLE instance,U32 reason,void* reserved){
    (void)reserved;if(reason==1)DisableThreadLibraryCalls(instance);return 1;
}
