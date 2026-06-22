#include <stdio.h>
#include <stdlib.h>
#include <string.h>
unsigned char* g_image = (unsigned char*)0x400000;
extern void dd2_relocate(void);
extern void dd2_com_init(void);
extern void __InitMultipleThread(void);
extern void Read_Directory(const char*);
extern void __WinMain(void);
void dd2_load_image(const char* path){
    FILE* f=fopen(path,"rb"); if(!f){ return; }
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    unsigned char* tmp=malloc(n); fread(tmp,1,n,f); fclose(f);
    memcpy((void*)0x400000, tmp, n);   /* image at fixed VA so raw-VA pointers resolve */
    free(tmp);
}
/* MSVC CRT thread-pointer: ~23 call sites do (*(code*)__GetThreadPtr)() for the per-thread data block.
   Original sets it to &__MultipleThread during init; provide a valid block up-front (single-threaded WASM). */
static int g_thread[256];
int dd2_getthread(void){ return (int)(long)g_thread; }
int dd2_crt_lock(int a){ return a; }   /* CRT lock no-op (single-threaded) */
/* CRT BYPASS: the user-WinMain that inits the game + runs the menu is buried in the CRT's alloca region
   (x86 stack manip can't run in WASM). Call the game's __InitRtns (C++ global ctors) + Init_Application
   (window) + Play_Game (race state machine, which does Init_Game) directly to reach the GAME LOGIC. */
extern void __InitRtns(void);
extern int Init_Application(void* hInst);
extern int Play_Game(void);
int main(){
    dd2_load_image("dd2_image.bin");
    dd2_relocate();
    *(int*)0x46c32c = (int)(long)&dd2_getthread;  /* __GetThreadPtr */
    /* CRT multithread file/heap-access locks (__Access* undecompiled) -> single-threaded no-ops */
    *(int*)0x46c330 = (int)(long)&dd2_crt_lock;   /* _AccessFileH */
    *(int*)0x46c340 = (int)(long)&dd2_crt_lock;   /* _AccessIOB   */
    __InitRtns();                 /* run global constructors (game data tables) */
    dd2_com_init();               /* set up DirectDraw COM interface vtables */
    { FILE* tf=fopen("Dirinfo","rb"); char b[40]={0};
      if(tf){ fread(b,1,32,tf); fclose(tf); fprintf(stderr,"[dd2] Dirinfo OK; idx[0] name='%.20s' bytes %d,%d,%d\n",b,(int)(unsigned char)b[0],(int)(unsigned char)b[18],(int)(unsigned char)b[20]); }
      else fprintf(stderr,"[dd2] Dirinfo NOT OPENABLE (cwd issue)\n"); }
    Read_Directory("Dirinfo");    /* load the Dirinfo asset index into dirbuf (skipped by the CRT bypass) */
    *(int*)0x462d68 = 1;          /* skip DirectSound COM init (needs WebAudio shim) - characterize next tier */
    Init_Application((void*)1);   /* register class + create window (shimmed) */
    Play_Game();                  /* the race: Init_Game + physics/AI/GTE/render loop */
    return 0;
}
