/* Exercise production USER32 activation, independent IDs, wrap-safe deadlines
 * and actual mixer sample time. The callback records transport invocations;
 * this fixture does not substitute or verify the engine Sound_Timer_ body. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
extern int RegisterClassA(int),ShowWindow(int,int),DestroyWindow(int);
extern void* CreateWindowExA(int,int,int,int,int,int,int,int,int,int,int,int);
extern int timeSetEvent(int,int,void*,int,int),timeKillEvent(int);
extern void dd2_mmtimer_poll(void),dd2_snd_mix_flip(void);
extern int DirectSoundCreate(int,void**,int),g_dd2_mmtimer_active;
extern unsigned dd2_audio_ms(void);
static unsigned now,fires,flips;
static int first,stored,activations,destructions;
unsigned dd2_platform_ms(void){return now;}
int dd2_frame_count(void){return (int)flips;}
void FUN_0041345c(void){fires++;dd2_mmtimer_poll();} /* Reentrancy must be harmless. */
static void require(int value,const char* reason){
    if(!value){fprintf(stderr,"Multimedia timer fixture: %s\n",reason);exit(2);}
}
static int procedure(void* window,unsigned message,unsigned active,unsigned detail){
    (void)window;(void)detail;
    if(message==0x1c){
        activations++;
        if(!active){require(timeKillEvent(stored)==0,"deactivation cancels stored ID");stored=0;}
        else if(!stored)stored=timeSetEvent(400,10,(void*)1,0,1);
    }else if(message==2)destructions++;
    return 0;
}
static void show_game(void){
    uintptr_t wc[10]={0};wc[1]=(uintptr_t)procedure;
    require(RegisterClassA((int)(uintptr_t)wc)==1,"register WNDCLASS procedure");
    require(CreateWindowExA(8,0,0,0,0,0,320,240,0,0,0,0)==(void*)1,"create logical game window");
    require(ShowWindow(1,1)==0 && activations==1 && stored,"activation before explicit timer");
    first=stored;
}
static void realtime(unsigned gap){
    unsigned i;int id=0;
    now=0xfffffff0u;show_game();
    now+=gap;dd2_mmtimer_poll();
    require(fires==gap/400,"first timer elapsed before explicit registration");
    stored=timeSetEvent(400,10,(void*)1,0,1);
    require(first==16 && stored==33 && g_dd2_mmtimer_active==2,"original independent startup IDs");
    now=0xfffffff0u+gap+399;dd2_mmtimer_poll();
    require(fires==(gap+399)/400,"second timer fired early");
    now++;dd2_mmtimer_poll();
    require(fires==(gap+400)/400+1,"independent wrapped timer deadlines");
    require(ShowWindow(1,1)==1 && activations==1,"repeated show must not reactivate");
    { int killed=stored;
      require(ShowWindow(1,0)==1 && g_dd2_mmtimer_active==1,"hide preserves first timer");
      require(timeKillEvent(killed)==97 && timeKillEvent(0)==97,"stale ID must not cancel first");
    }
    now=0xfffffff0u+gap+1600;dd2_mmtimer_poll();
    require(fires==gap/400+5,"first timer survives deactivation and catches up");
    require(ShowWindow(1,1)==0 && stored!=first && stored!=33 && g_dd2_mmtimer_active==2,"fresh activation ID");
    require(DestroyWindow(1)==1 && destructions==1 && g_dd2_mmtimer_active==1,"destroy cancels stored timer only");
    require(timeKillEvent(first)==0 && g_dd2_mmtimer_active==0,"cancel final independent timer");
    require(timeKillEvent(0)==0,"Wine's empty slot zero cancellation");
    for(i=0;i<4092;i++){
        id=timeSetEvent(400,10,(void*)1,0,1);
        require(id>0 && id<=65535 && timeKillEvent(id)==0,"WORD timer ID lifetime");
    }
    require(id==65520,"last WORD generation");
    id=timeSetEvent(400,10,(void*)1,0,1);
    require(id==16 && timeKillEvent(id)==0,"zero ID skipped on WORD wrap");
    printf("{\"startup_ids\":[16,33],\"registration_gap_ms\":%u,\"callback_count\":%u,\"wrap\":true,\"cancel_independent\":true,\"reentrant\":true,\"id_word_wrap\":true}\n",gap,fires);
}
static void observed(void){
    static const unsigned expected[]={0,2,2,4,5};
    void* device;unsigned i;
    show_game();stored=timeSetEvent(400,10,(void*)1,0,1);
    require(first==16 && stored==33 && g_dd2_mmtimer_active==2,"observed independent IDs");
    require(DirectSoundCreate(0,&device,0)==0,"open actual mixer device");
    for(i=0;i<5;i++){
        flips=i;dd2_snd_mix_flip();
        require(fires==expected[i],"fixed-cf device time did not drive timers at 400 ms");
        if(i==3)require(timeKillEvent(stored)==0,"cancel second observed timer");
    }
    require(dd2_audio_ms()==1200,"fractional device samples lost elapsed time");
    require(*(int*)(uintptr_t)0x462ff0==0,"menu timer test changed engine frame counter");
    printf("{\"startup_ids\":[16,33],\"callback_counts\":[0,2,2,4,5],\"elapsed_ms\":1200,\"engine_cf\":0}\n");
}
int main(int argc,char** argv){
    require(argc==3,"mode and independently observed registration interval required");
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map engine clock");
#endif
    *(int*)(uintptr_t)0x462ff0=0;
    if(argv[1][0]=='r')realtime((unsigned)strtoul(argv[2],NULL,10));
    else observed();
    return 0;
}
