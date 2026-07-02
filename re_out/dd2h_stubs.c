/* dd2h re-base: Win32 stubs + _ot_dispatch */
#include "ghidra_compat.h"
int _control87(){ return 0; }
int DirectSoundCreate(){ return 0; }
int _DZ(int x){ return x; }  /* Watcom checked-divide helper: Ghidra renders `a / _DZ(b)`; faithful = plain divisor */
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
#include "dd2_symbols.h"
/* Real _ot_dispatch (= dd2.exe): call the primitive handler from _primfuncs[type] (dd2_relocate has
   rewritten in-fnmap entries to real fn-pointers). A relocated entry is non-null OUTSIDE the image
   rasterizer range [0x410000,0x460000). No recursion into DrawPrim. */
void _ot_dispatch(int* piVar1,int* b,int* c){
  (void)b;(void)c;
  unsigned _v=((unsigned*)&_primfuncs)[*(unsigned char*)((int)piVar1+7)];  /* dword fn-ptr table (explicit: _primfuncs is byte-typed in dd2_symbols.h) */
  if(_v!=0 && (_v<0x410000u || _v>=0x460000u)) (*(void(*)(int*))(unsigned long)_v)(piVar1);
}
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

/* dd2h MSVC CRT fseek shim (= dd2.exe FUN_0045607b) */
int FUN_0045623b(void* file,long offset,int whence){ return fseek((FILE*)file,offset,whence); }
