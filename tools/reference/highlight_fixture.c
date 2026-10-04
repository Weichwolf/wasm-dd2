/* Pit-camera polygon highlights: literal original x86 or extracted engine C.
 * Explicit valid car-model groups; no whole-game or chronological A/V claim. */
#include "pe_fixture.h"
#ifdef DD2_ORIGINAL_HIGHLIGHT
#define Highlight ((void (*)(int,unsigned,unsigned char))(uintptr_t)0x43b6f0)
#else
void Highlight_Area(int,unsigned,unsigned char);
#define Highlight Highlight_Area
#endif
static const unsigned starts[]={0x781418,0x900000,0x900f00};
static const unsigned sizes[]={28,64,4608};
static const unsigned counts[]={8,16,8,6,10,74,25};
static const unsigned types[]={0,4,1,9,13,25,29};
static const unsigned strides[]={16,16,16,20,20,24,28};
static void snapshot(FILE *out) {
    unsigned r;
    for(r=0;r<3;r++)
        require(fwrite((void*)(uintptr_t)starts[r],1,sizes[r],out)==sizes[r],"highlight model and guards");
}
static void initialize(void) {
    unsigned r,i,poly=0x901000;
    for(r=0;r<3;r++) for(i=0;i<sizes[r];i++)
        *(unsigned char*)(uintptr_t)(starts[r]+i)=(unsigned char)(17*i+23*r);
    put32(0x78141c,0x900000);put32(0x900028,poly);
    for(i=0;i<7;i++) {
        put16(poly,counts[i]);
        *(unsigned char*)(uintptr_t)(poly+2)=types[i];
        *(unsigned char*)(uintptr_t)(poly+3)=strides[i];
        poly+=4+counts[i]*strides[i];
    }
    memset((void*)(uintptr_t)poly,0,4);
}
int main(int argc,char **argv) {
    unsigned area,c,description[2];const unsigned colors[]={0,128,245,255};FILE *out;
    require(argc==3,"original executable and output required");map_original(argv[1]);
    out=fopen(argv[2],"wb");require(out!=NULL,"open highlight output");
    for(area=0;area<10;area++) for(c=0;c<4;c++) {
        initialize();description[0]=area;description[1]=colors[c];
        require(fwrite(description,4,2,out)==2,"highlight input descriptor");snapshot(out);
        Highlight(0,area,(unsigned char)colors[c]);snapshot(out);
    }
    require(fclose(out)==0,"close highlight output");return 0;
}
