/* Exercise the production ALSA movie device with caller PCM from sample zero.
 * The fault build only injects EIO after a successful device submission. */
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if defined(DD2_MOVIE_DEVICE_FAULT) || defined(DD2_MOVIE_DEVICE_DELAY) || defined(DD2_MOVIE_DEVICE_JOIN)
#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <errno.h>
#ifdef DD2_MOVIE_DEVICE_FAULT
snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t* device,const void* buffer,snd_pcm_uframes_t frames){
    static __typeof__(snd_pcm_writei)* next;
    static unsigned calls;
    if(!next)next=dlsym(RTLD_NEXT,"snd_pcm_writei");
    if(!next || getenv("DD2_MOVIE_DEVICE_FAIL_START") || ++calls>1)return -EIO;
    return next(device,buffer,frames);
}
#elif defined(DD2_MOVIE_DEVICE_JOIN)
#include <time.h>
void SDL_WaitThread(SDL_Thread* thread,int* status){
    static __typeof__(SDL_WaitThread)* next;
    uint64_t begin,end;struct timespec now;FILE* log;
    if(!next)next=dlsym(RTLD_NEXT,"SDL_WaitThread");
    if(!next)exit(1);
    clock_gettime(CLOCK_MONOTONIC,&now);begin=(uint64_t)now.tv_sec*1000000000+now.tv_nsec;
    SDL_Delay(80); /* explicit cleanup delay; source data is unchanged */
    next(thread,status);
    clock_gettime(CLOCK_MONOTONIC,&now);end=(uint64_t)now.tv_sec*1000000000+now.tv_nsec;
    log=fopen(getenv("DD2_MOVIE_DEVICE_JOIN_LOG"),"a");if(!log)exit(1);
    fprintf(log,"{\"join_begin_ns\":%llu,\"join_end_ns\":%llu}\n",(unsigned long long)begin,(unsigned long long)end);
    fclose(log);
}
#else
#include <time.h>
SDL_Thread* SDL_CreateThread(SDL_ThreadFunction fn,const char* name,void* data){
    static __typeof__(SDL_CreateThread)* next;
    SDL_Thread* thread;struct timespec now;FILE* log;
    if(!next)next=dlsym(RTLD_NEXT,"SDL_CreateThread");
    if(!next || getenv("DD2_MOVIE_DEVICE_FAIL_THREAD"))return NULL;
    thread=next(fn,name,data);
    SDL_Delay(80); /* explicit busy-start model; no sample data/time fitting */
    clock_gettime(CLOCK_MONOTONIC,&now);
    log=fopen(getenv("DD2_MOVIE_DEVICE_DELAY_LOG"),"a");if(!log)exit(1);
    fprintf(log,"{\"ready_ns\":%llu}\n",(unsigned long long)((uint64_t)now.tv_sec*1000000000+now.tv_nsec));
    fclose(log);return thread;
}
#endif
#else
#include "dd2_native_movie_alsa.h"
static void require(int ok,const char* why){
    if(!ok){fprintf(stderr,"native movie device: %s\n",why);exit(1);}
}
int main(int argc,char** argv){
    int16_t source[22050*2];unsigned i,round;Uint32 started;
    const char* mode=argc>1?argv[1]:"full";
    int complete=!strcmp(mode,"full") || !strcmp(mode,"tail");
    require(!SDL_Init(SDL_INIT_TIMER),"SDL timer initialization");
    if(!strcmp(mode,"unavailable") || !strcmp(mode,"initial-error") || !strcmp(mode,"thread-error")){
        for(i=0;i<2205*2;i++)source[i]=12345;
        require(movie_alsa_start(source,2205,22050)==-1,"reject unavailable audio device");
        require(!movie_alsa.device && !movie_alsa.thread,"failed start cleans up");
        printf("{\"%s_rejected\":true}\n",!strcmp(mode,"unavailable")?"unavailable":
               !strcmp(mode,"initial-error")?"initial_error":"thread_error");SDL_Quit();return 0;
    }
    require(complete || !strcmp(mode,"cancel") || !strcmp(mode,"error"),"known mode");
    for(round=0;round<(complete?2u:1u);round++){
        int done;size_t frames=complete?2205:22050;
        for(i=0;i<frames;i++){
            source[i*2]=(int16_t)(12345+(i%2205)+round*4000);
            source[i*2+1]=(int16_t)(-23456+(i%2205));
        }
        started=SDL_GetTicks();
        require(!movie_alsa_start(source,frames,22050),"start caller PCM");
        do{
            done=movie_alsa_done();
            if(!strcmp(mode,"cancel") && SDL_GetTicks()-started>=15)break;
            if(done)break;
            require(SDL_GetTicks()-started<2000,"device completion timeout");
            SDL_Delay(1);
        }while(1);
        if(complete)require(done==1,"complete source playback");
        else if(!strcmp(mode,"error"))require(done==-1,"report actual device error");
        else require(done==0,"cancel while playing");
        if(!strcmp(mode,"tail"))SDL_Delay(45); /* observe driver output after the source drains */
        printf("{\"round\":%u,\"mode\":\"%s\",\"elapsed_ms\":%u,\"done\":%d}\n",
               round,mode,SDL_GetTicks()-started,done);
        started=SDL_GetTicks();movie_alsa_stop();
        require(!movie_alsa.device && !movie_alsa.thread,"closed device and joined producer");
        require(SDL_GetTicks()-started<1000,"bounded close");
        movie_alsa_stop(); /* repeated close must remain safe */
    }
    SDL_Quit();return 0;
}
#endif
