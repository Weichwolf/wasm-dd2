/* AVI MCI device for the actual dd2h.exe Play_Movie command sequence.
 * Its five calls are reconstructed in patch839. Source decoders and RIFF
 * transport are checked independently against real ICCVID/ACM. The movie
 * WaveOut clock/device is separate from the game's DirectSound/CD mixer. */
#include "dd2_movie.h"
#include "dd2_movie_platform.h"
#include "dd2_avi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DD2_BROWSER
#include <emscripten.h>
#endif
#define MOVIE_DEVICE 2
extern FILE* dd2_fopen_ci(const char*,const char*);
extern int FUN_004132f0(void*,unsigned,unsigned,unsigned);
static DD2AVI* movie;
static unsigned playing,start_ms,next_frame,window,callback,notify,audio_available;
static int32_t destination[4];
static uint32_t image[640*480];
static void finish(unsigned result) {
    unsigned hwnd=callback,pending=notify;
    playing=0;notify=0;
    if(pending && hwnd)FUN_004132f0((void*)(uintptr_t)hwnd,0x3b9,result,MOVIE_DEVICE);
}
static void close_movie(void) {
    dd2_movie_audio_stop();
    if(playing)finish(4); /* MCI_NOTIFY_ABORTED */
    dd2_avi_close(movie);movie=NULL;playing=window=callback=notify=next_frame=0;
}
static DD2AVI* load_movie(const char* filename) {
    uint8_t* bytes=NULL;
    size_t count;
    DD2AVI* result;
#ifdef DD2_BROWSER
    int error=0,length=0;
    emscripten_wget_data(filename,(void**)&bytes,&length,&error);
    if(error || !bytes || length<=0) { free(bytes);return NULL; }
    count=(size_t)length;
#else
    FILE* file=dd2_fopen_ci(filename,"rb");
    long length;
    if(!file)return NULL;
    if(fseek(file,0,SEEK_END) || (length=ftell(file))<=0 || fseek(file,0,SEEK_SET)) {
        fclose(file);return NULL;
    }
    count=(size_t)length;bytes=malloc(count);
    if(!bytes || fread(bytes,1,count,file)!=count) { free(bytes);fclose(file);return NULL; }
    fclose(file);
#endif
    result=dd2_avi_open(bytes,count);free(bytes);return result;
}
int dd2_movie_active(void) { return playing!=0; }
void dd2_movie_pump(void) {
    const DD2AVIInfo* info;
    unsigned elapsed;
    if(!playing)return;
    info=dd2_avi_info(movie);elapsed=dd2_movie_now_ms()-start_ms;
    while(next_frame<info->frames && (uint64_t)next_frame*info->video_scale*1000<= (uint64_t)elapsed*info->video_rate) {
        const uint8_t* rgb=dd2_avi_frame(movie,next_frame);
        if(!rgb || dd2_movie_render(rgb,info->width,info->height,destination,image)) {
            dd2_movie_audio_stop();finish(8);return; /* MCI_NOTIFY_FAILURE */
        }
        dd2_movie_present(image);next_frame++;
    }
    if(next_frame==info->frames && (uint64_t)elapsed*info->video_rate>=(uint64_t)info->frames*info->video_scale*1000
            && (!audio_available || dd2_movie_audio_done()))finish(1); /* MCI_NOTIFY_SUCCESSFUL */
}
int dd2_movie_mci_send(unsigned device,unsigned command,unsigned flags,uint32_t* params) {
    const DD2AVIInfo* info;
    if(command==0x803) {
        const char *type,*filename;
        DD2AVI* opened;
        if(!(flags&0x2000) || !(flags&0x200))return -1;
        if(!params)return 297;
        type=(const char*)(uintptr_t)params[2];
        if(!type || strcmp(type,"avivideo"))return -1;
        filename=(const char*)(uintptr_t)params[3];
        if(!filename)return 297;
        if(movie)return 291;
        opened=load_movie(filename);if(!opened)return 275; /* MCIERR_FILE_NOT_FOUND */
        movie=opened;info=dd2_avi_info(movie);params[1]=MOVIE_DEVICE;
        destination[0]=destination[1]=0;destination[2]=info->width;destination[3]=info->height;
        return 0;
    }
    if(device!=MOVIE_DEVICE)return -1;
    if(!movie)return 257;
    switch(command) {
    case 0x804: close_movie();return 0;
    case 0x841:
        if(!params)return 297;
        if(!(flags&0x10000))return 273;
        window=params[1];return 0;
    case 0x842:
        if(!params)return 297;
        if((flags&0x50000)!=0x50000)return 274;
        if((int32_t)params[3]<=0 || (int32_t)params[4]<=0)return 282;
        memcpy(destination,params+1,sizeof(destination));return 0;
    case 0x806:
        if(!params)return 297;
        if(flags&~3u)return 274;
        if(!window)return 346; /* MCIERR_NO_WINDOW */
        if(playing)return 0;
        info=dd2_avi_info(movie);
        /* Wine's real MCIAVI_player continues video when OpenAudio cannot
         * open WaveOut. Do not discard the entire film on a device failure. */
        audio_available=dd2_movie_audio_start(dd2_avi_pcm(movie),info->pcm_frames,info->pcm_rate,info->pcm_channels)==0;
        next_frame=0;callback=params[0];notify=flags&1;
        start_ms=dd2_movie_now_ms();playing=1;
        if(flags&2)while(playing) { dd2_movie_pump();dd2_movie_wait(); }
        return 0;
    default:return 261;
    }
}
int dd2_movie_run(const char* filename) {
    extern unsigned short Init_Application(void*);
    extern void Set_Draw_Mode(int);
    extern void Play_Movie(uint32_t);
    extern void Sound_Remove(void);
    if(!Init_Application((void*)1))return 1;
    Sound_Remove();
    *(int*)(uintptr_t)0x463010=-1;
    Set_Draw_Mode(1);
    Play_Movie((uint32_t)(uintptr_t)filename);
    return 0;
}
