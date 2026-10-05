/* Explicit post-result score inputs; unchanged original x86 or engine C.
 * No race execution, save/load, UI, clocks, RNG or audio/video claim. */
#include "pe_fixture.h"
#ifdef DD2_ORIGINAL_MULTI_POINTS
#define Copy_Human ((void (*)(int,int))(uintptr_t)0x44c5d8)
#define Add_Computer ((void (*)(int))(uintptr_t)0x44c64c)
#define Update ((void (*)(void))(uintptr_t)0x44c6c8)
#define Aggregate ((void (*)(int,int))(uintptr_t)0x44c6f8)
#define Sort_Race ((void (*)(void))(uintptr_t)0x44c7a0)
#define Sort_League ((void (*)(void))(uintptr_t)0x44c768)
#else
void FUN_0044c5d8(int,int), Add_Computer_Info(int), Update_League_Info(void);
void FUN_0044c6f8(int,int), Sort_RacePos(void), Sort_MultiLeague(void);
#define Copy_Human FUN_0044c5d8
#define Add_Computer Add_Computer_Info
#define Update Update_League_Info
#define Aggregate FUN_0044c6f8
#define Sort_Race Sort_RacePos
#define Sort_League Sort_MultiLeague
#endif
static const unsigned starts[]={0x93de80,0x795c20,0x467640};
static const unsigned lengths[]={0x600,0x200,0x40};
static unsigned total;
static void snapshot(FILE *out) {
    unsigned r;
    for(r=0;r<3;r++)
        require(fwrite((void*)(uintptr_t)starts[r],1,lengths[r],out)==lengths[r],
                "complete score regions and guards");
}
static void run(FILE *out,unsigned mode,unsigned a,unsigned b,unsigned rotation,unsigned shape) {
    static const unsigned scores[]={0,1,75,999,32760,65500,5,33};
    unsigned r,i,league,cars=mode==4?a:20,desc[]={mode,a,b,rotation,shape},s;
    for(r=0;r<3;r++) for(i=0;i<lengths[r];i++)
        *(unsigned char*)(uintptr_t)(starts[r]+i)=(unsigned char)(i*17+r*31+rotation*13);
    put32(0x46765c,cars);
    for(i=0;i<20;i++) {
        league=0x93dee0+i*54;s=scores[(i+shape)%8];
        snprintf((char*)(uintptr_t)league,16,"Driver%02u",i);
        /* Both original insertion sorts use a signed -1 sentinel. Negative
         * keys run past it in the original too, so they are not valid sort
         * inputs. Transfer/add/average still exercise full WORD boundaries. */
        put16(league+16,mode==5?scores[(i+shape+rotation)%8]%32768:
                                     scores[(i+shape+rotation)%8]);
        put16(league+18,0);put16(league+20,(i*7+rotation)%20);
        put16(league+22,s);put16(league+24,1+(i+rotation)%20);
        put16(league+26,1+(i+rotation)%20);
        put16(league+28,mode==4?s%1000:s);
        put16(0x795c42+i*20,1+(i+rotation)%20);
        put16(0x795c44+i*20,1+(i+rotation)%20);
        put16(0x795c46+i*20,s);
    }
    require(fwrite(desc,4,5,out)==5,"score descriptor");snapshot(out);
    if(mode==0)Copy_Human(a,b);else if(mode==1)Add_Computer(a);
    else if(mode==2)Update();else if(mode==3)Aggregate(a,b);
    else if(mode==4)Sort_Race();else Sort_League();
    snapshot(out);total++;
}
int main(int argc,char **argv) {
    unsigned a,b,rotation,shape,h,sim;FILE *out;
    static const unsigned humans[]={1,2,5,10,20},cars[]={1,2,8,20};
    require(argc==3,"original executable and output required");map_original(argv[1]);
    out=fopen(argv[2],"wb");require(out!=NULL,"open score output");
    for(rotation=0;rotation<8;rotation++) for(shape=0;shape<8;shape++) {
        for(a=0;a<20;a++) for(b=1;b<=2&&a+b<=20;b++)run(out,0,a,b,rotation,shape);
        for(a=0;a<=20;a++)run(out,1,a,0,rotation,shape);
        run(out,2,0,0,rotation,shape);
        for(h=0;h<5;h++) for(sim=1;sim<=2;sim++)
            if(humans[h]%sim==0)run(out,3,humans[h],humans[h]/sim,rotation,shape);
        for(a=0;a<4;a++)run(out,4,cars[a],0,rotation,shape);
        run(out,5,0,0,rotation,shape);
    }
    require(fclose(out)==0,"close score output");printf("CASES=%u\n",total);return 0;
}
