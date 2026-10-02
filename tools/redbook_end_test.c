/* Same finite MCI ranges on Wine and both ports. The real Wine driver owns
 * its ring/worker; the port fixture advances an explicit external clock.
 * Do not turn a Wine-specific missing tail into the port's expected source. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#define CALL WINAPI
__declspec(dllimport) int CALL DirectSoundCreate(void*,void**,void*);
#else
#include "dd2_cd.h"
#define CALL
int DirectSoundCreate(int,void**,int);
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
static unsigned now;
unsigned dd2_platform_ms(void) { return now; }
void FUN_0041345c(void) { abort(); }
#endif
static void require(int ok,const char* reason) {
    if(!ok){fprintf(stderr,"CD end: %s\n",reason);exit(1);}
}
static unsigned mci(unsigned device,unsigned command,unsigned flags,uint32_t* params) {
#ifdef _WIN32
    return mciSendCommandA(device,command,flags,(uintptr_t)params);
#else
    return dd2_mci_send(device,command,flags,params);
#endif
}
static void wait_ms(unsigned ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    now+=ms;dd2_cd_pump();
#endif
}
static unsigned mode(unsigned cd) {
    uint32_t status[4]={0,0,4,0};
    require(mci(cd,0x814,0x100,status)==0,"read mode");
    return status[1];
}
int main(void) {
    unsigned lengths[]={8,39,40,50,52,53,65,66},i,cd;
    uint32_t open[5]={0},set[3]={0,10,0},play[3]={0,2u|6u<<16,0};
    void* device;
#ifndef _WIN32
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map engine clock");
#endif
    *(int*)0x462ff0=0;
    setenv("DD2_REALTIME","1",1);setenv("DD2_SOUND","1",1);
#endif
    require(DirectSoundCreate(0,&device,0)==0,"retain shared device");
#ifdef _WIN32
    require(((int(CALL *)(void*,HWND,int))(*(void***)device)[6])
            (device,GetDesktopWindow(),2)==0,"cooperative level");
#endif
    open[2]=(uint32_t)(uintptr_t)"cdaudio";
    require(mci(0,0x803,0x2000,open)==0,"open CD");cd=open[1];
    require(mci(cd,0x80d,0x400,set)==0,"set TMSF");
    wait_ms(200);putchar('[');
    for(i=0;i<sizeof(lengths)/sizeof(lengths[0]);i++) {
        unsigned end=450+lengths[i],early,drained;
        play[2]=2u|(end/75u)<<16|(end%75u)<<24;
        require(mci(cd,0x806,12,play)==0,"play finite range");
        wait_ms(200);early=mode(cd);
        wait_ms(1300);drained=mode(cd);
        printf("%s{\"sectors\":%u,\"mode_200ms\":%u,\"mode_1500ms\":%u}",
               i?",":"",lengths[i],early,drained);
    }
    require(mci(cd,0x804,0,NULL)==0,"close CD");
    ((int(CALL *)(void*))(*(void***)device)[2])(device);
    puts("]");return 0;
}
