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
   Recorded by a transpile debug-patch in FUN_0041fb7c's walk loop; dumped here on crash. */
int g_plog[2048]; volatile int g_pidx = 0;
static void segv(int sig, siginfo_t* si, void* uc){
    (void)uc;
    const char* nm = sig==SIGFPE?"SIGFPE":sig==SIGSEGV?"SIGSEGV":sig==SIGBUS?"SIGBUS":"SIG?";
    fprintf(stderr, "\n*** %s at addr %p (code %d) ***\n", nm, si->si_addr, si->si_code);
    if (getenv("DD2_PLOG")){
        int i, k=g_pidx; fprintf(stderr,"--- last poly commands (type,count,gpoly) ---\n");
        for (i = k>16?k-16:0; i < k; i++)
            fprintf(stderr,"  [%d] type=%d count=%d gpoly=0x%x\n", i, g_plog[(i&511)*4], g_plog[(i&511)*4+1], (unsigned)g_plog[(i&511)*4+2]);
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
extern int  _current_level;
extern void Setup_Pad(int);
extern void Order_Cars(void);
extern void Init_Front_End(void);
extern int  rand(void);

/* Fixed-level demo entry: faithful copy of DemoMode@0x44b4e0 but with _current_level forced
 * from DD2_LEVEL (for matched ref-vs-WASM byte comparison — DemoMode's rand()%10+1 picks a level
 * that differs from dd2h's front-end-driven demo, so we pin both to the same level). Not game code. */
#define W32(va,val) (*(int*)(uintptr_t)(va) = (int)(val))
#define R32(va)     (*(int*)(uintptr_t)(va))
static int DemoModeLevel(int lvl){
    Setup_Pad(1);
    W32(0x905a1c, R32(0x4673f4));            /* save race_type */
    W32(0x905a18, R32(0x4673f8));            /* save race_mode */
    W32(0x46385c, 1);                        /* demo_mode = 1 */
    W32(0x905a14, R32(0x467400));            /* save race_car */
    W32(0x467400, 2);                        /* race_car = 2 */
    W32(0x905a10, R32(0x46765c));            /* save num_cars */
    W32(0x46765c, 0x14);                     /* num_cars = 0x14 */
    W32(0x4673f8, 0);                        /* race_mode = 0 */
    W32(0x4673f4, 0);                        /* race_type = 0 */
    { int iVar1 = rand(); _current_level = lvl ? lvl : (iVar1 % 10 + 1); }
    fprintf(stderr, "[native] DemoModeLevel: _current_level=%d\n", _current_level);
    Order_Cars();
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
    CK("Read_Directory(Dirinfo)");  Read_Directory("Dirinfo");
    *(int*)(uintptr_t)0x462d68 = 1;          /* skip DirectSound COM during init */
    CK("Init_Application()");       Init_Application((void*)1);
    *(int*)(uintptr_t)0x463010 = -1;
    CK("Set_Draw_Mode(0)");         Set_Draw_Mode(0);
    *(int*)(uintptr_t)0x462d68 = 0;
    /* Run the REAL attract demo (dd2h's path): DemoMode sets demo_mode=1, num_cars=0x14,
     * race_car=2, _current_level=rand()%10+1, Order_Cars(), then Play_Game() as a deterministic
     * replay. Calling Play_Game() directly (demo_mode=0, level=1) ran a live race expecting input
     * -> uninitialized demo state -> drift -> f1179 clut=0x6c6c crash AND no dd2h alignment. */
    if (getenv("DD2_FE")) {
        extern void Init_Front_End(void); extern void Front_End(void);
        W32(0x4673f4, 0);   /* race_type = 0 (valid string-table index; else FUN_00450640 OOB) */
        W32(0x4673f8, 0);   /* race_mode = 0 */
        CK("Init_Front_End()"); Init_Front_End();
        CK("Front_End()");      Front_End();
    } else if (getenv("DD2_LEVEL")) {
        CK("DemoModeLevel()");
        DemoModeLevel(atoi(getenv("DD2_LEVEL")));
    } else {
        CK("DemoMode()");
        DemoMode();
    }
    fprintf(stderr, "[native] demo returned (no crash!)\n");
    return 0;
}
