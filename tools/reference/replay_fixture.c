/* Isolated replay components: actual original x86 or actual reconstructed C.
 * Explicit input fixtures; this does not run or establish full-game parity. */
#include "pe_fixture.h"

#define PAD 0x754448u
#define SCRIPT 0x9376b0u
#define CURSOR 0x9392b4u
#define END 0x9392c4u
#ifdef DD2_ORIGINAL_REPLAY
#define Record_Event ((void (*)(unsigned,int))(uintptr_t)0x447940)
#define Terminate_Replay ((void (*)(int))(uintptr_t)0x447a2c)
#define Terminate_Replay_Bodge ((void (*)(int))(uintptr_t)0x447a60)
#define Control_Car_Replay ((void (*)(int,int))(uintptr_t)0x4474a0)
#else
void Record_Event(unsigned,int);
void Terminate_Replay(int);
void Terminate_Replay_Bodge(int);
void Control_Car_Replay(int,int);
#endif

static void reset(unsigned type) {
    memset((void*)0x467074,0,20);
    memset((void*)0x9392b0,0,24);
    memset((void*)SCRIPT,0,0x1c00);
    memset((void*)0x792a00,0,20*0x1b2);
    memset((void*)PAD,0,16);
    put32(0x7746ac,0);
    put32(CURSOR,SCRIPT);
    put32(END,SCRIPT);
    put32(0x467078,type);
    *((unsigned char*)PAD+9)=(unsigned char)type;
}
static unsigned snapshots;
static void snapshot(FILE *out,const char *name) {
    unsigned regions[5][2]={{SCRIPT,0x1c00},{0x9392b0,24},{0x467074,20},{0x792a00,20*0x1b2},{0x7746ac,4}};
    unsigned i;
    printf("%u %s\n",snapshots++,name);
    for(i=0;i<5;i++) require(fwrite((void*)(uintptr_t)regions[i][0],1,regions[i][1],out)==regions[i][1],"write checkpoint");
}
int main(int argc,char **argv) {
    FILE *out; unsigned type,i,n,car;
    const unsigned keyboard[]={0x1000,0x1000,0x9000,0x9000,0x9000,0x4800,0,0x800};
    const unsigned analog[]={0x80,0x80,0x9f,0x9f,0x40,0x21,0,0xc0};
    const uint16_t words[2][5]={{0x1003,0x9004,0x0002,0x4801,0xa001},{0x0280,0x039f,0x0240,0x0121,0x8080}};
    const int steering[]={-512,-256,-64,-32,0,32,64,256,512};
    const uint16_t controls[]={0,0x8000,0x4000,0xa000,0x6000,0xc000,0x1000,0x800,0x9000,0x4800};
    require(argc==3,"original executable and checkpoint output required");
    map_original(argv[1]); out=fopen(argv[2],"wb"); require(out!=NULL,"open checkpoints");
    for(type=1;type<=3;type++) {
        reset(type);
        for(i=0;i<8;i++) {Record_Event(type==1?keyboard[i]:analog[i],PAD);put32(0x9392b8,1);snapshot(out,"record-change");}
        Terminate_Replay(PAD); snapshot(out,"terminate");
        reset(type); memset((void*)SCRIPT,0x5a,8); Terminate_Replay_Bodge(PAD); snapshot(out,"bodge-word-neighbors");
        reset(type);
        for(n=0;n<2050;n++) {
            Record_Event(type==1?0x1000:0x80,PAD);put32(0x9392b8,1);
            if(n==252||n==253||n==254||n==255||n==2044||n==2045||n==2046||n==2047||n==2049) snapshot(out,"repeat-count-boundary");
        }
        reset(type); put32(CURSOR,0x9392b0);Record_Event(0,PAD);snapshot(out,"record-full-first");Record_Event(0,PAD);snapshot(out,"record-full-again");
        reset(type);put32(0x9392c0,1);Record_Event(0x1000,PAD);snapshot(out,"invalid-record");
        reset(type);put32(0x467074,1);put32(CURSOR,SCRIPT-2);put32(END,SCRIPT+8);
        for(i=0;i<5;i++) put16(SCRIPT+i*2,words[type==1?0:1][i]);
        for(i=0;i<16;i++) {Control_Car_Replay(0,PAD);snapshot(out,"decode-script");}
    }
    for(car=0;car<20;car+=car?12:7) for(n=0;n<sizeof steering/sizeof steering[0];n++) for(i=0;i<sizeof controls/sizeof controls[0];i++) {
        reset(1); put32(0x467074,1); put32(CURSOR,SCRIPT-2); put32(END,SCRIPT+2);
        put16(SCRIPT,(uint16_t)(controls[i]|1)); put32(0x792a82+car*0x1b2,(unsigned)steering[n]);
        Control_Car_Replay((int)car,PAD);snapshot(out,"keyboard-steering-throttle");
    }
    require(fclose(out)==0,"close checkpoints");return 0;
}
