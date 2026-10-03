/* Explicit valid standings: original machine code or actual extracted engine C.
 * No original code writes, external API substitutes or whole-game claims. */
#include "pe_fixture.h"
#ifdef DD2_ORIGINAL_SEASON
#define Promote ((void (*)(void))(uintptr_t)0x44c884)
#define Reset ((void (*)(void))(uintptr_t)0x44c498)
#define Sort ((void (*)(void))(uintptr_t)0x44c4b0)
#define End ((unsigned (*)(void))(uintptr_t)0x44c394)
#else
void Promote_And_Relegate(void), Reset_League_Info(void), Sort_Leagues(void);
unsigned Do_End_Of_Season_Stuff(void);
#define Promote Promote_And_Relegate
#define Reset Reset_League_Info
#define Sort Sort_Leagues
#define End Do_End_Of_Season_Stuff
#endif
static const unsigned starts[]={0x93de80,0x467400,0x467658,0x4682ec};
static const unsigned lengths[]={0x1c00,16,12,16};
static void snapshot(FILE *out) {
    unsigned i;
    for(i=0;i<4;i++)
        require(fwrite((void*)(uintptr_t)starts[i],1,lengths[i],out)==lengths[i],"targeted season regions");
}
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
int main(int argc,char **argv) {
    const unsigned strides[]={1,3,7,9,11,13,17,19};
    unsigned mode,rotation,stride,shape,stats,unlocks,result,description[6];FILE *out;
    require(argc==3,"original executable and output required");map_original(argv[1]);
    out=fopen(argv[2],"wb");require(out!=NULL,"open season output");
    for(mode=0;mode<4;mode++) for(rotation=0;rotation<20;rotation++)
    for(stride=0;stride<8;stride++) for(shape=0;shape<3;shape++)
    for(stats=0;stats<(mode==3?5:1);stats++) for(unlocks=0;unlocks<(mode==3?2:1);unlocks++) {
        initialize(rotation,strides[stride],shape,stats,unlocks);
        description[0]=mode;description[1]=rotation;description[2]=strides[stride];
        description[3]=shape;description[4]=stats;description[5]=unlocks;
        require(fwrite(description,4,6,out)==6,"season input descriptor");snapshot(out);
        result=0;
        if(mode==0)Promote();else if(mode==1)Reset();else if(mode==2)Sort();else result=End();
        require(fwrite(&result,4,1,out)==1,"season return");snapshot(out);
    }
    require(fclose(out)==0,"close season output");return 0;
}
