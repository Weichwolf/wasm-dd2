/* Observe Wine's actual ALSA output without replacing its DirectSound mixer.
 * DD2_AUDIO_CAPTURE enables this only in the dd2h.exe process. Optional
 * DD2_AUDIO_RATE restricts the virtual ALSA device's advertised sample rate.
 * Record accepted writes plus transport events; accepted/queued PCM is not
 * proof that every frame reached a DAC, or proof of original/port parity.
 */
#define _GNU_SOURCE
#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef struct capture {
    snd_pcm_t *pcm;
    FILE *audio, *events;
    uint64_t frames;
    unsigned frame_bytes;
    struct capture *next;
} Capture;
static Capture *captures;
static unsigned serial;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static void *alsa;
static void load_alsa(void) { alsa = dlopen("libasound.so.2", RTLD_LAZY | RTLD_LOCAL); }
static void *symbol(const char *name) {
    pthread_once(&once, load_alsa);
    if (!alsa) { fprintf(stderr,"[audio-capture] Cannot load libasound\n"); abort(); }
    void *result = dlsym(alsa, name);
    if (!result) { fprintf(stderr,"[audio-capture] Missing ALSA symbol %s\n",name); abort(); }
    return result;
}
#define REAL(name) ((__typeof__(&name))symbol(#name))
static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC,&ts);
    return (uint64_t)ts.tv_sec*1000000000+(uint64_t)ts.tv_nsec;
}
static const char *active_root(void) {
    char name[32]; FILE *file;
    const char *root = getenv("DD2_AUDIO_CAPTURE");
    if (!root || !(file=fopen("/proc/self/comm","r"))) return NULL;
    int valid = fgets(name,sizeof(name),file) && !strcmp(name,"dd2h.exe\n");
    fclose(file);
    return valid ? root : NULL;
}
static Capture *find(snd_pcm_t *pcm) {
    Capture *item;
    for(item=captures;item;item=item->next) if(item->pcm==pcm) return item;
    return NULL;
}
static void report_error(const char *root,const char *reason) {
    char path[4096];
    fprintf(stderr,"[audio-capture] ERROR: %s\n",reason);
    if(snprintf(path,sizeof(path),"%s/error.txt",root)<(int)sizeof(path)) {
        FILE *file=fopen(path,"a");
        if(file){fprintf(file,"%s\n",reason);fclose(file);}
    }
}
static void flush_capture(Capture *item) {
    int failed=fflush(item->audio)!=0;
    failed|=fflush(item->events)!=0;
    if(failed || ferror(item->audio) || ferror(item->events))
        report_error(getenv("DD2_AUDIO_CAPTURE"),"Cannot flush capture payload/journal");
}
static void close_capture(Capture *item) {
    fprintf(item->events,"{\"event\":\"close\",\"time_ns\":%"PRIu64",\"frames\":%"PRIu64"}\n",now_ns(),item->frames);
    flush_capture(item);
    fclose(item->audio);fclose(item->events);
    Capture **link=&captures;
    while(*link && *link!=item)link=&(*link)->next;
    if(*link)*link=item->next;
    free(item);
}
int snd_pcm_hw_params_any(snd_pcm_t *pcm,snd_pcm_hw_params_t *params) {
    int result=REAL(snd_pcm_hw_params_any)(pcm,params);
    int saved=errno;
    const char *rate=getenv("DD2_AUDIO_RATE");
    if(result>=0 && rate && active_root() && REAL(snd_pcm_stream)(pcm)==SND_PCM_STREAM_PLAYBACK) {
        unsigned value=(unsigned)strtoul(rate,NULL,10);
        result=REAL(snd_pcm_hw_params_set_rate)(pcm,params,value,0);
    }
    errno=saved;return result;
}
int snd_pcm_hw_params(snd_pcm_t *pcm,snd_pcm_hw_params_t *params) {
    int result=REAL(snd_pcm_hw_params)(pcm,params),saved=errno;
    const char *root=active_root();
    if(result>=0 && root && REAL(snd_pcm_stream)(pcm)==SND_PCM_STREAM_PLAYBACK) {
        snd_pcm_format_t format;unsigned rate,channels;int direction=0;
        pthread_mutex_lock(&mutex);
        Capture *old=find(pcm);if(old)close_capture(old);
        if(REAL(snd_pcm_hw_params_get_format)(params,&format)<0 ||
           REAL(snd_pcm_hw_params_get_rate)(params,&rate,&direction)<0 ||
           REAL(snd_pcm_hw_params_get_channels)(params,&channels)<0) {
            report_error(root,"Cannot read committed hardware format");
        }else{
            char path[4096];Capture *item=calloc(1,sizeof(*item));
            unsigned id=serial++;
            int width=REAL(snd_pcm_format_physical_width)(format);
            if(!item || width<=0 || width%8 || !channels) {
                report_error(root,"Invalid capture allocation/format");free(item);
            }else{
                item->pcm=pcm;item->frame_bytes=channels*(unsigned)width/8;
                snprintf(path,sizeof(path),"%s/stream-%d-%u.pcm",root,(int)getpid(),id);
                item->audio=fopen(path,"wbx");
                snprintf(path,sizeof(path),"%s/stream-%d-%u.jsonl",root,(int)getpid(),id);
                item->events=fopen(path,"wx");
                if(!item->audio || !item->events){
                    report_error(root,"Cannot create fresh capture files");
                    if(item->audio)fclose(item->audio);
                    if(item->events)fclose(item->events);
                    free(item);
                }else{
                    fprintf(item->events,"{\"event\":\"format\",\"time_ns\":%"PRIu64",\"pid\":%d,\"rate\":%u,\"channels\":%u,\"format\":\"%s\",\"frame_bytes\":%u}\n",
                            now_ns(),(int)getpid(),rate,channels,REAL(snd_pcm_format_name)(format),item->frame_bytes);
                    flush_capture(item);item->next=captures;captures=item;
                }
            }
        }
        pthread_mutex_unlock(&mutex);
    }
    errno=saved;return result;
}
snd_pcm_sframes_t snd_pcm_writei(snd_pcm_t *pcm,const void *data,snd_pcm_uframes_t frames) {
    uint64_t begin=now_ns();
    snd_pcm_sframes_t result=REAL(snd_pcm_writei)(pcm,data,frames);int saved=errno;
    uint64_t end=now_ns();
    pthread_mutex_lock(&mutex);
    Capture *item=find(pcm);
    if(item){
        if(result>0){
            if(fwrite(data,item->frame_bytes,(size_t)result,item->audio)!=(size_t)result)
                report_error(getenv("DD2_AUDIO_CAPTURE"),"Incomplete PCM write");
        }
        fprintf(item->events,"{\"event\":\"write\",\"time_ns\":%"PRIu64",\"call_begin_ns\":%"PRIu64",\"call_end_ns\":%"PRIu64",\"offset_frames\":%"PRIu64",\"requested\":%lu,\"accepted\":%ld}\n",
                now_ns(),begin,end,item->frames,(unsigned long)frames,(long)result);
        if(result>0)item->frames+=(uint64_t)result;
        flush_capture(item);
    }
    pthread_mutex_unlock(&mutex);errno=saved;return result;
}
static void transport(snd_pcm_t *pcm,const char *event,long result) {
    pthread_mutex_lock(&mutex);Capture *item=find(pcm);
    if(item){
        fprintf(item->events,"{\"event\":\"%s\",\"time_ns\":%"PRIu64",\"offset_frames\":%"PRIu64",\"result\":%ld}\n",event,now_ns(),item->frames,result);
        flush_capture(item);
    }
    pthread_mutex_unlock(&mutex);
}
#define TRANSPORT(name) \
int name(snd_pcm_t *pcm){int result=REAL(name)(pcm),saved=errno;transport(pcm,#name,result);errno=saved;return result;}
TRANSPORT(snd_pcm_prepare)
TRANSPORT(snd_pcm_start)
TRANSPORT(snd_pcm_drop)
TRANSPORT(snd_pcm_drain)
TRANSPORT(snd_pcm_reset)
snd_pcm_sframes_t snd_pcm_rewind(snd_pcm_t *pcm,snd_pcm_uframes_t frames) {
    snd_pcm_sframes_t result=REAL(snd_pcm_rewind)(pcm,frames);int saved=errno;
    transport(pcm,"snd_pcm_rewind",result);errno=saved;return result;
}
int snd_pcm_close(snd_pcm_t *pcm) {
    int result=REAL(snd_pcm_close)(pcm),saved=errno;
    pthread_mutex_lock(&mutex);Capture *item=find(pcm);if(item)close_capture(item);
    pthread_mutex_unlock(&mutex);errno=saved;return result;
}
