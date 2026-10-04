#include "winmm_timer_observer.h"
IMPORT U32 API timeSetEvent(U32,U32,Callback,U32,U32);
IMPORT U32 API timeKillEvent(U32);
static HANDLE event;
static volatile int counts[2],bad;
static U32 ids[2];
static void API callback(U32 id,U32 message,U32 user,U32 a,U32 b){
    U32 index=user-0xbeef0000u;
    if(index>1 || message || a || b){bad=1;SetEvent(event);return;}
    if(ids[index] && ids[index]!=id)bad=1;
    InterlockedIncrement(&counts[index]);SetEvent(event);
}
void start(void){
    int second,goal;U32 written;
    static const char ok[]="{\"ids\":[16,33],\"callback_arguments\":true,\"independent_cancel\":true}\n";
    event=CreateEventA(0,0,0,0);
    ids[0]=timeSetEvent(17,1,callback,0xbeef0000u,0x101);
    ids[1]=timeSetEvent(23,1,callback,0xbeef0001u,0x101);
    if(ids[0]!=16 || ids[1]!=33)ExitProcess(11);
    while(counts[0]<3 || counts[1]<3){
        if(WaitForSingleObject(event,3000) || bad)ExitProcess(12);
    }
    if(timeKillEvent(ids[1]))ExitProcess(13);
    second=counts[1];goal=counts[0]+2;
    while(counts[0]<goal){if(WaitForSingleObject(event,3000) || bad)ExitProcess(14);}
    if(counts[1]!=second || timeKillEvent(ids[0]) || timeKillEvent(ids[1])!=97)ExitProcess(15);
    WriteFile(GetStdHandle(-11),ok,sizeof(ok)-1,&written,0);ExitProcess(0);
}
