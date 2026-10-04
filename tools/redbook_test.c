/* Backend behavior tests with an explicit clock, using the provisioned CD. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dd2_cd.h"
#include "dd2_disc.h"
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
static unsigned now;
unsigned dd2_platform_ms(void) { return now; }
void FUN_0041345c(void) { abort(); }
int DirectSoundCreate(int,void**,int);
static void require(int ok, const char *reason) {
    if (!ok) { fprintf(stderr,"Redbook test failed: %s\n",reason); exit(1); }
}
static unsigned status(unsigned item) {
    uint32_t p[4] = {0,0,item,0};
    require(dd2_mci_send(1,0x814,0x100,p)==0,"status command");
    return p[1];
}
int main(int argc, char **argv) {
    uint32_t open[5] = {0}, set[3] = {0,10,0}, play[3] = {0};
    void *device=NULL;
    int track;
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map engine clock");
#endif
    *(int*)0x462ff0=0;
    setenv("DD2_REALTIME","1",1);
    setenv("DD2_SOUND","1",1);
    /* The game keeps its effects device alive during CD Stop/Play. Retain a
     * real backend device so each transport shares the same fractional clock,
     * instead of starting a fresh output epoch after every CD-only Stop. */
    require(DirectSoundCreate(0,&device,0)==0 && device,"shared audio device");
    dd2_cd_pump();
    require(argc==2,"PCM output path required");
    setenv("DD2_CDPCM",argv[1],1);
    open[2] = (uint32_t)(uintptr_t)"cdaudio";
    require(dd2_mci_send(0,0x803,0x2000,open)==0 && open[1]==1,"open CD device");
    require(status(2)==(10u | 6u<<8 | 15u<<16),"default MSF includes CD lead-in");
    set[1]=0;
    require(dd2_mci_send(1,0x80d,0x400,set)==0 && status(2)==606201,"absolute CD milliseconds");
    play[1]=606201; play[2]=606201;
    require(dd2_mci_send(1,0x806,12,play)==0 && status(2)==606201,"millisecond seek rounds to CD sector");
    set[1]=10;
    require(dd2_mci_send(1,0x80d,0x400,set)==0,"set TMSF");
    require(status(3)==19,"19 physical tracks");
    require(status(4)==525,"initial stop");
    require(dd2_mci_send(1,0x830,12,play)==261,"CDAudio CUE unsupported as in original driver");
    for (track=2;track<=19;track++) {
        unsigned began=now,resumed,played;
        unsigned end_flags = track<19 ? 8 : 0;
        play[1]=track; play[2]=track+1;
        require(dd2_mci_send(1,0x806,4|end_flags,play)==0,"play physical audio track");
        require(status(4)==526 && status(8)==(unsigned)track,"playing correct track");
        now += 101; dd2_cd_pump();
        played=(unsigned)((uint64_t)now*44100/1000-(uint64_t)began*44100/1000);
        require(status(2)==((unsigned)track | 7u<<24),"TMSF position after 101ms");
        require(dd2_mci_send(1,0x808,0,NULL)==0,"stop for pause");
        now += 777; dd2_cd_pump();
        require(status(4)==525 && status(2)==((unsigned)track | 7u<<24),"stopped cursor stays fixed");
        played=played/588u*588u;
        require(dd2_mci_send(1,0x806,end_flags,play)==0,"TO-only resume starts at public Q-channel sector");
        resumed=now;
        now += 99; dd2_cd_pump();
        played+=(unsigned)((uint64_t)now*44100/1000-(uint64_t)resumed*44100/1000);
        require(status(2)==((unsigned)track | (played/588u)<<24),"resume follows the shared fractional device clock");
        require(dd2_mci_send(1,0x808,0,NULL)==0,"stop before next track");
    }
    /* Every complete track, including the final physical disc boundary. */
    for (track=2;track<=19;track++) {
        unsigned sectors=dd2_cd_sectors[track]-dd2_cd_sectors[track-1];
        unsigned end=track<19 ? (unsigned)track+1u :
            19u | sectors/4500u<<8 | (sectors/75u%60u)<<16 | (sectors%75u)<<24;
        play[1]=track; play[2]=track+1;
        require(dd2_mci_send(1,0x806,track<19 ? 12 : 4,play)==0,"full track play");
        now += 500000; dd2_cd_pump();
        require(status(4)==525 && status(2)==end,"stopped at exact physical track boundary");
    }
    /* Pause retains the same buffer, unlike the engine's Stop/TO-only resume.
     * The source does not advance during the pause; the device clock does. */
    play[1]=3;play[2]=4;
    require(dd2_mci_send(1,0x806,12,play)==0,"play before MCI pause");
    now+=73;dd2_cd_pump();
    require(dd2_mci_send(1,0x809,0,NULL)==0,"pause existing CD buffer");
    require(status(4)==529 && status(2)==(3u|5u<<24),"paused source position");
    now+=911;dd2_cd_pump();
    require(status(4)==529 && status(2)==(3u|5u<<24),"pause retains source cursor");
    require(dd2_mci_send(1,0x855,0,NULL)==0,"resume existing CD buffer");
    now+=27;dd2_cd_pump();
    require(status(4)==526 && status(2)==(3u|7u<<24),"resumed source position");
    require(dd2_mci_send(1,0x808,0,NULL)==0,"stop resumed buffer");
    /* Four sectors straddle a physical track boundary and the page reader
     * must switch files without repeating, dropping or inventing samples. */
    {
        unsigned sector=dd2_cd_sectors[2]-dd2_cd_sectors[1]-2;
        play[1]=2u|sector/4500u<<8|(sector/75u%60u)<<16|(sector%75u)<<24;
        play[2]=3u|2u<<24;
    }
    require(dd2_mci_send(1,0x806,12,play)==0,"play across physical tracks");
    now+=54;dd2_cd_pump();
    require(status(4)==525 && status(2)==(3u|2u<<24),"exact end after crossing tracks");
    /* Replay loop is requested by the engine, not silently invented by the device. */
    play[1]=2;play[2]=3;
    require(dd2_mci_send(1,0x806,12,play)==0,"restart finished track");
    require(status(4)==526 && status(2)==2,"replay starts at track boundary");
    require(dd2_mci_send(1,0x808,0,NULL)==0,"stop replay");
    play[1]=20; play[2]=19;
    require(dd2_mci_send(1,0x806,12,play)==282,"invalid TMSF track");
    play[1]=3; play[2]=2;
    require(dd2_mci_send(1,0x806,12,play)==282,"reversed playback interval");
    require(dd2_mci_send(2,0x814,0x100,set)==257,"invalid device");
    require(dd2_mci_send(1,0x814,0x100,NULL)==297,"null parameter block");
    require(dd2_mci_send(1,0x804,0,NULL)==0,"close device");
    require(dd2_mci_send(1,0x814,0x100,set)==257,"closed device status");
    setenv("DD2_CD_ROOT","/dd2-missing-test-disc",1);
    require(dd2_mci_send(0,0x803,0x2000,open)==276,"missing disc rejected");
    require(((unsigned(*)(void*))(*(void***)device)[2])(device)==0,"release shared audio device");
    puts("Redbook: all 18 complete tracks, exact positions, stop/resume, disc end, replay and errors passed");
    return 0;
}
