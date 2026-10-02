/* Real WinMM CD playback and DirectSound share an ordered Float32 mixer.
 * One effect precedes the MCI buffer and one follows it. Compare their exact
 * summed waveform, not separate source captures or browser resampling. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#define CALL WINAPI
__declspec(dllimport) int CALL DirectSoundCreate(void*,void**,void*);
#else
#include "dd2_cd.h"
#include "dd2_sound.h"
#define CALL
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
int DirectSoundCreate(int,void**,int);
static unsigned now;
unsigned dd2_platform_ms(void){return now;}
void FUN_0041345c(void){abort();}
#endif
#define METHOD(buffer,index,type) ((type)(*(void***)(buffer))[index])
typedef int(CALL *Create)(void*,void*,void**,int);
typedef int(CALL *Lock)(void*,unsigned,unsigned,void**,unsigned*,void**,unsigned*,int);
typedef int(CALL *Unlock)(void*,void*,unsigned,void*,unsigned);
typedef int(CALL *Set)(void*,int);
typedef int(CALL *Play)(void*,int,int,int);
typedef int(CALL *Release)(void*);
static void require(int ok,const char* reason){
    if(!ok){fprintf(stderr,"Shared audio test: %s\n",reason);exit(1);}
}
static void wait_ms(unsigned ms){
#ifdef _WIN32
    Sleep(ms);
#else
    now+=ms;dd2_snd_mix_flip();
#endif
}
static unsigned mci(unsigned device,unsigned command,unsigned flags,uint32_t* params){
#ifdef _WIN32
    return mciSendCommandA(device,command,flags,(uintptr_t)params);
#else
    return dd2_mci_send(device,command,flags,params);
#endif
}
static void* make_buffer(void* device,short value,int volume){
    unsigned char wave[18]={1,0,1,0,0x44,0xac,0,0,0x88,0x58,1,0,2,0,16,0,0,0};
    uint32_t desc[5]={20,0x80e0,8820,0,(uint32_t)(uintptr_t)wave};
    void *buffer,*p1,*p2;unsigned n1,n2,i;
    require(METHOD(device,3,Create)(device,desc,&buffer,0)==0,"create mono16 source");
    require(METHOD(buffer,11,Lock)(buffer,0,8820,&p1,&n1,&p2,&n2,0)==0 && n1==8820 && !n2,"lock source");
    for(i=0;i<n1/2;i++)((short*)p1)[i]=value;
    require(METHOD(buffer,19,Unlock)(buffer,p1,n1,p2,n2)==0,"unlock source");
    require(METHOD(buffer,15,Set)(buffer,volume)==0,"set volume");
    return buffer;
}
int main(void){
    void *device,*first,*last;
    uint32_t open[5]={0},set[3]={0,10,0},play[3]={0,2,3};
    unsigned cd;
#ifndef _WIN32
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map engine clock");
#endif
    *(int*)0x462ff0=0;
    setenv("DD2_SOUND","1",1);setenv("DD2_REALTIME","1",1);
#endif
    require(DirectSoundCreate(0,&device,0)==0,"open DirectSound");
#ifdef _WIN32
    require(METHOD(device,6,int(CALL *)(void*,HWND,int))(device,GetDesktopWindow(),2)==0,"cooperative level");
#endif
    first=make_buffer(device,16384,0);
    require(METHOD(first,12,Play)(first,0,0,1)==0,"play first effect");
    wait_ms(80);
    open[2]=(uint32_t)(uintptr_t)"cdaudio";
    require(mci(0,0x803,0x2000,open)==0,"open real CD audio");cd=open[1];
    require(mci(cd,0x80d,0x400,set)==0,"set TMSF");
    require(mci(cd,0x806,12,play)==0,"play track02");
    last=make_buffer(device,-16385,-1376);
    require(METHOD(last,12,Play)(last,0,0,1)==0,"play later effect");
    wait_ms(700);
    require(mci(cd,0x808,0,NULL)==0,"stop CD");
    wait_ms(100);
    require(METHOD(first,18,Release)(first)==0,"stop first effect");
    require(METHOD(last,18,Release)(last)==0,"stop later effect");
    wait_ms(80);
    METHOD(last,2,Release)(last);METHOD(first,2,Release)(first);
    require(mci(cd,0x804,0,NULL)==0,"close CD");
    METHOD(device,2,Release)(device);
    puts("{\"rate\":44100,\"track\":2,\"order\":[\"effect-half\",\"cd\",\"effect-gain\"]}");
    return 0;
}
