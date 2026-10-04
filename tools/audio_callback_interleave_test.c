/* Exercise production API assertions with suspended nested callback stacks.
 * These controlled caller sequences test scheduling, not original engine PCM.
 * The three-control sequence is observed in the actual original replay trace.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
#include "dd2h_stubs.c"
static DSBuf* test_source;
static unsigned test_flip,test_callbacks;
unsigned dd2_platform_ms(void){return 0;}
unsigned dd2_tick_replay_calls(void){return 0;}
int dd2_frame_count(void){return test_flip;}
static void require(int ok,const char* reason){
    if(!ok){fprintf(stderr,"Callback continuation test: %s\n",reason);exit(2);}
}
static void nested_status(unsigned ordinal,unsigned second){
    volatile unsigned guard[64];unsigned i,status;
    for(i=0;i<64;i++)guard[i]=ordinal*37+i+second*101;
    require(dsb_getstatus(test_source,&status)==0,"own buffer status call");
    require(status==((ordinal%4==3 && second)?0u:1u),"own playback status after intervening main calls");
    for(i=0;i<64;i++)require(guard[i]==ordinal*37+i+second*101,"nested callback stack was overwritten");
}
void FUN_0041345c(void){
    unsigned ordinal=test_callbacks++;volatile unsigned saved=ordinal^0x51349abdu;
    nested_status(ordinal,0);
    if(ordinal%4==1 || ordinal%4==3)nested_status(ordinal,1);
    require(saved==(ordinal^0x51349abdu),"callback local lost across switches");
}
int main(int argc,char** argv){
    void* device;unsigned cycles,i;
    unsigned char format[18]={1,0,1,0};int descriptor[5]={20,0xe2,1024,0,0};
    require(argc==4,"service input, completion report and cycle count required");
    cycles=(unsigned)strtoul(argv[3],0,10);
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x590000,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map counter region");
#endif
    setenv("DD2_SOUND","1",1);setenv("DD2_SND_RATE","44100",1);
    setenv("DD2_AUDIO_SERVICES",argv[1],1);setenv("DD2_AUDIO_SERVICE_REPORT",argv[2],1);
    *(unsigned*)(format+4)=22050;*(unsigned*)(format+8)=22050;
    *(unsigned short*)(format+12)=1;*(unsigned short*)(format+14)=8;
    descriptor[4]=(int)(uintptr_t)format;
    require(DirectSoundCreate(0,&device,0)==0,"own device creation");
    require(ds_createbuffer(device,descriptor,&test_source,0)==0,"own source creation");
    require(timeSetEvent(400,10,(void*)(uintptr_t)0x41345c,0,1)==16,"own timer registration");
    require(dsb_play(test_source,0,0,0)==0,"own initial play");
    for(i=0;i<cycles;i++){
        volatile unsigned guard[64];unsigned j;
        for(j=0;j<64;j++)guard[j]=i*29+j;
        test_flip=i+1;dd2_audio_service_pump();
        if(i%4==0){
            require(dsb_setpan(test_source,0)==0,"main pan");
            require(dsb_setvolume(test_source,-1130)==0,"main volume");
            require(dsb_setfreq(test_source,8333)==0,"main frequency");
        }else if(i%4==1)require(dsb_setvolume(test_source,-900)==0,"main volume between callback APIs");
        else if(i%4==2)require(dsb_setpan(test_source,-100)==0,"main pan before callback API");
        else{
            require(dsb_stop(test_source)==0,"main stop between callback APIs");
            require(dsb_play(test_source,0,0,0)==0,"main play before callback end");
        }
        dd2_audio_service_pump();
        require(test_callbacks==i+1 && ds_service_callbacks==i+1,"callback executed/completed exactly once");
        require(!ds_service_context && !ds_service_callback_active,"main caller context restored");
        for(j=0;j<64;j++)require(guard[j]==i*29+j,"main stack was overwritten");
    }
    test_flip=cycles+1;dd2_audio_service_pump();
    require(ds_service_finished && ds_service_index==ds_service_count,"complete observed input");
    printf("{\"callbacks\":%u,\"patterns\":4,\"stack_guards\":true,\"own_status\":true}\n",test_callbacks);
    return 0;
}
