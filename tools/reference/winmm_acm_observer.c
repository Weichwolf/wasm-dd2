/* Forward actual Wine ACM calls and record consumed MS-ADPCM/decoded PCM.
 * No buffer, callback, header, flags, error or return is replaced. Logging
 * changes measured timing; source equality is separate from playback parity. */
#include "winmm_timer_observer.h"
IMPORT U32 API GetLastError(void);
IMPORT void API SetLastError(U32);
IMPORT int API HeapFree(HANDLE,U32,void*);
IMPORT U32 API GetModuleFileNameA(HANDLE,char*,U32);
IMPORT U32 API GetCurrentProcessId(void);
typedef struct {
    U32 size,status,user;unsigned char* source;U32 source_bytes,source_used,source_user;
    unsigned char* destination;U32 destination_bytes,destination_used,destination_user,reserved[10];
} Header;
typedef U32(API *Open)(HANDLE*,HANDLE,const void*,const void*,void*,U32,U32,U32);
typedef U32(API *Convert)(HANDLE,Header*,U32);
typedef U32(API *Close)(HANDLE,U32);
typedef struct {
    U32 magic,version,serial,kind,pid,thread,caller,handle,result,last_error;
    U32 flags,header,header_size,status_before,status_after,source_requested,source_used;
    U32 destination_requested,destination_used,source_pointer,destination_pointer;
    U32 source_format_bytes,destination_format_bytes,unused;
    U64 source_offset,destination_offset,begin,end,frequency;
    unsigned char source_format[64],destination_format[64];
} Record;
static HANDLE backend,mutex,events,input,output;
static Open real_open;static Convert real_convert;static Close real_close;
static volatile int initialized;
static U32 serial,pid;static U64 source_offset,destination_offset,frequency;
void* memset(void* destination,int value,unsigned count){
    volatile unsigned char* p=destination;while(count--)*p++=(unsigned char)value;return destination;
}
static char* decimal(char* p,U32 value){
    char digits[10];U32 count=0;do{digits[count++]=(char)('0'+value%10);value/=10;}while(value);
    while(count)*p++=digits[--count];return p;
}
static HANDLE create(const char* root,const char* suffix){
    char path[1200],*p=path;const char* s=root;
    while(*s)*p++=*s++;*p++='\\';s="acm-";while(*s)*p++=*s++;
    p=decimal(p,pid);while(*suffix)*p++=*suffix++;*p=0;
    HANDLE file=CreateFileA(path,0x40000000u,3,0,1,0x80,0);if(file==(HANDLE)-1)ExitProcess(161);return file;
}
static void initialize(void){
    char path[1024],exe[1024];U32 length,i;const char* wanted="dd2h.exe";char* name;
    if(InterlockedCompareExchange(&initialized,1,0)){
        while(InterlockedCompareExchange(&initialized,2,2)!=2)Sleep(0);return;
    }
    backend=LoadLibraryA("_msacm32_real.dll");if(!backend)ExitProcess(162);
    real_open=(Open)GetProcAddress(backend,"acmStreamOpen");
    real_convert=(Convert)GetProcAddress(backend,"acmStreamConvert");
    real_close=(Close)GetProcAddress(backend,"acmStreamClose");
    if(!real_open || !real_convert || !real_close)ExitProcess(163);
    length=GetEnvironmentVariableA("DD2_WINMM_CAPTURE",path,sizeof(path));
    if(length){
        if(length>=sizeof(path))ExitProcess(164);length=GetModuleFileNameA(0,exe,sizeof(exe));
        if(!length || length>=sizeof(exe))ExitProcess(165);
        name=exe;for(i=0;i<length;i++)if(exe[i]=='\\' || exe[i]=='/')name=exe+i+1;
        for(i=0;wanted[i] && name[i];i++){
            char c=name[i];if(c>='A' && c<='Z')c+=(char)('a'-'A');if(c!=wanted[i])break;
        }
        if(!wanted[i] && !name[i]){
            pid=GetCurrentProcessId();mutex=CreateMutexA(0,0,0);
            if(!mutex || !QueryPerformanceFrequency(&frequency) || !frequency)ExitProcess(166);
            events=create(path,".bin");input=create(path,".adpcm");output=create(path,".pcm");
        }
    }
    InterlockedExchange(&initialized,2);
}
static U32 format_copy(unsigned char* destination,const void* source){
    U32 i,length;const unsigned char* p=source;if(!p)return 0;
    length=18+p[16]+((U32)p[17]<<8);if(length>64)ExitProcess(167);
    for(i=0;i<length;i++)destination[i]=p[i];return length;
}
static void record(Record* r,const void* source,const void* destination){
    U32 written;if(!events)return;WaitForSingleObject(mutex,0xffffffffu);
    r->magic=0x41443244u;r->version=1;r->serial=++serial;r->pid=pid;
    r->thread=GetCurrentThreadId();r->frequency=frequency;
    r->source_offset=source_offset;r->destination_offset=destination_offset;
    if(r->kind==2 && r->result==0){
        if(r->source_used){
            if(!source || !WriteFile(input,source,r->source_used,&written,0) || written!=r->source_used)ExitProcess(168);
            source_offset+=r->source_used;
        }
        if(r->destination_used){
            if(!destination || !WriteFile(output,destination,r->destination_used,&written,0) || written!=r->destination_used)ExitProcess(169);
            destination_offset+=r->destination_used;
        }
    }
    if(sizeof(*r)!=264 || sizeof(Header)!=84 || !WriteFile(events,r,sizeof(*r),&written,0) || written!=sizeof(*r))ExitProcess(170);
    ReleaseMutex(mutex);
}
U32 API acmStreamOpen(HANDLE* handle,HANDLE driver,const void* source,const void* destination,void* filter,U32 callback,U32 instance,U32 flags){
    U32 incoming=GetLastError(),result;Record r={0};initialize();
    r.kind=1;r.caller=(U32)__builtin_return_address(0);r.flags=flags;
    if(events){r.source_format_bytes=format_copy(r.source_format,source);r.destination_format_bytes=format_copy(r.destination_format,destination);}
    QueryPerformanceCounter(&r.begin);SetLastError(incoming);
    result=real_open(handle,driver,source,destination,filter,callback,instance,flags);r.last_error=GetLastError();
    QueryPerformanceCounter(&r.end);r.result=result;r.handle=result==0 && handle?(U32)*handle:0;
    record(&r,0,0);SetLastError(r.last_error);return result;
}
U32 API acmStreamConvert(HANDLE handle,Header* header,U32 flags){
    U32 incoming=GetLastError(),result,i;void* copy=0;Record r={0};initialize();
    r.kind=2;r.caller=(U32)__builtin_return_address(0);r.handle=(U32)handle;r.header=(U32)header;r.flags=flags;
    if(events && header){
        r.header_size=header->size;r.status_before=header->status;r.source_requested=header->source_bytes;
        r.destination_requested=header->destination_bytes;r.source_pointer=(U32)header->source;r.destination_pointer=(U32)header->destination;
        if(r.source_requested){
            if(r.source_requested>1024*1024 || !header->source)ExitProcess(171);
            copy=HeapAlloc(GetProcessHeap(),0,r.source_requested);if(!copy)ExitProcess(172);
            for(i=0;i<r.source_requested;i++)((unsigned char*)copy)[i]=header->source[i];
        }
    }
    QueryPerformanceCounter(&r.begin);SetLastError(incoming);
    result=real_convert(handle,header,flags);r.last_error=GetLastError();QueryPerformanceCounter(&r.end);r.result=result;
    if(events && header){
        r.status_after=header->status;r.source_used=header->source_used;r.destination_used=header->destination_used;
        if(r.source_used>r.source_requested || r.destination_used>r.destination_requested)ExitProcess(173);
    }
    record(&r,copy,header?header->destination:0);
    if(copy)HeapFree(GetProcessHeap(),0,copy);SetLastError(r.last_error);return result;
}
U32 API acmStreamClose(HANDLE handle,U32 flags){
    U32 incoming=GetLastError(),result;Record r={0};initialize();
    r.kind=3;r.caller=(U32)__builtin_return_address(0);r.handle=(U32)handle;r.flags=flags;
    QueryPerformanceCounter(&r.begin);SetLastError(incoming);
    result=real_close(handle,flags);r.last_error=GetLastError();QueryPerformanceCounter(&r.end);r.result=result;
    record(&r,0,0);SetLastError(r.last_error);return result;
}
int API DllMain(HANDLE instance,U32 reason,void* reserved){
    (void)reserved;if(reason==1)DisableThreadLibraryCalls(instance);return 1;
}
