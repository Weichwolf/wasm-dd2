/* Short original-packet AVI fixture: real Wine MCI or production port MCI.
 * Controlled port clocks are component evidence, not live A/V parity. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#include <digitalv.h>
static void require(MCIERROR result,const char* op){
    if(result){fprintf(stderr,"movie drain: %s: %lu\n",op,(unsigned long)result);exit(1);}
}
int main(int argc,char** argv){
    MCI_OPEN_PARMSA open={0};MCI_DGV_WINDOW_PARMSA window={0};MCI_DGV_RECT_PARMS put={0};
    MCI_PLAY_PARMS play={0};MCI_GENERIC_PARMS close={0};
    LARGE_INTEGER begin,end,frequency;HWND hwnd;
    if(argc!=2)return 1;
    hwnd=CreateWindowA("STATIC","Movie drain fixture",WS_POPUP|WS_VISIBLE,0,0,640,480,
                       NULL,NULL,GetModuleHandleA(NULL),NULL);if(!hwnd)return 1;
    open.lpstrDeviceType="avivideo";open.lpstrElementName=argv[1];
    require(mciSendCommandA(0,MCI_OPEN,MCI_OPEN_TYPE|MCI_OPEN_ELEMENT|MCI_WAIT,(DWORD_PTR)&open),"open");
    window.hWnd=hwnd;
    require(mciSendCommandA(open.wDeviceID,MCI_WINDOW,MCI_DGV_WINDOW_HWND|MCI_WAIT,(DWORD_PTR)&window),"window");
    put.rc.left=0;put.rc.top=48;put.rc.right=640;put.rc.bottom=384;
    require(mciSendCommandA(open.wDeviceID,MCI_PUT,MCI_DGV_PUT_DESTINATION|MCI_DGV_RECT|MCI_WAIT,(DWORD_PTR)&put),"rectangle");
    QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&begin);
    require(mciSendCommandA(open.wDeviceID,MCI_PLAY,MCI_WAIT,(DWORD_PTR)&play),"play");
    QueryPerformanceCounter(&end);
    require(mciSendCommandA(open.wDeviceID,MCI_CLOSE,MCI_WAIT,(DWORD_PTR)&close),"close");
    DestroyWindow(hwnd);
    printf("{\"elapsed_ms\":%.6f,\"qpc_frequency\":%lld}\n",
           (end.QuadPart-begin.QuadPart)*1000.0/frequency.QuadPart,(long long)frequency.QuadPart);
    return 0;
}
#else
#include "dd2_movie.h"
#include "dd2_movie_platform.h"
static unsigned now_ms=0xfffffff0u,epoch,frames,notifications,closed,failed,done_ms,query_count;
static unsigned queries[4096];
static void require(int ok,const char* why){if(!ok){fprintf(stderr,"movie drain: %s\n",why);exit(1);}}
FILE* dd2_fopen_ci(const char* filename,const char* mode){return fopen(filename,mode);}
unsigned dd2_movie_now_ms(void){return now_ms;}
void dd2_movie_wait(void){now_ms++;}
void dd2_movie_present(const uint32_t* pixels){require(pixels!=NULL,"rendered frame");frames++;}
int dd2_movie_audio_start(const int16_t* pcm,size_t count,unsigned rate,unsigned channels){
    require(pcm && count==1012 && rate==22050 && channels==2,"original short audio format");
    epoch=now_ms;return failed?-1:0;
}
int dd2_movie_audio_done(void){
    require(query_count<4096,"bounded audio queries");queries[query_count++]=now_ms-epoch;
    return now_ms-epoch>=done_ms;
}
void dd2_movie_audio_stop(void){closed++;}
int FUN_004132f0(void* hwnd,unsigned message,unsigned result,unsigned device){
    require(hwnd==(void*)1 && message==0x3b9 && result==1 && device==2,"successful notify");
    notifications++;return 0;
}
int main(int argc,char** argv){
    uint32_t open[4]={0},window[3]={0},put[5]={0},play[3]={1},close[1]={0};unsigned i;
    require(argc==4,"movie/audio-done-ms/unavailable arguments");
    done_ms=(unsigned)strtoul(argv[2],NULL,10);failed=atoi(argv[3]);
    open[2]=(uint32_t)(uintptr_t)"avivideo";open[3]=(uint32_t)(uintptr_t)argv[1];
    require(!dd2_movie_mci_send(0,0x803,0x2202,open),"open");
    window[1]=1;require(!dd2_movie_mci_send(open[1],0x841,0x10002,window),"window");
    put[2]=48;put[3]=640;put[4]=384;
    require(!dd2_movie_mci_send(open[1],0x842,0x50002,put),"rectangle");
    require(!dd2_movie_mci_send(open[1],0x806,1,play),"play");
    while(dd2_movie_active()){
        require(now_ms-epoch<1000,"bounded completion");dd2_movie_pump();
        if(dd2_movie_active())now_ms++;
    }
    require(!dd2_movie_mci_send(open[1],0x804,2,close),"close");
    require(frames==1 && notifications==1 && closed==1,"complete actual MCI state");
    printf("{\"frames\":%u,\"elapsed_ms\":%u,\"audio_queries\":[",frames,now_ms-epoch);
    for(i=0;i<query_count;i++)printf("%s%u",i?",":"",queries[i]);
    printf("],\"notifications\":%u,\"closed\":%u}\n",notifications,closed);return 0;
}
#endif
