/* dd2h re-base: Win32 stubs + _ot_dispatch */
#include <stdio.h>
#include "ghidra_compat.h"
int _control87(){ return 0; }
int DirectSoundCreate(int a,void** b,int c){ (void)a;(void)c; if(b)*b=0; return 1; /* DSERR: no sound device */ }
int _DZ(int x){ return x; }  /* Watcom checked-divide helper: Ghidra renders `a / _DZ(b)` */
void FUN_00448e50(int _car){ (void)_car; }  /* @0x448e50: empty no-op strip-trigger handler (Ghidra didn't
  export it). Typed void(int) to match the Strip_Trigger_Handler indirect call (car index arg) -- all
  entries of the trigger table @0x467084 must share one signature for the wasm call_indirect type check. */
void ExitProcess(int a){ extern int printf(const char*,...); extern void exit(int); printf("[ExitProcess] code=%d\n",a); exit(a); }
int GetCommandLineA(){ return 0; }
int GetCurrentProcessId(){ return 0; }
int GetEnvironmentStrings(){ return 0; }
int GetLastError(){ return 0; }
int GetModuleFileNameA(void* a,char* b,int c){ (void)a; if(b&&c){b[0]=0;} return 0; }
int GetModuleHandleA(const char* a){ (void)a; return 0; }
int GetStdHandle(int a){ (void)a; return 0; }
int GetVersion(){ return 0; }
int joyGetDevCapsA(int a,void* b,int c){ (void)a;(void)b;(void)c; return 2; }
int joyGetPos(int a,void* b){ (void)a;(void)b; return 2; /* JOYERR_NOCANDO: no pad */ }
int mciSendCommandA(int a,int b,int c,int d){ (void)a;(void)b;(void)c;(void)d; return 0; }
#include "dd2_symbols.h"
/* Real _ot_dispatch (= dd2.exe): call the primitive handler from _primfuncs[type] (dd2_relocate has
   rewritten in-fnmap entries to real fn-pointers). A relocated entry is non-null OUTSIDE the image
   rasterizer range [0x410000,0x460000). No recursion into DrawPrim. */
void _ot_dispatch(int* piVar1,int* b,int* c){
  (void)b;(void)c;
  unsigned _v=((unsigned*)&_primfuncs)[*(unsigned char*)((int)piVar1+7)];  /* dword fn-ptr table (explicit: _primfuncs is byte-typed in dd2_symbols.h) */
  /* DD2_HIST=<cf>: per-frame prim-type histogram of the real OT walk (compare vs tools/refhist.sh). */
  { static int _want=-2; if(_want==-2){ extern char* getenv(const char*); char* e=getenv("DD2_HIST"); extern int atoi(const char*); _want=e?atoi(e):-1; }
    if(_want>=0){ static unsigned _h[256]; static int _n=0,_last=-1; int _cf=*(int*)(uintptr_t)0x462ff0;
      if(_cf==_want){ unsigned char _t=*(unsigned char*)((int)piVar1+7); _h[_t]++; _n++; _last=_cf;
        if(_t==0x2c){ static FILE* _f;
          if(!_f) _f=fopen("/tmp/our_ft4.txt","w");
          if(_f) fprintf(_f,"%d %d %u %u %#x %#x\n",*(short*)((int)piVar1+8),*(short*)((int)piVar1+10),
            *(unsigned char*)((int)piVar1+12),*(unsigned char*)((int)piVar1+13),
            (unsigned)*(unsigned short*)((int)piVar1+14),(unsigned)*(unsigned short*)((int)piVar1+22)); } }
      else if(_last==_want){ extern int printf(const char*,...); int _i; printf("[HIST] cf%d n=%d:",_want,_n);
        for(_i=0;_i<256;_i++) if(_h[_i]) printf(" %02x:%u",_i,_h[_i]); printf("\n"); _last=-1; } } }
  if(_v!=0 && (_v<0x410000u || _v>=0x460000u)) (*(void(*)(int,int,int))(unsigned long)_v)((int)(uintptr_t)piVar1,(int)(uintptr_t)b,(int)(uintptr_t)c);
}
int SetStdHandle(int a,int b){ (void)a;(void)b; return 0; }
int timeBeginPeriod(int a){ (void)a; return 0; }
int timeEndPeriod(int a){ (void)a; return 0; }
int timeKillEvent(int a){ (void)a; return 0; }
int timeSetEvent(int a,int b,void* c,int d,int e){ (void)a;(void)b;(void)c;(void)d;(void)e; return 0; }

#include <stdio.h>
#include <stdarg.h>
/* dd2h MSVC sprintf wrapper (= dd2.exe FUN_0045672e, shifted). The decompiled version reads x86-stack
   varargs which don't work in C -> reimplement with real varargs. */
int FUN_004568ee(char* buf, const char* fmt, ...){ va_list ap; va_start(ap,fmt); int n=vsprintf(buf,fmt,ap); va_end(ap); return n; }

int __prtf(){ return 0; }

/* dd2h MSVC CRT fseek shim (= dd2.exe FUN_0045607b) */
int FUN_0045623b(void* file,long offset,int whence){ return fseek((FILE*)file,offset,whence); }
