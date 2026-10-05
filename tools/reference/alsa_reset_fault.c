/* Scoped diagnostic fault: fail public ALSA reset in the selected process.
 * No source bytes, clocks, input or device pointers are rewritten. */
#define _GNU_SOURCE
#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
int snd_pcm_reset(snd_pcm_t* device){
    static __typeof__(snd_pcm_reset)* next;
    const char* path=getenv("DD2_RESET_FAULT_LOG");
    const char* process=getenv("DD2_AUDIO_PROCESS");
    char name[32]={0};FILE* comm;
    if(path && process && (comm=fopen("/proc/self/comm","r"))){
        int selected=fgets(name,sizeof(name),comm)!=NULL;
        fclose(comm);name[strcspn(name,"\n")]=0;
        if(selected && !strcmp(name,process)){
            struct timespec now;FILE* log=fopen(path,"a");
            if(!log)abort();
            clock_gettime(CLOCK_MONOTONIC,&now);
            fprintf(log,"{\"pid\":%d,\"time_ns\":%llu,\"event\":\"snd_pcm_reset\",\"result\":%d}\n",
                    (int)getpid(),(unsigned long long)((uint64_t)now.tv_sec*1000000000+now.tv_nsec),-EIO);
            if(fclose(log))abort();
            return -EIO;
        }
    }
    if(!next)next=dlsym(RTLD_NEXT,"snd_pcm_reset");
    return next?next(device):-ENOSYS;
}
