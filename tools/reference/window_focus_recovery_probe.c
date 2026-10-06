/* Real USER32 restore/foreground actions; never send fabricated engine messages. */
#include <windows.h>
#include <stdio.h>
#include <string.h>

static LRESULT CALLBACK procedure(HWND window,UINT message,WPARAM w,LPARAM l){
    if(message==WM_ACTIVATEAPP || message==WM_SETFOCUS || message==WM_KILLFOCUS ||
       message==WM_KEYDOWN || message==WM_KEYUP){
        printf("{\"kind\":\"sink-message\",\"message\":%u,\"wparam\":%u,\"lparam\":%u}\n",message,(unsigned)w,(unsigned)l);
        fflush(stdout);
    }
    return DefWindowProcA(window,message,w,l);
}

int main(int argc,char** argv){
    WNDCLASSA cls={0};HWND sink,game;MSG message;
    char previous[64]="",command[64];DWORD until;
    if(argc!=2)return 2;
    cls.lpfnWndProc=procedure;cls.hInstance=GetModuleHandleA(NULL);
    cls.lpszClassName="DD2FocusRecoveryProbe";
    if(!RegisterClassA(&cls))return 3;
    sink=CreateWindowA(cls.lpszClassName,"DD2 recovery sink",WS_OVERLAPPEDWINDOW,
                      0,0,320,240,0,0,cls.hInstance,0);
    game=FindWindowA("RMPEClass",NULL);
    if(!sink || !game)return 4;
    /* Starting the helper must not deactivate the game. */
    printf("{\"kind\":\"ready\",\"sink\":%u,\"game\":%u}\n",
           (unsigned)(UINT_PTR)sink,(unsigned)(UINT_PTR)game);fflush(stdout);
    until=GetTickCount()+60000;
    while((LONG)(until-GetTickCount())>0){
        FILE* input=fopen(argv[1],"r");
        if(input){
            if(fgets(command,sizeof(command),input) && strcmp(command,previous)){
                HWND target=strstr(command,"game")?game:sink;
                DEVMODEA mode={0};int result;
                mode.dmSize=sizeof(mode);
                printf("{\"kind\":\"before-foreground\",\"target\":%u,\"iconic\":%d,\"visible\":%d}\n",
                       (unsigned)(UINT_PTR)target,IsIconic(target),IsWindowVisible(target));
                if(target==sink)ShowWindow(sink,SW_SHOW);
                else ShowWindow(target,SW_RESTORE);
                result=SetForegroundWindow(target);
                printf("{\"kind\":\"activation-request\",\"target\":%u,\"return\":%d,\"foreground\":%u,\"iconic\":%d,\"visible\":%d,\"mode_read\":%d",
                       (unsigned)(UINT_PTR)target,result,(unsigned)(UINT_PTR)GetForegroundWindow(),
                       IsIconic(target),IsWindowVisible(target),
                       EnumDisplaySettingsA(NULL,ENUM_CURRENT_SETTINGS,&mode));
                printf(",\"width\":%lu,\"height\":%lu,\"bits\":%lu}\n",
                       mode.dmPelsWidth,mode.dmPelsHeight,mode.dmBitsPerPel);fflush(stdout);
                strcpy(previous,command);
            }
            fclose(input);
        }
        while(PeekMessageA(&message,NULL,0,0,PM_REMOVE)){
            TranslateMessage(&message);DispatchMessageA(&message);
        }
        Sleep(10);
    }
    DestroyWindow(sink);return 0;
}
