/* Pit camera controls against unchanged original x86, with explicit live/replay
 * inputs and complete high-detail polygon guards. No race or A/V parity claim. */
#include "pe_fixture.h"
#ifdef DD2_ORIGINAL_PIT_CAMERA
#define Camera ((void (*)(int))(uintptr_t)0x429b48)
#else
void Pit_Camera_Control(int);
#define Camera Pit_Camera_Control
#endif

static const unsigned counts[] = {8,16,8,6,10,74,25};
static const unsigned types[] = {0,4,1,9,13,25,29};
static const unsigned strides[] = {16,16,16,20,20,24,28};

static void model(void) {
    unsigned i, poly = 0x901000;
    memset((void *)0x900000, 0, 8192);
    put32(0x78141c, 0x900000);
    put32(0x900028, poly);
    for (i=0; i<7; i++) {
        put16(poly, counts[i]);
        *(unsigned char *)(uintptr_t)(poly+2) = types[i];
        *(unsigned char *)(uintptr_t)(poly+3) = strides[i];
        poly += 4 + counts[i]*strides[i];
    }
}

static void snapshot(FILE *out) {
    const unsigned addresses[] = {0x463f04,0x464a84,0x464a88,0x464a8c,
        0x46707c,0x9392b4,0x9376b0,0x7746ac,0x9392b0};
    unsigned i;
    for (i=0; i<9; i++)
        require(fwrite((void *)(uintptr_t)addresses[i],4,1,out)==1,"camera state");
    require(fwrite((void *)0x781418,1,28,out)==28,"model object");
    require(fwrite((void *)0x900000,1,64,out)==64,"model header");
    require(fwrite((void *)0x900f00,1,4608,out)==4608,"model polygons and guards");
}

int main(int argc, char **argv) {
    const unsigned rotations[] = {0xfffffff8,0xffffffff,0,1,8};
    const unsigned angles[] = {0xffffefff,0xffffffff,0,0x400,0x7ff};
    const unsigned flags[] = {0,0x20,0x80,0xa0};
    const unsigned offsets[] = {0xffc0,0,0x40};
    unsigned mode, section, action, rotation, angle, offset, index=0;
    unsigned description[6];
    FILE *out;
    require(argc==3,"original executable and output required");
    map_original(argv[1]);
    out=fopen(argv[2],"wb");
    require(out!=NULL,"open pit camera output");
    for (mode=0; mode<2; mode++)
    for (section=0; section<6; section++)
    for (action=0; action<4; action++)
    for (rotation=0; rotation<5; rotation++)
    for (angle=0; angle<5; angle++)
    for (offset=0; offset<3; offset++) {
        model();
        put32(0x467074,mode);
        put32(0x467078,0);
        put32(0x464a84,section);
        /* A neighboring guard must survive the original WORD store. */
        put32(0x464a88,0xa55a0000|offsets[offset]);
        put32(0x464a8c,rotations[rotation]);
        put32(0x463f04,angles[angle]);
        put32(0x9392b4,0x9376b0);
        put32(0x9376b0,mode ? (action==1 ? 0x4001 : action==2 ? 0x8001 : action==3 ? 0xc001 : 1) : 0xa55a1234);
        put32(0x9376b4,0);
        put32(0x46707c,2);
        put32(0x754448,flags[action]<<16);
        put32(0x7746ac,0);
        put32(0x9392b0,0);
        put32(0x9392c4,0x9376b8);
        put32(0x46706c,255);
        description[0]=mode;
        description[1]=section;
        description[2]=flags[action];
        description[3]=rotations[rotation];
        description[4]=angles[angle];
        description[5]=offsets[offset];
        require(fwrite(description,4,6,out)==6,"explicit camera inputs");
        snapshot(out);
        Camera(0);
        snapshot(out);
        index++;
    }
    require(index==3600,"all cases executed");
    require(fclose(out)==0,"close pit camera output");
    return 0;
}
