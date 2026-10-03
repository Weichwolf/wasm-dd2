/* Actual Watcom returns from a later original L6 attract race. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
extern unsigned dd2_random_replay_calls(void);
int main(void){
    static const unsigned want[]={5300,20703,32340,23162};unsigned i;
#ifndef __EMSCRIPTEN__
    if(mmap((void*)0x936000,0x2000,PROT_READ|PROT_WRITE,
            MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==MAP_FAILED)return 2;
#endif
    *(unsigned*)(uintptr_t)0x936ff4=1;
    if(rand()!=16838 || dd2_random_replay_calls()!=0)return 2;
    *(unsigned*)(uintptr_t)0x936ff4=6;
    for(i=0;i<4;i++)if((unsigned)rand()!=want[i] || dd2_random_replay_calls()!=i+1)return 2;
    puts("{\"calculated_original_returns\":4,\"level_gate\":true}");return 0;
}
