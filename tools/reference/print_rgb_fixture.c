/* Complete Print record and glyph-packet comparison against unchanged x86.
 * Fonts and text inputs are explicit; this does not render a frontend frame. */
#include "pe_fixture.h"
#ifdef DD2_ORIGINAL_PRINT_RGB
#define Text ((unsigned (*)(unsigned short,int))(uintptr_t)0x421404)
#else
unsigned Print(unsigned short,int);
#define Text Print
#endif

static const unsigned char colors[][3] = {
    {0,0,0},{255,255,255},{17,34,51},{85,102,119},{128,0,255},{255,128,0}
};
static const char *styles[] = {
    "AB","%C17,34,51/AB","A%C85,102,119/B","%C-1,256,511/AB"
};
static const char *alignments[] = {"%JL","%JC","%JR"};

static void initialize(unsigned font, unsigned shader, unsigned color,
                       unsigned style, unsigned alignment) {
    unsigned i,f;
    char *text=(char *)0x920000;
    memset((void *)0x900000,0,512);
    memset((void *)0x901000,0,96);
    memset((void *)0x902000,0x99,704);
    memset((void *)0x906000,0x99,704);
    memset((void *)0x911000,0xa5,64);
    memset((void *)0x920000,0,96);
    put32(0x754408,0x900000);
    put32(0x754400,0x901000);
    put32(0x754404,4);
    put32(0x7543f8,32);
    put32(0x75440c,0x911000);
    put32(0x7543a0,0x902000);
    put32(0x7543a4,0x906000);
    put32(0x463018,0);
    put32(0x46301c,0);
    put32(0x463020,0);
    put32(0x462fec,color&1);
    put32(0x462ff4,640);
    put32(0x462ff8,480);
    put32(0x900008,-40);
    put32(0x90000c,-30);
    put32(0x900010,256);
    put32(0x900014,-40);
    put32(0x900018,-30);
    put32(0x90001c,256);
    *(unsigned char *)0x900050=(unsigned char)font;
    *(unsigned char *)0x900051=(unsigned char)shader;
    put32(0x90005c,0x99000000u | colors[color][0] |
          (unsigned)colors[color][1]<<8 | (unsigned)colors[color][2]<<16);
    for (f=0; f<4; f++) {
        unsigned char *metadata=(unsigned char *)(uintptr_t)(0x911000+f*8);
        put32(0x7543d0+f*4,0x912000+f*1024);
        metadata[0]=(unsigned char)(255-f*47);
        metadata[1]=(unsigned char)(128+31*f);
        put16(0x911002+f*8,0x2345+f*1024);
        put16(0x911004+f*8,0x1100+f*64);
        put16(0x911006+f*8,(unsigned short)((int)f-2));
        for (i=0; i<256; i++) {
            unsigned char *glyph=(unsigned char *)(uintptr_t)(0x912000+f*1024+i*4);
            glyph[0]=(unsigned char)(3*i+f);
            glyph[1]=(unsigned char)(5*i+f);
            glyph[2]=(unsigned char)(8+i%3);
            glyph[3]=(unsigned char)(10+i%5);
        }
    }
    strcpy(text,alignments[alignment]);
    strcat(text,styles[style]);
}

static void snapshot(FILE *out) {
    const unsigned starts[] = {0x463018,0x462fec,0x900000,0x901000,
                              0x902000,0x906000,0x911000,0x912000,0x920000};
    const unsigned sizes[] = {12,16,512,96,704,704,64,4096,96};
    unsigned i;
    for (i=0; i<sizeof(starts)/sizeof(starts[0]); i++)
        require(fwrite((void *)(uintptr_t)starts[i],1,sizes[i],out)==sizes[i],
                "complete text records, glyph packets and input guards");
}

int main(int argc, char **argv) {
    const unsigned shaders[] = {0,1,2,0xffffffff};
    unsigned font,shader,color,style,alignment,description[5],result,count=0;
    FILE *out;
    require(argc==3,"original executable and output required");
    map_original(argv[1]);
    out=fopen(argv[2],"wb");
    require(out!=NULL,"open Print RGB output");
    for (font=0; font<4; font++)
    for (shader=0; shader<4; shader++)
    for (color=0; color<6; color++)
    for (style=0; style<4; style++)
    for (alignment=0; alignment<3; alignment++) {
        initialize(font,shaders[shader],color,style,alignment);
        description[0]=font;
        description[1]=shaders[shader];
        description[2]=color;
        description[3]=style;
        description[4]=alignment;
        require(fwrite(description,4,5,out)==5,"explicit Print RGB input");
        snapshot(out);
        result=Text((unsigned short)(0x1200+color),0x920000);
        snapshot(out);
        require(fwrite(&result,4,1,out)==1,"actual Print return");
        count++;
    }
    require(count==1152,"all Print RGB cases executed");
    require(fclose(out)==0,"close Print RGB output");
    return 0;
}
