/* Strict exact game-clock inputs, including uint32 wrap and repeated values. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
extern unsigned GetTickCount(void),dd2_platform_ms(void),dd2_tick_replay_calls(void);
int main(void){
    static const unsigned want[]={0xfffffff0u,0xfffffff8u,3,3};unsigned i;
    if(dd2_tick_replay_calls()!=0 || dd2_platform_ms()!=0)return 2;
    for(i=0;i<4;i++){
        if(GetTickCount()!=want[i] || dd2_platform_ms()!=want[i] || dd2_tick_replay_calls()!=i+1){
            fprintf(stderr,"Clock input changed, advanced without API read, or has wrong extent\n");return 2;
        }
    }
    puts("{\"exact_clock_returns\":4,\"uint32_wrap\":true,\"repeat_retained\":true}");return 0;
}
