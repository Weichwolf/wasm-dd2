/* Actual Watcom returns from a later original L6 attract race. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    if(getenv("DD2_RANDOM_LEVEL") && !strcmp(getenv("DD2_RANDOM_LEVEL"),"all")){
        static const unsigned levels[]={0,9,7,6};
        static const unsigned boot[]={16838,5758,10113,17515};
        for(i=0;i<4;i++){
            *(unsigned*)(uintptr_t)0x936ff4=levels[i];
            if((unsigned)rand()!=boot[i] || dd2_random_replay_calls()!=i+1)return 2;
        }
        puts("{\"calculated_boot_returns\":4,\"all_levels\":true}");return 0;
    }
    *(unsigned*)(uintptr_t)0x936ff4=1;
    if(rand()!=16838 || dd2_random_replay_calls()!=0)return 2;
    *(unsigned*)(uintptr_t)0x936ff4=6;
    for(i=0;i<4;i++)if((unsigned)rand()!=want[i] || dd2_random_replay_calls()!=i+1)return 2;
    puts("{\"calculated_original_returns\":4,\"level_gate\":true}");return 0;
}
