/* Observe forwarded producer waits with a declared 3-ms per-fill load model.
 * Source bytes and clocks are forwarded unchanged; this is a cadence test. */
#define _GNU_SOURCE
#include <SDL2/SDL.h>
#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static __thread int worker;
static __thread int nested_wait;
static __thread unsigned lifetime;
static unsigned serial;
static uint64_t now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
static FILE* journal(void){FILE* f=fopen(getenv("DD2_SCHEDULE_LOG"),"a");if(!f)abort();return f;}
static int real_sleep(const struct timespec* requested,struct timespec* remaining){
    static __typeof__(nanosleep)* next;if(!next)next=dlsym(RTLD_NEXT,"nanosleep");
    if(!next)abort();
    return next(requested,remaining);
}
int nanosleep(const struct timespec* requested,struct timespec* remaining){
    uint64_t begin=now(),end;int result=real_sleep(requested,remaining),saved=errno;end=now();
    if(worker && !nested_wait){FILE* f=journal();
        fprintf(f,"{\"event\":\"wait\",\"lifetime\":%u,\"begin_ns\":%llu,\"end_ns\":%llu,\"requested_ns\":%llu,\"result\":%d}\n",
                lifetime,(unsigned long long)begin,(unsigned long long)end,
                (unsigned long long)((uint64_t)requested->tv_sec*1000000000+requested->tv_nsec),result);fclose(f);}
    errno=saved;return result;
}
void SDL_Delay(Uint32 ms){
    static __typeof__(SDL_Delay)* next;uint64_t begin,end;
    if(!next)next=dlsym(RTLD_NEXT,"SDL_Delay");
    if(!next)abort();
    if(!worker){next(ms);return;}
    begin=now();nested_wait=1;next(ms);nested_wait=0;end=now();
    {FILE* f=journal();
     fprintf(f,"{\"event\":\"wait\",\"lifetime\":%u,\"begin_ns\":%llu,\"end_ns\":%llu,\"requested_ns\":%llu,\"result\":null}\n",
             lifetime,(unsigned long long)begin,(unsigned long long)end,(unsigned long long)ms*1000000);fclose(f);}
}
typedef struct {SDL_ThreadFunction fn;void* data;unsigned lifetime;} Launch;
static int observed(void* data){
    Launch call=*(Launch*)data;FILE* f;int result;free(data);worker=1;lifetime=call.lifetime;
    f=journal();fprintf(f,"{\"event\":\"thread-start\",\"lifetime\":%u,\"time_ns\":%llu}\n",lifetime,(unsigned long long)now());fclose(f);
    result=call.fn(call.data);
    f=journal();fprintf(f,"{\"event\":\"thread-end\",\"lifetime\":%u,\"time_ns\":%llu}\n",lifetime,(unsigned long long)now());fclose(f);
    worker=0;return result;
}
SDL_Thread* SDL_CreateThread(SDL_ThreadFunction fn,const char* name,void* data){
    static __typeof__(SDL_CreateThread)* next;Launch* call;SDL_Thread* thread;
    if(!next)next=dlsym(RTLD_NEXT,"SDL_CreateThread");
    if(!next)abort();
    if(!name || strcmp(name,"dd2-movie-alsa"))return next(fn,name,data);
    call=malloc(sizeof(*call));if(!call)abort();*call=(Launch){fn,data,serial++};
    thread=next(observed,name,call);if(!thread)free(call);return thread;
}
snd_pcm_sframes_t snd_pcm_avail_update(snd_pcm_t* device){
    static __typeof__(snd_pcm_avail_update)* next;uint64_t begin=now();snd_pcm_sframes_t result;
    if(!next)next=dlsym(RTLD_NEXT,"snd_pcm_avail_update");
    if(!next)abort();
    if(worker){struct timespec load={0,3000000},remaining;
        while(real_sleep(&load,&remaining)<0 && errno==EINTR)load=remaining;}
    result=next(device);
    if(worker){FILE* f=journal();fprintf(f,"{\"event\":\"work\",\"lifetime\":%u,\"begin_ns\":%llu,\"end_ns\":%llu}\n",
                                     lifetime,(unsigned long long)begin,(unsigned long long)now());fclose(f);}
    return result;
}
