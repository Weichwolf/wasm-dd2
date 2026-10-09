/* Execute original sprite/class selection and high-detail car paint/number
 * remapping. Only copied fixture data is relocated; original code and files
 * remain unchanged. Opponents use the high-detail mesh for this component
 * comparison, not the original runtime's low/medium detail selection. */
#include "pe_fixture.h"

#define SpriteInfo ((void (*)(void*))(uintptr_t)0x4166c0)
#define Sprite ((void (*)(int,char*,short*))(uintptr_t)0x41673c)
#define CarCluts ((void (*)(void))(uintptr_t)0x43a7b4)
#define CarDoors ((void (*)(void))(uintptr_t)0x43b064)
#define CarPaint ((void (*)(int,int,int))(uintptr_t)0x43b39c)

static unsigned sprites_size,mesh_size,definitions_size;
static unsigned load(const char *path,unsigned address,unsigned capacity) {
    FILE *f=fopen(path,"rb");long size;
    require(f!=NULL,"open bounded asset fixture");
    require(fseek(f,0,SEEK_END)==0,"seek fixture");size=ftell(f);
    require(size>=4 && (unsigned long)size<=capacity,"bounded fixture extent");
    require(fseek(f,0,SEEK_SET)==0,"rewind fixture");
    require(fread((void*)(uintptr_t)address,1,(size_t)size,f)==(size_t)size,"read fixture");
    require(fclose(f)==0,"close fixture");return (unsigned)size;
}
static void word(FILE *out,unsigned value) {
    require(fwrite(&value,4,1,out)==1,"write complete comparison word");
}
static unsigned raw_palette(const unsigned char *record,unsigned opcode) {
    unsigned family=opcode/8,within=opcode%8,corners=within>=4 ? 4 : 3;
    unsigned lit=opcode<32 && (opcode&1),gouraud=family==2 || family==3;
    unsigned colors=gouraud && !lit ? corners : 1;
    return read16(record+4+4*colors+2);
}
static const unsigned char *definition(const unsigned char *record,unsigned opcode) {
    unsigned family=opcode/8,within=opcode%8,corners=within>=4 ? 4 : 3;
    unsigned lit=opcode<32 && (opcode&1),gouraud=family==2 || family==3;
    unsigned colors=gouraud && !lit ? corners : 1;
    unsigned index=read16(record+4+4*colors);
    require(index<read32((void*)0x8f0000),"texture definition index");
    return (void*)(uintptr_t)(0x8f0004+index*12);
}
static void primitives(unsigned base) {
    unsigned offset=0,cursor=read32((void*)0x8e0028)-0x8e0000;
    while (cursor+4<=mesh_size) {
        unsigned char *group=(void*)(uintptr_t)(0x8e0000+cursor);
        unsigned count=read16(group),opcode=group[2],size=group[3],i,c;
        if (!size) return;
        require(opcode<32 && cursor+4+count*size<=mesh_size,"primitive group bounds");
        unsigned stride=read32((void*)(uintptr_t)(0x466734+(opcode/4)*4));
        require(offset+count*stride<=0x3000,"bounded original primitive buffer");
        for (i=0;i<count;i++) {
            unsigned char *record=group+4+i*size,*primitive=(void*)(uintptr_t)(base+offset+i*stride);
            unsigned family=opcode/8;
            if (family==1 || family==3) {
                const unsigned char *uv=definition(record,opcode);
                put16((uintptr_t)primitive+14,(unsigned short)raw_palette(record,opcode));
                if (family==3) {
                    unsigned corners=opcode%8>=4 ? 4 : 3;
                    put16((uintptr_t)primitive+26,read16(uv));
                    for (c=0;c<corners;c++) {
                        primitive[12+c*12]=uv[4+c*2];primitive[13+c*12]=uv[5+c*2];
                    }
                }
            }
        }
        offset+=count*stride;cursor+=4+count*size;
    }
    require(0,"terminated source mesh");
}
static void snapshot(FILE *out,unsigned driver,unsigned car_class) {
    unsigned i,c,offset=0,cursor=read32((void*)0x8e0028)-0x8e0000;
    unsigned descriptor=0x791cb0+driver*0x58;
    word(out,car_class);word(out,driver);
    for (i=0;i<8;i++) word(out,read16((void*)(uintptr_t)(0x791ca0+driver*0x58+i*2)));
    word(out,read16((void*)(uintptr_t)(descriptor+12))&31);
    word(out,read16((void*)(uintptr_t)(descriptor+14)));
    word(out,*(unsigned char*)(uintptr_t)(descriptor+18));
    word(out,*(unsigned char*)(uintptr_t)(descriptor+19));
    short original_number[12]={0};Sprite(0,"DR88A",original_number);
    word(out,(unsigned short)original_number[6]&31);word(out,(unsigned short)original_number[7]);
    word(out,((unsigned char*)original_number)[18]);word(out,((unsigned char*)original_number)[19]);
    while (cursor+4<=mesh_size) {
        unsigned char *group=(void*)(uintptr_t)(0x8e0000+cursor);
        unsigned count=read16(group),opcode=group[2],size=group[3];
        if (!size) return;
        unsigned stride=read32((void*)(uintptr_t)(0x466734+(opcode/4)*4));
        for (i=0;i<count;i++) if ((opcode&253)==25 || (opcode&253)==29) {
            const unsigned char *uv=definition(group+4+i*size,opcode);
            unsigned char *primitive=(void*)(uintptr_t)(0x860000+driver*0x3000+offset+i*stride);
            unsigned corners=(opcode&253)==25 ? 3 : 4;
            word(out,read16(primitive+14));word(out,read16(primitive+26)&31);
            for (c=0;c<4;c++) {
                word(out,c<corners ? primitive[12+c*12] : uv[4+c*2]);
                word(out,c<corners ? primitive[13+c*12] : uv[5+c*2]);
            }
        }
        offset+=count*stride;cursor+=4+count*size;
    }
    require(0,"terminated snapshot mesh");
}
int main(int argc,char **argv) {
    FILE *out;unsigned car_class,driver;
    require(argc==6,"original exe, sprites, high mesh, definitions and output required");
    map_original(argv[1]);
    sprites_size=load(argv[2],0x850000,0x10000);
    mesh_size=load(argv[3],0x8e0000,0x2000);
    definitions_size=load(argv[4],0x8f0000,0x10000);
    require(sprites_size==4+read32((void*)0x850000)*24,"sprite table extent");
    require(definitions_size==4+read32((void*)0x8f0000)*12,"definition table extent");
    require(read32((void*)0x8e0028)<mesh_size,"copied mesh polygon offset");
    put32(0x8e0028,read32((void*)0x8e0028)+0x8e0000);
    SpriteInfo((void*)0x850000);
    put32(0x754390,0x860000);put32(0x754394,0x8a0000);put32(0x46765c,20);
    memset((void*)0x8e2000,0,48);put32(0x8e2028,0x8e202c);
    for (driver=0;driver<20;driver++) {
        put32(0x78141c+driver*0x38,0x8e0000);put32(0x781420+driver*0x38,driver*0x3000);
        put32(0x781438+driver*0x38,0x8e2000);put32(0x78143c+driver*0x38,driver*0x3000);
    }
    out=fopen(argv[5],"wb");require(out!=NULL,"open original material output");
    for (car_class=0;car_class<3;car_class++) {
        put32(0x467400,car_class);memset((void*)0x791ca0,0,20*0x58);
        CarCluts();Sprite(0,"DR88A",(short*)0x7922e0);
        for (driver=0;driver<20;driver++) {
            primitives(0x860000+driver*0x3000);primitives(0x8a0000+driver*0x3000);
            CarPaint(driver,driver,0);
        }
        CarDoors();
        for (driver=0;driver<20;driver++) snapshot(out,driver,car_class);
    }
    require(fclose(out)==0,"close original material output");return 0;
}
