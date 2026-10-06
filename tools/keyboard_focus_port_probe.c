/* Real X11 input/focus oracle companion. The parent sends XTEST input to
 * foreground windows; this fixture only pumps the production SDL bridge and
 * reports API states/message counts. No SDL_PushEvent or state injection. */
#include <SDL2/SDL.h>
#include <sys/mman.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dd2_native.h"
extern unsigned char dd2_keystate[256];
static unsigned down_count,up_count;
void dd2_window_message(void){}
void dd2_window_focus(int active){(void)active;}
static void require(int ok,const char* why){if(!ok){fprintf(stderr,"keyboard focus bridge: %s\n",why);exit(1);}}
void dd2_pad_update(int connected,unsigned x,unsigned y,unsigned buttons){(void)connected;(void)x;(void)y;(void)buttons;}
int FUN_004132f0(void* window,unsigned message,unsigned key,unsigned flags){
    (void)window;(void)key;(void)flags;
    if(message==0x100 || message==0x104)down_count++;
    if(message==0x101 || message==0x105)up_count++;
    return 0;
}
int main(void){
    struct pollfd input={0,POLLIN,0};char label[128];
    require(mmap((void*)0x400000,0x65000,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map engine window slot");
    dd2_native_init();require(dd2_native_enabled(),"actual SDL window");
    require(!strcmp(SDL_GetCurrentVideoDriver(),"x11"),"actual X11 backend");
    puts("{\"ready\":true}");fflush(stdout);
    for(;;){
        dd2_native_poll();
        if(poll(&input,1,1)>0){
            unsigned end;
            if(!fgets(label,sizeof(label),stdin))break;
            label[strcspn(label,"\n")]=0;if(!strcmp(label,"quit"))break;
            end=SDL_GetTicks()+40;
            do{dd2_native_poll();SDL_Delay(1);}while((Sint32)(end-SDL_GetTicks())>0);
            printf("{\"label\":\"%s\",\"active\":%d,\"left\":%u,\"a\":%u,\"shift\":[%u,%u,%u],\"control\":[%u,%u,%u],\"down\":%u,\"up\":%u}\n",label,
                SDL_GetKeyboardFocus()!=NULL,dd2_keystate[0x25],dd2_keystate[0x41],dd2_keystate[0x10],dd2_keystate[0xa0],dd2_keystate[0xa1],
                dd2_keystate[0x11],dd2_keystate[0xa2],dd2_keystate[0xa3],down_count,up_count);fflush(stdout);
        }
    }
    return 0;
}
