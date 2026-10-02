/* Exercise real DirectSound COM methods with a controlled real-time clock and
 * fixed engine cf: menu playback, stop, one-shot exhaustion and frequency. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
static unsigned now;
unsigned dd2_platform_ms(void) { return now; }
void FUN_0041345c(void) { abort(); } /* Real-time mixing must not run the cf timer. */
int DirectSoundCreate(int,void**,int);
void dd2_snd_mix_flip(void);
static void require(int ok,const char *reason) {
    if(!ok){fprintf(stderr,"Menu audio test failed: %s\n",reason);exit(1);}
}
int main(int argc,char **argv) {
    void *device,*buffer,**vtable,*part1,*part2;
    unsigned len1,len2,status;
    unsigned char wave[18]={1,0,1,0,0x22,0x56,0,0,0x22,0x56,0,0,1,0,8,0,0,0};
    uint32_t descriptor[5]={20,0x20,4,0,(uint32_t)(uintptr_t)wave};
    const unsigned char source[4]={128,192,255,0};
#ifndef __EMSCRIPTEN__
    require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
        MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map fixed engine image");
#endif
    require(argc==2,"PCM output path");
    setenv("DD2_REALTIME","1",1);setenv("DD2_SOUND","1",1);setenv("DD2_SNDPCM",argv[1],1);
    *(int*)0x462ff0=0;
    require(DirectSoundCreate(0,&device,0)==0,"create sound device");
    vtable=*(void***)device;
    require(((int(*)(void*,void*,void**,int))vtable[3])(device,descriptor,&buffer,0)==0,"create mono8 buffer");
    vtable=*(void***)buffer;
    require(((int(*)(void*,unsigned,unsigned,void**,unsigned*,void**,unsigned*,int))vtable[11])
        (buffer,0,4,&part1,&len1,&part2,&len2,0)==0 && len1==4 && len2==0,"lock PCM");
    memcpy(part1,source,4);
    ((int(*)(void*,void*,unsigned,void*,unsigned))vtable[19])(buffer,part1,len1,part2,len2);
    ((int(*)(void*,int,int,int))vtable[12])(buffer,0,0,1);
    now=1;dd2_snd_mix_flip();now=20;dd2_snd_mix_flip();
    ((int(*)(void*))vtable[18])(buffer);
    now=53;dd2_snd_mix_flip(); /* 33ms of silence after Stop. */
    ((int(*)(void*,unsigned))vtable[13])(buffer,0); /* Replay explicitly seeks zero. */
    ((int(*)(void*,int,int,int))vtable[12])(buffer,0,0,0);
    now=54;dd2_snd_mix_flip();
    ((int(*)(void*,unsigned*))vtable[9])(buffer,&status);
    require(status==0,"one-shot ends while engine cf remains fixed");
    ((int(*)(void*,int))vtable[17])(buffer,11025);
    ((int(*)(void*,int,int,int))vtable[12])(buffer,0,0,1);
    now=55;dd2_snd_mix_flip();
    ((int(*)(void*))vtable[18])(buffer);
    require(*(int*)0x462ff0==0,"audio must not modify engine cf");
    puts("Menu audio COM: looping, stop, one-shot end and half-frequency with fixed cf passed");
    return 0;
}
