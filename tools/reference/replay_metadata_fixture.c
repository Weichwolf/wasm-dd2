/* Replay packing and load setup: actual original x86 or actual engine C.
 * Original runs stop at read-only hardware breakpoints before the external
 * file browser/player. Reconstructed runs observe the same call boundaries.
 * Explicit component inputs; not original whole-game save/load acceptance. */
#include "pe_fixture.h"

#define PACKED 0x93a490u
#define SCRIPT 0x9376b0u
#define ORDER 0x795c28u
#define PACKED_BYTES (18u+0x1c00u+20u)
#ifdef DD2_ORIGINAL_REPLAY
#define Save_Replay ((void (*)(void))(uintptr_t)0x44ab98)
#define Load_Replay ((void (*)(void))(uintptr_t)0x44ac60)
#else
void FUN_0044ab98(void);
void FUN_0044ac60(void);
#define Save_Replay FUN_0044ab98
#define Load_Replay FUN_0044ac60
#endif

/* car, mode, type, season, track level, controller type, last-packet offset.
 * First four cover real values. Remaining cases check signed WORDs and
 * discarded upper halves, without claiming those are valid gameplay modes. */
static const uint32_t cases[][7]={
    {7,1,3,4,10,1,12},
    {19,0,4,0,8,2,0x1bfe},
    {3,2,2,3,12,3,0},
    {0,0,0,0,1,1,2},
    {0x7fff,0x8000,0xffff,0x8001,0x7fff,0xfffe,254},
    {0xffff,0x7fff,0x8000,0xfffe,0xffff,0x8000,0x400},
    {0x12340008,0x11110001,0xabcd0003,0x76540004,0xfedc000a,0xaaaa0001,0x102},
    {0xffffffff,0xffff8000,0xaaaa7fff,0x80008001,0xbbbb000c,0xcccc0003,0x1bfc}
};
static const unsigned state[]={0x467400,0x4673f8,0x4673f4,0x93dec0,
    0x936ff4,0x9392bc,0x467078,0x46765c,0x9392c4};
static FILE *output;
static unsigned observed;
static void snapshot(void) {
    unsigned i;
    require(fwrite((void*)(PACKED-16),1,PACKED_BYTES+32,output)==PACKED_BYTES+32,"packed checkpoint");
    require(fwrite((void*)SCRIPT,1,0x1c00,output)==0x1c00,"script checkpoint");
    require(fwrite((void*)ORDER,1,20,output)==20,"order checkpoint");
    for(i=0;i<sizeof state/sizeof state[0];i++)
        require(fwrite((void*)(uintptr_t)state[i],1,4,output)==4,"metadata checkpoint");
    observed++;
}
#ifndef DD2_ORIGINAL_REPLAY
unsigned LoadSave(unsigned mode,void *data) {
    require(mode==5 && (uintptr_t)data==PACKED,"actual save API arguments");
    snapshot(); return 0;
}
unsigned View_Frontend_Replay(void) { snapshot(); return 0; }
#endif

int main(int argc,char **argv) {
    unsigned i,index,loading; const uint32_t *v;
    uint32_t original[4]; const unsigned restore[]={0x467400,0x4673f8,0x4673f4,0x46765c};
    require(argc==5,"executable, output, save/load and case index required");
    index=(unsigned)strtoul(argv[4],NULL,10);
    require(index<sizeof cases/sizeof cases[0],"known metadata case");
    require(strcmp(argv[3],"save")==0 || strcmp(argv[3],"load")==0,"save/load phase");
    loading=strcmp(argv[3],"load")==0; v=cases[index];
    map_original(argv[1]);
    output=fopen(argv[2],"wb"); require(output!=NULL,"open metadata checkpoint");
    memset((void*)(PACKED-16),0xa5,PACKED_BYTES+32);
    for(i=0;i<0x1c00;i++) *((unsigned char*)SCRIPT+i)=(unsigned char)(i*37u+index*13u);
    for(i=0;i<20;i++) *((unsigned char*)ORDER+i)=(unsigned char)((i+index)%20);
    put32(0x467400,v[0]); put32(0x4673f8,v[1]); put32(0x4673f4,v[2]);
    put32(0x93dec0,v[3]); put32(0x9392bc,v[4]); put32(0x467078,v[5]);
    put32(0x9392c4,SCRIPT+v[6]); put32(0x936ff4,99); put32(0x46765c,0x33445566);
    if(loading) {
        put16(PACKED,0x2020); put16(PACKED+2,(uint16_t)v[0]);
        put32(PACKED+4,SCRIPT+v[6]);
        for(i=1;i<6;i++) put16(PACKED+6+i*2,(uint16_t)v[i]);
        memcpy((void*)(PACKED+18),(void*)SCRIPT,0x1c00);
        memcpy((void*)(PACKED+18+0x1c00),(void*)ORDER,20);
        memset((void*)SCRIPT,0x5a,0x1c00); memset((void*)ORDER,0x5a,20);
        for(i=0;i<9;i++) put32(state[i],0x12340000+i);
        for(i=0;i<4;i++) original[i]=read32((void*)(uintptr_t)restore[i]);
        Load_Replay();
#ifndef DD2_ORIGINAL_REPLAY
        for(i=0;i<4;i++) require(read32((void*)(uintptr_t)restore[i])==original[i],"restore frontend configuration after playback");
#endif
    } else Save_Replay();
    /* Original executions must reach the hardware breakpoint, never return
     * through an uninitialized file browser or replay renderer. */
#ifdef DD2_ORIGINAL_REPLAY
    require(0,"original escaped the required external-call breakpoint");
#endif
    require(observed==1,"exactly one actual external API call observed");
    require(fclose(output)==0,"close metadata checkpoint"); return 0;
}
