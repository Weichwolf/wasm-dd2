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
static uintptr_t lower,upper;
static FILE* journal;
__attribute__((constructor)) static void initialize(void){
    const char* path=getenv("DD2_MOVIE_CLOCK_LOG");
    if(!path)return;
    lower=strtoul(getenv("DD2_MOVIE_CLOCK_LOWER"),NULL,16);
    upper=strtoul(getenv("DD2_MOVIE_CLOCK_UPPER"),NULL,16);
    journal=fopen(path,"wx");
    if(!lower || upper<=lower || !journal)abort();
}
int clock_gettime(clockid_t clock,struct timespec* stamp){
    static __typeof__(clock_gettime)* next;
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    int result,saved;
    if(!next)next=dlsym(RTLD_NEXT,"clock_gettime");
    if(!next)abort();
    result=next(clock,stamp);saved=errno;
    if(journal && clock==CLOCK_MONOTONIC && !result && caller>=lower && caller<upper){
        /* The verified caller is dd2_movie_now_ms, compiled with an EBP
         * frame. No other caller's stack is followed. */
        uintptr_t site=((uintptr_t*)__builtin_frame_address(1))[1];
        fprintf(journal,"{\"event\":\"movie_clock\",\"clock_domain\":\"CLOCK_MONOTONIC\","
                "\"time_ns\":%llu,\"site\":%lu}\n",
                (unsigned long long)((uint64_t)stamp->tv_sec*1000000000+stamp->tv_nsec),
                (unsigned long)site);
    }
    errno=saved;return result;
}
