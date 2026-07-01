/* dd2h re-base: Win32 stubs + _ot_dispatch */
#include "ghidra_compat.h"
int _control87(){ return 0; }
int DirectSoundCreate(){ return 0; }
int _DZ(){ return 0; }
int ExitProcess(){ return 0; }
int GetCommandLineA(){ return 0; }
int GetCurrentProcessId(){ return 0; }
int GetEnvironmentStrings(){ return 0; }
int GetLastError(){ return 0; }
int GetModuleFileNameA(){ return 0; }
int GetModuleHandleA(){ return 0; }
int GetStdHandle(){ return 0; }
int GetVersion(){ return 0; }
int joyGetDevCapsA(){ return 0; }
int joyGetPos(){ return 0; }
int mciSendCommandA(){ return 0; }
void _ot_dispatch(int* a,int* b,int* c){ extern void DrawPrim(int); (void)b;(void)c; DrawPrim((int)(long)a); }
int SetStdHandle(){ return 0; }
int timeBeginPeriod(){ return 0; }
int timeEndPeriod(){ return 0; }
int timeKillEvent(){ return 0; }
int timeSetEvent(){ return 0; }
