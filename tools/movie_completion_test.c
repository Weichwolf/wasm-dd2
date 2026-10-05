/* Gate only the notification/producer worker; caller PCM remains unchanged.
 * Independent journals must prove complete accepted and consumed source PCM. */
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdint.h>
static HANDLE entered,release;
static volatile LONG callbacks;
static void CALLBACK completed(HWAVEOUT device,UINT message,DWORD_PTR user,DWORD_PTR a,DWORD_PTR b){
    (void)device;(void)user;(void)a;(void)b;
    if(message==WOM_DONE && InterlockedIncrement(&callbacks)==1){
        SetEvent(entered);WaitForSingleObject(release,2000);
    }
}
static void require(int ok,const char *why){if(!ok){fprintf(stderr,"completion: %s\n",why);ExitProcess(1);}}
int main(void){
    WAVEFORMATEX format={WAVE_FORMAT_PCM,2,22050,88200,4,16,0};
    HWAVEOUT device;WAVEHDR headers[2]={{0}};int16_t source[882*2];MMTIME position={0};
    DWORD start;unsigned i;int pending;
    entered=CreateEventW(NULL,TRUE,FALSE,NULL);release=CreateEventW(NULL,TRUE,FALSE,NULL);
    require(entered && release,"create gates");
    for(i=0;i<882;i++){source[2*i]=12345+i;source[2*i+1]=-23456+i;}
    require(!waveOutOpen(&device,WAVE_MAPPER,&format,(DWORD_PTR)completed,0,CALLBACK_FUNCTION),"waveOutOpen");
    for(i=0;i<2;i++){
        headers[i].lpData=(char *)(source+i*441*2);headers[i].dwBufferLength=441*4;
        require(!waveOutPrepareHeader(device,&headers[i],sizeof(WAVEHDR)),"prepare header");
        require(!waveOutWrite(device,&headers[i],sizeof(WAVEHDR)),"write header");
    }
    require(WaitForSingleObject(entered,2000)==WAIT_OBJECT_0,"first completion callback");
    Sleep(80); /* declared blocked callback; device thread remains running */
    start=GetTickCount();
    do{
        position.wType=TIME_SAMPLES;
        require(!waveOutGetPosition(device,&position,sizeof(position)) && position.wType==TIME_SAMPLES,"sample position");
        require(GetTickCount()-start<1000,"hardware source completion");
        if(position.u.sample>=882)break;
        Sleep(1);
    }while(1);
    pending=!(headers[1].dwFlags & WHDR_DONE);
    require(pending,"second header remains pending while notification worker blocked");
    require(callbacks==1,"only first callback published");
    SetEvent(release);start=GetTickCount();
    while(!(headers[1].dwFlags&WHDR_DONE) || callbacks!=2){require(GetTickCount()-start<1000,"publish second completion");Sleep(1);}
    require(!waveOutReset(device),"reset");
    for(i=0;i<2;i++)require(!waveOutUnprepareHeader(device,&headers[i],sizeof(WAVEHDR)),"unprepare");
    require(!waveOutClose(device),"close");
    printf("{\"source_frames\":882,\"waveout_position_frames\":%lu,\"pending_before_publication\":%s,\"callbacks\":%ld}\n",position.u.sample,pending?"true":"false",callbacks);
    CloseHandle(entered);CloseHandle(release);return 0;
}
#else
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static SDL_sem *entered,*release;
static int gated;
static int fixture_nanosleep(const struct timespec* requested,struct timespec* remaining){
    if(!gated){gated=1;SDL_SemPost(entered);if(SDL_SemWaitTimeout(release,2000))exit(2);}
    return nanosleep(requested,remaining);
}
#define nanosleep fixture_nanosleep
#include "dd2_native_movie_alsa.h"
#undef nanosleep
static void require(int ok,const char* why){if(!ok){fprintf(stderr,"completion: %s\n",why);exit(1);}}
int main(int argc,char** argv){
    int16_t source[882*2];unsigned i;Uint32 started;int early,later;snd_pcm_sframes_t pending=0;size_t index;int published,status;
    require(!SDL_Init(SDL_INIT_TIMER),"init");entered=SDL_CreateSemaphore(0);release=SDL_CreateSemaphore(0);
    for(i=0;i<882;i++){source[2*i]=12345+i;source[2*i+1]=-23456+i;}
    require(!movie_alsa_start(source,882,22050),"start");
    require(!SDL_SemWaitTimeout(entered,1000),"producer gated");started=SDL_GetTicks();
    do{
        status=movie_snd_pcm_delay(movie_alsa.device,&pending);
        if(status==-EPIPE)pending=0;else require(status==0,"device pending");
        require(pending>=0,"nonnegative device pending");
        require(SDL_GetTicks()-started<1000,"source playback timeout");
        if(!pending)break;SDL_Delay(1);
    }while(1);
    early=movie_alsa_done();
    SDL_LockMutex(movie_alsa.mutex);index=movie_alsa.index;published=movie_alsa.source_done;SDL_UnlockMutex(movie_alsa.mutex);
    require(!published,"producer has not published completion");
    require(early==(argc>1?atoi(argv[1]):1),"completion before publication");
    SDL_SemPost(release);started=SDL_GetTicks();
    do{later=movie_alsa_done();if(later)break;require(SDL_GetTicks()-started<1000,"published completion timeout");SDL_Delay(1);}while(1);
    /* Let the producer finish at least one period even with the early-poll header. */
    started=SDL_GetTicks();
    do{SDL_LockMutex(movie_alsa.mutex);published=movie_alsa.source_done;SDL_UnlockMutex(movie_alsa.mutex);
        if(published)break;require(SDL_GetTicks()-started<1000,"queue completion timeout");SDL_Delay(1);
    }while(1);
    require(later==1,"source completion");SDL_SemPost(release);movie_alsa_stop();
    printf("{\"source_frames\":882,\"submitted_frames\":%zu,\"pending_frames\":%ld,\"done_before_publication\":%d,\"done_after_publication\":%d}\n",index,(long)pending,early,later);
    SDL_DestroySemaphore(entered);SDL_DestroySemaphore(release);SDL_Quit();return 0;
}
#endif
