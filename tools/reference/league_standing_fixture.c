/* Classification only: explicit packed inputs, actual original x86 or engine C.
 * Full real championship UI is checked separately. No engine-code writes. */
#include "pe_fixture.h"
#ifdef DD2_ORIGINAL_STANDING
#define Standing ((unsigned (*)(void))(uintptr_t)0x44c92c)
#else
unsigned Check_League_Standing(void);
#define Standing Check_League_Standing
#endif
int main(int argc,char **argv) {
    const unsigned leagues[]={0,1,2,3,0x7fff,0xffff};
    const unsigned ranks[]={0,1,2,3,4,0x7fff,0xffff};
    const unsigned adjacent[]={0,1,0x1234,0x8000,0xffff};
    unsigned l,r,a,result; FILE *out;
    require(argc==3,"original executable and output required");
    map_original(argv[1]);out=fopen(argv[2],"wb");require(out!=NULL,"open classification output");
    for(l=0;l<6;l++) for(r=0;r<7;r++) for(a=0;a<5;a++) {
        put16(0x93def0,0x5a5a);put16(0x93def2,leagues[l]);
        put16(0x93def4,ranks[r]);put16(0x93def6,adjacent[a]);
        require(fwrite((void*)0x93def0,1,8,out)==8,"input guard words");
        result=Standing();require(fwrite(&result,1,4,out)==4,"classification");
        require(fwrite((void*)0x93def0,1,8,out)==8,"unchanged packed words");
    }
    require(fclose(out)==0,"close classification output");return 0;
}
