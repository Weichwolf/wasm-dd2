/* Native platform boundary. SDL presents the existing indexed framebuffer,
 * accepts the final ordered Float32 mixer output and forwards physical input
 * through the same Win32 key/joystick bridge as the browser. Engine code and
 * the deterministic headless harness remain shared. */
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#ifdef SDL_VIDEO_DRIVER_X11
#include <X11/keysym.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "dd2_native.h"

extern void dd2_key_event(unsigned,int);
extern void dd2_key_state(unsigned,int);
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
#ifdef SDL_VIDEO_DRIVER_X11
/* SDL 2.32 sends ResetKeyboard KEYUPs before FOCUS_LOST and reconciles held
 * keys after FOCUS_GAINED. Neither is a physical key message for USER32.
 * X11's SYSWMEVENT precedes the corresponding translated SDL key event, so
 * retain its provenance rather than guessing from timestamps or lookahead. */
static Display* native_xdisplay;
static Window native_xwindow;
static void* native_xlibrary;
static int (*native_query_keymap)(Display*,char[32]);
static KeyCode (*native_keysym_keycode)(Display*,KeySym);
static unsigned native_xkeycode[SDL_NUM_SCANCODES];
static unsigned native_raw_keycode;
static Uint32 native_raw_keytype;
static Uint32 native_missing_keyup_event;
static int native_keyboard_watch(void*,SDL_Event*);
#endif
static uint32_t native_pixels[640*480];

static void native_fail(const char* operation){
    fprintf(stderr,"Native SDL %s: %s\n",operation,SDL_GetError());exit(1);
}
int dd2_native_enabled(void){return native_window!=NULL;}
static void native_shutdown(void){
#ifdef SDL_VIDEO_DRIVER_X11
    if(native_xdisplay)SDL_DelEventWatch(native_keyboard_watch,NULL);
#endif
    dd2_native_movie_audio_stop();
    dd2_native_audio_stop();
    if(native_controller)SDL_GameControllerClose(native_controller);
    else if(native_joystick)SDL_JoystickClose(native_joystick);
    SDL_DestroyTexture(native_texture);SDL_DestroyRenderer(native_renderer);
    SDL_DestroyWindow(native_window);SDL_Quit();
#ifdef SDL_VIDEO_DRIVER_X11
    if(native_xlibrary)SDL_UnloadObject(native_xlibrary);
#endif
}
static unsigned native_vk(SDL_Scancode key){
    if(key>=SDL_SCANCODE_A && key<=SDL_SCANCODE_Z)return 0x41u+key-SDL_SCANCODE_A;
    if(key>=SDL_SCANCODE_1 && key<=SDL_SCANCODE_9)return 0x31u+key-SDL_SCANCODE_1;
    if(key>=SDL_SCANCODE_F1 && key<=SDL_SCANCODE_F12)return 0x70u+key-SDL_SCANCODE_F1;
    if(key>=SDL_SCANCODE_F13 && key<=SDL_SCANCODE_F24)return 0x7cu+key-SDL_SCANCODE_F13;
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
#ifdef SDL_VIDEO_DRIVER_X11
static void native_keyboard_map(void){
    unsigned scancode;
    for(scancode=0;scancode<SDL_NUM_SCANCODES;scancode++){
        unsigned vk=native_vk((SDL_Scancode)scancode);
        SDL_Keycode code=SDL_GetKeyFromScancode((SDL_Scancode)scancode);
        KeySym symbol=0;
        /* Printable symbols use SDL's current layout, rather than assuming
         * the physical letter's US symbol. Special keys have X11 keysyms. */
        if(vk && code>0 && code<SDLK_SCANCODE_MASK)
            symbol=code<=0xff ? (KeySym)code : (KeySym)(0x01000000u|(unsigned)code);
        if(vk>=0x70 && vk<=0x87)symbol=XK_F1+vk-0x70;
        switch(vk){
        case 8:symbol=XK_BackSpace;break;case 9:symbol=XK_Tab;break;
        case 0x0d:symbol=scancode==SDL_SCANCODE_KP_ENTER ? XK_KP_Enter : XK_Return;break;
        case 0x1b:symbol=XK_Escape;break;
        case 0x21:symbol=XK_Page_Up;break;case 0x22:symbol=XK_Page_Down;break;
        case 0x23:symbol=XK_End;break;case 0x24:symbol=XK_Home;break;
        case 0x25:symbol=XK_Left;break;case 0x26:symbol=XK_Up;break;
        case 0x27:symbol=XK_Right;break;case 0x28:symbol=XK_Down;break;
        case 0x2d:symbol=XK_Insert;break;case 0x2e:symbol=XK_Delete;break;
        case 0xa0:symbol=XK_Shift_L;break;case 0xa1:symbol=XK_Shift_R;break;
        case 0xa2:symbol=XK_Control_L;break;case 0xa3:symbol=XK_Control_R;break;
        case 0xa4:symbol=XK_Alt_L;break;case 0xa5:symbol=XK_Alt_R;break;
        }
        native_xkeycode[scancode]=symbol ? native_keysym_keycode(native_xdisplay,symbol) : 0;
    }
}
/* SDL reconciles only modifiers held in another foreground application.
 * A subsequent real release of an ordinary outside key is discarded because
 * SDL still considers it up. Observe that state before X11 translation and
 * queue the missing USER32 release; do not alter SDL's keyboard state. */
static int native_keyboard_watch(void* data,SDL_Event* event){
    const SDL_SysWMmsg* wm;const XEvent* raw;unsigned scancode;
    (void)data;
    if(event->type!=SDL_SYSWMEVENT || !native_xdisplay)return 0;
    wm=event->syswm.msg;
    if(!wm || wm->subsystem!=SDL_SYSWM_X11)return 0;
    raw=&wm->msg.x11.event;
    if(raw->type!=KeyRelease || raw->xkey.window!=native_xwindow)return 0;
    for(scancode=0;scancode<SDL_NUM_SCANCODES;scancode++)
        if(native_xkeycode[scancode]==raw->xkey.keycode){
            unsigned vk=native_vk((SDL_Scancode)scancode);
            if(vk && !SDL_GetKeyboardState(NULL)[scancode]){
                SDL_Event release={0};release.type=native_missing_keyup_event;release.user.code=(Sint32)vk;
                SDL_PushEvent(&release);
            }
            break;
        }
    return 0;
}
#endif
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
#ifdef SDL_VIDEO_DRIVER_X11
    int keyboard_sync=0;
#endif
    if(!native_window || native_polling)return;
    native_polling=1;
    while(SDL_PollEvent(&event)){
#ifdef SDL_VIDEO_DRIVER_X11
        if(native_xdisplay && event.type==native_missing_keyup_event)
            dd2_key_event((unsigned)event.user.code,0);
        if(native_xdisplay && event.type==SDL_SYSWMEVENT){
            const SDL_SysWMmsg* wm=event.syswm.msg;
            native_raw_keytype=0;
            if(wm && wm->subsystem==SDL_SYSWM_X11){
                const XEvent* raw=&wm->msg.x11.event;
                if(raw->xany.window==native_xwindow &&
                        (raw->type==KeyPress || raw->type==KeyRelease)){
                    native_raw_keytype=raw->type==KeyPress ? SDL_KEYDOWN : SDL_KEYUP;
                    native_raw_keycode=raw->xkey.keycode;
                }
            }
        }
        if(native_xdisplay && event.type==SDL_KEYMAPCHANGED)native_keyboard_map();
#endif
        if(event.type==SDL_WINDOWEVENT &&
                (event.window.event==SDL_WINDOWEVENT_FOCUS_LOST ||
                 event.window.event==SDL_WINDOWEVENT_FOCUS_GAINED)){
#ifdef SDL_VIDEO_DRIVER_X11
            keyboard_sync=1;
#endif
        }
        /* The original consumes WM_CLOSE in its window procedure. Route the
         * request there too; normal exit remains the game's Quit action. */
        if(event.type==SDL_QUIT)FUN_004132f0((void*)1,0x10,0,0);
        if(event.type==SDL_KEYDOWN || event.type==SDL_KEYUP){
            unsigned vk=native_vk(event.key.keysym.scancode);
            int physical=1;
#ifdef SDL_VIDEO_DRIVER_X11
            if(native_xdisplay && event.key.windowID==SDL_GetWindowID(native_window)){
                physical=event.type==native_raw_keytype;
                if(physical && event.key.keysym.scancode<SDL_NUM_SCANCODES)
                    native_xkeycode[event.key.keysym.scancode]=native_raw_keycode;
                native_raw_keytype=0;
                /* Reconciliation exposes physical states without a window
                 * key message. ResetKeyboard releases need XQueryKeymap
                 * below: the physical key can still be held outside. */
                if(!physical){
                    keyboard_sync=1;
                    if(vk && (event.type==SDL_KEYDOWN || SDL_GetKeyboardFocus()==native_window))
                        dd2_key_state(vk,event.type==SDL_KEYDOWN);
                }
            }
#endif
            if(vk && physical)dd2_key_event(vk,event.type==SDL_KEYDOWN);
        }
    }
#ifdef SDL_VIDEO_DRIVER_X11
    if(native_xdisplay && (keyboard_sync || SDL_GetKeyboardFocus()!=native_window)){
        char keys[32];unsigned scancode,vk;
        unsigned char known[256]={0},down[256]={0};
        native_query_keymap(native_xdisplay,keys);
        for(scancode=0;scancode<SDL_NUM_SCANCODES;scancode++){
            unsigned keycode=native_xkeycode[scancode];vk=native_vk((SDL_Scancode)scancode);
            if(vk && keycode && keycode<256){
                known[vk]=1;down[vk]|=(keys[keycode/8] & (1u<<(keycode%8)))!=0;
            }
        }
        for(vk=0;vk<256;vk++)if(known[vk])dd2_key_state(vk,down[vk]);
    }
#endif
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
    SDL_EventState(SDL_SYSWMEVENT,SDL_ENABLE);
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
#ifdef SDL_VIDEO_DRIVER_X11
    {
        SDL_SysWMinfo wm;SDL_VERSION(&wm.version);
        if(SDL_GetWindowWMInfo(native_window,&wm) && wm.subsystem==SDL_SYSWM_X11){
            native_xdisplay=wm.info.x11.display;native_xwindow=wm.info.x11.window;
            native_xlibrary=SDL_LoadObject("libX11.so.6");
            if(!native_xlibrary)native_fail("load X11 keyboard state");
            native_query_keymap=(int (*)(Display*,char[32]))SDL_LoadFunction(native_xlibrary,"XQueryKeymap");
            if(!native_query_keymap)native_fail("query X11 keyboard state");
            native_keysym_keycode=(KeyCode (*)(Display*,KeySym))SDL_LoadFunction(native_xlibrary,"XKeysymToKeycode");
            if(!native_keysym_keycode)native_fail("map X11 keyboard state");
            native_keyboard_map();
            native_missing_keyup_event=SDL_RegisterEvents(1);
            if(native_missing_keyup_event==(Uint32)-1)native_fail("register X11 key release");
            SDL_AddEventWatch(native_keyboard_watch,NULL);
        }
    }
#endif
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
