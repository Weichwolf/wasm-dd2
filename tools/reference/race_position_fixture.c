/* Original x86 and actual engine C receive identical, explicit race inputs.
 * Component scope: ranks, laps, timers and finish flags, including region guards.
 * This neither drives a complete race nor accepts chronological audio/video. */
#include "pe_fixture.h"
#ifdef DD2_ORIGINAL_POSITIONS
#define End_Init ((void (*)(void))(uintptr_t)0x443970)
#define Position_Init ((void (*)(void))(uintptr_t)0x443c40)
#define Track_Positions ((void (*)(void))(uintptr_t)0x443990)
#define Race_Positions ((void (*)(void))(uintptr_t)0x443cbc)
#define Load_Records ((void (*)(void))(uintptr_t)0x44d810)
#else
void Init_End_Race(void), FUN_00443c40(void);
void Calc_Track_Positions(void), Get_Race_Positions(void);
void FUN_0044d810(void);
#define End_Init Init_End_Race
#define Position_Init FUN_00443c40
#define Track_Positions Calc_Track_Positions
#define Race_Positions Get_Race_Positions
#define Load_Records FUN_0044d810
#endif
static const unsigned starts[]={0x466dd0,0x4673f0,0x467658,0x792640,0x795c20,0x784290,0x936ff0,0x4680b0};
static const unsigned lengths[]={0xc8,0x60,0x20,0x25c0,0x220,0x10,0x10,0x260};
static unsigned char track_defaults[0xc8];
static void snapshot(FILE *out) {
    unsigned i;
    for(i=0;i<8;i++)
        require(fwrite((void*)(uintptr_t)starts[i],1,lengths[i],out)==lengths[i],"complete targeted race regions");
}
static void initialize(unsigned mode,unsigned level,unsigned cars,unsigned rotation,unsigned shape) {
    unsigned region,i,a,lap,progress,strips,first,laps;
    for(region=0;region<8;region++) for(i=0;i<lengths[region];i++)
        *(unsigned char*)(uintptr_t)(starts[region]+i)=(unsigned char)(i*17+rotation*13+region*23);
    /* Keep the actual original's packed track table and initial records. */
    memcpy((void*)0x466dd0,track_defaults,sizeof track_defaults);
    first=read16((void*)(0x466df0+level*6));
    strips=read16((void*)(0x466df2+level*6));
    laps=read16((void*)(0x466df4+level*6));
    put32(0x936ff4,level);put32(0x46765c,cars);
    put32(0x467660,cars<2?1:2);put32(0x4673f4,shape==8?1:shape==11?4:3);
    put32(0x466e90,shape==9?1:shape==10?25:0);
    put32(0x795df4,0);put32(0x795dd8,0);put32(0x795df8,0);
    put32(0x795ddc,1);put32(0x795de0,23);put32(0x795de4,1000);
    put32(0x795de8,1);put32(0x795dec,10);put32(0x795df0,500);
    put32(0x784298,-1);
    for(i=0;i<20;i++) {
        a=0x795c40+i*20;lap=1+(i+rotation)%3;
        progress=(i*13+rotation)%((strips&&strips<999)?strips:100);
        put16(a,shape%2?65+i:0); /* next car's real WORD speed is adjacent to finish */
        put16(a+2,1+(i+rotation)%cars);put16(a+4,21);put16(a+6,0);
        put16(a+8,lap);put16(a+10,progress);put16(a+12,lap);put16(a+14,progress);
        put16(a+16,0);put16(a+18,mode==0?i%2:0);
        put32(0x792ac6+i*434,0);put32(0x792a92+i*434,0);
        if(mode==3) {
            if(shape==0)put16(a+14,0); /* lap/strip ties resolve by driver index */
            if(shape==2 || (i==0&&(shape==3||shape==4||shape==8)))put16(a+12,laps+1);
            if(shape==4&&i==0){put16(a+18,1);put16(a+4,1);}
            if(shape==5&&i==0)put32(0x792ac6,1);
            if(shape==6&&i>0)put32(0x792ac6+i*434,1);
            if(shape==7){put32(0x792ac6+i*434,i%3==0);put32(0x792a92+i*434,i%3==1?5:0);}
            if(shape==11&&i==cars-1)put16(a+12,laps+1);
        }
        if(mode==2) {
            require(strips>1&&strips<=999,"original static road strip count");
            if(shape==1||(shape>=3&&shape<=6)) {
                put16(a+10,strips-1);put16(a+14,strips-1);progress=0;
                if(shape==3)put16(a+12,lap-1);
            } else if(shape==2) {
                put16(a+10,0);put16(a+14,0);progress=strips-1;
            } else {progress=(progress+1)%strips;}
            put32(0x7926ac+i*44,(first+progress)%strips);
        }
    }
    put32(0x792a76,shape==11?(uint32_t)-127:127);
    put32(0x795dd4,read16((void*)0x795c48));
    for(i=0;i<7;i++) {
        put32(0x467424+i*4,1+(i+rotation)%7);
        put16(0x4680ca+i*80,(shape+i)%100);
        put16(0x4680cc+i*80,(shape*7+i)%60);
        put16(0x4680ce + i*80,shape==0?0:shape==1?65535:1000+shape*6789+i);
    }
    if(mode==2) {
        if(shape==4){put32(0x466e3c+(level-1)*12,2);}
        if(shape==5||shape==6) {
            put32(0x466e3c+(level-1)*12,1);put32(0x466e40+(level-1)*12,23);
            put32(0x466e44+(level-1)*12,1000);
            /* Equal fractional record includes this function's timer increment. */
            if(shape==5)put32(0x466e44+(level-1)*12,1000+0x51e);
        }
        if(shape==7||shape==8){put32(0x795de4,65535);put32(0x795de0,59);}
        if(shape==8)put32(0x795ddc,99);
        if(shape==9)put32(0x784298,0);
        if(shape==10)put32(0x795df8,1);
    }
}
static void run_case(FILE *out,unsigned mode,unsigned level,unsigned cars,unsigned rotation,unsigned shape) {
    unsigned description[]={mode,level,cars,rotation,shape};
    initialize(mode,level,cars,rotation,shape);
    require(fwrite(description,4,5,out)==5,"race descriptor");snapshot(out);
    if(mode==0)End_Init();else if(mode==1)Position_Init();
    else if(mode==2)Track_Positions();else if(mode==3)Race_Positions();else Load_Records();
    snapshot(out);
}
int main(int argc,char **argv) {
    const unsigned counts[]={1,2,8,20};
    unsigned mode,level,c,rotation,shape;FILE *out;
    require(argc==3,"original executable and output required");map_original(argv[1]);
    memcpy(track_defaults,(void*)0x466dd0,sizeof track_defaults);
    out=fopen(argv[2],"wb");require(out!=NULL,"open race output");
    for(mode=0;mode<2;mode++)for(c=0;c<4;c++)for(rotation=0;rotation<counts[c];rotation++)
        run_case(out,mode,1,counts[c],rotation,0);
    for(level=1;level<=7;level++)for(rotation=0;rotation<20;rotation++)for(shape=0;shape<12;shape++)
        run_case(out,2,level,20,rotation,shape);
    for(level=1;level<=12;level++)for(c=0;c<4;c++)for(rotation=0;rotation<counts[c];rotation++)for(shape=0;shape<12;shape++)
        run_case(out,3,level,counts[c],rotation,shape);
    for(rotation=0;rotation<7;rotation++)for(shape=0;shape<8;shape++)
        run_case(out,4,1,20,rotation,shape);
    require(fclose(out)==0,"close race output");return 0;
}
