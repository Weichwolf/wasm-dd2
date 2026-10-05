/* Forward actual Linux clock calls in the named Wine client only.
 * No time values, game code, memory, inputs or PCM are rewritten. */
#define _GNU_SOURCE
#include <time.h>
#include <dlfcn.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
static __thread int recording;
static void record(clockid_t id,uint64_t ns){
    if(!recording && (id==CLOCK_MONOTONIC_RAW || id==CLOCK_MONOTONIC)){
        const char* target=getenv("DD2_QPC_PROCESS"),*path=getenv("DD2_QPC_CAPTURE");
        char name[16];
        if(target && path && !prctl(PR_GET_NAME,name) && !strcmp(name,target)){
            char row[192];int file,length;
            recording=1;
            length=snprintf(row,sizeof(row),"{\"pid\":%ld,\"clock\":%d,\"ns\":%llu}\n",(long)getpid(),(int)id,
                (unsigned long long)ns);
            file=open(path,O_WRONLY|O_CREAT|O_APPEND,0600);
            if(file<0 || length<=0 || write(file,row,length)!=length)_exit(1);
            close(file);recording=0;
        }
    }
}
int clock_gettime(clockid_t id,struct timespec* value){
    static int (*next)(clockid_t,struct timespec*);
    int result,saved;
    if(!next)next=dlsym(RTLD_NEXT,"clock_gettime");
    if(!next)_exit(1);
    result=next(id,value);saved=errno;
    if(!result)record(id,(uint64_t)value->tv_sec*1000000000+value->tv_nsec);
    errno=saved;return result;
}
#if __SIZEOF_LONG__ == 4
/* Debian Wine i386 uses glibc's 64-bit-time ABI. On little-endian i386
 * tv_sec is 64 bits; tv_nsec is a long followed by padding. */
typedef struct {int64_t sec;int32_t nsec,padding;} Clock64;
int __clock_gettime64(clockid_t id,Clock64* value){
    static int (*next)(clockid_t,Clock64*);
    int result,saved;
    if(!next)next=dlsym(RTLD_NEXT,"__clock_gettime64");
    if(!next)_exit(1);
    result=next(id,value);saved=errno;
    if(!result)record(id,(uint64_t)value->sec*1000000000+value->nsec);
    errno=saved;return result;
}
#endif
