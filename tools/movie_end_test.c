/* Observe real WinMM's default movie end after a bounded seek near the end.
 * This component fixture does not alter or execute the original game. */
#include <windows.h>
#include <digitalv.h>
#include <stdio.h>
#include <stdlib.h>
static void require(MCIERROR result,const char *operation){
    if(result){fprintf(stderr,"movie end: %s failed: %lu\n",operation,(unsigned long)result);exit(1);}
}
int main(int argc,char **argv){
    MCI_OPEN_PARMSA open={0};MCI_DGV_WINDOW_PARMSA window={0};
    MCI_DGV_RECT_PARMS put={0};MCI_SEEK_PARMS seek={0};
    MCI_PLAY_PARMS play={0};MCI_GENERIC_PARMS close={0};
    MCI_SET_PARMS set={0};HWND hwnd;unsigned start;
    if(argc!=3)return 1;
    start=(unsigned)strtoul(argv[2],NULL,10);
    hwnd=CreateWindowA("STATIC","Movie end fixture",WS_POPUP|WS_VISIBLE,
                      0,0,640,480,NULL,NULL,GetModuleHandleA(NULL),NULL);
    if(!hwnd)return 1;
    open.lpstrDeviceType="avivideo";open.lpstrElementName=argv[1];
    require(mciSendCommandA(0,MCI_OPEN,MCI_OPEN_TYPE|MCI_OPEN_ELEMENT|MCI_WAIT,(DWORD_PTR)&open),"open");
    window.hWnd=hwnd;
    require(mciSendCommandA(open.wDeviceID,MCI_WINDOW,MCI_DGV_WINDOW_HWND|MCI_WAIT,(DWORD_PTR)&window),"window");
    put.rc.left=0;put.rc.top=48;put.rc.right=640;put.rc.bottom=384;
    require(mciSendCommandA(open.wDeviceID,MCI_PUT,MCI_DGV_PUT_DESTINATION|MCI_DGV_RECT|MCI_WAIT,(DWORD_PTR)&put),"rectangle");
    set.dwTimeFormat=MCI_FORMAT_FRAMES;
    require(mciSendCommandA(open.wDeviceID,MCI_SET,MCI_SET_TIME_FORMAT|MCI_WAIT,(DWORD_PTR)&set),"frame time format");
    seek.dwTo=start;
    require(mciSendCommandA(open.wDeviceID,MCI_SEEK,MCI_TO|MCI_WAIT,(DWORD_PTR)&seek),"seek");
    /* No MCI_TO is supplied: observe the same default as original Play_Movie.
     * WAIT bounds this fixture; the original uses NOTIFY and pumps messages. */
    require(mciSendCommandA(open.wDeviceID,MCI_PLAY,MCI_WAIT,(DWORD_PTR)&play),"default play");
    require(mciSendCommandA(open.wDeviceID,MCI_CLOSE,MCI_WAIT,(DWORD_PTR)&close),"close");
    DestroyWindow(hwnd);
    printf("{\"start\":%u,\"default_play_completed\":true}\n",start);
    return 0;
}
