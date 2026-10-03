/* Observe genuine USER32 SendInput -> keyboard messages and GetKeyState.
 * Record both modifier sides, releasing one while the other remains down.
 * No engine/window messages or state are injected directly. F13-F24 use
 * SendInput virtual keys: this Wine layout has no scan mapping for them.
 * The fixture does not claim a physical F13-F24 keyboard/Windows layout. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "keyboard_events.h"
static unsigned step,seen;
static LRESULT CALLBACK window_proc(HWND window,UINT message,WPARAM key,LPARAM flags){
    if(message==WM_KEYDOWN || message==WM_KEYUP || message==WM_SYSKEYDOWN || message==WM_SYSKEYUP){
        printf("%s{\"step\":%u,\"message\":%u,\"vk\":%u,\"up\":%u,\"shift\":[%u,%u,%u],\"control\":[%u,%u,%u],\"alt\":[%u,%u,%u]}",
            seen?",":"",step,message,(unsigned)key,(unsigned)((UINT_PTR)flags>>31),
            (GetKeyState(VK_SHIFT)&0x8000)!=0,(GetKeyState(VK_LSHIFT)&0x8000)!=0,(GetKeyState(VK_RSHIFT)&0x8000)!=0,
            (GetKeyState(VK_CONTROL)&0x8000)!=0,(GetKeyState(VK_LCONTROL)&0x8000)!=0,(GetKeyState(VK_RCONTROL)&0x8000)!=0,
            (GetKeyState(VK_MENU)&0x8000)!=0,(GetKeyState(VK_LMENU)&0x8000)!=0,(GetKeyState(VK_RMENU)&0x8000)!=0);
        fflush(stdout);seen++;return 0;
    }
    return DefWindowProcA(window,message,key,flags);
}
static void require(int ok,const char* why){if(!ok){fprintf(stderr,"Win32 keyboard probe: %s\n",why);exit(1);}}
int main(void){
    WNDCLASSA cls={0};HWND window;MSG message;
    cls.lpfnWndProc=window_proc;cls.hInstance=GetModuleHandleA(NULL);cls.lpszClassName="DD2KeyboardProbe";
    require(RegisterClassA(&cls)!=0,"register window");
    window=CreateWindowA(cls.lpszClassName,"DD2 keyboard probe",WS_OVERLAPPEDWINDOW,0,0,320,240,NULL,NULL,cls.hInstance,NULL);
    require(window!=NULL,"create window");ShowWindow(window,SW_SHOW);SetForegroundWindow(window);SetFocus(window);
    Sleep(100);
    while(PeekMessageA(&message,NULL,0,0,PM_REMOVE))DispatchMessageA(&message);
    printf("[");
    for(step=0;step<sizeof(events)/sizeof(events[0]);step++){
        INPUT input={0};DWORD deadline=GetTickCount()+2000;unsigned before=seen;
        input.type=INPUT_KEYBOARD;input.ki.wVk=events[step].key;input.ki.wScan=(WORD)MapVirtualKeyA(events[step].key,MAPVK_VK_TO_VSC);
        if(events[step].key==VK_LSHIFT)input.ki.wScan=0x2a;
        if(events[step].key==VK_RSHIFT)input.ki.wScan=0x36;
        input.ki.dwFlags=KEYEVENTF_SCANCODE | (events[step].down?0:KEYEVENTF_KEYUP);
        if(events[step].key>=VK_F13 && events[step].key<=VK_F24)
            input.ki.dwFlags&=~KEYEVENTF_SCANCODE;
        else require(input.ki.wScan!=0,"declared key has a real Wine layout scan mapping");
        /* Wine10 server/queue.c normalizes every right modifier, including
         * Shift, from this input flag. It removes the extended bit from the
         * delivered Shift message. This declares that fixture transport;
         * it is not evidence about a physical Windows keyboard scan. */
        if(events[step].key==VK_RSHIFT || events[step].key==VK_RCONTROL || events[step].key==VK_RMENU ||
           (events[step].key>=VK_PRIOR && events[step].key<=VK_DOWN) || events[step].key==VK_INSERT || events[step].key==VK_DELETE)input.ki.dwFlags|=KEYEVENTF_EXTENDEDKEY;
        require(SendInput(1,&input,sizeof(input))==1,"send physical key");
        do{
            /* Observe the original keyboard messages independently of the
             * TranslateMessage/DefWindowProc character/menu side effects. */
            while(PeekMessageA(&message,NULL,0,0,PM_REMOVE))DispatchMessageA(&message);
            if(seen!=before)break;
            Sleep(1);
        }while((LONG)(deadline-GetTickCount())>0);
        if(seen!=before+1)fprintf(stderr,"step=%u key=%u down=%d before=%u seen=%u\n",step,events[step].key,events[step].down,before,seen);
        require(seen==before+1,"one message per physical key transition");
    }
    puts("]");DestroyWindow(window);return 0;
}
