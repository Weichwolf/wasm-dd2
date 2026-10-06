/* Exercise the production GetTickCount transport without supplying game state. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

extern unsigned GetTickCount(void);
extern unsigned dd2_tick_replay_calls(void);

int main(int argc,char **argv){
    char *end;
    unsigned long requested;
    unsigned i,value=0,fold=2166136261u;
    uint64_t sum=0;
    FILE *output=NULL;
    if(argc!=4 && argc!=5)return 2;
    errno=0;requested=strtoul(argv[1],&end,10);
    if(errno || !*argv[1] || *end || !requested || requested>0xffffffffu)return 2;
    /* Configure the same C environment in Node/WASM and native; Node's host
     * environment is not automatically imported by Emscripten's libc. */
    setenv("DD2_TICK_REPLAY",argv[2],1);
    if(strcmp(argv[3],"-"))setenv("DD2_TICK_REPLAY_FORMAT",argv[3],1);
    else unsetenv("DD2_TICK_REPLAY_FORMAT");
    if(argc==5){output=fopen(argv[4],"wb");if(!output)return 2;}
    for(i=0;i<(unsigned)requested;i++){
        unsigned char bytes[4];
        value=GetTickCount();sum+=value;fold=(fold^value)*16777619u;
        bytes[0]=value;bytes[1]=value>>8;bytes[2]=value>>16;bytes[3]=value>>24;
        if(output && fwrite(bytes,1,4,output)!=4)return 2;
    }
    if(output && fclose(output))return 2;
    printf("{\"calls\":%u,\"last\":%u,\"sum\":\"%llu\",\"fold\":%u}\n",
           dd2_tick_replay_calls(),value,(unsigned long long)sum,fold);
    return 0;
}
