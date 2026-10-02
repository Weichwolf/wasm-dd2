/* Exercise real SDL video/audio/input with an independent known fixture.
 * Stdout/stdin synchronization lets the parent inspect the actual X11 window
 * while the fixture waits, without racing the next presentation. */
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "dd2_native.h"

unsigned char dd2_keystate[256];
static int present,closes;
static unsigned padx,pady,padb;
void dd2_key_event(unsigned vk,int down){dd2_keystate[vk]=down!=0;}
void dd2_pad_update(int connected,unsigned x,unsigned y,unsigned buttons){present=connected;padx=x;pady=y;padb=buttons;}
int FUN_004132f0(void* window,unsigned message,unsigned a,unsigned b){(void)window;(void)a;(void)b;if(message==0x10)closes++;return 0;}
static void require(int ok,const char* reason){if(!ok){fprintf(stderr,"SDL backend fixture: %s (%s)\n",reason,SDL_GetError());exit(1);}}
static void event(SDL_Scancode code,int down){
    SDL_Event e={0};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.keysym.scancode=code;
    require(SDL_PushEvent(&e)==1,"push SDL key");dd2_native_poll();
}
static void request(unsigned index){
    char path[4096];FILE* file;
    snprintf(path,sizeof(path),"%s/request",getenv("DD2_NATIVE_OBSERVE"));
    file=fopen(path,"w");require(file!=NULL,"request actual rendering");fprintf(file,"%u\n",index);fclose(file);
}
int main(void){
    unsigned char *fb=(void*)0x700450,*pal=(void*)0x700050;
    unsigned i,step;SDL_Joystick* virtual_pad;int index;float pcm[997*2];
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map known source");
    require(SDL_Init(SDL_INIT_JOYSTICK)==0,"initialize virtual controller");
    index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,16,0);
    require(index>=0,"attach boot-time virtual controller");
    virtual_pad=SDL_JoystickOpen(index);require(virtual_pad!=NULL,"open virtual controller");
    require(SDL_JoystickSetVirtualAxis(virtual_pad,0,-16384)==0 &&
        SDL_JoystickSetVirtualAxis(virtual_pad,1,20000)==0 &&
        SDL_JoystickSetVirtualButton(virtual_pad,0,1)==0 &&
        SDL_JoystickSetVirtualButton(virtual_pad,1,1)==0,"set virtual controller input");
    dd2_native_init();require(dd2_native_enabled(),"native window enabled");dd2_native_poll();
    require(present && padx==16384 && pady==52768 && padb==3,"SDL controller reaches Win32 joystick coordinates/buttons");
    event(SDL_SCANCODE_D,1);require(dd2_keystate[0x44],"generic rebindable letter");
    event(SDL_SCANCODE_D,0);require(!dd2_keystate[0x44],"key release");
    event(SDL_SCANCODE_7,1);require(dd2_keystate[0x37],"digit");event(SDL_SCANCODE_7,0);
    event(SDL_SCANCODE_F2,1);require(dd2_keystate[0x71],"function key");event(SDL_SCANCODE_F2,0);
    event(SDL_SCANCODE_LSHIFT,1);event(SDL_SCANCODE_RSHIFT,1);event(SDL_SCANCODE_LSHIFT,0);
    require(dd2_keystate[0x10] && dd2_keystate[0xa1],"remaining physical shift stays down");
    event(SDL_SCANCODE_RSHIFT,0);require(!dd2_keystate[0x10],"last shift releases generic shift");
    event(SDL_SCANCODE_A,1);
    {SDL_Event e={0};e.type=SDL_WINDOWEVENT;e.window.event=SDL_WINDOWEVENT_FOCUS_LOST;SDL_PushEvent(&e);dd2_native_poll();}
    require(!dd2_keystate[0x41],"focus loss releases held keys");
    {SDL_Event e={0};e.type=SDL_QUIT;SDL_PushEvent(&e);dd2_native_poll();}
    require(closes==1,"WM_CLOSE reaches the original window procedure");
    for(step=0;step<2;step++){
        for(i=0;i<640*480;i++)fb[i]=(unsigned char)(i*17+i/640);
        for(i=0;i<256;i++){
            pal[i*4]=(unsigned char)(i*37+(step?91:0));pal[i*4+1]=(unsigned char)(i*73+(step?137:0));
            pal[i*4+2]=(unsigned char)(i*13+(step?203:0));pal[i*4+3]=(unsigned char)(i^123);
        }
        request(step);dd2_native_present(fb,pal);
        printf("{\"ready\":%u}\n",step);fflush(stdout);require(getchar()=='\n',"external screenshot acknowledgment");
    }
    for(i=0;i<997*2;i++)pcm[i]=((int)i-997)/512.0f;
    dd2_native_audio(pcm,221,44100);dd2_native_audio(pcm+221*2,776,44100);
    SDL_JoystickClose(virtual_pad);
    puts("{\"pass\":true,\"keyboard\":true,\"gamepad\":true,\"frames\":2,\"audio_frames\":997}");
    return 0;
}
