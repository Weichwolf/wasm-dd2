/* Actual original/C record update with explicit component inputs.
 * The name-entry callback's result is controlled; live UI is not exercised.
 * No original instructions are changed. Compare complete regions and guards. */
#include "pe_fixture.h"
unsigned name_response, name_calls, case_id;
int Enter_Driver_Names(int driver,int lap) {
    require(driver==0 && lap==1,"record name callback arguments");
    name_calls++;
    return name_response;
}
#ifdef DD2_ORIGINAL_RECORD_UPDATE
#define Update ((void (*)(int,int))(uintptr_t)0x44d85c)
#define Pack ((void (*)(uint16_t*,uint16_t))(uintptr_t)0x44ae0c)
#else
void Update_Jimmy_Spunk_Times(int,int);
void FUN_0044ae0c(uint16_t*,uint16_t);
#define Update Update_Jimmy_Spunk_Times
#define Pack FUN_0044ae0c
#endif
static const unsigned starts[]={0x466e20,0x4680a0,0x93e790,0x93a470};
static const unsigned sizes[]={0x90,0x270,0x40,0x19c0};
static void snapshot(FILE *out) {
    unsigned i;
    for(i=0;i<4;i++)require(fwrite((void*)(uintptr_t)starts[i],1,sizes[i],out)==sizes[i],"full records, serialized payload and guards");
}
static void initialize(unsigned saved,unsigned level,unsigned shape,unsigned response,unsigned name) {
    const unsigned lengths[]={1,2,8,9};
    unsigned r,i,j,a;
    for(r=0;r<4;r++)for(i=0;i<sizes[r];i++)
        *(unsigned char*)(uintptr_t)(starts[r]+i)=(unsigned char)(i*17+r*23+saved*13);
    for(i=0;i<7;i++)for(j=0;j<5;j++) {
        a=0x4680c0+i*80+j*16;
        snprintf((char*)(uintptr_t)a,10,"T%u_D%u",i,j);
        put16(a+10,2);put16(a+12,23);put16(a+14,40000);
    }
    for(i=0;i<7;i++) {
        put32(0x466e3c+i*12,2);put32(0x466e40+i*12,23);put32(0x466e44+i*12,40000);
    }
    if(shape==1)put32(0x466e44+level*12,40001);
    if(shape==2)put32(0x466e44+level*12,39999);
    if(shape==3){put32(0x466e40+level*12,24);put32(0x466e44+level*12,0);}
    if(shape==4){put32(0x466e40+level*12,22);put32(0x466e44+level*12,65535);}
    if(shape==5){put32(0x466e3c+level*12,3);put32(0x466e40+level*12,0);put32(0x466e44+level*12,0);}
    if(shape==6){put32(0x466e3c+level*12,1);put32(0x466e40+level*12,59);put32(0x466e44+level*12,65535);}
    if(shape==7||shape==8){put32(0x466e3c+level*12,0);put32(0x466e40+level*12,0);put32(0x466e44+level*12,shape==8?1:0);}
    if(shape==8){put16(0x4680ca+saved*80,0);put16(0x4680cc+saved*80,0);put16(0x4680ce + saved*80,0);}
    memcpy((void*)0x93e7b0,"ABCDEFGHI",lengths[name]);
    *(char*)(0x93e7b0+lengths[name])=0;
    name_response=response;name_calls=0;
}
int main(int argc,char **argv) {
    unsigned saved,level,shape,response,name,description[5];
    unsigned char code_before[0x168+32+0x22c],code_after[0x168+32+0x22c];
    FILE *out,*code;
    require(argc==4,"original exe, record output and code output required");map_original(argv[1]);
    memcpy(code_before,(void*)0x44d85c,0x168);memcpy(code_before+0x168,(void*)0x4522b0,32);
    memcpy(code_before+0x168+32,(void*)0x44ae0c,0x22c);
    out=fopen(argv[2],"wb");require(out!=NULL,"open record output");case_id=0;
    for(saved=0;saved<7;saved++)for(level=0;level<7;level++)for(shape=0;shape<9;shape++)
    for(response=0;response<2;response++)for(name=0;name<4;name++) {
        initialize(saved,level,shape,response,name);
        description[0]=saved;description[1]=level;description[2]=shape;description[3]=response;description[4]=name;
        require(fwrite(description,4,5,out)==5,"explicit record input");snapshot(out);
        Update(saved,level);
        Pack((uint16_t*)0x93a490,0x1010);
        require(fwrite(&name_calls,4,1,out)==1,"callback count");snapshot(out);case_id++;
    }
    require(case_id==3528,"all record update cases");require(fclose(out)==0,"close record output");
    memcpy(code_after,(void*)0x44d85c,0x168);memcpy(code_after+0x168,(void*)0x4522b0,32);
    memcpy(code_after+0x168+32,(void*)0x44ae0c,0x22c);
    require(memcmp(code_before,code_after,sizeof code_before)==0,"original instructions unchanged");
    code=fopen(argv[3],"wb");require(code!=NULL,"code evidence output");
    require(fwrite(code_after,1,sizeof code_after,code)==sizeof code_after,"unchanged original bytes");
    require(fclose(code)==0,"close code evidence");return 0;
}
