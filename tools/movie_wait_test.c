/* Independent relative timer boundaries and MCI drain with declared clocks.
 * No live original A/V or whole PCM equality is established by this fixture. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "dd2_movie_platform.h"
#ifdef DD2_WAIT_TIMER
#include <time.h>
static uint64_t wall_ns;
static unsigned calls;
int __wrap_clock_gettime(clockid_t id,struct timespec* value){
    if(id!=CLOCK_MONOTONIC){fprintf(stderr,"drain clock is not MONOTONIC\n");exit(1);}
    calls++;value->tv_sec=wall_ns/1000000000;value->tv_nsec=wall_ns%1000000000;return 0;
}
int main(void){
    static const uint64_t epochs[]={123456789ULL,999999999ULL,4294967295999999ULL};
    static const unsigned probes[]={0,1,99999999,100000000,100000001};
    unsigned i,j;
    for(i=0;i<sizeof(epochs)/sizeof(epochs[0]);i++){
        dd2_movie_drain_cancel();wall_ns=epochs[i];calls=0;
        if(dd2_movie_drain_ready() || calls)exit(1);
        dd2_movie_drain_start();
        for(j=0;j<sizeof(probes)/sizeof(probes[0]);j++){
            wall_ns=epochs[i]+probes[j];
            printf("{\"epoch_ns\":%llu,\"elapsed_ns\":%u,\"ready\":%d}\n",
                (unsigned long long)epochs[i],probes[j],dd2_movie_drain_ready());
        }
        dd2_movie_drain_start();
        if(dd2_movie_drain_ready())exit(1);
        dd2_movie_drain_cancel();wall_ns+=200000000;
        if(dd2_movie_drain_ready())exit(1);
        if(calls!=8)exit(1);
    }
    return 0;
}
#else
#include "dd2_movie.h"
static uint64_t wall_ns,raw_ns,deadline_ns;
static unsigned armed,mode,queries,frames,result,notifications;
static uint64_t query_times[32];
static void require(int ok,const char* why){if(!ok){fprintf(stderr,"movie wait: %s\n",why);exit(1);}}
FILE* dd2_fopen_ci(const char* name,const char* flags){return fopen(name,flags);}
unsigned dd2_movie_now_ms(void){return (unsigned)(raw_ns/1000000);}
double dd2_movie_now_us(void){return (double)(raw_ns/1000);}
void dd2_movie_drain_start(void){deadline_ns=wall_ns+100000000;armed=1;}
int dd2_movie_drain_ready(void){return armed && wall_ns>=deadline_ns;}
void dd2_movie_drain_cancel(void){armed=0;}
void dd2_movie_wait(void){wall_ns+=100000;}
void dd2_movie_present(const uint32_t* image){require(image!=NULL,"frame image");frames++;}
int dd2_movie_render_window(const uint8_t* rgb,unsigned width,unsigned height,const int32_t* rect,uint32_t* image){
    require(rgb && width==320 && height==192 && rect && image,"decoded original frame");return 0;
}
int dd2_movie_audio_start(const int16_t* pcm,size_t count,unsigned rate,unsigned channels){
    require(pcm && count==1012 && rate==22050 && channels==2,"short original source");return 0;
}
int dd2_movie_audio_done(void){
    require(queries<32,"bounded completion checks");query_times[queries++]=wall_ns;
    return wall_ns>=46000000;
}
void dd2_movie_audio_stop(void){}
int FUN_004132f0(void* hwnd,unsigned message,unsigned status,unsigned device){
    require(hwnd==(void*)1 && message==0x3b9 && device==2,"notify target");result=status;notifications++;return 0;
}
int main(int argc,char** argv){
    uint32_t open[4]={0},window[3]={0},play[3]={1},close[1]={0};unsigned completed,i;
    require(argc==3,"movie/clock mode required");mode=atoi(argv[2]);require(mode<=3,"declared clock mode");
    raw_ns=123456789000ULL;
    open[2]=(uint32_t)(uintptr_t)"avivideo";open[3]=(uint32_t)(uintptr_t)argv[1];
    require(!dd2_movie_mci_send(0,0x803,0x2202,open),"open");window[1]=1;
    require(!dd2_movie_mci_send(open[1],0x841,0x10002,window),"window");
    require(!dd2_movie_mci_send(open[1],0x806,1,play),"play");
    while(dd2_movie_active() && wall_ns<=1000000000){
        dd2_movie_pump();if(dd2_movie_active()){
            wall_ns+=100000;
            raw_ns=123456789000ULL+(mode==0?0:mode==1?wall_ns:mode==2?wall_ns*2:wall_ns/2);
        }
    }
    completed=!dd2_movie_active();
    require(!dd2_movie_mci_send(open[1],0x804,2,close),"close");
    require(frames==1 && notifications==1,"frame and notification counts");
    printf("{\"mode\":%u,\"completed\":%u,\"elapsed_ns\":%llu,\"result\":%u,\"timer_armed_after_close\":%u,\"queries_ns\":[",
        mode,completed,(unsigned long long)wall_ns,result,armed);
    for(i=0;i<queries;i++)printf("%s%llu",i?",":"",(unsigned long long)query_times[i]);puts("]}");return 0;
}
#endif
