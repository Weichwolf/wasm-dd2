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

#include <stdio.h>
#include <stdarg.h>
/* dd2h MSVC sprintf wrapper (= dd2.exe FUN_0045672e, shifted). The decompiled version reads x86-stack
   varargs which don't work in C -> reimplement with real varargs. */
int FUN_004568ee(char* buf, const char* fmt, ...){ va_list ap; va_start(ap,fmt); int n=vsprintf(buf,fmt,ap); va_end(ap); return n; }

int __prtf(){ return 0; }
