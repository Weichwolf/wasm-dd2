/* Read-only SDL boundary observer for tests. It forwards real SDL calls,
 * records only accepted audio, and reads back rendered pixels on request.
 * It never changes engine state, input, device settings or queued audio. */
#define _GNU_SOURCE
#include <SDL2/SDL.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static FILE *audio,*journal;
static uint64_t audio_offset;
static int audio_initialized;
static const char* root(void){return getenv("DD2_NATIVE_OBSERVE");}
static void save(const char* suffix,const void* buffer,size_t bytes){
    char path[4096];FILE* file;
    snprintf(path,sizeof(path),"%s/%s",root(),suffix);
    file=fopen(path,"wx");
    if(!file || fwrite(buffer,1,bytes,file)!=bytes || fclose(file)){
        fprintf(stderr,"SDL observer failed to save %s\n",path);exit(1);
    }
}
SDL_AudioDeviceID SDL_OpenAudioDevice(const char* name,int capture,const SDL_AudioSpec* wanted,SDL_AudioSpec* actual,int changes){
    SDL_AudioDeviceID (*original)(const char*,int,const SDL_AudioSpec*,SDL_AudioSpec*,int)=dlsym(RTLD_NEXT,"SDL_OpenAudioDevice");
    SDL_AudioDeviceID device=original(name,capture,wanted,actual,changes);
    if(root() && device && !capture){
        char metadata[256];const SDL_AudioSpec* spec=actual?actual:wanted;
        snprintf(metadata,sizeof(metadata),"{\"rate\":%d,\"format\":%u,\"channels\":%u,\"device\":%u}\n",
            spec->freq,spec->format,spec->channels,device);
        save("audio.json",metadata,strlen(metadata));
    }
    return device;
}
int SDL_QueueAudio(SDL_AudioDeviceID device,const void* pcm,Uint32 bytes){
    int (*original)(SDL_AudioDeviceID,const void*,Uint32)=dlsym(RTLD_NEXT,"SDL_QueueAudio");
    int result=original(device,pcm,bytes);
    if(root() && !result){
        if(!audio_initialized){
            char path[4096];audio_initialized=1;
            snprintf(path,sizeof(path),"%s/audio.pcm",root());audio=fopen(path,"wx");
            snprintf(path,sizeof(path),"%s/audio.jsonl",root());journal=fopen(path,"wx");
            if(!audio || !journal){fprintf(stderr,"SDL observer output already exists or cannot open\n");exit(1);}
        }
        if(fwrite(pcm,1,bytes,audio)!=bytes){fprintf(stderr,"SDL observer audio write failed\n");exit(1);}
        fprintf(journal,"{\"device\":%u,\"offset\":%llu,\"bytes\":%u,\"result\":0}\n",device,(unsigned long long)audio_offset,bytes);
        audio_offset+=bytes;fflush(audio);fflush(journal);
    }
    return result;
}
void SDL_RenderPresent(SDL_Renderer* renderer){
    void (*original)(SDL_Renderer*)=dlsym(RTLD_NEXT,"SDL_RenderPresent");
    char request[4096],suffix[128],metadata[256];FILE* file;
    unsigned index;int width,height;unsigned char* pixels;
    if(!root()){original(renderer);return;}
    snprintf(request,sizeof(request),"%s/request",root());file=fopen(request,"r");
    if(!file){original(renderer);return;}
    if(fscanf(file,"%u",&index)!=1){fclose(file);fprintf(stderr,"Invalid SDL observer request\n");exit(1);}
    fclose(file);unlink(request);
    if(SDL_GetRendererOutputSize(renderer,&width,&height) || width<=0 || height<=0){fprintf(stderr,"SDL readback dimensions failed\n");exit(1);}
    pixels=malloc((size_t)width*height*4);
    if(!pixels || SDL_RenderReadPixels(renderer,NULL,SDL_PIXELFORMAT_RGBA32,pixels,width*4)){
        fprintf(stderr,"SDL actual renderer readback failed: %s\n",SDL_GetError());exit(1);
    }
    snprintf(suffix,sizeof(suffix),"frame%05u.rgba",index);save(suffix,pixels,(size_t)width*height*4);free(pixels);
    snprintf(suffix,sizeof(suffix),"frame%05u.bin",index);save(suffix,(void*)0x700450,640*480);
    {
        const char* address=getenv("DD2_NATIVE_PALETTE_ADDRESS");
        uintptr_t palette=address?(uintptr_t)strtoul(address,NULL,0):0x700050;
        snprintf(suffix,sizeof(suffix),"frame%05u.pal",index);save(suffix,(void*)palette,256*4);
    }
    snprintf(metadata,sizeof(metadata),"{\"width\":%d,\"height\":%d,\"cf\":%d,\"level\":%d,\"audio_bytes\":%llu}\n",
        width,height,*(int*)0x462ff0,*(int*)0x936ff4,(unsigned long long)audio_offset);
    original(renderer);
    /* Written after actual presentation: a reader can now acknowledge the
     * capture. Use fixture input synchronization for external X11 screenshots. */
    snprintf(suffix,sizeof(suffix),"frame%05u.json",index);save(suffix,metadata,strlen(metadata));
}
