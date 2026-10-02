/* DuplicateSoundBuffer shares sample storage independently of COM objects.
 * Run the identical release/duplicate/lock/AddRef probe against Wine and ports. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
typedef int(CALL *Duplicate)(void*,void*,void**);
typedef int(CALL *Lock)(void*,unsigned,unsigned,void**,unsigned*,void**,unsigned*,int);
typedef int(CALL *Unlock)(void*,void*,unsigned,void*,unsigned);
typedef unsigned(CALL *Reference)(void*);
typedef int(CALL *Seek)(void*,unsigned);
typedef int(CALL *Position)(void*,unsigned*,unsigned*);
typedef int(CALL *Play)(void*,int,int,int);
static void require(int ok,const char* why){
    if(!ok){fprintf(stderr,"Sound lifetime test: %s\n",why);exit(1);}
}
static short* samples(void* buffer){
    void *p1,*p2;unsigned n1,n2;
    require(METHOD(buffer,11,Lock)(buffer,0,128,&p1,&n1,&p2,&n2,0)==0 && n1==128 && !n2 && p1,"lock live shared source");
    return (short*)p1;
}
static void unlock(void* buffer,short* data){
    require(METHOD(buffer,19,Unlock)(buffer,data,128,NULL,0)==0,"unlock shared source");
}
static void check_source(void* buffer){
    unsigned i;short* data=samples(buffer);
    for(i=0;i<64;i++)require(data[i]==(i==32?-12345:(short)((int)i*201-6201)),"shared source lost or changed after release");
    unlock(buffer,data);
}
static void check_cursor(void* buffer,unsigned expected){
    unsigned play,write;
    require(METHOD(buffer,4,Position)(buffer,&play,&write)==0 && play==expected && write==expected,"duplicates must retain independent stopped cursors");
}
int main(int argc,char** argv){
    unsigned char wave[18]={1,0,1,0,0x44,0xac,0,0,0x88,0x58,1,0,2,0,16,0,0,0};
    uint32_t desc[5]={20,0x20,128,0,(uint32_t)(uintptr_t)wave};
    void *device,*original,*a,*b,*c;
    short* data;unsigned i,retained,added,final;
#ifndef _WIN32
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map engine clock");
#endif
    *(int*)0x462ff0=0;
    setenv("DD2_SOUND","1",1);setenv("DD2_REALTIME","1",1);
    if(argc==2)setenv("DD2_SNDPCM",argv[1],1);
#else
    (void)argc;(void)argv;
#endif
    require(DirectSoundCreate(0,&device,0)==0,"open DirectSound");
    require(METHOD(device,3,Create)(device,desc,&original,0)==0,"create source");
    data=samples(original);
    for(i=0;i<64;i++)data[i]=(short)((int)i*201-6201);
    unlock(original,data);
    require(METHOD(original,13,Seek)(original,12)==0,"seek original");
    require(METHOD(device,5,Duplicate)(device,original,&a)==0,"first duplicate");
    require(METHOD(device,5,Duplicate)(device,original,&b)==0,"second duplicate");
    check_cursor(a,0);check_cursor(b,0);
    require(METHOD(original,2,Reference)(original)==0,"release original COM object");
    /* Duplicate an existing duplicate after the original object has gone. */
    require(METHOD(a,13,Seek)(a,6)==0,"seek first duplicate");check_cursor(b,0);
    require(METHOD(device,5,Duplicate)(device,a,&c)==0,"duplicate surviving source");check_cursor(c,0);
    data=samples(b);data[32]=-12345;unlock(b,data);
    check_source(a);check_source(c);
    require(METHOD(a,2,Reference)(a)==0,"release first duplicate");
    require(METHOD(b,2,Reference)(b)==0,"release second duplicate");
    check_source(c);
    added=METHOD(c,1,Reference)(c);require(added==2,"COM AddRef result");
    retained=METHOD(c,2,Reference)(c);require(retained==1,"COM nonfinal Release result");
    check_source(c);
    require(METHOD(c,13,Seek)(c,34)==0,"seek surviving source");check_cursor(c,34);
#ifndef _WIN32
    require(METHOD(c,12,Play)(c,0,0,1)==0,"play surviving source");
    now=10;dd2_snd_mix_flip();
    require(METHOD(c,18,Reference)(c)==0,"stop surviving source");
#endif
    final=METHOD(c,2,Reference)(c);require(final==0,"final COM Release result");
    METHOD(device,2,Reference)(device);
    printf("{\"shared_source_frames\":64,\"addref\":%u,\"release_retained\":%u,\"final_release\":%u}\n",added,retained,final);
    return 0;
}
