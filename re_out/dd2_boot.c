/* Native/browser user entry after platform CRT/image initialization.
 * Original user WinMain @0x423ae0 calls Init_Application, then the same
 * pal_flag/Play_Intro/Init_Main/Init_Front_End/Front_End sequence as ddmain
 * @0x423b28, then Close_Application and returns 1. Do not apply the direct-
 * race harness's post-frontend snapshot to this actual startup path. */
#include <stdint.h>
extern uint16_t Init_Application(void*);
extern void ddmain(void);
extern void Close_Application(const char*,const char*);

int dd2_game_run(void){
    if(Init_Application((void*)1))ddmain();
    Close_Application(0,0);
    return 1; /* Original user WinMain return value, including normal Quit. */
}
