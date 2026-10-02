/* A separate, unmodified DirectSound API fixture. Deterministic nonconstant
 * PCM exposes resampler/filter errors hidden by the constant gain fixture.
 * The same source, format and controls run against Wine and the port shim. */
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
#define METHOD(object,index,type) ((type)(*(void***)(object))[index])
typedef int(CALL *Create)(void*,void*,void**,int);
typedef int(CALL *Lock)(void*,unsigned,unsigned,void**,unsigned*,void**,unsigned*,int);
typedef int(CALL *Unlock)(void*,void*,unsigned,void*,unsigned);
typedef int(CALL *Set)(void*,int);
typedef int(CALL *Play)(void*,int,int,int);
typedef int(CALL *GetStatus)(void*,unsigned*);
typedef int(CALL *Release)(void*);
static void require(int ok,const char *reason){
    if(!ok){fprintf(stderr,"Sound resample test: %s\n",reason);exit(1);}
}
int main(int argc,char **argv){
    void *device,*buffer,*p1,*p2;
    unsigned n1,n2,i,c,status,rate,bits,channels,align,size,frames;
    int looping;
    uint32_t state=0x12345678u;
    unsigned char wave[18]={1,0};
    uint32_t descriptor[5]={20,0x80e0,0,0,0};
    require(argc==5 || (argc==6 && !strcmp(argv[5],"loop")),"output, frequency, bits, channels and optional loop mode required");
    looping=argc==6;
    rate=atoi(argv[2]);bits=atoi(argv[3]);channels=atoi(argv[4]);
    require(rate>=11025 && rate<=200000 && (bits==8 || bits==16) && (channels==1 || channels==2),"format");
    require(!looping || (bits==8 && channels==1 && (rate==11025 || rate==176400)),"loop format");
    frames=looping?4:4096;align=bits/8*channels;size=frames*align;
    wave[2]=channels;*(uint32_t*)(wave+4)=22050;
    *(uint32_t*)(wave+8)=22050*align;wave[12]=align;wave[14]=bits;
    descriptor[2]=size;descriptor[4]=(uint32_t)(uintptr_t)wave;
#ifndef _WIN32
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
            MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map image");
#endif
    *(int*)0x462ff0=0;
    setenv("DD2_SOUND","1",1);setenv("DD2_REALTIME","1",1);setenv("DD2_SNDPCM",argv[1],1);
#endif
    require(DirectSoundCreate(0,&device,0)==0,"create device");
#ifdef _WIN32
    require(METHOD(device,6,int(CALL *)(void*,HWND,int))(device,GetDesktopWindow(),2)==0,"cooperative level");
#endif
    require(METHOD(device,3,Create)(device,descriptor,&buffer,0)==0,"create source");
    require(METHOD(buffer,11,Lock)(buffer,0,size,&p1,&n1,&p2,&n2,0)==0 && n1==size && !n2,"lock source");
    for(i=0;i<frames;i++)for(c=0;c<channels;c++){
        state=state*1664525u+1013904223u;
        if(bits==16)((short*)p1)[i*channels+c]=(short)(1000+(state>>16)%20000);
        else ((unsigned char*)p1)[i*channels+c]=(unsigned char)(129+(state>>16)%127);
    }
    if(looping){const unsigned char small[4]={128,192,255,0};memcpy(p1,small,4);}
    require(METHOD(buffer,19,Unlock)(buffer,p1,n1,p2,n2)==0,"unlock source");
    require(METHOD(buffer,17,Set)(buffer,rate)==0,"set frequency");
    if(looping && rate==176400)require(METHOD(buffer,13,Set)(buffer,2)==0,"seek short loop");
    require(METHOD(buffer,12,Play)(buffer,0,0,looping)==0,"play source");
#ifdef _WIN32
    Sleep(looping?100:600);
    if(looping){METHOD(buffer,18,Release)(buffer);Sleep(80);}
#else
    for(now=1;now<=(looping?100u:600u);now++)dd2_snd_mix_flip();
    if(looping)METHOD(buffer,18,Release)(buffer);
#endif
    require(METHOD(buffer,9,GetStatus)(buffer,&status)==0 && status==0,"one shot completed");
    METHOD(buffer,2,Release)(buffer);METHOD(device,2,Release)(device);
    printf("{\"source_frames\":%u,\"frequency\":%u,\"bits\":%u,\"channels\":%u,\"status\":%u%s}\n",
           frames,rate,bits,channels,status,looping?",\"loop\":true":"");
    return 0;
}
