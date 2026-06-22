#include <stdio.h>
#include <stdlib.h>
#include <string.h>
unsigned char* g_image = (unsigned char*)0x400000;
extern void dd2_relocate(void);
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
/* CRT BYPASS: the user-WinMain that inits the game + runs the menu is buried in the CRT's alloca region
   (x86 stack manip can't run in WASM). Call the game's __InitRtns (C++ global ctors) + Init_Application
   (window) + Play_Game (race state machine, which does Init_Game) directly to reach the GAME LOGIC. */
extern void __InitRtns(void);
extern int Init_Application(void* hInst);
extern int Play_Game(void);
int main(){
    dd2_load_image("dd2_image.bin");
    dd2_relocate();
    *(int*)0x46c32c = (int)(long)&dd2_getthread;
    __InitRtns();                 /* run global constructors (game data tables) */
    *(int*)0x462d68 = 1;          /* skip DirectSound COM init (needs WebAudio shim) - characterize next tier */
    Init_Application((void*)1);   /* register class + create window (shimmed) */
    Play_Game();                  /* the race: Init_Game + physics/AI/GTE/render loop */
    return 0;
}
