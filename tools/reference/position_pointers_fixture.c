/* Actual top-three pointer packets, ordering table and GTE state versus x86.
 * Explicit isolated inputs; this does not render a complete racing frame. */
#include "pe_fixture.h"
#ifdef DD2_ORIGINAL_POSITION_POINTERS
#define Pointers ((void (*)(void))(uintptr_t)0x4300ac)
#else
void Display_Position_Pointers(void);
#define Pointers Display_Position_Pointers
#endif

static void initialize(unsigned car,unsigned buffer,unsigned rank,unsigned scenario) {
    const int depths[]={1000,256,257,32767,32768,-32768};
    unsigned i;
    int depth=scenario<6?depths[scenario]:1000;
    memset((void *)0x462fa4,0,30);
    put16(0x462fa4,4096);put16(0x462fac,4096);put16(0x462fb4,4096);
    put32(0x462fec,buffer);
    put32(0x463eec,scenario==10?car:(car+1)%20);
    put32(0x46765c,scenario==7?1:20);
    put32(0x936ff4,scenario==8?8:7);
    memset((void *)0x465804,0,40);
    if(scenario==9)put32(0x465820,1);
    put32(0x784298,scenario==6?1:0);
    memset((void *)0x795c40,0,400);
    memset((void *)0x78a744,0,20*0x27c);
    memset((void *)0x74c4e0,0,576);
    put32(0x74c6d4,256);
    memset((void *)0x4604b6,0x91,16);
    put32(0x4604c2,32768);
    memset((void *)0x784178,0x9a,288);
    memset((void *)0x900000,0xa5,256);
    put32(0x754380,0x900000);
    put32(0x90008a,0x910020);
    for(i=0;i<32832/4;i++)put32(0x910000+4*i,0x9a000000+4*i);
    if(scenario==12) {
        /* Quarter-turn and signed WORD subtraction across both boundaries. */
        put16(0x462fa4,0);put16(0x462fa6,-4096);
        put16(0x462faa,4096);put16(0x462fac,0);
        put32(0x462fb6,32760);put32(0x462fba,-32760);
    }
    for(i=0;i<20;i++) {
        unsigned active=i==car || scenario==13;
        unsigned r=scenario==13?(i+car)%20+1:rank;
        if(!active)continue;
        put32(0x795c40+i*20,0x1234u|(r<<16));
        put32(0x78a744+i*0x27c,scenario==12?-32760:(int)i*3-29);
        put32(0x78a748+i*0x27c,scenario==12?32760:(int)i*2-19);
        put32(0x78a74c+i*0x27c,depth);
        put32(0x78a798+i*0x27c,scenario==11?0:1);
    }
}

static void snapshot(FILE *out) {
    const unsigned starts[]={0x462fa4,0x462fec,0x463eec,0x46765c,0x936ff4,
        0x465804,0x784298,0x795c40,0x78a744,0x74c4e0,0x4604b6,
        0x784178,0x900000,0x910000};
    const unsigned sizes[]={30,4,4,4,4,40,4,400,12720,576,16,288,256,32832};
    unsigned i;
    for(i=0;i<sizeof(starts)/sizeof(starts[0]);i++)
        require(fwrite((void *)(uintptr_t)starts[i],1,sizes[i],out)==sizes[i],
                "complete packets, ordering table, GTE and input guards");
}

int main(int argc,char **argv) {
    unsigned car,buffer,rank,scenario,description[4],count=0;
    FILE *out;
    require(argc==3,"original executable and output required");
    map_original(argv[1]);out=fopen(argv[2],"wb");
    require(out!=NULL,"open position pointer output");
    for(car=0;car<20;car++)
    for(buffer=0;buffer<2;buffer++)
    for(rank=0;rank<5;rank++)
    for(scenario=0;scenario<14;scenario++) {
        initialize(car,buffer,rank,scenario);
        description[0]=car;description[1]=buffer;
        description[2]=rank;description[3]=scenario;
        require(fwrite(description,4,4,out)==4,"explicit pointer inputs");
        snapshot(out);Pointers();snapshot(out);count++;
    }
    require(count==2800,"all position pointer cases executed");
    require(fclose(out)==0,"close position pointer output");
    return 0;
}
