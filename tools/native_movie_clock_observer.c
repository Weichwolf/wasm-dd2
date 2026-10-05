/* Observe movie-clock calls in an unchanged non-PIE, frame-pointer native
 * binary. Caller bounds come from that binary's nm table. Forward the clock
 * unchanged; journaling cost remains part of this diagnostic run. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static uintptr_t lower,upper,us_lower,us_upper;
static FILE* journal;
__attribute__((constructor)) static void initialize(void){
    const char* path=getenv("DD2_MOVIE_CLOCK_LOG");
    if(!path)return;
    lower=strtoul(getenv("DD2_MOVIE_CLOCK_LOWER"),NULL,16);
    upper=strtoul(getenv("DD2_MOVIE_CLOCK_UPPER"),NULL,16);
    if(getenv("DD2_MOVIE_CLOCK_US_LOWER")){
        us_lower=strtoul(getenv("DD2_MOVIE_CLOCK_US_LOWER"),NULL,16);
        us_upper=strtoul(getenv("DD2_MOVIE_CLOCK_US_UPPER"),NULL,16);
        if(!us_lower || us_upper<=us_lower)abort();
    }
    journal=fopen(path,"wx");
    if(!lower || upper<=lower || !journal)abort();
}
int clock_gettime(clockid_t clock,struct timespec* stamp){
    static __typeof__(clock_gettime)* next;
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    int result,saved,selected;struct timespec before,after;
    if(!next)next=dlsym(RTLD_NEXT,"clock_gettime");
    if(!next)abort();
    selected=journal && (clock==CLOCK_MONOTONIC || clock==CLOCK_MONOTONIC_RAW) && ((caller>=lower && caller<upper) || (caller>=us_lower && caller<us_upper));
    if(selected && clock==CLOCK_MONOTONIC_RAW && next(CLOCK_MONOTONIC,&before))abort();
    result=next(clock,stamp);saved=errno;
    if(selected && !result){
        if(clock==CLOCK_MONOTONIC_RAW){if(next(CLOCK_MONOTONIC,&after))abort();}
        else before=after=*stamp;
        /* The verified caller is dd2_movie_now_ms/us, compiled with an EBP
         * frame. No other caller's stack is followed. */
        uintptr_t site=((uintptr_t*)__builtin_frame_address(1))[1];
        fprintf(journal,"{\"event\":\"movie_clock\",\"clock_domain\":\"%s\","
                "\"time_ns\":%llu,\"site\":%lu,\"monotonic_begin_ns\":%llu,\"monotonic_end_ns\":%llu}\n",
                clock==CLOCK_MONOTONIC_RAW?"CLOCK_MONOTONIC_RAW":"CLOCK_MONOTONIC",
                (unsigned long long)((uint64_t)stamp->tv_sec*1000000000+stamp->tv_nsec),
                (unsigned long)site,
                (unsigned long long)((uint64_t)before.tv_sec*1000000000+before.tv_nsec),
                (unsigned long long)((uint64_t)after.tv_sec*1000000000+after.tv_nsec));
    }
    errno=saved;return result;
}
