/* Explicit card inputs; actual original x86 or actual reconstructed C.
 * Original executions stop before external seek or at the fixture return
 * marker. No original code/register writes, no full application claim. */
#include "pe_fixture.h"

#define CARD 0x754460u
#define TABLE 0x774464u
#define SOURCE 0x93a490u
#define TARGET 0x94c000u
#define NAME 0x94f000u
#define CARD_WINDOW (16u+0x20000u+4u+15u*36u+16u)
#ifdef DD2_ORIGINAL_CARD
#define SaveCardFile ((unsigned (*)(char*,unsigned,void*))(uintptr_t)0x42335c)
#define DeleteFileMC ((unsigned (*)(char*))(uintptr_t)0x423428)
#define LoadCardFiles ((void (*)(void))(uintptr_t)0x4234c0)
#define LoadCardFile ((unsigned (*)(char*,unsigned,void*))(uintptr_t)0x423594)
#define DupFileCheck ((unsigned (*)(unsigned,char*,int))(uintptr_t)0x423620)
#define FirstSavedGame ((unsigned (*)(void))(uintptr_t)0x4235f0)
#else
unsigned SaveCardFile(char*,unsigned,void*);
unsigned DeleteFileMC(char*);
void LoadCardFiles(void);
unsigned LoadCardFile(char*,unsigned,void*);
unsigned DupFileCheck(unsigned,char*,int);
unsigned FirstSavedGame(void);
#endif

/* operation: save=0, delete=1, load=2, list=3, duplicate=4, first-config=5.
 * Fields: operation, occupied mask, requested name index, excluded slot,
 * config magic slot, file handle, optional reserved-flag slot. */
static const int cases[][7]={
 {0,0,-1,-1,-1,1,-1}, {0,0x7f7f,-1,-1,-1,1,-1}, {0,0x3fff,-1,-1,-1,1,-1},
 {0,0x7fff,-1,-1,-1,1,-1}, {0,0,-1,-1,-1,0,-1},
 {1,0x7fff,0,-1,-1,1,-1}, {1,0x7fff,7,-1,-1,1,-1}, {1,0x7fff,14,-1,-1,1,-1},
 {1,0x7fff,-1,-1,-1,1,-1}, {1,0x7fff,7,-1,-1,0,-1},
 {2,0x7fff,0,-1,-1,1,-1}, {2,0x7fff,7,-1,-1,1,-1}, {2,0x7fff,14,-1,-1,1,-1},
 {2,0x7fff,-1,-1,-1,1,-1}, {2,0x7f7f,7,-1,-1,1,-1},
 {3,0,-1,-1,-1,1,-1}, {3,0x4105,-1,-1,-1,1,-1}, {3,0x7fff,-1,-1,-1,1,-1},
 {4,0x7fff,7,0,-1,1,-1}, {4,0x7fff,7,7,-1,1,-1}, {4,0x7fff,-1,0,-1,1,-1},
 {5,0x7fff,-1,-1,7,1,-1}, {5,0x7fff,-1,-1,-1,1,-1},
 {0,0x7fff,-1,-1,-1,1,7}, {3,0x7fff,-1,-1,-1,1,7},
 {5,0x4081,-1,-1,7,1,-1}
};
static FILE *output;
static void capture(unsigned kind,unsigned result) {
    require(fwrite((void*)(CARD-16),1,CARD_WINDOW,output)==CARD_WINDOW,"complete card/table/guards");
    require(fwrite((void*)SOURCE,1,0x2000,output)==0x2000,"source block");
    require(fwrite((void*)TARGET,1,0x2000,output)==0x2000,"loaded block");
    require(fwrite(&kind,4,1,output)==1 && fwrite(&result,4,1,output)==1,"card operation outcome");
    require(fclose(output)==0,"close card checkpoint");
}
__attribute__((noinline)) void fixture_done(unsigned result) {
#ifdef DD2_ORIGINAL_CARD
    require(0,"original return marker hardware breakpoint missed");
#else
    capture(0,result);
#endif
}
#ifndef DD2_ORIGINAL_CARD
int FUN_0045623b(int *file,int offset,int whence) {
    require((uintptr_t)file==1 && offset==0 && whence==0,"actual card seek arguments");
    capture(1,0); exit(0);
}
#endif

int main(int argc,char **argv) {
    unsigned index,i,j,result=0; const int *v;
    require(argc==4,"executable, checkpoint and case index required");
    index=(unsigned)strtoul(argv[3],NULL,10);
    require(index<sizeof cases/sizeof cases[0],"supported card case");
    v=cases[index]; map_original(argv[1]);
    output=fopen(argv[2],"wb"); require(output!=NULL,"open card checkpoint");
    memset((void*)(CARD-16),0xa5,CARD_WINDOW);
    for(i=0;i<15;i++) {
        put32(CARD+i*0x200,(v[1]&(1<<i))?1:0);
        if((int)i==v[6]) put32(CARD+i*0x200,2);
        sprintf((char*)(CARD+i*0x200+4),"SLOT%02u",i);
        for(j=0;j<0x2000;j++) *((unsigned char*)(CARD+0x2000+i*0x2000+j))=(unsigned char)(i*17+j*37);
        put16(CARD+0x2000+i*0x2000,(int)i==v[4]?0x1010:0x2020);
    }
    put32(0x774460,(unsigned)v[5]);
    for(i=0;i<0x2000;i++) *((unsigned char*)SOURCE+i)=(unsigned char)(i*53+index);
    memset((void*)TARGET,0x5a,0x2000);
    if(v[2]>=0) sprintf((char*)NAME,"SLOT%02d",v[2]); else strcpy((char*)NAME,"NEW");
    printf("CASE %u OP %d\n",index,v[0]);
    switch(v[0]) {
    case 0: result=SaveCardFile((char*)NAME,0,(void*)SOURCE); break;
    case 1: result=DeleteFileMC((char*)NAME); break;
    case 2: result=LoadCardFile((char*)NAME,0,(void*)TARGET); break;
    case 3: LoadCardFiles(); break;
    case 4: result=DupFileCheck(0,(char*)NAME,v[3]); break;
    case 5: result=FirstSavedGame(); break;
    }
    fixture_done(result); return 0;
}
