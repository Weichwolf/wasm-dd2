/* Native platform boundary. SDL presents the existing indexed framebuffer,
 * accepts the final ordered Float32 mixer output and forwards physical input
 * through the same Win32 key/joystick bridge as the browser. Engine code and
 * the deterministic headless harness remain shared. */
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "dd2_native.h"

extern void dd2_key_event(unsigned,int);
extern unsigned char dd2_keystate[256];
extern void dd2_pad_update(int,unsigned,unsigned,unsigned);
extern int FUN_004132f0(void*,unsigned,unsigned,unsigned);

static SDL_Window* native_window;
static SDL_Renderer* native_renderer;
static SDL_Texture* native_texture;
static SDL_AudioDeviceID native_device;
static SDL_AudioDeviceID movie_device;
static unsigned native_rate;
static SDL_Joystick* native_joystick;
static SDL_GameController* native_controller;
static int native_polling;
static uint32_t native_pixels[640*480];

static void native_fail(const char* operation){
    fprintf(stderr,"Native SDL %s: %s\n",operation,SDL_GetError());exit(1);
}
int dd2_native_enabled(void){return native_window!=NULL;}
static void native_shutdown(void){
    dd2_native_movie_audio_stop();
    dd2_native_audio_stop();
    if(native_controller)SDL_GameControllerClose(native_controller);
    else if(native_joystick)SDL_JoystickClose(native_joystick);
    SDL_DestroyTexture(native_texture);SDL_DestroyRenderer(native_renderer);
    SDL_DestroyWindow(native_window);SDL_Quit();
}
static unsigned native_vk(SDL_Scancode key){
    if(key>=SDL_SCANCODE_A && key<=SDL_SCANCODE_Z)return 0x41u+key-SDL_SCANCODE_A;
    if(key>=SDL_SCANCODE_1 && key<=SDL_SCANCODE_9)return 0x31u+key-SDL_SCANCODE_1;
    if(key>=SDL_SCANCODE_F1 && key<=SDL_SCANCODE_F12)return 0x70u+key-SDL_SCANCODE_F1;
    switch(key){
    case SDL_SCANCODE_0:return 0x30;
    case SDL_SCANCODE_RETURN:case SDL_SCANCODE_KP_ENTER:return 0x0d;
    case SDL_SCANCODE_ESCAPE:return 0x1b;
    case SDL_SCANCODE_SPACE:return 0x20;
    case SDL_SCANCODE_LEFT:return 0x25;case SDL_SCANCODE_UP:return 0x26;
    case SDL_SCANCODE_RIGHT:return 0x27;case SDL_SCANCODE_DOWN:return 0x28;
    case SDL_SCANCODE_BACKSPACE:return 8;case SDL_SCANCODE_TAB:return 9;
    case SDL_SCANCODE_DELETE:return 0x2e;case SDL_SCANCODE_INSERT:return 0x2d;
    case SDL_SCANCODE_HOME:return 0x24;case SDL_SCANCODE_END:return 0x23;
    case SDL_SCANCODE_PAGEUP:return 0x21;case SDL_SCANCODE_PAGEDOWN:return 0x22;
    case SDL_SCANCODE_LSHIFT:return 0xa0;case SDL_SCANCODE_RSHIFT:return 0xa1;
    case SDL_SCANCODE_LCTRL:return 0xa2;case SDL_SCANCODE_RCTRL:return 0xa3;
    case SDL_SCANCODE_LALT:return 0xa4;case SDL_SCANCODE_RALT:return 0xa5;
    default:return 0;
    }
}
static void native_key(unsigned vk,int down){
    unsigned generic=0;
    dd2_key_event(vk,down);
    if(vk==0xa0 || vk==0xa1)generic=0x10;
    if(vk==0xa2 || vk==0xa3)generic=0x11;
    if(vk==0xa4 || vk==0xa5)generic=0x12;
    if(generic)dd2_key_event(generic,dd2_keystate[vk&~1u] || dd2_keystate[vk|1u]);
}
static void native_pad(void){
    unsigned buttons=0;int index;
    if(!native_joystick || !SDL_JoystickGetAttached(native_joystick)){
        dd2_pad_update(0,32768,32768,0);return;
    }
    if(native_controller){
        /* Match the browser's standard Gamepad button order, including the
         * trigger axes' pressed state and D-pad buttons. */
        static const SDL_GameControllerButton order[]={
            SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,
            SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y,
            SDL_CONTROLLER_BUTTON_LEFTSHOULDER,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
            SDL_CONTROLLER_BUTTON_INVALID,SDL_CONTROLLER_BUTTON_INVALID,
            SDL_CONTROLLER_BUTTON_BACK,SDL_CONTROLLER_BUTTON_START,
            SDL_CONTROLLER_BUTTON_LEFTSTICK,SDL_CONTROLLER_BUTTON_RIGHTSTICK,
            SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN,
            SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT};
        for(index=0;index<16;index++)if(order[index]!=SDL_CONTROLLER_BUTTON_INVALID &&
            SDL_GameControllerGetButton(native_controller,order[index]))buttons|=1u<<index;
        if(SDL_GameControllerGetAxis(native_controller,SDL_CONTROLLER_AXIS_TRIGGERLEFT)>16383)buttons|=1u<<6;
        if(SDL_GameControllerGetAxis(native_controller,SDL_CONTROLLER_AXIS_TRIGGERRIGHT)>16383)buttons|=1u<<7;
        dd2_pad_update(1,(unsigned)((int)SDL_GameControllerGetAxis(native_controller,SDL_CONTROLLER_AXIS_LEFTX)+32768),
            (unsigned)((int)SDL_GameControllerGetAxis(native_controller,SDL_CONTROLLER_AXIS_LEFTY)+32768),buttons);
    }else{
        for(index=0;index<SDL_JoystickNumButtons(native_joystick) && index<16;index++)
            if(SDL_JoystickGetButton(native_joystick,index))buttons|=1u<<index;
        dd2_pad_update(1,(unsigned)((int)SDL_JoystickGetAxis(native_joystick,0)+32768),
            (unsigned)((int)SDL_JoystickGetAxis(native_joystick,1)+32768),buttons);
    }
}
void dd2_native_poll(void){
    SDL_Event event;
    if(!native_window || native_polling)return;
    native_polling=1;
    while(SDL_PollEvent(&event)){
        /* The original consumes WM_CLOSE in its window procedure. Route the
         * request there too; normal exit remains the game's Quit action. */
        if(event.type==SDL_QUIT)FUN_004132f0((void*)1,0x10,0,0);
        if(event.type==SDL_KEYDOWN || event.type==SDL_KEYUP){
            unsigned vk=native_vk(event.key.keysym.scancode);
            if(vk)native_key(vk,event.type==SDL_KEYDOWN);
        }
        if(event.type==SDL_WINDOWEVENT && event.window.event==SDL_WINDOWEVENT_FOCUS_LOST){
            unsigned vk;
            for(vk=0;vk<256;vk++)if(dd2_keystate[vk])dd2_key_event(vk,0);
        }
    }
    native_pad();native_polling=0;
}
void dd2_native_init(void){
    int index;
    if(!getenv("DD2_WINDOW"))return;
    setenv("DD2_SOUND","1",1);setenv("DD2_REALTIME","1",1);
    if(!getenv("DD2_LEVEL") && !getenv("DD2_PLAY") && !getenv("DD2_CHAMP"))setenv("DD2_FE","1",1);
    /* Preserve Unix termination signals. SDL otherwise turns SIGTERM/SIGINT
     * into the WM_CLOSE request that the original window procedure ignores. */
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS,"1");
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMECONTROLLER))native_fail("initialize");
    atexit(native_shutdown);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
    native_window=SDL_CreateWindow("Destruction Derby 2",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,640,480,SDL_WINDOW_RESIZABLE);
    if(!native_window)native_fail("create window");
    native_renderer=SDL_CreateRenderer(native_window,-1,SDL_RENDERER_SOFTWARE);
    if(!native_renderer)native_fail("create renderer");
    if(SDL_RenderSetLogicalSize(native_renderer,640,480))native_fail("set display size");
    native_texture=SDL_CreateTexture(native_renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,640,480);
    if(!native_texture)native_fail("create indexed display texture");
    for(index=0;index<SDL_NumJoysticks();index++){
        if(SDL_IsGameController(index)){
            native_controller=SDL_GameControllerOpen(index);
            if(native_controller)native_joystick=SDL_GameControllerGetJoystick(native_controller);
        }else{
            native_joystick=SDL_JoystickOpen(index);
            if(native_joystick && SDL_JoystickNumAxes(native_joystick)<2){
                SDL_JoystickClose(native_joystick);native_joystick=NULL;
            }
        }
        if(native_joystick)break;
    }
    /* Detect before Init_Controller_. The engine retains its own boot-time
     * device choice; this backend does not silently switch to hot-plugged pads. */
    SDL_JoystickUpdate();native_pad();
}
void dd2_native_present(const unsigned char* fb,const unsigned char* pal){
    unsigned i;
    if(!native_window)return;
    dd2_native_poll();
    for(i=0;i<640*480;i++){
        const unsigned char* entry=pal+fb[i]*4;
        native_pixels[i]=0xff000000u|((uint32_t)entry[0]<<16)|((uint32_t)entry[1]<<8)|entry[2];
    }
    if(SDL_UpdateTexture(native_texture,NULL,native_pixels,640*4) ||
        SDL_SetRenderDrawColor(native_renderer,0,0,0,255) || SDL_RenderClear(native_renderer) ||
        SDL_RenderCopy(native_renderer,native_texture,NULL,NULL))native_fail("present framebuffer");
    SDL_RenderPresent(native_renderer);
}
void dd2_native_audio(const float* pcm,unsigned frames,unsigned rate){
    if(!native_window)return;
    if(!native_device){
        SDL_AudioSpec requested={0},actual;
        requested.freq=(int)rate;requested.format=AUDIO_F32LSB;requested.channels=2;requested.samples=1024;
        native_device=SDL_OpenAudioDevice(NULL,0,&requested,&actual,0);
        if(!native_device)native_fail("open Float32 audio device");
        if(actual.freq!=(int)rate || actual.format!=AUDIO_F32LSB || actual.channels!=2){
            fprintf(stderr,"Native SDL audio device must accept Float32 stereo at %uHz\n",rate);exit(1);
        }
        native_rate=rate;SDL_PauseAudioDevice(native_device,0);
    }
    if(rate!=native_rate){fprintf(stderr,"Native SDL device rate changed during playback\n");exit(1);}
    if(SDL_QueueAudio(native_device,pcm,frames*2*sizeof(float)))native_fail("queue combined audio");
}
void dd2_native_audio_stop(void){
    if(native_device){SDL_ClearQueuedAudio(native_device);SDL_CloseAudioDevice(native_device);native_device=0;}
    native_rate=0;
}
void dd2_native_movie_present(const uint32_t* argb){
    if(!native_window)return;
    if(SDL_UpdateTexture(native_texture,NULL,argb,640*4) ||
        SDL_SetRenderDrawColor(native_renderer,0,0,0,255) || SDL_RenderClear(native_renderer) ||
        SDL_RenderCopy(native_renderer,native_texture,NULL,NULL))native_fail("present movie");
    SDL_RenderPresent(native_renderer);
}
void dd2_native_movie_audio_stop(void){
    if(movie_device){SDL_ClearQueuedAudio(movie_device);SDL_CloseAudioDevice(movie_device);movie_device=0;}
}
int dd2_native_movie_audio_start(const int16_t* pcm,size_t frames,unsigned rate,unsigned channels){
    SDL_AudioSpec requested={0},actual;
    if(!native_window)return 0;
    if(!pcm || !frames || !rate || channels!=2 || frames>UINT32_MAX/(channels*2))return -1;
    dd2_native_movie_audio_stop();
    requested.freq=(int)rate;requested.format=AUDIO_S16LSB;requested.channels=channels;requested.samples=1024;
    movie_device=SDL_OpenAudioDevice(NULL,0,&requested,&actual,0);
    if(!movie_device)return -1;
    if(actual.freq!=(int)rate || actual.format!=AUDIO_S16LSB || actual.channels!=channels ||
            SDL_QueueAudio(movie_device,pcm,(unsigned)(frames*channels*2))){
        dd2_native_movie_audio_stop();return -1;
    }
    SDL_PauseAudioDevice(movie_device,0);return 0;
}
int dd2_native_movie_audio_done(void){return !movie_device || SDL_GetQueuedAudioSize(movie_device)==0;}
