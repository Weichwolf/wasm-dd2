/* Observe actual SDL movie texture, renderer and accepted device data.
 * Calls and pixels are forwarded unchanged; this does not alter engine state,
 * input, device clocks, PCM or rendering. Readback and capture still affect
 * scheduling; bracket the forwarded presentation separately from that work.
 * Captures use an isolated process, not a physical display timing probe. */
#define _GNU_SOURCE
#include <SDL2/SDL.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
static uint32_t expected[640*480],actual[640*480];
static int texture_ready;
static unsigned frame,streams;
static FILE* journal;
static FILE* video_pipe;
static struct {SDL_AudioDeviceID id;FILE* pcm;uint64_t bytes;unsigned serial;int open;} devices[16];
static const char* root(void){return getenv("DD2_NATIVE_MOVIE_OBSERVE");}
static int timing_only(void){return getenv("DD2_NATIVE_MOVIE_TIMING_ONLY")!=NULL;}
static void fail(const char* why){fprintf(stderr,"Native movie observer: %s\n",why);exit(1);}
static FILE* create(const char* suffix){
    char path[4096];FILE* file;
    snprintf(path,sizeof(path),"%s/%s",root(),suffix);file=fopen(path,"wx");
    if(!file)fail("fresh output file");
    return file;
}
static uint64_t now(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return (uint64_t)ts.tv_sec*1000000000+ts.tv_nsec;}
static FILE* events(void){if(!journal)journal=create("events.jsonl");return journal;}
int SDL_PollEvent(SDL_Event* event){
    int (*call)(SDL_Event*)=dlsym(RTLD_NEXT,"SDL_PollEvent");
    uint64_t begin=now(),end;int result=call(event);end=now();
    if(root() && !frame){
        fprintf(events(),"{\"event\":\"initial_poll\",\"result\":%d,\"call_begin_ns\":%llu,\"call_end_ns\":%llu,\"time_ns\":%llu}\n",
                result,(unsigned long long)begin,(unsigned long long)end,(unsigned long long)now());fflush(journal);
    }
    return result;
}
SDL_Thread* SDL_CreateThread(SDL_ThreadFunction fn,const char* name,void* data){
    SDL_Thread* (*call)(SDL_ThreadFunction,const char*,void*)=dlsym(RTLD_NEXT,"SDL_CreateThread");
    uint64_t begin=now(),end;SDL_Thread* thread=call(fn,name,data);end=now();
    if(root() && name && !strcmp(name,"dd2-movie-alsa")){
        fprintf(events(),"{\"event\":\"movie_thread\",\"call_begin_ns\":%llu,\"call_end_ns\":%llu,\"success\":%s,\"time_ns\":%llu}\n",
                (unsigned long long)begin,(unsigned long long)end,thread?"true":"false",(unsigned long long)now());
        fflush(journal);
    }
    return thread;
}
SDL_AudioDeviceID SDL_OpenAudioDevice(const char* name,int capture,const SDL_AudioSpec* requested,SDL_AudioSpec* obtained,int flags){
    SDL_AudioDeviceID (*call)(const char*,int,const SDL_AudioSpec*,SDL_AudioSpec*,int)=dlsym(RTLD_NEXT,"SDL_OpenAudioDevice");
    SDL_AudioDeviceID id=call(name,capture,requested,obtained,flags);
    if(root() && !capture){
        const SDL_AudioSpec* spec=obtained && id ? obtained : requested;
        fprintf(events(),"{\"event\":\"open\",\"device\":%u,\"rate\":%d,\"format\":%u,\"channels\":%u,\"time_ns\":%llu}\n",
                id,spec->freq,spec->format,spec->channels,(unsigned long long)now());fflush(journal);
        if(id){char suffix[64];unsigned slot=streams++;
            if(slot>=16)fail("device slots");
            devices[slot].id=id;devices[slot].open=1;devices[slot].serial=slot;
            snprintf(suffix,sizeof(suffix),"stream%u.pcm",slot);devices[slot].pcm=create(suffix);
        }
    }
    return id;
}
static unsigned slot(SDL_AudioDeviceID id){unsigned n=streams;while(n--)if(devices[n].id==id && devices[n].open)return n;fail("unknown device");return 0;}
int SDL_QueueAudio(SDL_AudioDeviceID id,const void* pcm,Uint32 bytes){
    int (*call)(SDL_AudioDeviceID,const void*,Uint32)=dlsym(RTLD_NEXT,"SDL_QueueAudio");int result=call(id,pcm,bytes);
    if(root() && !result){unsigned n=slot(id);
        if(fwrite(pcm,1,bytes,devices[n].pcm)!=bytes)fail("accepted audio write");
        fprintf(events(),"{\"event\":\"queue\",\"device\":%u,\"stream\":%u,\"offset\":%llu,\"bytes\":%u,\"result\":0,\"time_ns\":%llu}\n",
            id,n,(unsigned long long)devices[n].bytes,bytes,(unsigned long long)now());
        devices[n].bytes+=bytes;fflush(devices[n].pcm);fflush(journal);
    }
    return result;
}
void SDL_CloseAudioDevice(SDL_AudioDeviceID id){
    void (*call)(SDL_AudioDeviceID)=dlsym(RTLD_NEXT,"SDL_CloseAudioDevice");
    if(root()){unsigned n=slot(id);
        fprintf(events(),"{\"event\":\"close\",\"device\":%u,\"stream\":%u,\"bytes\":%llu,\"time_ns\":%llu}\n",
                id,n,(unsigned long long)devices[n].bytes,(unsigned long long)now());
        fclose(devices[n].pcm);devices[n].open=0;fflush(journal);
    }
    call(id);
}
int SDL_UpdateTexture(SDL_Texture* texture,const SDL_Rect* rect,const void* pixels,int pitch){
    int (*call)(SDL_Texture*,const SDL_Rect*,const void*,int)=dlsym(RTLD_NEXT,"SDL_UpdateTexture");
    uint64_t begin=now(),end;int result=call(texture,rect,pixels,pitch);end=now();
    if(root() && !result){Uint32 format;int width,height,y;
        if(rect || SDL_QueryTexture(texture,&format,NULL,&width,&height) || format!=SDL_PIXELFORMAT_ARGB8888 || width!=640 || height!=480 || pitch!=640*4)
            fail("actual movie texture format");
        if(!timing_only())for(y=0;y<480;y++)memcpy(expected+y*640,(const uint8_t*)pixels+y*pitch,640*4);
        texture_ready=1;
        fprintf(events(),"{\"event\":\"texture_update\",\"frame\":%u,\"call_begin_ns\":%llu,\"call_end_ns\":%llu,\"time_ns\":%llu}\n",
                frame,(unsigned long long)begin,(unsigned long long)end,(unsigned long long)now());fflush(journal);
    }
    return result;
}
int SDL_RenderCopy(SDL_Renderer* renderer,SDL_Texture* texture,const SDL_Rect* source,const SDL_Rect* target){
    int (*call)(SDL_Renderer*,SDL_Texture*,const SDL_Rect*,const SDL_Rect*)=dlsym(RTLD_NEXT,"SDL_RenderCopy");
    uint64_t begin=now(),end;int result=call(renderer,texture,source,target);end=now();
    if(root() && !result){
        fprintf(events(),"{\"event\":\"render_copy\",\"frame\":%u,\"call_begin_ns\":%llu,\"call_end_ns\":%llu,\"time_ns\":%llu}\n",
                frame,(unsigned long long)begin,(unsigned long long)end,(unsigned long long)now());fflush(journal);
    }
    return result;
}
void SDL_RenderPresent(SDL_Renderer* renderer){
    void (*call)(SDL_Renderer*)=dlsym(RTLD_NEXT,"SDL_RenderPresent");
    if(root()){
        int width,height;
        if(timing_only()){
            uint64_t begin,end;
            if(!texture_ready || getenv("DD2_NATIVE_MOVIE_VIDEO_PIPE"))fail("timing-only texture or conflicting pixel capture");
            begin=now();call(renderer);end=now();
            fprintf(events(),"{\"event\":\"present\",\"frame\":%u,\"exact_pixels\":0,"
                    "\"record_version\":3,\"clock_domain\":\"CLOCK_MONOTONIC\","
                    "\"present_begin_ns\":%llu,\"present_end_ns\":%llu,\"time_ns\":%llu}\n",
                    frame++,(unsigned long long)begin,(unsigned long long)end,(unsigned long long)now());
            texture_ready=0;fflush(journal);return;
        }
        uint64_t readback_begin=now(),readback_end,present_begin,present_end;
        if(!texture_ready || SDL_GetRendererOutputSize(renderer,&width,&height) || width!=640 || height!=480
                || SDL_RenderReadPixels(renderer,NULL,SDL_PIXELFORMAT_ARGB8888,actual,640*4) || memcmp(expected,actual,sizeof(actual)))
            fail("actual SDL renderer pixels differ from movie texture");
        readback_end=now();
        /* Optional bounded FIFO exports only the actual renderer readback.
         * No reference data enters this process or the engine. */
        if(getenv("DD2_NATIVE_MOVIE_VIDEO_PIPE")){
            if(!video_pipe){char path[4096];struct stat status;
                snprintf(path,sizeof(path),"%s/video.pipe",root());
                if(stat(path,&status) || !S_ISFIFO(status.st_mode))fail("actual video FIFO required");
                video_pipe=fopen(path,"wb");if(!video_pipe)fail("video pipe open");
            }
            if(fwrite(actual,1,sizeof(actual),video_pipe)!=sizeof(actual) || fflush(video_pipe))fail("actual video pipe write");
        }
        present_begin=now();call(renderer);present_end=now();
        fprintf(events(),"{\"event\":\"present\",\"frame\":%u,\"exact_pixels\":307200,"
                "\"record_version\":2,\"clock_domain\":\"CLOCK_MONOTONIC\","
                "\"readback_begin_ns\":%llu,\"readback_end_ns\":%llu,"
                "\"present_begin_ns\":%llu,\"present_end_ns\":%llu,\"time_ns\":%llu}\n",
                frame++,(unsigned long long)readback_begin,(unsigned long long)readback_end,
                (unsigned long long)present_begin,(unsigned long long)present_end,(unsigned long long)now());
        texture_ready=0;fflush(journal);
    }else call(renderer);
}
