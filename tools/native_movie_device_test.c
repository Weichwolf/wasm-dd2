/* Exercise the production ALSA movie device with caller PCM from sample zero.
 * The fault build only injects EIO after a successful device submission. */
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef DD2_MOVIE_DEVICE_FAULT
#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <errno.h>
snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t* device,const void* buffer,snd_pcm_uframes_t frames){
    static __typeof__(snd_pcm_writei)* next;
    static unsigned calls;
    if(!next)next=dlsym(RTLD_NEXT,"snd_pcm_writei");
    if(!next || ++calls>1)return -EIO;
    return next(device,buffer,frames);
}
#else
#include "dd2_native_movie_alsa.h"
static void require(int ok,const char* why){
    if(!ok){fprintf(stderr,"native movie device: %s\n",why);exit(1);}
}
int main(int argc,char** argv){
    int16_t source[22050*2];unsigned i,round;Uint32 started;
    const char* mode=argc>1?argv[1]:"full";
    require(!SDL_Init(SDL_INIT_TIMER),"SDL timer initialization");
    if(!strcmp(mode,"unavailable")){
        require(movie_alsa_start(source,2205,22050)==-1,"reject unavailable audio device");
        require(!movie_alsa.device && !movie_alsa.thread,"failed start cleans up");
        puts("{\"unavailable_rejected\":true}");SDL_Quit();return 0;
    }
    require(!strcmp(mode,"full") || !strcmp(mode,"cancel") || !strcmp(mode,"error"),"known mode");
    for(round=0;round<(!strcmp(mode,"full")?2u:1u);round++){
        int done;size_t frames=!strcmp(mode,"full")?2205:22050;
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
        if(!strcmp(mode,"full"))require(done==1,"complete source playback");
        else if(!strcmp(mode,"error"))require(done==-1,"report actual device error");
        else require(done==0,"cancel while playing");
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
