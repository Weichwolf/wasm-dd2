#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Independent USER32 oracle. All input is SendInput to the real foreground
 * process; inactive states are observed without injecting window messages. */
static int active,down_count,up_count;
static LRESULT CALLBACK procedure(HWND h,UINT m,WPARAM w,LPARAM l){
    if(m==WM_ACTIVATEAPP)active=(int)w;
    if(m==WM_KEYDOWN || m==WM_SYSKEYDOWN)down_count++;
    if(m==WM_KEYUP || m==WM_SYSKEYUP)up_count++;
    return DefWindowProcA(h,m,w,l);
}
static void require(int ok,const char* why){if(!ok){fprintf(stderr,"focus API probe: %s\n",why);exit(1);}}
static void pump(unsigned ms){
    DWORD end=GetTickCount()+ms;MSG m;
    do{while(PeekMessageA(&m,0,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageA(&m);}Sleep(1);}while((LONG)(end-GetTickCount())>0);
}
static void key(unsigned vk,int down){
    INPUT input={0};input.type=INPUT_KEYBOARD;input.ki.wVk=(WORD)vk;
    input.ki.wScan=(WORD)MapVirtualKeyA(vk,MAPVK_VK_TO_VSC);
    input.ki.dwFlags=KEYEVENTF_SCANCODE | (down?0:KEYEVENTF_KEYUP);
    if(vk==VK_LEFT || vk==VK_RSHIFT)input.ki.dwFlags|=KEYEVENTF_EXTENDEDKEY;
    require(SendInput(1,&input,sizeof(input))==1,"SendInput");pump(20);
}
static void row(const char* label){
    printf("{\"label\":\"%s\",\"active\":%d,\"left\":%d,\"a\":%d,\"shift\":[%d,%d,%d],\"control\":[%d,%d,%d],\"down\":%d,\"up\":%d}",label,active,
       !!(GetKeyState(VK_LEFT)&0x8000),!!(GetKeyState('A')&0x8000),!!(GetKeyState(VK_SHIFT)&0x8000),!!(GetKeyState(VK_LSHIFT)&0x8000),!!(GetKeyState(VK_RSHIFT)&0x8000),
       !!(GetKeyState(VK_CONTROL)&0x8000),!!(GetKeyState(VK_LCONTROL)&0x8000),!!(GetKeyState(VK_RCONTROL)&0x8000),down_count,up_count);fflush(stdout);
}
int main(int argc,char** argv){
    int sink_mode=argc>1;WNDCLASSA cls={0};HWND window,sink;char command[2048],self[1024];STARTUPINFOA startup={0};PROCESS_INFORMATION process={0};
    (void)argv;cls.hInstance=GetModuleHandleA(0);cls.lpfnWndProc=procedure;cls.lpszClassName=sink_mode?"DD2FocusStateSink":"DD2FocusStateProbe";
    require(RegisterClassA(&cls)!=0,"register");window=CreateWindowA(cls.lpszClassName,cls.lpszClassName,WS_OVERLAPPEDWINDOW,0,0,320,240,0,0,cls.hInstance,0);require(window!=0,"window");
    if(sink_mode){pump(15000);return 0;}
    ShowWindow(window,SW_SHOW);SetForegroundWindow(window);SetFocus(window);pump(100);
    GetModuleFileNameA(0,self,sizeof(self));snprintf(command,sizeof(command),"\"%s\" sink",self);startup.cb=sizeof(startup);
    require(CreateProcessA(0,command,0,0,FALSE,0,0,0,&startup,&process)!=0,"child");
    {DWORD end=GetTickCount()+3000;do{sink=FindWindowA("DD2FocusStateSink",0);pump(5);}while(!sink && (LONG)(end-GetTickCount())>0);}require(sink!=0,"sink ready");
    printf("[");row("baseline");
    key(VK_LEFT,1);key(VK_LSHIFT,1);key(VK_RSHIFT,1);key(VK_LCONTROL,1);printf(",");row("down-active");
    ShowWindow(sink,SW_SHOW);require(SetForegroundWindow(sink),"activate sink");pump(100);require(!active,"parent deactivated");printf(",");row("inactive-held");
    key(VK_LEFT,0);printf(",");row("released-outside");
    require(SetForegroundWindow(window),"reactivate parent");pump(100);require(active,"parent active");printf(",");row("reactivated-left-up-shifts-held");
    key(VK_LEFT,1);key(VK_LEFT,0);key(VK_LSHIFT,0);key(VK_RSHIFT,0);key(VK_LCONTROL,0);printf(",");row("released-active");
    require(SetForegroundWindow(sink),"activate sink for unseen key");pump(100);key('A',1);printf(",");row("pressed-outside");
    require(SetForegroundWindow(window),"reactivate with unseen key held");pump(100);printf(",");row("reactivated-outside-key-held");
    key('A',0);printf(",");row("outside-key-released-active");printf("]\n");
    TerminateProcess(process.hProcess,0);WaitForSingleObject(process.hProcess,3000);CloseHandle(process.hThread);CloseHandle(process.hProcess);DestroyWindow(window);return 0;
}
