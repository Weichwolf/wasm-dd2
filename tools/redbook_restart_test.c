/* Observe real WinMM Stop/TO-only Play with an explicit hardware Q sector.
 * The file is a fixture input to the virtual Linux device, not a transport
 * clock or an inference from the accepted Wine waveform. Both public MCI
 * positions agree at the control point; compare the actual restart source. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "dd2_disc.h"
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
static void require(int ok,const char* why) {
    if(!ok){fprintf(stderr,"CD restart: %s\n",why);exit(1);}
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
static void q_position(const char* path,unsigned sector,unsigned status) {
    FILE *file=fopen(path,"w");
    require(file!=NULL,"open explicit Q-channel fixture input");
    require(fprintf(file,"%u %u\n",sector,status)>0,"write explicit hardware sector/status");
    require(fclose(file)==0,"close explicit hardware sector");
}
int main(int argc,char** argv) {
    uint32_t open[5]={0},set[3]={0,10,0},play[3]={0,2u|6u<<16,3};
    uint32_t position[4]={0,0,2,0};
    void *device;
    unsigned cd;
    require(argc==2,"Q-channel fixture path required");
#ifndef _WIN32
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map engine clock");
#endif
    *(int*)0x462ff0=0;
    setenv("DD2_REALTIME","1",1);setenv("DD2_SOUND","1",1);
#endif
    /* Keep Wine's shared device alive across MCI_STOP; observe silence and
     * both starts on the same actual accepted Float32 stream. */
    require(DirectSoundCreate(0,&device,0)==0,"retain shared DirectSound device");
#ifdef _WIN32
    require(((int(CALL *)(void*,HWND,int))(*(void***)device)[6])
            (device,GetDesktopWindow(),2)==0,"cooperative level");
#endif
    q_position(argv[1],dd2_cd_sectors[1]+450,0x11);
    open[2]=(uint32_t)(uintptr_t)"cdaudio";
    require(mci(0,0x803,0x2000,open)==0,"open CD");cd=open[1];
    require(mci(cd,0x80d,0x400,set)==0,"set TMSF");
    require(mci(cd,0x806,12,play)==0,"play from track2 at six seconds");
    wait_ms(101);
    /* Wine ntdll/unix/cdrom.c caches a playing Q position, retaining it for
     * AUDIO_COMPLETED. AUDIO_NO_STATUS clears it and discards even a supplied
     * address. Declare and observe this hardware snapshot before stopping;
     * this is fixture input, not an inferred digital-buffer transport clock. */
    q_position(argv[1],dd2_cd_sectors[1]+457,0x11);
    require(mci(cd,0x814,0x100,position)==0 && position[1]==(2u|6u<<16|7u<<24),
            "observe the explicit playing Q position");
    require(mci(cd,0x808,0,NULL)==0,"original CD_Pause uses MCI_STOP");
    q_position(argv[1],dd2_cd_sectors[1]+457,0x13);
    {
    unsigned result=mci(cd,0x814,0x100,position);
    if(result || position[1]!=(2u|6u<<16|7u<<24))
        fprintf(stderr,"MCI position result=%u actual=%08x expected=%08x\n",
            result,position[1],2u|6u<<16|7u<<24);
    require(result==0 && position[1]==(2u|6u<<16|7u<<24),
            "both public MCI positions report track2 six seconds seven sectors");
    }
    wait_ms(300);
    require(mci(cd,0x806,8,play)==0,"original CD_Restart uses TO-only MCI_PLAY");
    wait_ms(300);
    require(mci(cd,0x808,0,NULL)==0,"stop restart source");
    wait_ms(200);
    require(mci(cd,0x804,0,NULL)==0,"close CD");
    ((int(CALL *)(void*))(*(void***)device)[2])(device);
    puts("{\"track\":2,\"start_sector_offset\":450,\"restart_sector_offset\":457,\"public_position\":117833730}");
    return 0;
}
