/* Render the original slab effect through the production port mixer.
 * The caller extracts BANK1's RIFF source; no original engine state is injected.
 * Controls are observed in dd2h.exe's Wine trace, not fitted from output samples.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
int DirectSoundCreate(int,void**,int);
void dd2_snd_mix_flip(void);
static unsigned now;
unsigned dd2_platform_ms(void){return now;}
void FUN_0041345c(void){abort();}
#define METHOD(object,index,type) ((type)(*(void***)(object))[index])
typedef int (*Create)(void*,void*,void**,int);
typedef int (*Lock)(void*,unsigned,unsigned,void**,unsigned*,void**,unsigned*,int);
typedef int (*Unlock)(void*,void*,unsigned,void*,unsigned);
typedef int (*Set)(void*,int);
typedef int (*Play)(void*,int,int,int);
typedef int (*Status)(void*,unsigned*);
typedef int (*Release)(void*);
static void require(int ok,const char *reason){
    if(!ok){fprintf(stderr,"Original menu source: %s\n",reason);exit(1);}
}
int main(int argc,char **argv){
    void *device,*buffer,*p1,*p2;FILE *source;
    unsigned n1,n2,status;
    unsigned char wave[18]={1,0,1,0,0x11,0x2b,0,0,0x11,0x2b,0,0,1,0,8,0,0,0};
    uint32_t descriptor[5]={20,0xe2,6314,0,0};
    require(argc==3,"original mono8 source and output path required");
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map image");
#endif
    *(int*)0x462ff0=0;
    setenv("DD2_SOUND","1",1);setenv("DD2_REALTIME","1",1);
    setenv("DD2_SND_RATE","44100",1);setenv("DD2_SNDPCM",argv[2],1);
    descriptor[4]=(uint32_t)(uintptr_t)wave;
    require(DirectSoundCreate(0,&device,0)==0,"create device");
    require(METHOD(device,3,Create)(device,descriptor,&buffer,0)==0,"create source");
    require(METHOD(buffer,11,Lock)(buffer,0,6314,&p1,&n1,&p2,&n2,0)==0 && n1==6314 && !n2,"lock source");
    source=fopen(argv[1],"rb");
    require(source && fread(p1,1,n1,source)==n1 && fgetc(source)==EOF,"read exact original source");
    require(fclose(source)==0,"close source");
    require(METHOD(buffer,19,Unlock)(buffer,p1,n1,p2,n2)==0,"unlock source");
    require(METHOD(buffer,13,Set)(buffer,0)==0,"seek zero");
    require(METHOD(buffer,16,Set)(buffer,0)==0,"center pan");
    require(METHOD(buffer,15,Set)(buffer,-1)==0,"original volume");
    require(METHOD(buffer,17,Set)(buffer,5512)==0,"original frequency");
    require(METHOD(buffer,12,Play)(buffer,0,0,0)==0,"one-shot play");
    for(now=1;now<=1500;now++)dd2_snd_mix_flip();
    now=1500;
    require(METHOD(buffer,9,Status)(buffer,&status)==0 && !status,"one shot completed");
    METHOD(buffer,2,Release)(buffer);METHOD(device,2,Release)(device);
    puts("{\"source_frames\":6314,\"source_rate\":11025,\"frequency\":5512,\"volume\":-1,\"pan\":0,\"device_rate\":44100,\"output_frames\":66150}");
    return 0;
}
