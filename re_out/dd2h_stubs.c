/* dd2h re-base: Win32 stubs + _ot_dispatch */
int dd2_dbg_prim=0;
#include <stdio.h>
#include <stdlib.h>
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
  /* DD2_BYTEWATCH: poll a byte each dispatch; report the OT-walk position where it changed
     (wasm-compatible "watchpoint" -- gdb can't attach to the wasm target). */
  { static int _bw=-2; extern char* getenv(const char*); extern int atoi(const char*);
    if(_bw==-2){ char* e=getenv("DD2_BYTEWATCH"); _bw=e?(int)strtoul(e,0,0):-1; }
    if(_bw>0){ static unsigned char _last; static int _init=0; static void* _bwprev;
      unsigned char _c=*(unsigned char*)(uintptr_t)_bw;
      if(!_init){ _init=1; _last=_c; }
      if(_c!=_last){ extern int printf(const char*,...);
        printf("[BW] %#x %02x->%02x cf=%d by prev-prim=%p type=%02x (next=%p)\n",_bw,_last,_c,
          *(int*)(uintptr_t)0x462ff0,_bwprev,_bwprev?*(unsigned char*)((int)(uintptr_t)_bwprev+7):0,(void*)piVar1);
        _last=_c; }
      _bwprev=(void*)piVar1; } }
  /* DD2_PIXWIN=<cf>: winner-prim probe -- before each prim, sample probe pixels; when one changed,
     the PREVIOUS prim painted it. Probes: (100,40),(500,60) cloud band; (320,150) box face; (320,430) ground. */
  { static int _pw=-2; if(_pw==-2){ extern char* getenv(const char*); char* e=getenv("DD2_PIXWIN"); extern int atoi(const char*); _pw=e?atoi(e):-1; }
    if(_pw>=0){
      static const int PX[4][2]={{113,431},{300,460},{623,351},{612,248}};
      static unsigned char _last[4]; static void* _prev; static int _init=0, _done=0;
      int _cf=*(int*)(uintptr_t)0x462ff0;
      if(_cf<=_pw && !_done){
        extern int printf(const char*,...);
        int _i;
        if(!_init){ _init=1; _prev=0;
          for(_i=0;_i<4;_i++) _last[_i]=*(unsigned char*)(uintptr_t)(0x700450+PX[_i][1]*640+PX[_i][0]); }
        for(_i=0;_i<4;_i++){
          unsigned char _c=*(unsigned char*)(uintptr_t)(0x700450+PX[_i][1]*640+PX[_i][0]);
          if(_c!=_last[_i]){
            printf("[PIXWIN] cf%d probe%d (%d,%d) %02x->%02x by prim=%p type=%02x\n",_cf,_i,PX[_i][0],PX[_i][1],
              _last[_i],_c,_prev,_prev?*(unsigned char*)((int)(uintptr_t)_prev+7):0);
            _last[_i]=_c; } }
        _prev=(void*)piVar1;
      } else if(_cf>_pw) _done=1; } }
  /* DD2_HIST=<cf>: per-frame prim-type histogram of the real OT walk (compare vs tools/refhist.sh). */
  { static int _want=-2; if(_want==-2){ extern char* getenv(const char*); char* e=getenv("DD2_HIST"); extern int atoi(const char*); _want=e?atoi(e):-1; }
    if(_want>=0){ static unsigned _h[256]; static int _n=0,_last=-1; int _cf=*(int*)(uintptr_t)0x462ff0;
      if(_cf==_want){ unsigned char _t=*(unsigned char*)((int)piVar1+7); _h[_t]++; _n++; _last=_cf;
        if(_t==0x2c||_t==0x1c){ static FILE* _f;
          if(!_f) _f=fopen("/tmp/our_ft4.txt","w");
          if(_f) fprintf(_f,"%d %d %u %u %#x %#x %p\n",*(short*)((int)piVar1+8),*(short*)((int)piVar1+10),
            *(unsigned char*)((int)piVar1+12),*(unsigned char*)((int)piVar1+13),
            (unsigned)*(unsigned short*)((int)piVar1+14),(unsigned)*(unsigned short*)((int)piVar1+22),(void*)piVar1); } }
      else if(_last==_want){ extern int printf(const char*,...); int _i; printf("[HIST] cf%d n=%d:",_want,_n);
        for(_i=0;_i<256;_i++) if(_h[_i]) printf(" %02x:%u",_i,_h[_i]); printf("\n"); _last=-1; } } }
  { extern int dd2_dbg_prim; extern char* getenv(const char*);
    static int _dbgp=-2; if(_dbgp==-2){ char* e=getenv("DD2_DBGPRIM"); extern long strtol(const char*,char**,int); _dbgp=e?(int)strtol(e,0,0):-1; }
    if(_dbgp>0 && (int)(uintptr_t)piVar1==_dbgp && *(int*)(uintptr_t)0x462ff0==3) dd2_dbg_prim=1; }
  if(_v!=0 && (_v<0x410000u || _v>=0x460000u)) (*(void(*)(int,int,int))(unsigned long)_v)((int)(uintptr_t)piVar1,(int)(uintptr_t)b,(int)(uintptr_t)c);
  { extern int dd2_dbg_prim; dd2_dbg_prim=0; }
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
