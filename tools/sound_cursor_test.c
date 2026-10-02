/* The same stopped-buffer COM probe runs against Wine DirectSound and both
 * ports. Controlled-clock PCM checks additionally exercise port transport.
 * API contract: https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee418076(v=vs.85)
 * Play continues at the cursor: https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee418074(v=vs.85)
 * This is an API fixture, not a capture of dd2h.exe's mixed output. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#define CALL WINAPI
__declspec(dllimport) int CALL DirectSoundCreate(void*,void**,void*);
#else
#define CALL
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
int DirectSoundCreate(int,void**,int);
static unsigned now;
unsigned dd2_platform_ms(void){return now;}
void FUN_0041345c(void){abort();}
void dd2_snd_mix_flip(void);
#endif
static void require(int ok,const char *reason){
    if(!ok){fprintf(stderr,"Sound cursor test: %s\n",reason);exit(1);}
}
#define METHOD(buffer,index,type) ((type)(*(void***)(buffer))[index])
typedef int(CALL *Position)(void*,unsigned*,unsigned*);
typedef int(CALL *Seek)(void*,unsigned);
typedef int(CALL *Status)(void*,unsigned*);
typedef int(CALL *Release)(void*);
typedef int(CALL *Play)(void*,int,int,int);
typedef int(CALL *Lock)(void*,unsigned,unsigned,void**,unsigned*,void**,unsigned*,int);
typedef int(CALL *Unlock)(void*,void*,unsigned,void*,unsigned);
typedef int(CALL *Create)(void*,void*,void**,int);
static void put16(unsigned char *p,unsigned v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);}
static void put32(unsigned char *p,unsigned v){put16(p,v);put16(p+2,v>>16);}
static void *make_buffer(void *device,unsigned channels,unsigned bits,unsigned size){
    unsigned char wave[18]={0};
    uint32_t descriptor[5]={20,0x20,size,0,(uint32_t)(uintptr_t)wave};
    void *buffer=NULL;unsigned align=channels*(bits/8);
    put16(wave,1);put16(wave+2,channels);put32(wave+4,22050);
    put32(wave+8,22050*align);put16(wave+12,align);put16(wave+14,bits);
    require(METHOD(device,3,Create)(device,descriptor,&buffer,0)==0,"create buffer");
    return buffer;
}
#ifndef _WIN32
static unsigned cursor(void *buffer){
    unsigned play=0xffffffff,write=0xffffffff;
    require(METHOD(buffer,4,Position)(buffer,&play,&write)==0,"get cursor");
    return play;
}
#endif
static void stopped_probe(void *device){
    const unsigned offsets[]={0,1,3,17,63,64,0xffffffff};
    unsigned mode,i;
    puts("[");
    for(mode=0;mode<2;mode++){
        unsigned align=mode?4:1;
        void *buffer=make_buffer(device,mode?2:1,mode?16:8,64);
        for(i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++){
            unsigned play=0xffffffff,write=0xffffffff;
            unsigned result=(unsigned)METHOD(buffer,13,Seek)(buffer,offsets[i]);
            require(METHOD(buffer,4,Position)(buffer,&play,&write)==0,"read stopped cursor");
            printf("{\"align\":%u,\"request\":%u,\"result\":%u,\"play\":%u,\"write\":%u}%s\n",
                   align,offsets[i],result,play,write,(mode==1 && i==6)?"":",");
        }
        { unsigned status=0xffffffff;
          require(METHOD(buffer,9,Status)(buffer,&status)==0 && status==0,"stopped buffer status");
          require((unsigned)METHOD(buffer,9,Status)(buffer,NULL)==0x80070057u,"missing status output must fail"); }
        require(METHOD(buffer,4,Position)(buffer,NULL,NULL)==0,"optional cursor outputs");
        METHOD(buffer,2,Release)(buffer);
    }
    puts("]");
}
#ifndef _WIN32
static void controlled_pcm(void *device){
    void *buffer=make_buffer(device,1,8,256),*p1,*p2;
    unsigned n1,n2,i,status;
    require(METHOD(buffer,11,Lock)(buffer,0,256,&p1,&n1,&p2,&n2,0)==0 && n1==256 && !n2,"lock test samples");
    for(i=0;i<256;i++)((unsigned char*)p1)[i]=(unsigned char)i;
    METHOD(buffer,19,Unlock)(buffer,p1,n1,p2,n2);
    require(METHOD(buffer,13,Seek)(buffer,17)==0,"seek before play");
    METHOD(buffer,12,Play)(buffer,0,0,1);
    now=1;dd2_snd_mix_flip(); /* 22 samples from source offset 17. */
    require(cursor(buffer)==39,"Play reset the selected source offset");
    { unsigned play,write;
      METHOD(buffer,4,Position)(buffer,&play,&write);
      require(play==39 && write==3,"active write cursor requires the 10ms source lead"); }
    METHOD(buffer,12,Play)(buffer,0,0,1); /* Already playing: continue. */
    now=2;dd2_snd_mix_flip();
    require(cursor(buffer)==61,"repeated Play restarted a playing buffer");
    METHOD(buffer,18,Release)(buffer);
    now=12;dd2_snd_mix_flip(); /* Stop leaves cursor unchanged and emits silence. */
    require(cursor(buffer)==61,"Stop lost its source cursor");
    METHOD(buffer,12,Play)(buffer,0,0,1);
    now=13;dd2_snd_mix_flip();
    require(cursor(buffer)==83,"Play did not resume the stopped buffer");
    require(METHOD(buffer,13,Seek)(buffer,250)==0,"seek while playing");
    now=14;dd2_snd_mix_flip();
    require(cursor(buffer)==16,"loop wrap after live seek");
    require((unsigned)METHOD(buffer,13,Seek)(buffer,256)==0x80070057u,"out-of-buffer seek must fail");
    require(cursor(buffer)==16,"invalid seek changed cursor");
    METHOD(buffer,18,Release)(buffer);
    METHOD(buffer,13,Seek)(buffer,255);
    METHOD(buffer,12,Play)(buffer,0,0,0);
    now=15;dd2_snd_mix_flip();
    require(cursor(buffer)==0,"one-shot end must reset cursor");
    require(METHOD(buffer,9,Status)(buffer,&status)==0 && status==0,"one-shot must stop at end");
    METHOD(buffer,12,Play)(buffer,0,0,0); /* Replay after automatic end begins at zero. */
    now=16;dd2_snd_mix_flip();
    require(cursor(buffer)==22,"one-shot replay source offset");
    METHOD(buffer,18,Release)(buffer);
    METHOD(buffer,2,Release)(buffer);
    /* Eight source frames per output step on a four-frame loop used to leave
       the cursor out of bounds after subtracting the loop length only once. */
    buffer=make_buffer(device,1,8,4);
    require(METHOD(buffer,11,Lock)(buffer,0,4,&p1,&n1,&p2,&n2,0)==0,"lock short loop");
    { const unsigned char small[4]={128,192,255,0};memcpy(p1,small,4); }
    METHOD(buffer,19,Unlock)(buffer,p1,n1,p2,n2);
    require(METHOD(buffer,17,Seek)(buffer,176400)==0,"set short-loop frequency");
    METHOD(buffer,13,Seek)(buffer,2);METHOD(buffer,12,Play)(buffer,0,0,1);
    now=17;dd2_snd_mix_flip();
    require(cursor(buffer)==2,"high-frequency step escaped the short loop");
    METHOD(buffer,18,Release)(buffer);METHOD(buffer,2,Release)(buffer);
    require(*(int*)0x462ff0==0,"transport changed engine frame counter");
}
#endif
int main(int argc,char **argv){
    void *device;
#ifndef _WIN32
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
            MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map image");
#endif
    *(int*)0x462ff0=0;
    setenv("DD2_SOUND","1",1);setenv("DD2_REALTIME","1",1);
    if(argc==2)setenv("DD2_SNDPCM",argv[1],1);
#else
    (void)argc;(void)argv;
#endif
    require(DirectSoundCreate(0,&device,0)==0,"create DirectSound device");
    stopped_probe(device);
#ifndef _WIN32
    if(argc==2)controlled_pcm(device);
#endif
    METHOD(device,2,Release)(device);
    return 0;
}
