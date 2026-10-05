/* Actual Wine WaveOut producer observation, and production native worker with
 * declared clock inputs. Controlled cases do not prove live movie A/V parity. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
int main(void){
    WAVEFORMATEX format={WAVE_FORMAT_PCM,2,22050,88200,4,16,0};
    HWAVEOUT device;WAVEHDR header={0};int16_t source[2205*2];unsigned i;
    LARGE_INTEGER frequency;
    for(i=0;i<2205;i++){source[i*2]=12345+i;source[i*2+1]=-23456+i;}
    if(waveOutOpen(&device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL))return 1;
    header.lpData=(char*)source;header.dwBufferLength=sizeof(source);
    if(waveOutPrepareHeader(device,&header,sizeof(header)) ||
       waveOutWrite(device,&header,sizeof(header)))return 2;
    Sleep(180);
    if(!(header.dwFlags&WHDR_DONE))return 3;
    if(waveOutReset(device) || waveOutUnprepareHeader(device,&header,sizeof(header)) ||
       waveOutClose(device) || !QueryPerformanceFrequency(&frequency))return 4;
    printf("{\"source_frames\":2205,\"frequency\":%llu,\"header_done\":true}\n",
           (unsigned long long)frequency.QuadPart);
    return 0;
}
#else
#include <SDL2/SDL.h>
#include <time.h>
#include <errno.h>
static uint64_t raw[4],mono[4],sleep_ns[4];
static unsigned sample,sleeps,clock_calls;
static int fallback,clocks[8];
static void fixture_stop(void);
static int fixture_clock(clockid_t clock,struct timespec* ts){
    uint64_t ns;
    if(clock_calls>=8 || sample>=4)exit(2);
    clocks[clock_calls++]=clock;
    if(clock==CLOCK_MONOTONIC_RAW && fallback){errno=EINVAL;return -1;}
    if(clock!=CLOCK_MONOTONIC_RAW && clock!=CLOCK_MONOTONIC)exit(3);
    ns=(clock==CLOCK_MONOTONIC_RAW?raw:mono)[sample++];
    ts->tv_sec=ns/1000000000;ts->tv_nsec=ns%1000000000;return 0;
}
static int fixture_sleep(const struct timespec* ts,struct timespec* remaining){
    (void)remaining;
    if(sleeps>=4 || ts->tv_sec || ts->tv_nsec<=0)exit(4);
    sleep_ns[sleeps++]=(uint64_t)ts->tv_nsec;
    if(sleeps==4)fixture_stop();
    return 0;
}
#define clock_gettime fixture_clock
#define nanosleep fixture_sleep
#include "dd2_native_movie_alsa.h"
#undef nanosleep
#undef clock_gettime
static void fixture_stop(void){SDL_AtomicSet(&movie_alsa.stop,1);}
static snd_pcm_sframes_t no_data(snd_pcm_t* device){(void)device;return 0;}
static void emit(unsigned id){
    unsigned i;
    sample=sleeps=clock_calls=0;memset(&movie_alsa,0,sizeof(movie_alsa));
    movie_alsa.mutex=SDL_CreateMutex();if(!movie_alsa.mutex)exit(5);
    movie_snd_pcm_avail_update=no_data;
    if(movie_alsa_worker(NULL))exit(6);
    SDL_DestroyMutex(movie_alsa.mutex);
    printf("{\"case\":%u,\"fallback\":%d,\"raw_ns\":[",id,fallback);
    for(i=0;i<4;i++)printf("%s%llu",i?",":"",(unsigned long long)raw[i]);
    printf("],\"mono_ns\":[");
    for(i=0;i<4;i++)printf("%s%llu",i?",":"",(unsigned long long)mono[i]);
    printf("],\"clock_calls\":[");
    for(i=0;i<clock_calls;i++)printf("%s%d",i?",":"",clocks[i]);
    printf("],\"sleep_ns\":[");
    for(i=0;i<sleeps;i++)printf("%s%llu",i?",":"",(unsigned long long)sleep_ns[i]);
    puts("]}");
}
int main(void){
    static const uint64_t offsets[][4]={
        {0,10000000,20000000,30000000},
        {79,10000109,20000139,30000169},
        {999999989,1009999989,1019999989,1029999989},
        {0,4000000,8000000,12000000},
        {0,16000000,32000000,48000000},
        {0,0,0,0},
        {0,15000000,30000000,45000000},
        {99,15000099,30000099,45000099},
        {0,15000100,30000200,45000300},
        {0,214758364700ULL,214768364700ULL,214778364700ULL},
        {0,214758364800ULL,214768364800ULL,214778364800ULL},
        {0,214758364900ULL,214768364900ULL,214778364900ULL}
    };
    unsigned c,i;
    if(SDL_Init(SDL_INIT_TIMER))return 1;
    for(fallback=0;fallback<2;fallback++)for(c=0;c<sizeof(offsets)/sizeof(offsets[0]);c++){
        for(i=0;i<4;i++){
            raw[i]=1000000000000ULL+offsets[c][i];
            /* An independently adjusted MONOTONIC rate must not enter QPC
             * correction unless the declared RAW call fails. */
            mono[i]=2000000000000ULL+offsets[c][i]+i*100001;
        }
        emit(c);
    }
    SDL_Quit();return 0;
}
#endif
