/* Forward Wine OS waits. Optional declared RAW-clock freeze affects only the
 * named MCI fixture; source packets, game code and PCM remain unchanged. */
#define _GNU_SOURCE
#include <sys/select.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <time.h>
#include <pthread.h>
#include <dlfcn.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static pthread_mutex_t frozen_lock=PTHREAD_MUTEX_INITIALIZER;
static uint64_t frozen_ns;
static int have_frozen;
static int selected(void){
    char name[16];const char* target=getenv("DD2_WAIT_PROCESS");
    return target && !prctl(PR_GET_NAME,name) && !strcmp(target,name);
}
static uint64_t freeze(uint64_t ns){
    if(!getenv("DD2_WAIT_FREEZE_RAW") || !selected())return ns;
    pthread_mutex_lock(&frozen_lock);
    if(!have_frozen){frozen_ns=ns;have_frozen=1;}
    ns=frozen_ns;pthread_mutex_unlock(&frozen_lock);return ns;
}
int clock_gettime(clockid_t id,struct timespec* value){
    static int (*next)(clockid_t,struct timespec*);
    int result,saved;uint64_t ns;
    if(!next)next=dlsym(RTLD_NEXT,"clock_gettime");
    if(!next)_exit(1);
    result=next(id,value);saved=errno;
    if(!result && id==CLOCK_MONOTONIC_RAW){
        ns=freeze((uint64_t)value->tv_sec*1000000000+value->tv_nsec);
        value->tv_sec=ns/1000000000;value->tv_nsec=ns%1000000000;
    }
    errno=saved;return result;
}
static uint64_t now_ns(void){
    struct timespec value;if(clock_gettime(CLOCK_MONOTONIC,&value))_exit(1);
    return (uint64_t)value.tv_sec*1000000000+value.tv_nsec;
}
static void record(uint64_t requested,uint64_t begin,uint64_t end,int result){
    const char* path=getenv("DD2_WAIT_CAPTURE");char row[256];int file,length;
    if(!path)_exit(1);
    length=snprintf(row,sizeof(row),"{\"pid\":%ld,\"tid\":%ld,\"requested_us\":%llu,\"begin_ns\":%llu,\"end_ns\":%llu,\"result\":%d}\n",
        (long)getpid(),(long)syscall(SYS_gettid),(unsigned long long)requested,(unsigned long long)begin,(unsigned long long)end,result);
    file=open(path,O_WRONLY|O_CREAT|O_APPEND,0600);
    if(file<0 || length<=0 || write(file,row,length)!=length)_exit(1);
    close(file);
}
int select(int nfds,fd_set* readfds,fd_set* writefds,fd_set* exceptfds,struct timeval* timeout){
    static int (*next)(int,fd_set*,fd_set*,fd_set*,struct timeval*);
    int observe,result,saved;uint64_t requested=0,begin=0;
    if(!next)next=dlsym(RTLD_NEXT,"select");
    if(!next)_exit(1);
    if(timeout)requested=(uint64_t)timeout->tv_sec*1000000+timeout->tv_usec;
    observe=!nfds && !readfds && !writefds && !exceptfds && requested>0 && requested<=100000 && selected();
    if(observe)begin=now_ns();
    result=next(nfds,readfds,writefds,exceptfds,timeout);saved=errno;
    if(observe)record(requested,begin,now_ns(),result);
    errno=saved;return result;
}
#if __SIZEOF_LONG__ == 4
typedef struct {int64_t sec;int32_t nsec,pad;} Clock64;
typedef struct {int64_t sec,usec;} Timeval64;
int __clock_gettime64(clockid_t id,Clock64* value){
    static int (*next)(clockid_t,Clock64*);
    int result,saved;uint64_t ns;
    if(!next)next=dlsym(RTLD_NEXT,"__clock_gettime64");
    if(!next)_exit(1);
    result=next(id,value);saved=errno;
    if(!result && id==CLOCK_MONOTONIC_RAW){
        ns=freeze((uint64_t)value->sec*1000000000+value->nsec);
        value->sec=ns/1000000000;value->nsec=ns%1000000000;
    }
    errno=saved;return result;
}
int __select64(int nfds,fd_set* readfds,fd_set* writefds,fd_set* exceptfds,Timeval64* timeout){
    static int (*next)(int,fd_set*,fd_set*,fd_set*,Timeval64*);
    int observe,result,saved;uint64_t requested=0,begin=0;
    if(!next)next=dlsym(RTLD_NEXT,"__select64");
    if(!next)_exit(1);
    if(timeout)requested=(uint64_t)timeout->sec*1000000+timeout->usec;
    observe=!nfds && !readfds && !writefds && !exceptfds && requested>0 && requested<=100000 && selected();
    if(observe)begin=now_ns();
    result=next(nfds,readfds,writefds,exceptfds,timeout);saved=errno;
    if(observe)record(requested,begin,now_ns(),result);
    errno=saved;return result;
}
#endif
