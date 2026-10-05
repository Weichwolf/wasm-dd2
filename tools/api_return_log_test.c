/* Exercise actual providers in the unchanged original's mixed API order.
 * This is a transport/observer fixture, not an engine/video/PCM acceptance. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
extern unsigned GetTickCount(void),dd2_tick_replay_calls(void),dd2_random_replay_calls(void);
#ifdef DD2_HAVE_API_OBSERVER
extern unsigned dd2_api_observer_flush(void);
#endif
int main(int argc,char** argv){
    FILE* schedule;FILE* returned;unsigned calls=0;int kind;
    if(argc!=3)return 2;
#ifndef __EMSCRIPTEN__
    if(mmap((void*)0x936000,0x2000,PROT_READ|PROT_WRITE,
            MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0)==MAP_FAILED)return 2;
#endif
    schedule=fopen(argv[1],"rb");if(!schedule)return 2;
    returned=fopen(argv[2],"wbx");if(!returned)return 2;
    while((kind=fgetc(schedule))!=EOF){
        unsigned value;errno=EDOM;
        if(kind==1)value=GetTickCount();
        else if(kind==2)value=(unsigned)rand();
        else return 2;
        if(errno!=EDOM || (kind==2 && value>32767))return 2;
        {
            unsigned char bytes[4];unsigned i;
            for(i=0;i<4;i++)bytes[i]=(unsigned char)(value>>(i*8));
            if(fwrite(bytes,1,4,returned)!=4)return 2;
        }
        calls++;
    }
    fclose(schedule);if(fclose(returned)!=0)return 2;
#ifdef DD2_HAVE_API_OBSERVER
    {
        unsigned observed;errno=ERANGE;observed=dd2_api_observer_flush();
        if(errno!=ERANGE || (getenv("DD2_API_OBSERVE") && observed!=calls) ||
                (!getenv("DD2_API_OBSERVE") && observed!=0))return 2;
    }
#endif
    printf("{\"clock_calls\":%u,\"random_calls\":%u,\"schedule_records\":%u,\"errno_preserved\":true}\n",
           dd2_tick_replay_calls(),dd2_random_replay_calls(),calls);
    return 0;
}
