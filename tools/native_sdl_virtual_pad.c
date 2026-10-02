/* Test-only external SDL controller driver. Creates a device before engine
 * detection and supplies real SDL virtual-axis/button events from a file.
 * This does not write engine memory or change the read-only output observer. */
#define _GNU_SOURCE
#include <SDL2/SDL.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

static SDL_Joystick* pad;
static void fail(void){fprintf(stderr,"SDL virtual controller: %s\n",SDL_GetError());exit(1);}
int SDL_Init(Uint32 flags){
    int (*original)(Uint32)=dlsym(RTLD_NEXT,"SDL_Init");
    int result=original(flags);
    if(!result && !pad && (flags&SDL_INIT_GAMECONTROLLER) && getenv("DD2_NATIVE_PAD_INPUT")){
        int index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,16,0);
        if(index<0 || !(pad=SDL_JoystickOpen(index)))fail();
    }
    return result;
}
int SDL_PollEvent(SDL_Event* event){
    int (*original)(SDL_Event*)=dlsym(RTLD_NEXT,"SDL_PollEvent");
    if(pad){
        const char* path=getenv("DD2_NATIVE_PAD_INPUT");FILE* input=path?fopen(path,"r"):NULL;
        int x,y,index;unsigned buttons;
        if(input){
            if(fscanf(input,"%d %d %x",&x,&y,&buttons)!=3 || x<-32768 || x>32767 || y<-32768 || y>32767 || buttons>65535){
                fclose(input);fprintf(stderr,"Invalid SDL virtual controller input\n");exit(1);
            }
            fclose(input);
            if(SDL_JoystickSetVirtualAxis(pad,0,(Sint16)x) || SDL_JoystickSetVirtualAxis(pad,1,(Sint16)y))fail();
            for(index=0;index<16;index++)if(SDL_JoystickSetVirtualButton(pad,index,(buttons>>index)&1))fail();
            SDL_JoystickUpdate();
        }
    }
    return original(event);
}
