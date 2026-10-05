/* Forward Linux clocks and ALSA producer queries unchanged in a named probe.
 * Thread IDs correlate actual Wine timer calls with the source PCM device.
 * No application state, clock values or buffers are rewritten. */
#define _GNU_SOURCE
#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <time.h>
#include <errno.h>
static __thread int busy;
static void record(const char* event,int clock,uint64_t ns){
    const char* target=getenv("DD2_WORKER_PROCESS"),*path=getenv("DD2_WORKER_CAPTURE");
    char name[32],row[240];FILE* process;int file,length,matches;
    if(busy || !target || !path)return;
    busy=1;process=fopen("/proc/self/comm","r");
    matches=process && fgets(name,sizeof(name),process);
    if(process)fclose(process);
    if(matches){name[strcspn(name,"\n")]=0;matches=!strcmp(name,target);}
    if(matches){
        length=snprintf(row,sizeof(row),"{\"event\":\"%s\",\"pid\":%ld,\"tid\":%ld,\"clock\":%d,\"ns\":%llu}\n",
            event,(long)getpid(),(long)syscall(SYS_gettid),clock,(unsigned long long)ns);
        file=open(path,O_WRONLY|O_CREAT|O_APPEND,0600);
        if(file<0 || length<=0 || write(file,row,length)!=length)_exit(1);
        close(file);
    }
    busy=0;
}
int clock_gettime(clockid_t id,struct timespec* value){
    static int(*next)(clockid_t,struct timespec*);int result,saved;
    if(!next)next=dlsym(RTLD_NEXT,"clock_gettime");
    if(!next)_exit(1);
    result=next(id,value);saved=errno;
    if(!result && (id==CLOCK_MONOTONIC_RAW || id==CLOCK_MONOTONIC))
        record("clock",id,(uint64_t)value->tv_sec*1000000000+value->tv_nsec);
    errno=saved;return result;
}
#if __SIZEOF_LONG__ == 4
typedef struct {int64_t sec;int32_t nsec,padding;} Clock64;
int __clock_gettime64(clockid_t id,Clock64* value){
    static int(*next)(clockid_t,Clock64*);int result,saved;
    if(!next)next=dlsym(RTLD_NEXT,"__clock_gettime64");
    if(!next)_exit(1);
    result=next(id,value);saved=errno;
    if(!result && (id==CLOCK_MONOTONIC_RAW || id==CLOCK_MONOTONIC))
        record("clock",id,(uint64_t)value->sec*1000000000+value->nsec);
    errno=saved;return result;
}
#endif
snd_pcm_sframes_t snd_pcm_avail_update(snd_pcm_t* pcm){
    static __typeof__(snd_pcm_avail_update)* next;
    if(!next)next=dlsym(RTLD_NEXT,"snd_pcm_avail_update");
    /* Wine loads ALSA locally; its symbols need not enter RTLD_NEXT's scope. */
    if(!next){
        void* library=dlopen("libasound.so.2",RTLD_LAZY|RTLD_LOCAL);
        if(library)next=dlsym(library,"snd_pcm_avail_update");
    }
    if(!next)_exit(1);
    record("avail",-1,0);return next(pcm);
}
int snd_pcm_recover(snd_pcm_t* pcm,int error,int silent){
    static __typeof__(snd_pcm_recover)* next;int result;
    if(!next)next=dlsym(RTLD_NEXT,"snd_pcm_recover");
    if(!next){
        void* library=dlopen("libasound.so.2",RTLD_LAZY|RTLD_LOCAL);
        if(library)next=dlsym(library,"snd_pcm_recover");
    }
    if(!next)_exit(1);
    record("recover",error,0);result=next(pcm,error,silent);
    record("recover_return",result,0);return result;
}
