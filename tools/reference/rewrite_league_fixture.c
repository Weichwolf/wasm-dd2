/* Compact functional reference for the handwritten rewrite league.
 * Executes unmodified original x86 only; no patched engine or game-code writes.
 * Outputs little-endian 6*u32 descriptor, u32 standing, 20*3*u16 league fields. */
#include "pe_fixture.h"
#define Promote ((void (*)(void))(uintptr_t)0x44c884)
#define Reset ((void (*)(void))(uintptr_t)0x44c498)
#define Sort ((void (*)(void))(uintptr_t)0x44c4b0)
#define End ((unsigned (*)(void))(uintptr_t)0x44c394)
#define Initial ((void (*)(void))(uintptr_t)0x44c430)
#define Standing ((unsigned (*)(void))(uintptr_t)0x44c92c)
static const unsigned starts[]={0x93de80,0x467400,0x467658,0x4682ec};
static const unsigned lengths[]={0x1c00,16,12,16};
static void initialize(unsigned rotation,unsigned stride,unsigned shape,unsigned stats,unsigned unlocks) {
    unsigned region,i,slot,league,rank,human;
    for(region=0;region<4;region++) for(i=0;i<lengths[region];i++)
        *(unsigned char*)(uintptr_t)(starts[region]+i)=(unsigned char)(i*17+rotation*13+region*23);
    for(i=0;i<20;i++) {
        slot=(rotation+i*stride)%20;league=slot/5;rank=slot%5;
        snprintf((char*)(uintptr_t)(0x93dee0+i*54),16,"Driver%02u",i);
        put16(0x93def0+i*54,shape==0?0:shape==1?(4-rank)*100:1000-i*31);
        put16(0x93def2+i*54,league);put16(0x93def4+i*54,rank);
        put16(0x93def6+i*54,0x1234+i);
    }
    human=rotation/5;
    put32(0x93dec0,3-human);put32(0x93dec8,5);
    put32(0x46765c,20);put32(0x467404,unlocks?7:4);put32(0x467408,unlocks?4:1);
    put32(0x4682f0,stats);put32(0x4682f4,100+rotation);
}
static void write_record(FILE *out,const unsigned *description,unsigned standing,const uint16_t *league) {
        require(fwrite(description,4,6,out)==6,"case descriptor");
        require(fwrite(&standing,4,1,out)==1,"standing");
        require(fwrite(league,2,60,out)==60,"league fields");
}
int main(int argc,char **argv) {
    const unsigned strides[]={1,3,7,9,11,13,17,19};
    unsigned mode,rotation,stride,shape,stats,unlocks,standing,description[6],i;
    uint16_t league[60]; FILE *out;
    require(argc==3,"original executable and output required");map_original(argv[1]);
    out=fopen(argv[2],"wb");require(out!=NULL,"open compact league output");
    for(mode=0;mode<4;mode++) for(rotation=0;rotation<20;rotation++)
    for(stride=0;stride<8;stride++) for(shape=0;shape<3;shape++)
    for(stats=0;stats<(mode==3?5:1);stats++) for(unlocks=0;unlocks<(mode==3?2:1);unlocks++) {
        initialize(rotation,strides[stride],shape,stats,unlocks);
        description[0]=mode;description[1]=rotation;description[2]=strides[stride];
        description[3]=shape;description[4]=stats;description[5]=unlocks;
        standing=Standing();
        if(mode==0)Promote();else if(mode==1)Reset();else if(mode==2)Sort();
        else require(End()==(standing==4?1u:standing==5?0xffffffffu:0u),"season end return");
        for(i=0;i<20;i++) {
            league[i*3]=read16((void*)(uintptr_t)(0x93def0+i*54));
            league[i*3+1]=read16((void*)(uintptr_t)(0x93def2+i*54));
            league[i*3+2]=read16((void*)(uintptr_t)(0x93def4+i*54));
        }
        write_record(out,description,standing,league);
    }
    initialize(0,1,0,0,0);standing=Standing();Initial();
    description[0]=4;description[1]=0;description[2]=1;description[3]=0;description[4]=0;description[5]=0;
    for(i=0;i<20;i++) {
        league[i*3]=read16((void*)(uintptr_t)(0x93def0+i*54));
        league[i*3+1]=read16((void*)(uintptr_t)(0x93def2+i*54));
        league[i*3+2]=read16((void*)(uintptr_t)(0x93def4+i*54));
    }
    write_record(out,description,standing,league);
    require(fclose(out)==0,"close compact league output");return 0;
}
