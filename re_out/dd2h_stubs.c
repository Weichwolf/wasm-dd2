/* dd2h re-base: Win32 stubs + _ot_dispatch */
int dd2_dbg_prim=0;
#include <stdio.h>
#include <stdlib.h>
#ifdef DD2_BROWSER
#include <emscripten.h>   /* MUST precede dd2_symbols.h: its symbol #defines (e.g. `data`)
                             would otherwise mangle emscripten header parameter names */
#endif
#include "ghidra_compat.h"
int _control87(){ return 0; }
int DirectSoundCreate(int a,void** b,int c); /* impl below (DD2_SOUND=1 -> real COM shim, else DSERR no-sound path) */
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
/* ---- gamepad backend (Stage 3) ----
   The game detects a joystick ONCE at Init_Controller_ (joyGetPos(0/1) success ->
   joyGetDevCapsA ranges -> input mode DAT_0046303e=1) and then polls joyGetPos per frame
   (faithful Windows semantics: hot-plug after boot is ignored, like the original).
   Backends fill dd2_pad_{present,x,y,buttons}:
   - browser: the shell polls navigator.getGamepads() each frame and calls the exported
     dd2_pad_update(present,x,y,buttons) (x/y 0..65535, center 32768).
   - deterministic tests: DD2_PADSCRIPT=<file>, lines "<flipno> <x> <y> <buttons>", applied
     in ids_flip like DD2_SCRIPT (so pad runs are bit-reproducible on both targets). */
int dd2_pad_present = 0;
unsigned dd2_pad_x = 32768, dd2_pad_y = 32768, dd2_pad_buttons = 0;
void dd2_pad_update(int present, unsigned x, unsigned y, unsigned buttons){
    dd2_pad_present = present; dd2_pad_x = x; dd2_pad_y = y; dd2_pad_buttons = buttons; }
int joyGetDevCapsA(int a,void* b,int c){ (void)a;
    if(!dd2_pad_present || c < 0x20) return 2; /* JOYERR */
    /* JOYCAPSA: wMid+wPid @0, szPname @4 (32), wXmin @0x24, wXmax @0x28, wYmin @0x2c,
       wYmax @0x30 (the game reads 4 UINTs from the stack block at those offsets) */
    { unsigned char* p=(unsigned char*)b; int i; for(i=0;i<c;i++) p[i]=0;
      *(unsigned*)(p+0x24)=0;      /* wXmin */
      *(unsigned*)(p+0x28)=65535;  /* wXmax */
      *(unsigned*)(p+0x2c)=0;      /* wYmin */
      *(unsigned*)(p+0x30)=65535;  /* wYmax */ }
    return 0; }
int joyGetPos(int a,void* b){
    /* DD2_PADSCRIPT implies a pad is plugged in from boot (detection runs in Init_Main) */
    { static int init; if(!init){ init=1; if(getenv("DD2_PADSCRIPT")) dd2_pad_present=1; } }
    if(!dd2_pad_present || a!=0) return 2; /* JOYERR: only pad id 0 */
    /* JOYINFO: wXpos, wYpos, wZpos, wButtons (4 UINTs) */
    { unsigned* ji=(unsigned*)b;
      ji[0]=dd2_pad_x; ji[1]=dd2_pad_y; ji[2]=32768; ji[3]=dd2_pad_buttons; }
    return 0; }
#include "dd2_cd.h"
int mciSendCommandA(int a,int b,int c,int d){ return dd2_mci_send((unsigned)a,(unsigned)b,(unsigned)c,(uint32_t*)(uintptr_t)d); }
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
      static const int PX[4][2]={{372,226},{380,226},{376,227},{384,226}};
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
  /* DD2_OTLOG=1: print every dispatch (type + resolved handler) -- for localizing wasm
     call_indirect signature traps (the trap message has no wasm-side context). */
  { static int _ol=-1; if(_ol<0){ extern char* getenv(const char*); _ol=getenv("DD2_OTLOG")?1:0; }
    if(_ol){ extern int fprintf(FILE*,const char*,...);
      fprintf(stderr,"[OT] t=%02x fn=%p prim=%p\n",*(unsigned char*)((int)piVar1+7),(void*)(unsigned long)_v,(void*)piVar1); } }
  if(_v!=0 && (_v<0x410000u || _v>=0x460000u)) (*(void(*)(int,int,int))(unsigned long)_v)((int)(uintptr_t)piVar1,(int)(uintptr_t)b,(int)(uintptr_t)c);
  { extern int dd2_dbg_prim; dd2_dbg_prim=0; }
}
int SetStdHandle(int a,int b){ (void)a;(void)b; return 0; }
int timeBeginPeriod(int a){ (void)a; return 0; }
int timeEndPeriod(int a){ (void)a; return 0; }
/* mm-timer: the game registers ONE periodic 400ms timer whose callback is the reconstructed
   FUN_0041345c (patch 730, drives Sound_Timer_). Deterministic driver: dd2_snd_mix_flip fires
   it every 10 engine frames (= 400ms at 25 engine fps) instead of wallclock. */
int g_dd2_mmtimer_active = 0;
static unsigned dd2_timer_ms;
int timeKillEvent(int a){ (void)a; g_dd2_mmtimer_active = 0; return 0; }
int timeSetEvent(int a,int b,void* c,int d,int e){ (void)b;(void)c;(void)d;(void)e;
    if(a==400) { g_dd2_mmtimer_active = 1; dd2_timer_ms = dd2_platform_ms(); }
    return 1; }

/* Interactive timer continues in menus and pause, where current_frame stays
   fixed. Deterministic demos retain the existing cf/phase-5 timer below. */
void dd2_mmtimer_poll(void) {
    unsigned now;
    extern void FUN_0041345c(void);
    if (!getenv("DD2_REALTIME") || !g_dd2_mmtimer_active) return;
    now = dd2_platform_ms();
    while ((unsigned)(now-dd2_timer_ms) >= 400) {
        dd2_timer_ms += 400;
        FUN_0041345c();
    }
}

#include <stdio.h>
#include <stdarg.h>
/* dd2h MSVC sprintf wrapper (= dd2.exe FUN_0045672e, shifted). The decompiled version reads x86-stack
   varargs which don't work in C -> reimplement with real varargs. */
int FUN_004568ee(char* buf, const char* fmt, ...){ va_list ap; va_start(ap,fmt); int n=vsprintf(buf,fmt,ap); va_end(ap); return n; }

int __prtf(){ return 0; }

/* dd2h MSVC CRT fseek shim (= dd2.exe FUN_0045607b) */
int FUN_0045623b(void* file,long offset,int whence){ return fseek((FILE*)file,offset,whence); }

/* ================= DirectSound COM shim =================
   Same pattern as the DirectDraw shim in dd2_com.c: real vtable objects so the decompiled
   sound path runs unmodified. The decompile calls (**(code**)(*iface+off))(iface,args).
   Call sites (build/dd2.c): IDirectSound: CreateSoundBuffer +0xc (4 args, DSLoadSoundBuffer
   @0x4156a8), DuplicateSoundBuffer +0x14 (3 args, @0x415f10 path), SetCooperativeLevel +0x18
   (3 args, FUN_00415c8c), Release +8. IDirectSoundBuffer: Release +8, GetStatus +0x24 (2),
   Lock +0x2c (8, FUN_00415838), Play +0x30 (4), SetCurrentPosition +0x34 (2), SetVolume +0x3c
   (2), SetPan +0x40 (2), SetFrequency +0x44 (2), Stop +0x48 (1), Unlock +0x4c (5), Restore
   +0x50 (1, DSFillSoundBuffer @0x415770).
   Gated behind DD2_SOUND=1: without it DirectSoundCreate keeps returning DSERR (the proven
   no-sound path all Stage-2 video verification ran on). DD2_SNDLOG=1 logs every call with the
   engine frame counter @0x462ff0 (deterministic, wallclock-free) for ref alignment.
   Deterministic playback uses the cf counter (25 engine fps), keeping Sound_Timer's
   GetStatus->Release lifecycle reproducible. Interactive DD2_REALTIME playback uses
   elapsed time, including menus and pause where the race counter is fixed. */
typedef struct DSBuf {
    void** vtbl;
    unsigned char* pcm; unsigned size;      /* PCM payload (dwBufferBytes) */
    int freq;                                 /* current sample rate (SetFrequency) */
    int nAvgBytesPerSec;                      /* from WAVEFORMATEX at create time */
    int channels, bits, blockalign;           /* from WAVEFORMATEX at create time */
    int vol, pan;
    int playing, looping;
    int play_cf;                              /* cf @ Play() */
    long long pos_fp;                         /* source position, FRAMES in Q16 (mixer clock) */
    struct DSBuf* master;                     /* dup source (shares data) */
} DSBuf;
static DSBuf* g_dsbufs[256]; static int g_ndsbufs;
static void* g_dsnd_vtbl[32];
static void* g_dsnd_obj = g_dsnd_vtbl;
static void* g_dsb_vtbl[32];
#define SND_CF (*(int*)(unsigned long)0x462ff0u)
void dd2_snd_mix_flip(void);
/* A real DirectSound device keeps playing in menus and advances up to each
   control/query call. Flush elapsed samples before changing its buffer state. */
static void ds_realtime_pump(void){
    if(getenv("DD2_REALTIME")) dd2_snd_mix_flip();
}
static FILE* snd_log(void){ static FILE* f; static int init;
    if(!init){ init=1; if(getenv("DD2_SNDLOG")) f=fopen("/tmp/dd2_sndlog.txt","w"); }
    return f; }
#define SLOG(...) do{ FILE* _f=snd_log(); if(_f){ fprintf(_f,"cf%d ",SND_CF); fprintf(_f,__VA_ARGS__); fputc('\n',_f); fflush(_f);} }while(0)
static int dsb_dur_cf(DSBuf* b){ /* whole cf-ticks a one-shot stays PLAYING */
    int bps = b->nAvgBytesPerSec>0 ? b->nAvgBytesPerSec : 22050;
    long n = ((long)b->size * 25 + bps - 1) / bps;
    return n<1 ? 1 : (int)n; }
/* --- IDirectSoundBuffer methods --- */
static int dsb_release(DSBuf* b){ ds_realtime_pump(); SLOG("DSB %p Release",(void*)b);
    { int i; for(i=0;i<g_ndsbufs;i++) if(g_dsbufs[i]==b){ g_dsbufs[i]=g_dsbufs[--g_ndsbufs]; break; } }
    if(!b->master && b->pcm) free(b->pcm);
    free(b); return 0; }
static int dsb_getstatus(DSBuf* b,unsigned* st){ unsigned s=0;
    ds_realtime_pump();
    /* The mixer owns playing-state and clears it at end-of-data. dur_cf is only
       the deterministic pre-first-flip fallback; real-time queries flush first. */
    if(b->playing){ if(b->looping) s=0x5; /* PLAYING|LOOPING */
        else if(getenv("DD2_REALTIME") || SND_CF - b->play_cf < dsb_dur_cf(b) || b->pos_fp>0) s=0x1; else b->playing=0; }
    if(st)*st=s; SLOG("DSB %p GetStatus -> %u",(void*)b,s); return 0; }
static int dsb_lock(DSBuf* b,unsigned off,unsigned bytes,void** p1,unsigned* s1,void** p2,unsigned* s2,int fl){
    ds_realtime_pump();
    (void)fl; if(off>b->size) off=b->size; if(bytes>b->size) bytes=b->size;
    unsigned first = (off+bytes<=b->size)? bytes : b->size-off;
    if(p1)*p1=b->pcm+off; if(s1)*s1=first;
    if(p2)*p2=(bytes>first)? b->pcm:0; if(s2)*s2=(bytes>first)? bytes-first:0;
    SLOG("DSB %p Lock off=%u bytes=%u",(void*)b,off,bytes); return 0; }
static int dsb_unlock(DSBuf* b,void* p1,unsigned s1,void* p2,unsigned s2){
    (void)p1;(void)p2; SLOG("DSB %p Unlock %u/%u",(void*)b,s1,s2); return 0; }
static int dsb_play(DSBuf* b,int r1,int r2,int flags){ (void)r1;(void)r2;
    ds_realtime_pump();
    b->playing=1; b->looping=(flags&1); b->play_cf=SND_CF; b->pos_fp=0;
    SLOG("DSB %p Play flags=%d freq=%d vol=%d pan=%d",(void*)b,flags,b->freq,b->vol,b->pan); return 0; }
static int dsb_stop(DSBuf* b){ ds_realtime_pump(); b->playing=0; SLOG("DSB %p Stop",(void*)b); return 0; }
static int dsb_setpos(DSBuf* b,unsigned pos){ SLOG("DSB %p SetCurrentPosition %u",(void*)b,pos); return 0; }
static int dsb_setvolume(DSBuf* b,int v){ ds_realtime_pump(); b->vol=v; SLOG("DSB %p SetVolume %d",(void*)b,v); return 0; }
static int dsb_setpan(DSBuf* b,int p){ ds_realtime_pump(); b->pan=p; SLOG("DSB %p SetPan %d",(void*)b,p); return 0; }
static int dsb_setfreq(DSBuf* b,int f){ ds_realtime_pump(); b->freq=f; SLOG("DSB %p SetFrequency %d",(void*)b,f); return 0; }
static int dsb_restore(DSBuf* b){ SLOG("DSB %p Restore",(void*)b); return 0; }
static DSBuf* dsb_new(void){ DSBuf* b=(DSBuf*)calloc(1,sizeof(DSBuf)); b->vtbl=g_dsb_vtbl;
    if(g_ndsbufs<256) g_dsbufs[g_ndsbufs++]=b; return b; }
/* --- IDirectSound methods --- */
static int ds_createbuffer(void* t,int* desc,DSBuf** pp,int outer){ (void)t;(void)outer;
    /* DSBUFFERDESC: +0 dwSize, +4 dwFlags, +8 dwBufferBytes, +0x10 lpwfxFormat */
    DSBuf* b=dsb_new();
    b->size = desc? (unsigned)desc[2] : 0;
    if(b->size){ b->pcm=(unsigned char*)calloc(1,b->size); }
    if(desc && desc[4]){ /* WAVEFORMATEX: wFormatTag+nChannels, nSamplesPerSec, nAvgBytesPerSec,
                             nBlockAlign+wBitsPerSample */
        unsigned char* wfx=(unsigned char*)(unsigned long)(unsigned)desc[4];
        b->channels  = *(unsigned short*)(wfx+2);
        b->freq      = *(int*)(wfx+4);
        b->nAvgBytesPerSec = *(int*)(wfx+8);
        b->blockalign= *(unsigned short*)(wfx+12);
        b->bits      = *(unsigned short*)(wfx+14); }
    if(b->channels<1) b->channels=1; if(b->blockalign<1) b->blockalign=(b->bits==16?2:1)*b->channels;
    if(b->bits!=16) b->bits=8;
    if(pp)*pp=b;
    SLOG("DS CreateSoundBuffer flags=%#x bytes=%u freq=%d -> %p",desc?desc[1]:0,b->size,b->freq,(void*)b);
    return 0; }
static int ds_dupbuffer(void* t,DSBuf* src,DSBuf** pp){ (void)t;
    DSBuf* b=dsb_new(); DSBuf* m=src->master? src->master:src;
    b->pcm=m->pcm; b->size=m->size; b->freq=m->freq; b->nAvgBytesPerSec=m->nAvgBytesPerSec;
    b->channels=m->channels; b->bits=m->bits; b->blockalign=m->blockalign;
    b->master=m; if(pp)*pp=b;
    SLOG("DS DuplicateSoundBuffer %p -> %p",(void*)src,(void*)b); return 0; }
static int ds_setcooplevel(void* t,int hwnd,int level){ (void)t;(void)hwnd;
    SLOG("DS SetCooperativeLevel %d",level); return 0; }
static int ds_release(void* t){ (void)t; SLOG("DS Release"); return 0; }
static int ds_ok(void){ return 0; }
int DirectSoundCreate(int a,void** b,int c){ (void)a;(void)c;
    if(!getenv("DD2_SOUND")){ if(b)*b=0; return 1; /* DSERR: no sound device (proven default) */ }
    { int i; for(i=0;i<32;i++){ g_dsnd_vtbl[i]=(void*)&ds_ok; g_dsb_vtbl[i]=(void*)&ds_ok; } }
    g_dsnd_vtbl[0x08/4]=(void*)&ds_release;
    g_dsnd_vtbl[0x0c/4]=(void*)&ds_createbuffer;
    g_dsnd_vtbl[0x14/4]=(void*)&ds_dupbuffer;
    g_dsnd_vtbl[0x18/4]=(void*)&ds_setcooplevel;
    g_dsb_vtbl[0x08/4]=(void*)&dsb_release;
    g_dsb_vtbl[0x24/4]=(void*)&dsb_getstatus;
    g_dsb_vtbl[0x2c/4]=(void*)&dsb_lock;
    g_dsb_vtbl[0x30/4]=(void*)&dsb_play;
    g_dsb_vtbl[0x34/4]=(void*)&dsb_setpos;
    g_dsb_vtbl[0x3c/4]=(void*)&dsb_setvolume;
    g_dsb_vtbl[0x40/4]=(void*)&dsb_setpan;
    g_dsb_vtbl[0x44/4]=(void*)&dsb_setfreq;
    g_dsb_vtbl[0x48/4]=(void*)&dsb_stop;
    g_dsb_vtbl[0x4c/4]=(void*)&dsb_unlock;
    g_dsb_vtbl[0x50/4]=(void*)&dsb_restore;
    if(b)*b=&g_dsnd_obj;
    SLOG("DirectSoundCreate -> OK");
    return 0; }

/* ---------------- deterministic PCM mixdown ----------------
   dd2_snd_mix_flip() is called once per presented frame (ids_flip, dd2_com.c). It advances the
   mixer clock by the engine frame counter @0x462ff0 (25 engine fps -> 882 output frames per cf
   at 22050 Hz) in deterministic runs. DD2_REALTIME uses elapsed milliseconds and
   carries fractional samples across calls; it also flushes before DS controls.
   Output: s16le stereo 22050 Hz raw to $DD2_SNDPCM.
   Volume/pan use the DirectSound centi-dB model (amp = 10^(centidb/2000), volumes add in dB;
   pan attenuates the far channel) computed in FIXED POINT (hardcoded 2^(i/16) table, no libm --
   glibc/musl pow() differ, this must be bit-identical native vs wasm). Resampling is a Q16
   phase-accumulator point-sampler (classic pre-Vista dsound behavior). These semantics are the
   DEFINITION of the rebuild's audio stream; wine/Windows mixer equivalence is verified against
   captures separately. */
static const unsigned short exp2_q15[17]={
    32768,34219,35734,37316,38968,40693,42495,44376,
    46341,48393,50535,52773,55109,57549,60097,62757,0 /*[16] handled as <<1 of [0]*/};
static int dd2_amp_q15(int centidb){ /* 10^(centidb/2000) in Q15 for centidb<=0 */
    long long e_q16; int n,f,hi,lo; unsigned a,b2;
    if(centidb>=0) return 32768;
    if(centidb<=-10000) return 0;
    e_q16=(long long)centidb*108853/1000;      /* * log2(10)/20/100 in Q16 */
    n=(int)(e_q16>>16); f=(int)(e_q16-((long long)n<<16));   /* n<=0, 0<=f<65536 */
    hi=f>>12; lo=f&0xfff;
    a=exp2_q15[hi]; b2=(hi==15)?65536u:exp2_q15[hi+1];
    a=a+(unsigned)(((b2-a)*(unsigned)lo)>>12);               /* linear interp, Q15 (32768..65536) */
    n=-n; if(n>=16) return 0;
    return (int)(a>>n); }
#ifdef DD2_BROWSER
/* WebAudio sink: schedule each mixed 882-frame tick (22050 Hz s16 stereo) on a running time
   cursor. Lazy AudioContext (browsers require a user gesture before audio can start). */
EM_JS(void, dd2_audio_push, (const short* pcm, int frames), {
    if (!Module._dd2ac) {
        /* Keep the shared device at CD rate even when effects start first.
           Effects retain their original 22050Hz source buffers. */
        try { Module._dd2ac = new AudioContext({sampleRate:44100}); } catch(e){ return; }
        Module._dd2t = 0;
        var resume = function(){ if (Module._dd2ac.state==='suspended') Module._dd2ac.resume(); };
        window.addEventListener('keydown', resume); window.addEventListener('click', resume);
    }
    var ac = Module._dd2ac;
    if (ac.state==='suspended') return;   /* drop ticks until the user gesture */
    var buf = ac.createBuffer(2, frames, 22050);
    var l = buf.getChannelData(0), r = buf.getChannelData(1);
    for (var i=0;i<frames;i++){
        l[i] = HEAP16[(pcm>>1)+i*2]   / 32768;
        r[i] = HEAP16[(pcm>>1)+i*2+1] / 32768;
    }
    var src = ac.createBufferSource(); src.buffer = buf; src.connect(ac.destination);
    if (Module._dd2t < ac.currentTime) Module._dd2t = ac.currentTime + 0.04;
    src.start(Module._dd2t); Module._dd2t += frames/22050;
});
#endif
static unsigned dd2_audio_virtual_ms;
unsigned dd2_audio_ms(void) {
    /* Headless comparisons use the SAME 25Hz simulation clock as effects.
       GetTickCount's synthetic +16/call ticker only drives the frame limiter;
       counting those polling calls would make CD music run over twice as fast. */
    return getenv("DD2_REALTIME") ? dd2_platform_ms() : dd2_audio_virtual_ms;
}
void dd2_snd_mix_flip(void){
    static int last_cf=-1; static FILE* pf; static int pf_init;
    static int clock_init;
    static unsigned last_ms, remainder;
    uint64_t pending;
    int cf,dt,i,t;
    int realtime = getenv("DD2_REALTIME") != NULL;
    static int mix_out = -1;
#ifdef DD2_BROWSER
    if(mix_out<0) mix_out = 1;            /* browser: always produce PCM for the WebAudio sink */
#endif
    if(realtime){
        unsigned now=dd2_platform_ms(), elapsed;
        if(!clock_init){ clock_init=1; last_ms=now; return; }
        elapsed=now-last_ms; last_ms=now;
        pending=(uint64_t)elapsed*22050+remainder;
        remainder=(unsigned)(pending%1000); pending/=1000;
        if(!pending || !getenv("DD2_SOUND")) return;
        dt=(int)((pending+881)/882);
    }else{
        cf=SND_CF;
        if(last_cf<0){ last_cf=cf; return; }
        dt=cf-last_cf; last_cf=cf;
        if(dt<=0||dt>250) return;
        if(!getenv("DD2_SOUND")) { dd2_audio_virtual_ms += (unsigned)dt*40; return; }
        pending=(uint64_t)dt*882;
    }
    if(!pf_init){ pf_init=1; { const char* p=getenv("DD2_SNDPCM"); if(p) pf=fopen(p,"wb"); } }
    for(t=0;t<dt;t++){
        static short out[882*2];
        int frames=pending>882 ? 882 : (int)pending;
        if(pf||mix_out>0){ int k; for(k=0;k<frames*2;k++) out[k]=0; }
        for(i=0;i<g_ndsbufs;i++){
            DSBuf* b=g_dsbufs[i];
            long long step,end_fp; int al,gl,gr,k;
            if(!b->playing||!b->pcm||!b->size||b->freq<=0) continue;
            al=b->blockalign; if(al<1) al=1;
            step=((long long)b->freq<<16)/22050;
            end_fp=(long long)(b->size/al)<<16;
            gl=dd2_amp_q15(b->vol-(b->pan>0?b->pan:0));
            gr=dd2_amp_q15(b->vol+(b->pan<0?b->pan:0));
            for(k=0;k<frames;k++){
                unsigned fr; int sl,sr; const unsigned char* sp;
                if(b->pos_fp>=end_fp){ if(b->looping) b->pos_fp-=end_fp; else { b->playing=0; break; } }
                fr=(unsigned)(b->pos_fp>>16); sp=b->pcm+(long long)fr*al;
                if(b->bits==16){ sl=*(short*)sp; sr=(b->channels>1)?*(short*)(sp+2):sl; }
                else { sl=((int)sp[0]-128)<<8; sr=(b->channels>1)?(((int)sp[1]-128)<<8):sl; }
                b->pos_fp+=step;
                if(pf||mix_out>0){ int vl=out[k*2]+((sl*gl)>>15), vr=out[k*2+1]+((sr*gr)>>15);
                    out[k*2]  =(short)(vl>32767?32767:vl<-32768?-32768:vl);
                    out[k*2+1]=(short)(vr>32767?32767:vr<-32768?-32768:vr); }
            }
        }
        if(pf) fwrite(out,2,frames*2,pf);
#ifdef DD2_BROWSER
        dd2_audio_push(out, frames);
#endif
        pending-=frames;
        if(!realtime) dd2_audio_virtual_ms += 40;
        /* deterministic mm-timer: 400ms period = every 10 engine frames (patch 730 callback).
           Fired AFTER this tick's buffer advance -- the original's timer thread is asynchronous
           and sees playback positions of audio already played by the end of the tick. Phase 5:
           the original's fire phase is wallclock (registration time) with inherent one-period
           jitter; reconstructed from the sound-enabled reference (cf128/cf256 retriggers need
           their channel freed before cf128/cf256; the blocking one-shots exhaust at mixer
           tick 125/255 under our flip-quantized Play starts -- sub-tick start offsets shift
           end positions by <1 tick vs wallclock, so the phase absorbs that quantization). */
        { extern int g_dd2_mmtimer_active; extern void FUN_0041345c(void);
          int tick_cf = last_cf - dt + 1 + t;
          if(!getenv("DD2_REALTIME") && g_dd2_mmtimer_active && tick_cf % 10 == 5) FUN_0041345c(); }
    }
    if(pf) fflush(pf);
}
