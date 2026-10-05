/* The extracted original Play_Movie calls the production AVI/MCI backend.
 * A controlled device clock delays audio completion past the video end.
 * This is transport evidence; native/browser sinks have separate live tests. */
#include "dd2_movie.h"
#include "dd2_movie_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t uint,undefined4,MCIERROR;
typedef uintptr_t DWORD_PTR;
#define __cdecl
static uint DAT_0046047c=1,Movie_Playing;
static int in_640=1;
static char* s_avivideo_0046c74c="avivideo";
static unsigned clock_now=0xfffff000u,epoch,frames,notifications,pumps,closed,started,skipping,audio_failure;
static unsigned rate,channels;
static size_t pcm_frames;
static FILE *video,*audio,*timeline;
static void require(int ok,const char* why){if(!ok){fprintf(stderr,"movie playback: %s\n",why);exit(1);}}
FILE* dd2_fopen_ci(const char* filename,const char* mode){return fopen(filename,mode);}
unsigned dd2_movie_now_ms(void){return clock_now;}
void dd2_movie_wait(void){clock_now+=40;}
void dd2_movie_present(const uint32_t* argb){
    require(fwrite(argb,4,640*480,video)==640*480,"full actual backend display frame");
    fprintf(timeline,"%u %u\n",frames++,clock_now-epoch);
}
int dd2_movie_audio_start(const int16_t* pcm,size_t count,unsigned frequency,unsigned n){
    require(!started++,"one WaveOut source");epoch=clock_now;pcm_frames=count;rate=frequency;channels=n;
    if(audio_failure)return -1;
    require(fwrite(pcm,2*n,count,audio)==count,"whole source audio submission");return 0;
}
int dd2_movie_audio_done(void){
    return !audio_failure && clock_now-epoch>=(pcm_frames*1000+rate-1)/rate+120;
}
void dd2_movie_audio_stop(void){closed++;}
int FUN_004132f0(void* hwnd,unsigned message,unsigned result,unsigned device){
    require(hwnd==(void*)1 && message==0x3b9 && device==2,"original notify recipient/device");
    require(result==(skipping?4u:1u),"completion vs aborted notification");
    if(!skipping && !audio_failure)require(dd2_movie_audio_done(),"no successful notify before device drains");
    require(closed==(audio_failure?0u:1u),"audio reset before notification");
    notifications++;Movie_Playing=0;return 0;
}
static MCIERROR mciSendCommandA(unsigned device,unsigned command,unsigned flags,DWORD_PTR pointer){
    int result=dd2_movie_mci_send(device,command,flags,(uint32_t*)pointer);
    require(result==0,"original MCI sequence succeeds");return result;
}
static void FUN_00413054(void){
    require(++pumps<1900,"movie completes");
    dd2_movie_pump();
    if(skipping && frames>=26)Movie_Playing=0;
    clock_now+=40;
}
#include "movie-function.c"
int main(int argc,char** argv){
    require(argc==6,"movie/video/audio/timeline/skip arguments");
    video=!strcmp(argv[2],"-")?stdout:fopen(argv[2],"wb");audio=fopen(argv[3],"wb");timeline=fopen(argv[4],"w");
    require(video && audio && timeline,"capture files");skipping=atoi(argv[5])==1;audio_failure=atoi(argv[5])==2;
    Play_Movie((uint)(uintptr_t)argv[1]);
    require(!dd2_movie_active() && !Movie_Playing && notifications==1 &&
            closed==(!skipping && !audio_failure?2u:1u),"finished/closed state");
    require(!fflush(video) && (video==stdout || !fclose(video)) && !fclose(audio) && !fclose(timeline),"close full capture streams");
    printf("{\"frames\":%u,\"pcm_frames\":%lu,\"rate\":%u,\"channels\":%u,\"pumps\":%u,\"notifications\":%u,\"closed\":%u,\"skip\":%u,\"audio_failure\":%u}\n",
           frames,(unsigned long)pcm_frames,rate,channels,pumps,notifications,closed,skipping,audio_failure);
    return 0;
}
