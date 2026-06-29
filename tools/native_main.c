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

static void segv(int sig, siginfo_t* si, void* uc){
    (void)sig;(void)uc;
    fprintf(stderr, "\n*** SIGSEGV at fault addr %p ***\n", si->si_addr);
    void* bt[24]; int n = backtrace(bt, 24);
    backtrace_symbols_fd(bt, n, 2);
    _exit(139);
}
static void install_segv(void){
    struct sigaction sa; memset(&sa,0,sizeof sa);
    sa.sa_sigaction = segv; sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, 0); sigaction(SIGBUS, &sa, 0);
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
extern void Set_Draw_Mode(int);
extern int  _current_level;

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
    install_segv();
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
    _current_level = 1;
    *(int*)(uintptr_t)0x463010 = -1;
    CK("Set_Draw_Mode(0)");         Set_Draw_Mode(0);
    *(int*)(uintptr_t)0x462d68 = 0;
    CK("Play_Game()");
    Play_Game();                             /* Init_Game -> the Track_Follow OOB */
    fprintf(stderr, "[native] Play_Game returned (no crash!)\n");
    return 0;
}
