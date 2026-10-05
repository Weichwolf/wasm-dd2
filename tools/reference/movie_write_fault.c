/* Inject one declared ALSA result before accepting source sample zero.
 * Forward the retry and all other calls unchanged; log actual recovery calls.
 * The default selects the dedicated nonzero PCM fixture; whole-movie captures
 * explicitly select their first write with DD2_RECOVERY_ANY_SOURCE. */
#define _GNU_SOURCE
#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static snd_pcm_t* failed_device;
static int failed,retried;
static int active(void){
    char name[32];FILE* file;int result;
    const char* target=getenv("DD2_AUDIO_PROCESS");
    if(!target || !(file=fopen("/proc/self/comm","r")))return 0;
    result=fgets(name,sizeof(name),file)!=NULL;fclose(file);
    if(result)name[strcspn(name,"\n")]=0;
    return result&&!strcmp(name,target);
}
static void record(const char* event,long result,int silent,unsigned long frames){
    FILE* file=fopen(getenv("DD2_RECOVERY_LOG"),"a");if(!file)abort();
    fprintf(file,"{\"event\":\"%s\",\"result\":%ld,\"silent\":%d,\"frames\":%lu,\"pid\":%d}\n",
            event,result,silent,frames,(int)getpid());fclose(file);
}
snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t* pcm,const void* data,snd_pcm_uframes_t frames){
    static __typeof__(snd_pcm_writei)* next;
    const int16_t* samples=data;
    int source=frames&&(getenv("DD2_RECOVERY_ANY_SOURCE") ||
                       (samples[0]==12345&&samples[1]==-23456));
    snd_pcm_sframes_t result;
    if(!next)next=dlsym(RTLD_NEXT,"snd_pcm_writei");
    if(!next)abort();
    if(source&&!failed&&active()){
        failed=1;failed_device=pcm;result=-atoi(getenv("DD2_RECOVERY_ERRNO"));
        record("fault",result,-1,(unsigned long)frames);return result;
    }
    result=next(pcm,data,frames);
    if(failed&&pcm==failed_device&&source&&!retried){
        retried=1;record("retry",result,-1,(unsigned long)frames);
    }
    return result;
}
int snd_pcm_recover(snd_pcm_t* pcm,int error,int silent){
    static __typeof__(snd_pcm_recover)* next;int result;
    if(!next)next=dlsym(RTLD_NEXT,"snd_pcm_recover");
    /* Wine can load ALSA locally; RTLD_NEXT from a preloader then sees no
     * recovery symbol. Resolve its real library handle in that case. */
    if(!next){void* library=dlopen("libasound.so.2",RTLD_NOW|RTLD_LOCAL);
        if(library)next=dlsym(library,"snd_pcm_recover");}
    if(!next)abort();
    result=next(pcm,error,silent);
    if(failed&&pcm==failed_device){record("recover-error",error,silent,0);record("recover-result",result,silent,0);}
    return result;
}
