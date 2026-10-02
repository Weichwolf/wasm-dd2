/* Observe actual SDL movie texture, renderer and accepted device data.
 * Calls and pixels are forwarded unchanged; this does not alter engine state,
 * input, device clocks, PCM or rendering. Captures use an isolated process. */
#define _GNU_SOURCE
#include <SDL2/SDL.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static uint32_t expected[640*480],actual[640*480];
static int texture_ready;
static unsigned frame,streams;
static FILE* journal;
static struct {SDL_AudioDeviceID id;FILE* pcm;uint64_t bytes;unsigned serial;int open;} devices[16];
static const char* root(void){return getenv("DD2_NATIVE_MOVIE_OBSERVE");}
static void fail(const char* why){fprintf(stderr,"Native movie observer: %s\n",why);exit(1);}
static FILE* create(const char* suffix){
    char path[4096];FILE* file;
    snprintf(path,sizeof(path),"%s/%s",root(),suffix);file=fopen(path,"wx");
    if(!file)fail("fresh output file");
    return file;
}
static uint64_t now(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return (uint64_t)ts.tv_sec*1000000000+ts.tv_nsec;}
static FILE* events(void){if(!journal)journal=create("events.jsonl");return journal;}
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
    int result=call(texture,rect,pixels,pitch);
    if(root() && !result){Uint32 format;int width,height,y;
        if(rect || SDL_QueryTexture(texture,&format,NULL,&width,&height) || format!=SDL_PIXELFORMAT_ARGB8888 || width!=640 || height!=480 || pitch!=640*4)
            fail("actual movie texture format");
        for(y=0;y<480;y++)memcpy(expected+y*640,(const uint8_t*)pixels+y*pitch,640*4);
        texture_ready=1;
    }
    return result;
}
void SDL_RenderPresent(SDL_Renderer* renderer){
    void (*call)(SDL_Renderer*)=dlsym(RTLD_NEXT,"SDL_RenderPresent");
    if(root()){
        int width,height;
        if(!texture_ready || SDL_GetRendererOutputSize(renderer,&width,&height) || width!=640 || height!=480
                || SDL_RenderReadPixels(renderer,NULL,SDL_PIXELFORMAT_ARGB8888,actual,640*4) || memcmp(expected,actual,sizeof(actual)))
            fail("actual SDL renderer pixels differ from movie texture");
        fprintf(events(),"{\"event\":\"present\",\"frame\":%u,\"exact_pixels\":307200,\"time_ns\":%llu}\n",frame++,(unsigned long long)now());
        texture_ready=0;fflush(journal);
    }
    call(renderer);
}
