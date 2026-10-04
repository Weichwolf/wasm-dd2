/* Execute the two actual font call orders; compare duplicate metadata,
 * every copied glyph and adjacent guards. Opaque font page residue is excluded. */
#include "pe_fixture.h"
#ifdef DD2_ORIGINAL_DUPLICATE_FONT
#define Font ((void (*)(char*,int,unsigned short))(uintptr_t)0x4211a8)
#define Copy ((void (*)(int,int,char*))(uintptr_t)0x4211f4)
#else
void Setup_Font(char*,int,unsigned short);
void Duplicate_Font(int,int,char*);
#define Font Setup_Font
#define Copy Duplicate_Font
#endif

static char *names[] = {"FONT","FONT2","FONT3","CFONT","CFONT2","CFONT2A"};
static void initialize(unsigned page,unsigned pattern) {
    unsigned f,i;
    memset((void*)0x911000,0xa5,64);
    put32(0x75440c,0x911008);
    put32(0x74f1a0,0x920000); put32(0x74f1a4,6);
    for (f=0;f<6;f++) {
        unsigned a=0x920000+24*f, glyph=0x912000+448*f;
        memset((void*)(uintptr_t)a,0x99,24);
        put16(a,(unsigned short)(pattern*257+f*71));
        put16(a+2,(unsigned short)(((page+f)%32)*256+f*19));
        put16(a+4,12+f); put16(a+6,18+f);
        put16(a+8,0x1234);
        put16(a+10,(unsigned short)((pattern==3 ? 65535 : pattern*16)+f));
        strcpy((char*)(uintptr_t)(a+14),names[f]);
        put32(0x7543d0+f*4,glyph+32);
        memset((void*)(uintptr_t)glyph,0x99,448);
        for (i=0;i<384;i++)
            *(unsigned char*)(uintptr_t)(glyph+32+i)=(unsigned char)(i*13+f*37+pattern*61);
    }
}

static void snapshot(FILE *out) {
    unsigned f;
#define WRITE(a,n) require(fwrite((void*)(uintptr_t)(a),1,(n),out)==(n),"complete font fields, glyphs and guards")
    WRITE(0x911000,8);
    for (f=0;f<3;f++) { WRITE(0x911008+f*8,2); WRITE(0x91100c+f*8,4); }
    WRITE(0x911020,24); WRITE(0x911038,8);
    WRITE(0x912000,6*448); WRITE(0x920000,6*24);
    WRITE(0x7543d0,24); WRITE(0x74f1a0,8);
#undef WRITE
}

int main(int argc,char **argv) {
    unsigned order,page,pattern,description[3],count=0;
    FILE *out;
    require(argc==3,"original executable and output required");
    map_original(argv[1]); out=fopen(argv[2],"wb"); require(out!=NULL,"open font output");
    for (order=0;order<2;order++) for (page=0;page<32;page++) for (pattern=0;pattern<4;pattern++) {
        initialize(page,pattern);
        description[0]=order;description[1]=page;description[2]=pattern;
        require(fwrite(description,4,3,out)==3,"explicit font inputs"); snapshot(out);
        /* No intervening calls: unchanged original Setup_Font's literal zero
         * argument occupies Duplicate_Font descriptor+9 at equal caller ESP. */
        if (order==0) {
            Font(names[0],0,0); Font(names[1],1,0); Font(names[2],2,1);
            Copy(0,3,names[3]); Copy(1,4,names[4]); Copy(1,5,names[5]);
        } else {
            Font(names[0],0,0); Copy(0,3,names[3]);
            Font(names[1],1,0); Copy(1,4,names[4]); Copy(1,5,names[5]);
            Font(names[2],2,1);
        }
        snapshot(out);count++;
    }
    require(count==256,"all font cases"); require(fclose(out)==0,"close font output");return 0;
}
