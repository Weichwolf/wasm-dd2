/* matching-signature stubs for excluded-CRT + a few Win32 fns (replace emcc abort-stubs to run past CRT init) */
#include "ghidra_compat.h"
unsigned __doclose(void* p,int q){ return 0; }
char* __cvt(double v,int n,void* d,void* s){ if(d)*(int*)d=0; if(s)*(int*)s=0; return ""; }
void FUN_0042304c(byte* p){ }
void FUN_00456b30(uint a,uint b){ }
uint FUN_0045a174(void* h){ return 0; }
short GetKeyState(int k){ return 0; }
unsigned GetTickCount(void){ static unsigned t=0; t+=16; return t; }
void* LockResource(void* h){ return (void*)1; }

/* dd2h's exact CRT rand() LCG, shared by native+WASM so the rand()-seeded attract demo is
   bit-identical across builds (and dd2h.exe). The decompiled rand/srand (dd2.c @0x456afc/0x456b1f,
   MSVCRT _threadid-state based) are #if 0'd as "libc CRT"; without this, native links glibc rand()
   and WASM links musl/emscripten rand() -- different LCGs -> the demo's rand()%level sequence
   diverges (native vs WASM bit-diff starts exactly at frame 3, the first demo race frame).
   Matches the decompiled body: seed = seed*0x41c64e6d + 0x3039; return (seed>>16)&0x7fff. */
static unsigned _dd2_rand_seed = 1;
unsigned g_rand_calls = 0;   /* divergence locator: native-vs-WASM rand() call-count per frame */
int rand(void){ g_rand_calls++; _dd2_rand_seed = _dd2_rand_seed * 0x41c64e6dU + 0x3039U; return (int)((_dd2_rand_seed >> 0x10) & 0x7fff); }
void srand(unsigned _Seed){ _dd2_rand_seed = _Seed; }
