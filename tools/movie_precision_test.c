/* Production clock arithmetic and MCI frame boundaries under declared clocks.
 * Decoder/source correctness and original live A/V have separate acceptance. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "dd2_movie_platform.h"
#ifdef DD2_PRECISION_CLOCK
#include <time.h>
#include <errno.h>
static struct timespec raw,mono;
static int fail_raw;
static unsigned count;
static int ids[4];
int __wrap_clock_gettime(clockid_t id,struct timespec* stamp){
    if(count>=4)exit(1);
    ids[count++]=id;
    if(id==CLOCK_MONOTONIC_RAW){if(fail_raw){errno=EINVAL;return -1;}*stamp=raw;return 0;}
    if(id==CLOCK_MONOTONIC){*stamp=mono;return 0;}
    exit(1);
}
int main(void){
    static const struct timespec cases[]={{0,0},{0,99},{0,100},{0,999},{0,1000},
        {0,999999},{0,1000000},{0,999999999},{1234,987654321},
        {922337,203685399},{922337,203685400},{922337,203685500},
        {4294967,295999999},{4294967,296000000},{9000000,999999999}};
    unsigned i,j;
    for(fail_raw=0;fail_raw<2;fail_raw++)for(i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
        double value;raw=cases[i];mono.tv_sec=raw.tv_sec+123;
        mono.tv_nsec=(raw.tv_nsec+123456789)%1000000000;count=0;
        value=dd2_movie_now_us();
        printf("{\"case\":%u,\"fallback\":%d,\"source_ns\":%llu,\"us\":%.0f,\"calls\":[",
            i,fail_raw,(unsigned long long)((uint64_t)(fail_raw?mono.tv_sec:raw.tv_sec)*1000000000+(fail_raw?mono.tv_nsec:raw.tv_nsec)),value);
        for(j=0;j<count;j++)printf("%s%d",j?",":"",ids[j]);puts("]}");
    }
    return 0;
}
#else
#include "dd2_movie.h"
static uint64_t now_ns;
static unsigned frames,notified;
static void require(int ok,const char* why){if(!ok){fprintf(stderr,"movie precision: %s\n",why);exit(1);}}
FILE* dd2_fopen_ci(const char* name,const char* mode){return fopen(name,mode);}
unsigned dd2_movie_now_ms(void){return (unsigned)(now_ns/1000000);}
double dd2_movie_now_us(void){return (double)(now_ns/1000);}
void dd2_movie_drain_start(void){}
int dd2_movie_drain_ready(void){return 0;}
void dd2_movie_drain_cancel(void){}
void dd2_movie_wait(void){now_ns+=1000;}
void dd2_movie_present(const uint32_t* pixels){require(pixels!=NULL,"frame image");frames++;}
int dd2_movie_render_window(const uint8_t* rgb,unsigned width,unsigned height,const int32_t* rect,uint32_t* pixels){
    require(rgb && width==320 && height==192 && rect && pixels,"original decoder frame");return 0;
}
int dd2_movie_audio_start(const int16_t* pcm,size_t count,unsigned rate,unsigned channels){
    require(pcm && count==1012 && rate==22050 && channels==2,"short original source");return 0;
}
int dd2_movie_audio_done(void){return 0;}
void dd2_movie_audio_stop(void){}
int FUN_004132f0(void* hwnd,unsigned message,unsigned result,unsigned device){
    require(hwnd==(void*)1 && message==0x3b9 && result==4 && device==2,"close abort notify");notified++;return 0;
}
int main(int argc,char** argv){
    static const uint64_t epochs[]={123456789000ULL,123456789999ULL,4294967295999999ULL,4294967296000000ULL};
    static const unsigned probes[]={0,39000000,39211000,39999000,39999999,40000000,79999000,80000000};
    unsigned i,j;
    require(argc==2,"three-frame short movie required");
    for(i=0;i<sizeof(epochs)/sizeof(epochs[0]);i++){
        uint32_t open[4]={0},window[3]={0},play[3]={1},close[1]={0};
        now_ns=epochs[i];frames=notified=0;
        open[2]=(uint32_t)(uintptr_t)"avivideo";open[3]=(uint32_t)(uintptr_t)argv[1];
        require(!dd2_movie_mci_send(0,0x803,0x2202,open),"open");
        window[1]=1;require(!dd2_movie_mci_send(open[1],0x841,0x10002,window),"window");
        require(!dd2_movie_mci_send(open[1],0x806,1,play),"play");
        for(j=0;j<sizeof(probes)/sizeof(probes[0]);j++){
            now_ns=epochs[i]+probes[j];dd2_movie_pump();
            printf("{\"epoch_ns\":%llu,\"now_ns\":%llu,\"frames\":%u}\n",
                (unsigned long long)epochs[i],(unsigned long long)now_ns,frames);
        }
        require(!dd2_movie_mci_send(open[1],0x804,2,close),"close");require(notified==1,"one abort");
    }
    return 0;
}
#endif
