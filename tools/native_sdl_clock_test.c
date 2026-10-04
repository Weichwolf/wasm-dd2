/* Exercise the real SDL initialization and production clock reader together.
 * Window input callbacks are outside this clock-transport fixture's scope. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "dd2_native.h"
unsigned char dd2_keystate[256];
void dd2_key_event(unsigned vk,int down){(void)vk;(void)down;}
void dd2_pad_update(int connected,unsigned x,unsigned y,unsigned buttons){
    (void)connected;(void)x;(void)y;(void)buttons;
}
int FUN_004132f0(void* window,unsigned message,unsigned a,unsigned b){
    (void)window;(void)message;(void)a;(void)b;return 0;
}
extern unsigned GetTickCount(void),dd2_platform_ms(void),dd2_tick_replay_calls(void);
static int recorded_clock_test(void){
    static const unsigned wanted[]={0xfffffff0u,0xfffffff8u,3,3};unsigned i;
    for(i=0;i<4;i++){
        unsigned actual=GetTickCount();
        if(actual!=wanted[i] || dd2_platform_ms()!=wanted[i] || dd2_tick_replay_calls()!=i+1){
            fprintf(stderr,"SDL startup changed the recorded clock input\n");return 2;
        }
    }
    puts("{\"exact_clock_returns\":4,\"uint32_wrap\":true,\"repeat_retained\":true}");return 0;
}
static unsigned monotonic_ms(void){
    struct timespec now;clock_gettime(CLOCK_MONOTONIC,&now);
    return (unsigned)((uint64_t)now.tv_sec*1000+now.tv_nsec/1000000);
}
int main(void){
    unsigned i;
    dd2_native_init();
    if(!dd2_native_enabled()){fprintf(stderr,"Real native window not initialized\n");return 2;}
    if(getenv("DD2_TICK_REPLAY"))return recorded_clock_test();
    for(i=0;i<2;i++){
        unsigned begin=monotonic_ms(),actual=GetTickCount(),end=monotonic_ms();
        if((unsigned)(actual-begin)>(unsigned)(end-begin) || dd2_tick_replay_calls()){
            fprintf(stderr,"Interactive SDL clock is not the actual monotonic clock\n");return 2;
        }
    }
    puts("{\"live_clock\":true,\"native_window\":true}");return 0;
}
