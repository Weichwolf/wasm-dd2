/* Short original-packet AVI fixture: real Wine MCI or production port MCI.
 * Controlled port clocks are component evidence, not live A/V parity. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <digitalv.h>
static void require(MCIERROR result,const char* op){
    if(result){fprintf(stderr,"movie drain: %s: %lu\n",op,(unsigned long)result);exit(1);}
}
static void require_closed(const char* root,const char* pattern){
    WIN32_FIND_DATAA entry;HANDLE files;char path[MAX_PATH],tail[512];unsigned count=0;
    snprintf(path,sizeof(path),"%s\\%s",root,pattern);
    files=FindFirstFileA(path,&entry);
    if(files==INVALID_HANDLE_VALUE){fprintf(stderr,"movie drain: missing device journal\n");exit(1);}
    do{
        FILE* file;long length;size_t got;
        snprintf(path,sizeof(path),"%s\\%s",root,entry.cFileName);
        file=fopen(path,"rb");if(!file)exit(1);
        if(fseek(file,0,SEEK_END) || (length=ftell(file))<0)exit(1);
        if(fseek(file,length>511?length-511:0,SEEK_SET))exit(1);
        got=fread(tail,1,511,file);tail[got]=0;fclose(file);
        if(!strstr(tail,"\"event\":\"close\"")){
            fprintf(stderr,"movie drain: device still open at play return\n");exit(1);
        }
        count++;
    }while(FindNextFileA(files,&entry));
    FindClose(files);
    if(count!=1){fprintf(stderr,"movie drain: one movie device required\n");exit(1);}
}
int main(int argc,char** argv){
    MCI_OPEN_PARMSA open={0};MCI_DGV_WINDOW_PARMSA window={0};MCI_DGV_RECT_PARMS put={0};
    MCI_PLAY_PARMS play={0};MCI_GENERIC_PARMS close={0};
    LARGE_INTEGER begin,end,frequency;HWND hwnd;
    if(argc!=2 && argc!=3)return 1;
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
    if(argc==3){require_closed(argv[2],"stream-*.jsonl");require_closed(argv[2],"played-*.jsonl");}
    require(mciSendCommandA(open.wDeviceID,MCI_CLOSE,MCI_WAIT,(DWORD_PTR)&close),"close");
    DestroyWindow(hwnd);
    printf("{\"elapsed_ms\":%.6f,\"qpc_frequency\":%lld,\"device_closed_before_play_return\":%s}\n",
           (end.QuadPart-begin.QuadPart)*1000.0/frequency.QuadPart,(long long)frequency.QuadPart,
           argc==3?"true":"null");
    return 0;
}
#else
#include "dd2_movie.h"
#include "dd2_movie_platform.h"
static unsigned now_ms=0xfffffff0u,epoch,frames,notifications,closed,closed_at_notify,failed,done_ms,query_count;
static unsigned queries[4096];
static void require(int ok,const char* why){if(!ok){fprintf(stderr,"movie drain: %s\n",why);exit(1);}}
FILE* dd2_fopen_ci(const char* filename,const char* mode){return fopen(filename,mode);}
unsigned dd2_movie_now_ms(void){return now_ms;}
void dd2_movie_wait(void){now_ms++;}
void dd2_movie_present(const uint32_t* pixels){require(pixels!=NULL,"rendered frame");frames++;}
int dd2_movie_audio_start(const int16_t* pcm,size_t count,unsigned rate,unsigned channels){
    require(pcm && count==1012 && rate==22050 && channels==2,"original short audio format");
    epoch=now_ms;return failed==1?-1:0;
}
int dd2_movie_audio_done(void){
    require(query_count<4096,"bounded audio queries");queries[query_count++]=now_ms-epoch;
    if(failed==2 && now_ms-epoch>=done_ms)return -1;
    return now_ms-epoch>=done_ms;
}
void dd2_movie_audio_stop(void){closed++;}
int FUN_004132f0(void* hwnd,unsigned message,unsigned result,unsigned device){
    require(hwnd==(void*)1 && message==0x3b9 && result==(failed==2?8u:1u) && device==2,"completion/failure notify");
    require(closed==(failed==1?0u:1u),"audio reset before notification");
    closed_at_notify=closed;notifications++;return 0;
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
    require(frames==1 && notifications==1 && closed==(failed==1?1u:2u),"complete actual MCI state");
    printf("{\"frames\":%u,\"elapsed_ms\":%u,\"audio_queries\":[",frames,now_ms-epoch);
    for(i=0;i<query_count;i++)printf("%s%u",i?",":"",queries[i]);
    printf("],\"notifications\":%u,\"closed\":%u,\"closed_at_notify\":%u}\n",notifications,closed,closed_at_notify);return 0;
}
#endif
