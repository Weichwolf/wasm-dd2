#include <stdio.h>
#include <stdint.h>
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
extern void Set_Draw_Mode(int);
extern int _current_level;
int main(){
    dd2_load_image("dd2_image.bin");
    dd2_relocate();
    *(int*)0x46c32c = (int)(long)&dd2_getthread;  /* __GetThreadPtr */
    /* CRT multithread file/heap-access locks (Access/Release @0x46c330-0x46c364) -> single-threaded no-ops */
    { unsigned va; for(va=0x46c330; va<=0x46c364; va+=4) *(int*)(uintptr_t)va = (int)(long)&dd2_crt_lock; }
    __InitRtns();
    dd2_com_init();
    Read_Directory("Dirinfo");
    *(int*)0x462d68 = 1;  /* during init: make FUN_004159a8 skip DirectSound COM setup */
    Init_Application((void*)1);
    /* front-end normally sets the video mode (creates the DDraw primary surface) before the race;
       the cold Play_Game bypass skips it -> Set_Draw_Mode(0) here so SetPalette's surface exists.
       Force DAT_00463010 (current mode) != 0 so Set_Draw_Mode(0) actually runs the mode-set. */
    _current_level = 1;
    *(int*)0x463010 = -1;
    Set_Draw_Mode(0);
    *(int*)0x462d68 = 0;  /* before the race loop: sound funcs (Modify_Sound etc.) skip uninit DirectSound COM (audio = deferred WebAudio tier) */
    Play_Game();                  /* the race: Init_Game + physics/AI/GTE/render loop */
    return 0;
}
