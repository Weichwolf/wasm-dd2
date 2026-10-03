/* Actual DirectSound interface lifetime plus controlled port device epochs.
 * A second default interface keeps the shared device/buffers alive. Final
 * release destroys the remaining buffers, including the engine's primary.
 * The PCM test deliberately includes a long closed-device movie interval. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#define CALL WINAPI
__declspec(dllimport) int CALL DirectSoundCreate(void*,void**,void*);
#else
#define CALL
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
int DirectSoundCreate(int,void**,int);
void dd2_snd_mix_flip(void);
#include "dd2_sound.h"
static float music_read(void* context,uint64_t frame,int channel){
    (void)context;(void)frame;(void)channel;return 1234.0f/32768;
}
static void music_consume(void* context,unsigned first,unsigned frames){
    (void)context;(void)first;(void)frames;
}
static unsigned now,accepted,stops,open_device;
unsigned dd2_platform_ms(void){return now;}
void FUN_0041345c(void){abort();}
int dd2_native_enabled(void){return 1;}
void dd2_native_audio(const float* pcm,unsigned frames,unsigned rate){
    unsigned i;
    if(rate!=44100)abort();
    for(i=0;i<frames*2;i++)if(pcm[i]!=1234.0f/32768){
        fprintf(stderr,"Sound device test: queued sample crosses the closed device epoch\n");exit(1);
    }
    accepted+=frames;open_device=1;
}
void dd2_native_audio_stop(void){if(open_device){stops++;open_device=0;}}
#endif
#define METHOD(object,index,type) ((type)(*(void***)(object))[index])
typedef int(CALL *Create)(void*,void*,void**,int);
typedef unsigned(CALL *Reference)(void*);
typedef int(CALL *Lock)(void*,unsigned,unsigned,void**,unsigned*,void**,unsigned*,int);
typedef int(CALL *Unlock)(void*,void*,unsigned,void*,unsigned);
typedef int(CALL *Play)(void*,int,int,int);
typedef int(CALL *Position)(void*,unsigned*,unsigned*);
static void require(int ok,const char* message){
    if(!ok){fprintf(stderr,"Sound device test: %s\n",message);exit(1);}
}
static void* buffer(void* device,unsigned flags){
    unsigned char wave[18]={1,0,1,0,0x44,0xac,0,0,0x88,0x58,1,0,2,0,16,0,0,0};
    uint32_t desc[5]={20,flags,flags==1?0:128,0,flags==1?0:(uint32_t)(uintptr_t)wave};
    void* result;
    require(!METHOD(device,3,Create)(device,desc,&result,0),"create buffer");
    if(flags!=1){
        void *a,*b;unsigned na,nb,i;
        require(!METHOD(result,11,Lock)(result,0,128,&a,&na,&b,&nb,0) && na==128 && !nb,"lock source");
        for(i=0;i<64;i++)((short*)a)[i]=1234;
        require(!METHOD(result,19,Unlock)(result,a,na,b,nb),"unlock source");
    }
    return result;
}
int main(int argc,char** argv){
    void *first,*second,*sample,*primary,*reopened;
    unsigned added,retained,final,play,write;
#ifndef _WIN32
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map engine clock");
#endif
    *(int*)0x462ff0=0;
    setenv("DD2_SOUND","1",1);setenv("DD2_REALTIME","1",1);
    if(argc==2)setenv("DD2_MIXPCM",argv[1],1);
#else
    (void)argc;(void)argv;
#endif
    require(!DirectSoundCreate(0,&first,0),"create first interface");
    primary=buffer(first,1);sample=buffer(first,0x20);
    require(!DirectSoundCreate(0,&second,0),"create second default interface");
    added=METHOD(first,1,Reference)(first);require(added==2,"device AddRef");
    retained=METHOD(first,2,Reference)(first);require(retained==1,"nonfinal device Release");
    final=METHOD(first,2,Reference)(first);require(final==0,"final interface Release");
    require(!METHOD(sample,4,Position)(sample,&play,&write),"shared device survives first interface Release");
#ifndef _WIN32
    require(!METHOD(sample,12,Play)(sample,0,0,1),"play first epoch");
    now=9;dd2_snd_mix_flip(); /* 396 frames plus a 900/1000-frame remainder. */
#endif
    /* The original loses the primary pointer and leaves loaded master banks
     * to the device's final destruction. Do not dereference them afterwards. */
    (void)primary;
    require(METHOD(second,2,Reference)(second)==0,"final shared device Release");
#ifndef _WIN32
    now=70000;dd2_snd_mix_flip(); /* No device: none of this interval is audio. */
#endif
    require(!DirectSoundCreate(0,&reopened,0),"reopen device after movie");
    primary=buffer(reopened,1);sample=buffer(reopened,0x20);
#ifndef _WIN32
    require(!METHOD(sample,12,Play)(sample,0,0,1),"play second epoch");
    now=70001;dd2_snd_mix_flip(); /* Fresh fraction: 44 frames, not 45. */
#endif
    (void)primary;
    require(METHOD(reopened,2,Reference)(reopened)==0,"release reopened device");
#ifndef _WIN32
    /* MCI CD owns a separate source lifetime. Removing DirectSound while
     * this source is active must keep its clock and shared output alive. */
    {
        void* music=dd2_snd_music_create(128,music_read,music_consume,NULL);
        dd2_snd_music_play(music);
        now=70002;dd2_snd_mix_flip();
        require(!DirectSoundCreate(0,&reopened,0),"open DS alongside active MCI source");
        primary=buffer(reopened,1);
        require(METHOD(reopened,2,Reference)(reopened)==0,"release DS alongside active MCI source");
        now=70003;dd2_snd_mix_flip();
        dd2_snd_music_destroy(music);
    }
#endif
#ifdef DD2_NATIVE_SDL
    require(accepted==528 && stops==3 && !open_device,"source bytes or sink lifetime crosses idle epoch or kills independent MCI music");
#endif
    printf("{\"device_addref\":%u,\"device_release_retained\":%u,\"interface_final_release\":%u,\"shared_device_survived\":true,\"reopened\":true}\n",added,retained,final);
    return 0;
}
