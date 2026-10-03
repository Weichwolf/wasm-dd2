/* Record the production input bridge boundary, before the engine WndProc.
 * Native runs both direct VKs and actual SDL events; WASM runs direct VKs and
 * browser-code mapping. This tests messages/state, not physical audio or a
 * Windows keyboard layout. Genuine USER32 is the separate oracle. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
#ifdef DD2_KEYBOARD_SDL
#include <SDL2/SDL.h>
#include "dd2_native.h"
#endif
#include "keyboard_events.h"
extern unsigned char dd2_keystate[256];
extern void dd2_key_event(unsigned,int);
extern unsigned dd2_browser_key_event(const char*,int);
static unsigned step,seen;
static void require(int ok,const char* why){if(!ok){fprintf(stderr,"keyboard bridge: %s\n",why);exit(1);}}
int FUN_004132f0(void* window,unsigned message,unsigned key,unsigned flags){
    (void)window;
    if(message!=0x100 && message!=0x101 && message!=0x104 && message!=0x105)return 0;
    printf("%s{\"step\":%u,\"message\":%u,\"vk\":%u,\"up\":%u,\"shift\":[%u,%u,%u],\"control\":[%u,%u,%u],\"alt\":[%u,%u,%u]}",
        seen?",":"",step,message,key,flags>>31,
        dd2_keystate[0x10],dd2_keystate[0xa0],dd2_keystate[0xa1],
        dd2_keystate[0x11],dd2_keystate[0xa2],dd2_keystate[0xa3],
        dd2_keystate[0x12],dd2_keystate[0xa4],dd2_keystate[0xa5]);
    seen++;return 0;
}
#ifdef DD2_KEYBOARD_SDL
void dd2_pad_update(int connected,unsigned x,unsigned y,unsigned buttons){(void)connected;(void)x;(void)y;(void)buttons;}
static SDL_Scancode physical_key(unsigned vk){
    switch(vk){
    case 0xa0:return SDL_SCANCODE_LSHIFT;case 0xa1:return SDL_SCANCODE_RSHIFT;
    case 0xa2:return SDL_SCANCODE_LCTRL;case 0xa3:return SDL_SCANCODE_RCTRL;
    case 0xa4:return SDL_SCANCODE_LALT;case 0xa5:return SDL_SCANCODE_RALT;
    case 0x41:return SDL_SCANCODE_A;case 0x79:return SDL_SCANCODE_F10;
    default:{SDL_Scancode code=SDL_GetScancodeFromName(events[step].code);require(code!=SDL_SCANCODE_UNKNOWN,"unknown physical fixture key");return code;}
    }
}
#endif
int main(int argc,char** argv){
    require(argc==2,"select vk/browser/sdl transport");
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x65000,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map engine window slot");
#endif
#ifdef DD2_KEYBOARD_SDL
    if(!strcmp(argv[1],"sdl")){dd2_native_init();require(dd2_native_enabled(),"SDL window");dd2_native_poll();}
#endif
    {
        static const char* invalid[]={NULL,"","F0","F25","F99","F01","F1x","F-1"};
        unsigned i;
        for(i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++)require(dd2_browser_key_event(invalid[i],1)==0 && seen==0,"invalid browser code must not dispatch a key");
    }
    printf("[");
    for(step=0;step<sizeof(events)/sizeof(events[0]);step++){
        unsigned before=seen;
        if(!strcmp(argv[1],"browser"))require(dd2_browser_key_event(events[step].code,events[step].down)==events[step].key,"browser VK mapping");
        else if(!strcmp(argv[1],"vk"))dd2_key_event(events[step].key,events[step].down);
#ifdef DD2_KEYBOARD_SDL
        else if(!strcmp(argv[1],"sdl")){
            SDL_Event e={0};e.type=events[step].down?SDL_KEYDOWN:SDL_KEYUP;e.key.keysym.scancode=physical_key(events[step].key);
            require(SDL_PushEvent(&e)==1,"push SDL physical key");dd2_native_poll();
        }
#endif
        else require(0,"unknown transport");
        require(seen==before+1,"one window message per transition");
    }
    puts("]");return 0;
}
