/* Same MCI transport commands on real Wine and the two port backends.
 * Wait a full second before testing a completed eight-sector interval: this
 * observes the drained transport, not Wine's asynchronous ring-buffer edge. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include "dd2_cd.h"
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
static unsigned now;
unsigned dd2_platform_ms(void) { return now; }
void FUN_0041345c(void) { abort(); }
#endif
static void require(int ok, const char* reason) {
    if (!ok) { fprintf(stderr,"CD controls: %s\n",reason); exit(1); }
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
static void record(unsigned cd,const char* label,int comma) {
    uint32_t status[4]={0,0,4,0};
    require(mci(cd,0x814,0x100,status)==0,"read transport mode");
    printf("%s{\"label\":\"%s\",\"mode\":%u}",comma ? "," : "",label,status[1]);
}
int main(void) {
    uint32_t open[5]={0},set[3]={0,10,0};
    uint32_t play[3]={0,2u|6u<<16,2u|6u<<16|8u<<24};
    unsigned cd;
#ifndef _WIN32
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map engine clock");
#endif
    *(int*)0x462ff0=0;
    setenv("DD2_REALTIME","1",1);
#endif
    open[2]=(uint32_t)(uintptr_t)"cdaudio";
    require(mci(0,0x803,0x2000,open)==0,"open CD");cd=open[1];
    require(mci(cd,0x80d,0x400,set)==0,"set TMSF");
    require(mci(cd,0x806,12,play)==0,"play eight-sector interval");
    wait_ms(1000);
    putchar('[');record(cd,"drained",0);
    require(mci(cd,0x809,0,NULL)==0,"pause drained transport");
    wait_ms(20);record(cd,"drained-pause",1);
    require(mci(cd,0x855,0,NULL)==0,"resume drained transport");
    wait_ms(20);record(cd,"drained-resume",1);
    require(mci(cd,0x855,0,NULL)==0,"repeat drained resume");
    wait_ms(20);record(cd,"drained-resume-again",1);
    require(mci(cd,0x809,0,NULL)==0,"repeat drained pause");
    wait_ms(20);record(cd,"drained-pause-again",1);
    play[1]=2;play[2]=3;
    require(mci(cd,0x806,12,play)==0,"explicit replay creates a new transport");
    wait_ms(80);record(cd,"replay",1);
    require(mci(cd,0x809,0,NULL)==0,"pause live transport");
    wait_ms(100);record(cd,"paused",1);
    require(mci(cd,0x809,0,NULL)==0,"repeat live pause");
    wait_ms(100);record(cd,"paused-again",1);
    require(mci(cd,0x855,0,NULL)==0,"resume live transport");
    wait_ms(80);record(cd,"resumed",1);
    require(mci(cd,0x855,0,NULL)==0,"repeat live resume");
    wait_ms(20);record(cd,"resumed-again",1);
    require(mci(cd,0x808,0,NULL)==0,"stop live transport");
    record(cd,"stopped",1);puts("]");
    require(mci(cd,0x804,0,NULL)==0,"close CD");
    return 0;
}
