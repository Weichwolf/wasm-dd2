/* Native 32-bit debug harness for the DD2 engine — same init sequence as the WASM
 * dd2_runtime.c main(), but mmaps the image region at its real VA first (WASM gets that
 * for free via GLOBAL_BASE; native must MAP_FIXED it). Purpose: run the demo path under
 * ASan/gdb to localize the Track_Follow OOB with a real backtrace + faulting address.
 * Build: tools/build_native.sh   Run: (from DestructionDerby2/) /tmp/dd2_native */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <execinfo.h>

/* DEBUG: poly-command ring buffer (BSS -> no heap shift -> doesn't move the _gpoly-desync heisenbug).
   Recorded by a debug-patch in FUN_0041fb7c's walk loop; dumped here on crash. */
int g_plog[2048]; volatile int g_pidx = 0;
/* DEBUG: last Draw_Scene_Object dispatch {block, objidx, num_scene_objects, param_1, param_1[1]} */
volatile int g_lastobj[5];
static void segv(int sig, siginfo_t* si, void* uc){
    (void)uc;
    const char* nm = sig==SIGFPE?"SIGFPE":sig==SIGSEGV?"SIGSEGV":sig==SIGBUS?"SIGBUS":"SIG?";
    fprintf(stderr, "\n*** %s at addr %p (code %d) ***\n", nm, si->si_addr, si->si_code);
    if (getenv("DD2_PLOG")){
        int i, k=g_pidx; fprintf(stderr,"--- last poly commands (type,count,gpoly) ---\n");
        for (i = k>16?k-16:0; i < k; i++)
            fprintf(stderr,"  [%d] type=%d count=%d gpoly=0x%x\n", i, g_plog[(i&511)*4], g_plog[(i&511)*4+1], (unsigned)g_plog[(i&511)*4+2]);
    }
    if (getenv("DD2_OBJLOG")){
        fprintf(stderr,"--- last Draw_Scene_Object: block=%d objidx=%d num=%d param_1=0x%x p1[1]=0x%x %s ---\n",
            g_lastobj[0], g_lastobj[1], g_lastobj[2], (unsigned)g_lastobj[3], (unsigned)g_lastobj[4],
            g_lastobj[1] >= g_lastobj[2] ? "OVER-WALK" : "in-bounds");
        unsigned g = (unsigned)g_lastobj[4]; int j;   /* geometry struct hexdump */
        if (g >= 0x400000u && g < 0x900000u){ fprintf(stderr,"  geom@0x%x:", g);
            for (j=0;j<0x30;j+=4) fprintf(stderr," %08x", *(unsigned*)(unsigned long)(g+j));
            fprintf(stderr,"\n  (+0x28 poly-ptr = %08x)\n", *(unsigned*)(unsigned long)(g+0x28)); }
    }
    void* bt[24]; int n = backtrace(bt, 24);
    backtrace_symbols_fd(bt, n, 2);
    _exit(139);
}
static void install_segv(void){
    struct sigaction sa; memset(&sa,0,sizeof sa);
    sa.sa_sigaction = segv; sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, 0); sigaction(SIGBUS, &sa, 0); sigaction(SIGFPE, &sa, 0);
}
#define CK(s) do{ fprintf(stderr,"[native] " s "\n"); fflush(stderr); }while(0)

#define IMG_BASE 0x400000u
/* image is ~5.5MB at 0x400000; the engine's heap pool lives just above (WASM put it at GLOBAL_BASE
 * 0xA00000+). Map a big contiguous region 0x400000..0x7000000 (~108MB) to cover image BSS + heap pool.
 * Safe under 32-bit -no-pie: stays below the 0x08048000 text base. */
#define IMG_SIZE 0x06C00000u

unsigned char* g_image = (unsigned char*)(uintptr_t)IMG_BASE;
extern void dd2_relocate(void);
extern void dd2_com_init(void);
extern void Read_Directory(const char*);
extern void __InitRtns(void);
extern int  Init_Application(void* hInst);
extern int  Play_Game(void);
extern int  DemoMode(void);
extern void Set_Draw_Mode(int);
#define _current_level (*(int*)0x936ff4)  /* image slot (dual-symbol fix) */
extern void Setup_Pad(int);
extern void Order_Cars(void);
extern void Init_Front_End(void);
extern int  rand(void);
extern int  dd2_input_selftest(void);   /* dd2_input.c: verify key->Translate_Keypress->pad-state path */

/* Fixed-level demo entry: faithful copy of DemoMode@0x44b4e0 but with _current_level forced
 * from DD2_LEVEL (for matched ref-vs-WASM byte comparison — DemoMode's rand()%10+1 picks a level
 * that differs from dd2h's front-end-driven demo, so we pin both to the same level). Not game code. */
#define W32(va,val) (*(int*)(uintptr_t)(va) = (int)(val))
#define R32(va)     (*(int*)(uintptr_t)(va))
static int DemoModeLevel(int lvl){
    /* Seed keyboard pad type BEFORE Setup_Pad records _recorded_pad_type -- MUST match the WASM
     * runtime (patch 315) and the reference (dd2h front-end sets 0x754451=1 for keyboard). Without
     * this, _recorded_pad_type=0 here vs 1 on wasm -> the demo replay decodes differently ->
     * native/wasm state divergence from the first pad-dependent frame. */
    *(unsigned char*)0x754451 = 1;
    Setup_Pad(1);
    W32(0x93de1c, R32(0x4673f4));            /* save race_type */
    W32(0x93de18, R32(0x4673f8));            /* save race_mode */
    W32(0x46385c, 1);                        /* demo_mode = 1 */
    W32(0x93de14, R32(0x467400));            /* save race_car */
    W32(0x467400, 2);                        /* race_car = 2 */
    W32(0x93de10, R32(0x46765c));            /* save num_cars */
    W32(0x46765c, 0x14);                     /* num_cars = 0x14 */
    W32(0x4673f8, 0);                        /* race_mode = 0 */
    W32(0x4673f4, 0);                        /* race_type = 0 */
    { int iVar1 = rand(); _current_level = lvl ? lvl : (iVar1 % 10 + 1); }
    fprintf(stderr, "[native] DemoModeLevel: _current_level=%d\n", _current_level);
    Order_Cars();
    return Play_Game();
}

/* Stage 3: a LIVE race (demo_mode=0) — same setup as DemoModeLevel but reads live input instead of
 * the recorded-pad replay. The player car responds to _pad_* (set via Translate_Keypress); AI drives
 * the rest. Tests whether the Stage-1 crash fixes made real gameplay (not just the attract demo) run. */
static int PlayModeLevel(int lvl){
    /* Seed keyboard pad type BEFORE Setup_Pad records _recorded_pad_type -- MUST match the WASM
     * runtime (patch 315) and the reference (dd2h front-end sets 0x754451=1 for keyboard). Without
     * this, _recorded_pad_type=0 here vs 1 on wasm -> the demo replay decodes differently ->
     * native/wasm state divergence from the first pad-dependent frame. */
    /* DD2_PADSCRIPT/DD2_PAD: play on the joystick path -- set the FE options choice
       _pad_option=1 (what the controls menu writes) so Setup_Pad runs Setup_Joystick's
       detection (joyGetPos/joyGetDevCapsA against the shim backend) instead of keyboard.
       Pad-type seed = 2 (joystick): the value the per-frame poll FUN_00422da4 reports in
       mode 1, so _recorded_pad_type matches and Play_Game doesn't auto-pause. */
    if (getenv("DD2_PADSCRIPT") || getenv("DD2_PAD")) {
        *(int*)(uintptr_t)0x467414 = 1;          /* _pad_option */
        *(unsigned char*)0x754451 = 2;
    } else
        *(unsigned char*)0x754451 = 1;
    Setup_Pad(1);
    W32(0x93de1c, R32(0x4673f4));
    W32(0x93de18, R32(0x4673f8));
    W32(0x46385c, 0);                        /* demo_mode = 0 -> LIVE input */
    W32(0x93de14, R32(0x467400));
    W32(0x467400, 2);
    W32(0x93de10, R32(0x46765c));
    W32(0x46765c, 0x14);
    W32(0x4673f8, 0);
    W32(0x4673f4, 0);
    { int iVar1 = rand(); _current_level = lvl ? lvl : (iVar1 % 10 + 1); }
    fprintf(stderr, "[native] PlayModeLevel (LIVE, demo_mode=0): _current_level=%d\n", _current_level);
    Order_Cars();
    /* DD2_HOLD=<vk>: hold a key down for the whole race (no key-up) to prove input controls the car.
     * e.g. DD2_HOLD=0x25 (LEFT) makes the player car steer left every frame it's read. */
    { const char* h = getenv("DD2_HOLD");
      if (h) { extern void dd2_key_event(unsigned int, int);
               unsigned int vk = (unsigned int)strtol(h,0,0);
               dd2_key_event(vk, 1);
               fprintf(stderr, "[native] holding VK 0x%x for the whole race\n", vk); } }
    return Play_Game();
}

/* CRT helpers (mirror dd2_runtime.c — that file is excluded from the native link to avoid a 2nd main) */
static int g_thread[256];
int dd2_getthread(void){ return (int)(uintptr_t)g_thread; }
int dd2_crt_lock(int a){ return a; }

/* CRT-disable-list functions undefined at native link (cold CRT paths; WASM allows undefined refs) */
int FUN_0045987f(int param_1){ (void)param_1; return 0; }
int FUN_0045b75d(void){ return 0; }
/* __math2err is #if 0'd (-> libc) but __math1err still calls it; stub the error-corrector as pass-through */
long long __math2err(unsigned a, void* b, void* c){ (void)a;(void)b;(void)c; return 0; }

static void map_image_region(void){
    void* p = mmap((void*)(uintptr_t)IMG_BASE, IMG_SIZE,
                   PROT_READ|PROT_WRITE,
                   MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (p != (void*)(uintptr_t)IMG_BASE){
        fprintf(stderr, "FATAL: could not map image region at 0x%x (got %p)\n", IMG_BASE, p);
        exit(1);
    }
}

static void load_image(const char* path){
    FILE* f = fopen(path, "rb");
    if(!f){ fprintf(stderr, "FATAL: cannot open %s\n", path); exit(1); }
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    if (n > (long)IMG_SIZE){ fprintf(stderr,"FATAL: image %ld > region %u\n", n, IMG_SIZE); exit(1); }
    fread((void*)(uintptr_t)IMG_BASE, 1, n, f); fclose(f);
    fprintf(stderr, "[native] loaded %ld bytes of image at 0x%x\n", n, IMG_BASE);
}

int main(void){
    if(!getenv("DD2_NOSEGV")) install_segv();
    /* FP-PRECISION TEST: force x87 to 53-bit (double) precision to match WASM's 64-bit IEEE
     * (and likely dd2h's MSVC CRT default). Linux default is 0x037f = 64-bit extended (80-bit
     * intermediates) which would diverge from WASM on any float op. */
    { unsigned short cw = 0x027f; __asm__ __volatile__("fldcw %0" :: "m"(cw)); }
    map_image_region();
    load_image("dd2_image.bin");
    CK("dd2_relocate()");           dd2_relocate();
    /* CRT constructors (FPU init: _fpreset/__init_8087, etc.) aren't in the dispatch map, so relocate
     * leaves them as raw x86 VAs -> __InitRtns would jump into non-exec image data. They're irrelevant
     * to the port; zero any ctor slot still holding a raw code VA so __InitRtns skips it (it null-checks). */
    { unsigned va; for(va=0x46ff48; va<0x46ff60; va+=6){
        unsigned* slot=(unsigned*)(uintptr_t)(va+2);
        if(*slot>=0x410000 && *slot<0x460000) *slot=0; } }
    *(int*)(uintptr_t)0x46c32c = (int)(uintptr_t)&dd2_getthread;      /* __GetThreadPtr */
    { unsigned va; for(va=0x46c330; va<=0x46c364; va+=4) *(int*)(uintptr_t)va = (int)(uintptr_t)&dd2_crt_lock; }
    CK("__InitRtns()");             __InitRtns();
    CK("dd2_com_init()");           dd2_com_init();
    /* The original boots through Init_Main @0x445814: Init_Controller_, Profile_Init, Sound_Init,
     * VSync+VSyncCallback, InitCardSystem (loads/creates SaveGames -> card buffer 0x754460),
     * Read_Directory("DIRINFO"), Read_CD_Toc_, Load_Game_Vags. Run the real thing. */
    /* DD2_SOUND=1: original boot order is ddmain: Play_Intro (-> FUN_004159f8 = DSInit) BEFORE
     * Init_Main -- Load_Game_Vags' bank loader FUN_00416688 only creates the DS master buffers
     * (table 0x74f020) when DAT_00462d68 is already 1, and Play_Sound derefs them unchecked.
     * So run the real DSInit against the COM shim first. Default: proven no-sound path. */
    if(getenv("DD2_SOUND")){ extern void FUN_004159f8(int); *(int*)(uintptr_t)0x462d68 = 0;
        CK("DSInit FUN_004159f8()"); FUN_004159f8(1); }
    { extern void Init_Main(void); CK("Init_Main()"); Init_Main(); }
    /* default: force "skip DirectSound COM" in Init_Application; with DD2_SOUND the flag already
     * holds the live init state (1) and Init_Application's own DSInit call skips itself. */
    if(!getenv("DD2_SOUND")) *(int*)(uintptr_t)0x462d68 = 1;
    CK("Init_Application()");       Init_Application((void*)1);
    *(int*)(uintptr_t)0x463010 = -1;
    /* ddmain@0x423b28 sets pal_flag=0 before any init; our CRT bypass skips ddmain, and the image
     * default is 1 -> Init_Overlays would skip its -0x10 tpage adjustment and every overlay/HUD/
     * lamp texture would sample one VRAM page off (the unlit-LED / wrong-texture defect). */
    *(int*)(uintptr_t)0x462fe8 = 0;
    /* Replicate the rest of the front-end's exit state (fog window, far_z_clip, attract-camera
     * params, track_lookup tables, player count, ...) captured from the original: dd2_festate.c. */
    { extern void dd2_apply_frontend_state(void); dd2_apply_frontend_state(); }
    CK("Set_Draw_Mode(0)");         Set_Draw_Mode(0);
    if(!getenv("DD2_SOUND")) *(int*)(uintptr_t)0x462d68 = 0;   /* keep "initialized" state when the DS shim is live */
    /* Run the REAL attract demo (dd2h's path): DemoMode sets demo_mode=1, num_cars=0x14,
     * race_car=2, _current_level=rand()%10+1, Order_Cars(), then Play_Game() as a deterministic
     * replay. Calling Play_Game() directly (demo_mode=0, level=1) ran a live race expecting input
     * -> uninitialized demo state -> drift -> f1179 clut=0x6c6c crash AND no dd2h alignment. */
    if (getenv("DD2_INPUTTEST")) {
        /* Stage 3: verify the live-input path (browser/native key -> Translate_Keypress -> pad state).
         * Setup_Pad(1) loads the active keymap; then dd2_input_selftest injects synthetic key events. */
        CK("Setup_Pad(1)"); Setup_Pad(1);
        int rc = dd2_input_selftest();
        fprintf(stderr, "[native] input selftest: %s (%d failures)\n", rc==0?"PASS":"FAIL", rc);
        return rc;
    }
    if (getenv("DD2_PLAY")) {
        CK("PlayModeLevel() [LIVE demo_mode=0]");
        PlayModeLevel(atoi(getenv("DD2_PLAY")));
        fprintf(stderr, "[native] live race returned (no crash!)\n");
        return 0;
    }
    if (getenv("DD2_FE")) {
        extern void Init_Front_End(void); extern void Front_End(void);
        W32(0x4673f4, 0);   /* race_type = 0 (valid string-table index; else FUN_00450640 OOB) */
        W32(0x4673f8, 0);   /* race_mode = 0 */
        /* dd2_apply_frontend_state is the FE-EXIT snapshot (race direct-start); it zeroes the
         * copyright-once flag 0x467564 (already consumed in the captured reference). The FE
         * CYCLE mode replays the FE from its start, where the original still has the image
         * default 1: FUN_0044b7f4 shows LEV0\COPYRIGH.BMP on the FIRST loading screen and
         * consumes the flag. With 0 our first loading screen showed LOADING.BMP instead ->
         * one screen/frame off + different leftover fb border pixels -> the slab-zoom into
         * demo #2 magnified them (the cf4..cf150 divergence vs the reference FE cycle). */
        W32(0x467564, 1);
        CK("Init_Front_End()"); Init_Front_End();
        /* Seed keyboard pad type BEFORE Setup_Pad records _recorded_pad_type -- MUST match the WASM
     * runtime (patch 315) and the reference (dd2h front-end sets 0x754451=1 for keyboard). Without
     * this, _recorded_pad_type=0 here vs 1 on wasm -> the demo replay decodes differently ->
     * native/wasm state divergence from the first pad-dependent frame. */
    *(unsigned char*)0x754451 = 1;
    Setup_Pad(1);
        { const char* h = getenv("DD2_HOLD");
          if (h) { extern void dd2_key_event(unsigned int, int);
                   dd2_key_event((unsigned int)strtol(h,0,0), 1);
                   fprintf(stderr, "[native] menu: injected key 0x%lx\n", strtol(h,0,0)); } }
        CK("Front_End()");      Front_End();
    } else if (getenv("DD2_LEVEL")) {
        /* The original boots through Init_Front_End (LEV0: LEVEL.TX0/TX1.., LEVEL.SPR, FONT.BNK)
         * before any DemoMode: those front-end textures fill texturespace pages the race levels
         * never overwrite (pages 20-25 + parts of 6-19 @0x5d0000+). Skipping it left them zero ->
         * black sky-panorama wedges / missing backdrop texels vs the reference. Run the REAL FE
         * init, then re-apply the ref-captured FE exit state on top (it IS the post-FE state). */
        W32(0x4673f4, 0);   /* race_type = 0 (valid string-table index; else FUN_00450640 OOB) */
        W32(0x4673f8, 0);   /* race_mode = 0 */
        CK("Init_Front_End()"); Init_Front_End();
        *(int*)(uintptr_t)0x462fe8 = 0;  /* pal_flag: FE init must not undo the ddmain default */
        *(int*)(uintptr_t)0x467420 = 0;  /* restart_cd_audio: the FE menu loop's first pass clears it
                                            (Front_End @0x4502a8 after Start_CD_Audio); we skip the loop */
        { extern void dd2_apply_frontend_state(void); dd2_apply_frontend_state(); }
        CK("DemoModeLevel()");
        DemoModeLevel(atoi(getenv("DD2_LEVEL")));
    } else {
        CK("DemoMode()");
        DemoMode();
    }
    fprintf(stderr, "[native] demo returned (no crash!)\n");
    return 0;
}
