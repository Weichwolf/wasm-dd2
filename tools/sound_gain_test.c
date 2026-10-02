/* Isolate real DirectSound gain without a resampler: mono16, source/device
 * 22050Hz, constant 0.5. A matching source rate removes filter/phase ambiguity.
 * Wine observes real mixer PCM; ports additionally sweep every volume value
 * with a controlled sample clock. This is not a dd2h.exe full-run audio oracle. */
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
#define METHOD(buffer,index,type) ((type)(*(void***)(buffer))[index])
typedef int(CALL *Create)(void*,void*,void**,int);
typedef int(CALL *Lock)(void*,unsigned,unsigned,void**,unsigned*,void**,unsigned*,int);
typedef int(CALL *Unlock)(void*,void*,unsigned,void*,unsigned);
typedef int(CALL *Set)(void*,int);
typedef int(CALL *Get)(void*,int*);
typedef int(CALL *Play)(void*,int,int,int);
typedef int(CALL *Release)(void*);
typedef int(CALL *Duplicate)(void*,void*,void**);
static void require(int ok,const char *reason){
    if(!ok){fprintf(stderr,"Sound gain test: %s\n",reason);exit(1);}
}
static void *make_buffer(void *device,short value){
    unsigned char wave[18]={1,0,1,0,0x22,0x56,0,0,0x44,0xac,0,0,2,0,16,0,0,0};
    uint32_t descriptor[5]={20,0x80e0,4410,0,(uint32_t)(uintptr_t)wave};
    void *buffer,*p1,*p2;unsigned n1,n2,i;
    require(METHOD(device,3,Create)(device,descriptor,&buffer,0)==0,"create mono16 source");
    require(METHOD(buffer,11,Lock)(buffer,0,4410,&p1,&n1,&p2,&n2,0)==0 && n1==4410 && !n2,"lock source");
    for(i=0;i<n1/2;i++)((short*)p1)[i]=value;
    METHOD(buffer,19,Unlock)(buffer,p1,n1,p2,n2);
    return buffer;
}
static void *make_mono8(void *device){
    unsigned char wave[18]={1,0,1,0,0x22,0x56,0,0,0x22,0x56,0,0,1,0,8,0,0,0};
    uint32_t descriptor[5]={20,0x80e0,2205,0,(uint32_t)(uintptr_t)wave};
    void *buffer,*p1,*p2;unsigned n1,n2;
    require(METHOD(device,3,Create)(device,descriptor,&buffer,0)==0,"create mono8 source");
    require(METHOD(buffer,11,Lock)(buffer,0,2205,&p1,&n1,&p2,&n2,0)==0 && n1==2205 && !n2,"lock mono8 source");
    memset(p1,192,n1);METHOD(buffer,19,Unlock)(buffer,p1,n1,p2,n2);
    return buffer;
}
static void controls(void *buffer,int volume,int pan){
    int value=123456;
    require(METHOD(buffer,15,Set)(buffer,volume)==0,"set volume");
    require(METHOD(buffer,16,Set)(buffer,pan)==0,"set pan");
    require(METHOD(buffer,6,Get)(buffer,&value)==0 && value==volume,"read volume");
    require(METHOD(buffer,7,Get)(buffer,&value)==0 && value==pan,"read pan");
    require(METHOD(buffer,8,Get)(buffer,&value)==0 && value==22050,"read original frequency");
}
static void control_probe(void *device,void *buffer){
    int readback;void *copy;
    controls(buffer,-1376,600);
    require(METHOD(buffer,17,Set)(buffer,11025)==0,"set half frequency");
    require(METHOD(buffer,8,Get)(buffer,&readback)==0 && readback==11025,"read changed frequency");
    require(METHOD(device,5,Duplicate)(device,buffer,&copy)==0,"duplicate configured buffer");
    require(METHOD(copy,6,Get)(copy,&readback)==0 && readback==-1376,"duplicate volume");
    require(METHOD(copy,7,Get)(copy,&readback)==0 && readback==600,"duplicate pan");
    require(METHOD(copy,8,Get)(copy,&readback)==0 && readback==11025,"duplicate frequency");
    METHOD(copy,2,Release)(copy);
    require(METHOD(buffer,17,Set)(buffer,0)==0,"restore original frequency");
    require(METHOD(buffer,8,Get)(buffer,&readback)==0 && readback==22050,"original frequency changed");
    require((unsigned)METHOD(buffer,17,Set)(buffer,99)==0x80070057u,"frequency lower bound");
    require((unsigned)METHOD(buffer,17,Set)(buffer,200001)==0x80070057u,"frequency upper bound");
    require((unsigned)METHOD(buffer,15,Set)(buffer,1)==0x80070057u,"volume upper bound");
    require((unsigned)METHOD(buffer,15,Set)(buffer,-10001)==0x80070057u,"volume lower bound");
    require((unsigned)METHOD(buffer,16,Set)(buffer,10001)==0x80070057u,"pan upper bound");
    require((unsigned)METHOD(buffer,16,Set)(buffer,-10001)==0x80070057u,"pan lower bound");
    require((unsigned)METHOD(buffer,6,Get)(buffer,NULL)==0x80070057u,"volume missing output");
    require((unsigned)METHOD(buffer,7,Get)(buffer,NULL)==0x80070057u,"pan missing output");
    require((unsigned)METHOD(buffer,8,Get)(buffer,NULL)==0x80070057u,"frequency missing output");
    require(METHOD(buffer,6,Get)(buffer,&readback)==0 && readback==-1376,"invalid volume changed state");
    require(METHOD(buffer,7,Get)(buffer,&readback)==0 && readback==600,"invalid pan changed state");
    require(METHOD(buffer,8,Get)(buffer,&readback)==0 && readback==22050,"invalid frequency changed state");
    { unsigned char wave[18]={1,0,1,0,0x22,0x56,0,0,0x44,0xac,0,0,2,0,16,0,0,0};
      uint32_t descriptor[5]={20,0,64,0,(uint32_t)(uintptr_t)wave};
      void *limited;
      require(METHOD(device,3,Create)(device,descriptor,&limited,0)==0,"buffer without control caps");
      require((unsigned)METHOD(limited,15,Set)(limited,0)==0x8878001eu,"volume capability");
      require((unsigned)METHOD(limited,16,Set)(limited,0)==0x8878001eu,"pan capability");
      require((unsigned)METHOD(limited,17,Set)(limited,22050)==0x8878001eu,"frequency capability");
      require((unsigned)METHOD(limited,6,Get)(limited,&readback)==0x8878001eu,"volume query capability");
      require((unsigned)METHOD(limited,7,Get)(limited,&readback)==0x8878001eu,"pan query capability");
      METHOD(limited,2,Release)(limited); }
}
int main(int argc,char **argv){
    void *device,*buffer;int volume,pan;
#ifdef _WIN32
    void *mixed_second=NULL,*mixed_third=NULL;
#endif
    require(argc==4 || (argc==5 && (!strcmp(argv[4],"mix") || !strcmp(argv[4],"mono8"))),
            "output, volume, pan and optional mix/mono8 mode required");
    volume=atoi(argv[2]);pan=atoi(argv[3]);
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
    buffer=(argc==5 && !strcmp(argv[4],"mono8"))?make_mono8(device):make_buffer(device,16384);
    control_probe(device,buffer);
    controls(buffer,volume,pan);
    METHOD(buffer,12,Play)(buffer,0,0,1);
#ifdef _WIN32
    if(argc==5 && !strcmp(argv[4],"mix")){
        require(volume==0 && pan==0,"mix fixture requires unity main source");
        mixed_second=make_buffer(device,32767);mixed_third=make_buffer(device,-16385);
        controls(mixed_second,0,0);controls(mixed_third,-1376,0);
        METHOD(mixed_second,12,Play)(mixed_second,0,0,1);METHOD(mixed_third,12,Play)(mixed_third,0,0,1);
    }
    Sleep(200);
#else
    now=10;dd2_snd_mix_flip();
#endif
    METHOD(buffer,18,Release)(buffer);
#ifdef _WIN32
    if(mixed_second)METHOD(mixed_second,18,Release)(mixed_second);
    if(mixed_third)METHOD(mixed_third,18,Release)(mixed_third);
    Sleep(80);
    if(mixed_second)METHOD(mixed_second,2,Release)(mixed_second);
    if(mixed_third)METHOD(mixed_third,2,Release)(mixed_third);
#else
    /* Preserve exact gains for every centidB and representative pans. */
    if(volume==0 && pan==0){
        int i;const int pans[]={600,-600,3000,-3000,10000,-10000};
        for(i=0;i<=10000;i++){
            controls(buffer,-i,0);METHOD(buffer,12,Play)(buffer,0,0,1);
            now++;dd2_snd_mix_flip();METHOD(buffer,18,Release)(buffer);
        }
        for(i=0;i<6;i++){
            controls(buffer,-1376,pans[i]);METHOD(buffer,12,Play)(buffer,0,0,1);
            now++;dd2_snd_mix_flip();METHOD(buffer,18,Release)(buffer);
        }
        /* A float mixer must preserve sums above 1.0 instead of clipping each
           buffer separately. Include a non-power-of-two product to test f32
           rounding before summing on x87 and WASM. */
        { void *second=make_buffer(device,32767),*third=make_buffer(device,-16385);
          controls(buffer,0,0);controls(second,0,0);controls(third,-1376,0);
          METHOD(buffer,12,Play)(buffer,0,0,1);METHOD(second,12,Play)(second,0,0,1);
          METHOD(third,12,Play)(third,0,0,1);
          now++;dd2_snd_mix_flip();METHOD(second,18,Release)(second);METHOD(third,18,Release)(third);
          METHOD(third,2,Release)(third);METHOD(second,2,Release)(second);METHOD(buffer,18,Release)(buffer); }
    }
#endif
    METHOD(buffer,2,Release)(buffer);METHOD(device,2,Release)(device);
    printf("{\"volume\":%d,\"pan\":%d,\"source_rate\":22050,\"source_sample\":%d%s}\n",
           volume,pan,(argc==5 && !strcmp(argv[4],"mono8"))?192:16384,
           argc==5?(!strcmp(argv[4],"mono8")?",\"mode\":\"mono8\"":",\"mode\":\"mix\""):"");
    return 0;
}
