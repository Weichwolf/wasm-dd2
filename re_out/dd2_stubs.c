/* matching-signature stubs for excluded-CRT + a few Win32 fns (replace emcc abort-stubs to run past CRT init) */
#include <time.h>
#include "dd2_native.h"
#include "ghidra_compat.h"
unsigned __doclose(void* p,int q){ return 0; }
char* __cvt(double v,int n,void* d,void* s){ if(d)*(int*)d=0; if(s)*(int*)s=0; return ""; }
void FUN_0042304c(byte* p){ }
void FUN_00456b30(uint a,uint b){ }
uint FUN_0045a174(void* h){ return 0; }
/* GetKeyState: only caller is the keyboard-rebind poller (FUN_0044fe64), which tests bit 0x8000
   for "key down". Back it with the live key-state array dd2_input.c maintains (0 stub = rebind
   never detected a key). Not on any bit-exact path (demo/race never call it). */
extern unsigned char dd2_keystate[256];
short GetKeyState(int k){ dd2_native_poll();return (k>=0 && k<256 && dd2_keystate[k]) ? (short)0x8000 : 0; }
/* GetTickCount: the engine paces itself to 25fps with this (frame limiter in Play_Game).
   Deterministic default: a fake +16ms/call ticker (proven for all bit-exact runs).
   DD2_REALTIME=1 (interactive browser/native play): real milliseconds, so the game runs at
   its faithful realtime speed instead of as-fast-as-possible. */
static unsigned dd2_virtual_ms = 0;
/* Read the platform clock without consuming an engine GetTickCount call. MCI
   and the audio sink share this timeline; deterministic inputs stay unchanged. */
unsigned dd2_platform_ms(void){
    static int mode=-1;
    if(mode<0){ extern char* getenv(const char*); mode = getenv("DD2_REALTIME") ? 1 : 0; }
    if(mode){
#ifdef DD2_BROWSER
        double emscripten_get_now(void);
        return (unsigned)emscripten_get_now();
#else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (unsigned)((uint64_t)ts.tv_sec*1000u + ts.tv_nsec/1000000u);
#endif
    }
    return dd2_virtual_ms;
}
unsigned GetTickCount(void){
    if(!getenv("DD2_REALTIME")) dd2_virtual_ms += 16;
    return dd2_platform_ms();
}
void* LockResource(void* h){ return h; /* dd2h passes raw in-memory WAV pointers (sound-bank blob
    + offset, FUN_00416688 -> DSLoadSoundBuffer), never real HRSRC handles: identity is the
    faithful Windows behavior for already-mapped memory */ }

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
