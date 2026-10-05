/* Production movie clock provider, declared clock inputs and actual Wine QPC.
 * This clock boundary fixture is not complete original movie A/V acceptance. */
#include <stdio.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
int main(void){
    LARGE_INTEGER counter,frequency;unsigned i;
    if(!QueryPerformanceFrequency(&frequency))return 1;
    printf("{\"frequency\":%llu,\"counters\":[",(unsigned long long)frequency.QuadPart);
    for(i=0;i<16;i++){
        if(!QueryPerformanceCounter(&counter))return 1;
        printf("%s%llu",i?",":"",(unsigned long long)counter.QuadPart);
    }
    puts("]}");return 0;
}
#else
#include <time.h>
#include <errno.h>
#include <string.h>
#include "dd2_movie_platform.h"
static int mocked,raw_failed;
static struct timespec raw_time,mono_time;
static struct {int id,result;uint64_t ns;} observations[8];
static unsigned count;
int __real_clock_gettime(clockid_t,struct timespec*);
int __wrap_clock_gettime(clockid_t id,struct timespec* out){
    int result;
    if(count>=8)return -1;
    if(!mocked)result=__real_clock_gettime(id,out);
    else if(id==CLOCK_MONOTONIC_RAW && raw_failed){errno=EINVAL;result=-1;}
    else if(id==CLOCK_MONOTONIC_RAW){*out=raw_time;result=0;}
    else if(id==CLOCK_MONOTONIC){*out=mono_time;result=0;}
    else {errno=EINVAL;result=-1;}
    observations[count].id=id;observations[count].result=result;
    observations[count].ns=result?0:(uint64_t)out->tv_sec*1000000000+out->tv_nsec;
    count++;return result;
}
static void emit(unsigned index){
    unsigned value,i;count=0;value=dd2_movie_now_ms();
    printf("{\"case\":%u,\"movie_ms\":%u,\"raw_failed\":%s,\"calls\":[",index,value,raw_failed?"true":"false");
    for(i=0;i<count;i++)printf("%s{\"clock\":%d,\"result\":%d,\"ns\":%llu}",i?",":"",observations[i].id,observations[i].result,(unsigned long long)observations[i].ns);
    puts("]}");
}
int main(int argc,char** argv){
    static const struct timespec cases[]={{0,0},{0,999999},{0,1000000},{0,999999999},
        {1234,987654321},{4294967,294999999},{4294967,295000000},{4294967,295999999},
        {4294967,296000000},{4294967,999999999},{9000000,999999999}};
    unsigned i;
    if(argc>1 && !strcmp(argv[1],"live")){emit(0);return 0;}
    mocked=1;
    for(raw_failed=0;raw_failed<=1;raw_failed++)for(i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
        raw_time=cases[i];mono_time.tv_sec=raw_time.tv_sec+123;
        mono_time.tv_nsec=(raw_time.tv_nsec+123456789)%1000000000;
        emit((unsigned)raw_failed*11+i);
    }
    return 0;
}
#endif
