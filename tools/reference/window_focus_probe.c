/* Ordinary USER32 foreground requests; never send engine messages or write engine memory. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
static LRESULT CALLBACK procedure(HWND window,UINT message,WPARAM w,LPARAM l){
    if(message==WM_ACTIVATEAPP || message==WM_SETFOCUS || message==WM_KILLFOCUS ||
       message==WM_KEYDOWN || message==WM_KEYUP){
        printf("{\"kind\":\"sink-message\",\"message\":%u,\"wparam\":%u,\"lparam\":%u}\n",message,(unsigned)w,(unsigned)l);fflush(stdout);
    }
    return DefWindowProcA(window,message,w,l);
}
int main(int argc,char** argv){
    WNDCLASSA cls={0};HWND sink,game;MSG message;char previous[64]="",command[64];DWORD until;
    if(argc!=2)return 2;
    cls.lpfnWndProc=procedure;cls.hInstance=GetModuleHandleA(NULL);cls.lpszClassName="DD2FocusProbe";
    if(!RegisterClassA(&cls))return 3;
    sink=CreateWindowA(cls.lpszClassName,"DD2 focus sink",WS_OVERLAPPEDWINDOW,0,0,320,240,0,0,cls.hInstance,0);
    game=FindWindowA("RMPEClass",NULL);
    if(!sink || !game)return 4;
    /* Keep the sink hidden until the first requested focus loss. */
    printf("{\"kind\":\"ready\",\"sink\":%u,\"game\":%u}\n",(unsigned)(UINT_PTR)sink,(unsigned)(UINT_PTR)game);fflush(stdout);
    until=GetTickCount()+60000;
    while((LONG)(until-GetTickCount())>0){
        FILE* input=fopen(argv[1],"r");
        if(input){
            if(fgets(command,sizeof(command),input) && strcmp(command,previous)){
                HWND target=strstr(command,"game")?game:sink;
                int result;if(target==sink)ShowWindow(sink,SW_SHOW);result=SetForegroundWindow(target);
                printf("{\"kind\":\"activation-request\",\"target\":%u,\"return\":%d,\"foreground\":%u}\n",(unsigned)(UINT_PTR)target,result,(unsigned)(UINT_PTR)GetForegroundWindow());fflush(stdout);
                strcpy(previous,command);
            }
            fclose(input);
        }
        while(PeekMessageA(&message,NULL,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageA(&message);}
        Sleep(10);
    }
    DestroyWindow(sink);return 0;
}
