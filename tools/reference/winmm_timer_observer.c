/* Reference-only observation of Wine's existing multimedia timers.
 * Every unobserved export forwards to the original Wine DLL. The observed
 * calls retain callback arguments/results and execute the original callback
 * once. Contexts remain alive until process exit: nonsynchronous cancellation
 * can return while an already selected callback still owns its context.
 * Logging changes measured timing. This provides explicit service inputs;
 * it does not establish port or physical-clock audio/video equivalence. */
#include "winmm_timer_observer.h"
typedef U32(API *SetTimer)(U32,U32,Callback,U32,U32);
typedef U32(API *KillTimer)(U32);
typedef struct {U32 period,resolution,flags,user;Callback callback;} Context;
typedef struct {
    U32 magic,serial,type,id,period,resolution,flags,callback,user,thread,caller,result;
    U64 qpc,frequency;
    U32 ticks,reserved;
} Record;
static HANDLE output,mutex,backend;
static volatile int initialized;
static U32 serial;
static SetTimer real_set;
static KillTimer real_kill;
static void initialize(void){
    char path[1024];U32 length;
    if(InterlockedCompareExchange(&initialized,1,0)){
        while(InterlockedCompareExchange(&initialized,2,2)!=2)Sleep(0);
        return;
    }
    backend=LoadLibraryA("_winmm_real.dll");
    if(!backend)ExitProcess(121);
    real_set=(SetTimer)GetProcAddress(backend,"timeSetEvent");
    real_kill=(KillTimer)GetProcAddress(backend,"timeKillEvent");
    if(!real_set || !real_kill)ExitProcess(122);
    mutex=CreateMutexA(0,0,0);
    length=GetEnvironmentVariableA("DD2_TIMER_CAPTURE",path,sizeof(path));
    if(length){
        if(length>=sizeof(path))ExitProcess(123);
        output=CreateFileA(path,0x40000000u,3,0,2,0x80,0);
        if(output==(HANDLE)-1)ExitProcess(124);
    }
    InterlockedExchange(&initialized,2);
}
static char* decimal(char* p,U32 value){
    char digits[10];U32 count=0;
    do{digits[count++]=(char)('0'+value%10);value/=10;}while(value);
    while(count)*p++=digits[--count];return p;
}
static void record(U32 type,U32 id,Context* context,U32 caller,U32 result){
    Record r;U32 written;char marker[80],*p;const char* prefix="DD2_TIMER record=";
    if(!output)return;
    WaitForSingleObject(mutex,0xffffffffu);
    r.magic=0x32544d44u;r.serial=++serial;r.type=type;r.id=id;
    r.period=context?context->period:0;r.resolution=context?context->resolution:0;
    r.flags=context?context->flags:0;r.callback=context?(U32)context->callback:0;
    r.user=context?context->user:0;r.thread=GetCurrentThreadId();r.caller=caller;
    QueryPerformanceCounter(&r.qpc);QueryPerformanceFrequency(&r.frequency);
    r.ticks=GetTickCount();r.result=result;r.reserved=0;
    if(sizeof(r)!=72 || !WriteFile(output,&r,sizeof(r),&written,0) || written!=sizeof(r))ExitProcess(125);
    p=marker;while(*prefix)*p++=*prefix++;p=decimal(p,r.serial);
    *p++=' ';*p++='t';*p++='y';*p++='p';*p++='e';*p++='=';p=decimal(p,type);
    *p++=' ';*p++='i';*p++='d';*p++='=';p=decimal(p,id);*p=0;
    OutputDebugStringA(marker);
    ReleaseMutex(mutex);
}
static void API observe_callback(U32 id,U32 message,U32 pointer,U32 a,U32 b){
    Context* context=(Context*)pointer;
    record(3,id,context,0,0);
    context->callback(id,message,context->user,a,b);
    record(4,id,context,0,0);
}
U32 API timeSetEvent(U32 delay,U32 resolution,Callback callback,U32 user,U32 flags){
    Context* context;U32 result,caller=(U32)__builtin_return_address(0);
    initialize();
    context=HeapAlloc(GetProcessHeap(),8,sizeof(*context));if(!context)ExitProcess(126);
    context->period=delay;context->resolution=resolution;context->callback=callback;
    context->user=user;context->flags=flags;
    record(1,0,context,caller,0);
    if(!(flags&0x30) && callback)result=real_set(delay,resolution,observe_callback,(U32)context,flags);
    else result=real_set(delay,resolution,callback,user,flags);
    record(2,result,context,caller,result);
    return result;
}
U32 API timeKillEvent(U32 id){
    U32 result,caller=(U32)__builtin_return_address(0);
    initialize();record(5,id,0,caller,0);
    result=real_kill(id);record(6,id,0,caller,result);return result;
}
int API DllMain(HANDLE instance,U32 reason,void* reserved){
    (void)reserved;if(reason==1)DisableThreadLibraryCalls(instance);return 1;
}
