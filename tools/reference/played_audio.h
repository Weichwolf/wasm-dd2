/* Read-only shadow of the virtual device's actual playback ring.
 * Accepted writes and played samples are different streams: rewinds replace
 * queued data, drop/prepare discard it and pauses create gaps in the clock.
 * No gap is silently filled or removed from the event timeline.
 */
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    FILE *pcm, *events;
    unsigned char *ring, *valid;
    unsigned frame_bytes;
    uint64_t last, frames;
    const char *root;
} PlayedAudio;

static const char *played_root(void){
    const char *root=getenv("DD2_AUDIO_CAPTURE"), *wanted=getenv("DD2_AUDIO_PROCESS");
    char name[32];FILE *file;int matches;
    if(!wanted)wanted="dd2h.exe";
    if(!root || !(file=fopen("/proc/self/comm","r")))return NULL;
    matches=fgets(name,sizeof(name),file)!=NULL;
    fclose(file);
    if(matches){name[strcspn(name,"\n")]=0;matches=!strcmp(name,wanted);}
    return matches?root:NULL;
}
static void played_error(PlayedAudio *capture,const char *reason){
    char path[4096];FILE *file;
    fprintf(stderr,"[played-audio] %s\n",reason);
    if(capture->root && snprintf(path,sizeof(path),"%s/error.txt",capture->root)<(int)sizeof(path)){
        file=fopen(path,"a");if(file){fprintf(file,"%s\n",reason);fclose(file);}
    }
}
static void played_flush(PlayedAudio *capture){
    if(capture->events && capture->pcm && (fflush(capture->pcm) || fflush(capture->events) ||
       ferror(capture->pcm) || ferror(capture->events)))played_error(capture,"Cannot flush played PCM/journal");
}
static void played_event(PlayedAudio *capture,const char *event,uint64_t time,
                         snd_pcm_ioplug_t *io){
    if(!capture->events)return;
    fprintf(capture->events,"{\"event\":\"%s\",\"time_ns\":%"PRIu64",\"played_frames\":%"PRIu64",\"appl_ptr\":%"PRIu64",\"hw_ptr\":%"PRIu64"}\n",
            event,time,capture->frames,(uint64_t)io->appl_ptr,(uint64_t)io->hw_ptr);
    played_flush(capture);
}
static void played_close(PlayedAudio *capture,uint64_t time,snd_pcm_ioplug_t *io){
    played_event(capture,"close",time,io);
    if(capture->pcm)fclose(capture->pcm);
    if(capture->events)fclose(capture->events);
    free(capture->ring);free(capture->valid);
    memset(capture,0,sizeof(*capture));
}
static void played_open(PlayedAudio *capture,uint64_t time,snd_pcm_ioplug_t *io){
    static unsigned serial;
    unsigned id;char path[4096];int width;
    played_close(capture,time,io);
    capture->root=played_root();if(!capture->root)return;
    width=snd_pcm_format_physical_width(io->format);
    if(width<=0 || width%8 || !io->channels || !io->buffer_size){
        played_error(capture,"Invalid played format/buffer");return;
    }
    capture->frame_bytes=(unsigned)width/8*io->channels;
    capture->ring=malloc(io->buffer_size*capture->frame_bytes);
    capture->valid=calloc(io->buffer_size,1);
    if(!capture->ring || !capture->valid){played_error(capture,"Cannot allocate playback ring");return;}
    id=__atomic_fetch_add(&serial,1,__ATOMIC_RELAXED);
    if(snprintf(path,sizeof(path),"%s/played-%d-%u.pcm",capture->root,(int)getpid(),id)>=(int)sizeof(path)){
        played_error(capture,"Played capture path too long");return;
    }
    capture->pcm=fopen(path,"wbx");
    snprintf(path,sizeof(path),"%s/played-%d-%u.jsonl",capture->root,(int)getpid(),id);
    capture->events=fopen(path,"wx");
    if(!capture->pcm || !capture->events){played_error(capture,"Cannot create fresh played files");return;}
    fprintf(capture->events,"{\"event\":\"format\",\"kind\":\"virtual-device-played\",\"time_ns\":%"PRIu64",\"pid\":%d,\"rate\":%u,\"channels\":%u,\"frame_bytes\":%u,\"format\":\"%s\",\"buffer_frames\":%"PRIu64",\"period_frames\":%"PRIu64"}\n",
            time,(int)getpid(),io->rate,io->channels,capture->frame_bytes,
            snd_pcm_format_name(io->format),(uint64_t)io->buffer_size,(uint64_t)io->period_size);
    played_flush(capture);
}
static void played_write(PlayedAudio *capture,snd_pcm_ioplug_t *io,
                          const snd_pcm_channel_area_t *areas,
                          snd_pcm_uframes_t offset,snd_pcm_uframes_t count){
    const unsigned char *source;uint64_t k,index;
    if(!capture->events || !capture->pcm || !capture->ring || !capture->valid)return;
    if(areas[0].first%8 || areas[0].step!=capture->frame_bytes*8){
        played_error(capture,"Unexpected interleaved playback layout");return;
    }
    source=(const unsigned char*)areas[0].addr+areas[0].first/8+offset*capture->frame_bytes;
    for(k=0;k<count;k++){
        index=(io->appl_ptr+k)%io->buffer_size;
        memcpy(capture->ring+index*capture->frame_bytes,source+k*capture->frame_bytes,capture->frame_bytes);
        capture->valid[index]=1;
    }
}
static uint64_t played_sample_time(uint64_t epoch,uint64_t samples,unsigned rate){
    return epoch+samples/rate*1000000000+
           (samples%rate*1000000000+rate-1)/rate;
}
static void played_until(PlayedAudio *capture,snd_pcm_ioplug_t *io,uint64_t current,
                         uint64_t boundary,uint64_t epoch,uint64_t origin,uint64_t observed){
    uint64_t count,queued,begin,k,index,clock_begin;
    if(!capture->events || !capture->pcm)return;
    begin=capture->last;
    count=(current+boundary-begin)%boundary;
    queued=(io->appl_ptr+boundary-begin)%boundary;
    if(count>queued)count=queued;
    if(!count)return;
    if(count>io->buffer_size){played_error(capture,"Played extent exceeds ring capacity");return;}
    for(k=0;k<count;k++){
        index=(begin+k)%io->buffer_size;
        if(!capture->valid[index]){played_error(capture,"Playback reached unwritten ring data");return;}
    }
    for(k=0;k<count;){
        uint64_t part;
        index=(begin+k)%io->buffer_size;
        part=io->buffer_size-index;if(part>count-k)part=count-k;
        if(fwrite(capture->ring+index*capture->frame_bytes,capture->frame_bytes,part,capture->pcm)!=part){
            played_error(capture,"Incomplete played PCM write");return;
        }
        k+=part;
    }
    clock_begin=(begin+boundary-origin%boundary)%boundary;
    capture->last=(begin+count)%boundary;
    fprintf(capture->events,"{\"event\":\"played\",\"time_ns\":%"PRIu64",\"offset_frames\":%"PRIu64",\"frames\":%"PRIu64",\"source_begin\":%"PRIu64",\"source_end\":%"PRIu64",\"boundary_frames\":%"PRIu64",\"sample_begin_ns\":%"PRIu64",\"sample_end_ns\":%"PRIu64"}\n",
            observed,capture->frames,count,begin,capture->last,boundary,
            played_sample_time(epoch,clock_begin,io->rate),
            played_sample_time(epoch,clock_begin+count,io->rate));
    capture->frames+=count;
    played_flush(capture);
}
