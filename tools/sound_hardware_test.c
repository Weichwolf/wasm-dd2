/* The same stopped-buffer COM probe runs against real Wine and both ports.
 * Exercise the actual hardware rejection/software retry branch and pointer
 * result, including a primary buffer that is exempt from that rejection. */
#ifdef _WIN32
#include "reference/winmm_timer_observer.h"
#define CALL API
IMPORT int CALL DirectSoundCreate(int,void**,int);
static void fail(const char* message){
    U32 size=0,written;while(message[size])size++;
    WriteFile(GetStdHandle(-12),message,size,&written,0);ExitProcess(2);
}
#else
#include <stdio.h>
#include <stdlib.h>
#define CALL
int DirectSoundCreate(int,void**,int);
unsigned dd2_platform_ms(void){return 0;}
void FUN_0041345c(void){abort();}
static void fail(const char* message){fputs(message,stderr);exit(2);}
#endif
typedef int(CALL *Create)(void*,unsigned*,void**,int);
typedef unsigned(CALL *Reference)(void*);
typedef int(CALL *Status)(void*,unsigned*);
#define METHOD(p,i,t) ((t)(*(void***)(p))[i])
static void require(int ok,const char* message){if(!ok)fail(message);}
static void run(void){
    unsigned char wave[18]={1,0,1,0,0x22,0x56,0,0,0x22,0x56,0,0,1,0,8,0,0,0};
    unsigned desc[5]={20,0xe6,27748,0,0},st=999;
    unsigned primary_desc[5]={20,5,0,0,0};
    void *device,*buffer=(void*)0x1234,*primary;
    int result;
    require(!DirectSoundCreate(0,&device,0),"create device\n");
    desc[4]=(unsigned)wave;
    result=METHOD(device,3,Create)(device,desc,&buffer,0);
    require((unsigned)result==0x80004001u && !buffer,"hardware unsupported result/pointer\n");
    /* This is the original engine's branch: retry only after a negative HRESULT. */
    require(result<0,"hardware HRESULT sign\n");
    desc[1]=0xe2;
    require(!METHOD(device,3,Create)(device,desc,&buffer,0) && buffer,"software fallback\n");
    require(!METHOD(buffer,9,Status)(buffer,&st) && !st,"fallback starts stopped\n");
    require(!METHOD(buffer,2,Reference)(buffer),"release fallback\n");
    require(!METHOD(device,3,Create)(device,primary_desc,&primary,0) && primary,"primary hardware flag exemption\n");
    require(!METHOD(primary,2,Reference)(primary),"release primary\n");
    require(!METHOD(device,2,Reference)(device),"release device\n");
}
#ifdef _WIN32
void start(void){
    static const char ok[]="{\"hardware_result\":2147500033,\"hardware_pointer_null\":true,\"software_retry\":true,\"primary_exempt\":true}\n";
    U32 written;run();WriteFile(GetStdHandle(-11),ok,sizeof(ok)-1,&written,0);ExitProcess(0);
}
#else
int main(void){
    run();puts("{\"hardware_result\":2147500033,\"hardware_pointer_null\":true,\"software_retry\":true,\"primary_exempt\":true}");return 0;
}
#endif
