/* Execute unmodified original class drive motion with flat synthetic ground.
 * Isolated engine and saturated rear-axle cases recover class coefficients;
 * effects are inactive and the original executable is never written. */
#include "pe_fixture.h"
#define Drive ((void (*)(int))(uintptr_t)0x4414dc)
static void trial(unsigned car_class,int engine,int vx,int friction,int front_factor) {
    memset((void*)0x792a00,0,0x1b2);
    memset((void*)0x850000,0,16);
    put32(0x7926a0,0x850000);
    put32(0x792b5a,car_class);
    put32(0x792a86,(unsigned)engine);
    put32(0x792b2a,4096);
    put32(0x792a8e,(unsigned)friction);
    put32(0x792b26,(unsigned)front_factor);
    put32(0x792a10,(unsigned)vx);
    put32(0x78a798,0);
    Drive(0);
    printf("%u,%d,%d,%d,%d,%d,%d,%d,%u\n",car_class,engine,vx,friction,front_factor,
           (int)read32((void*)0x792a10),(int)read32((void*)0x792a18),
           (int)read32((void*)0x792a04),read32((void*)0x792a76));
}
int main(int argc,char **argv) {
    unsigned c,i;
    static const int engines[]={-4096,0,4096};
    require(argc==2,"supported original executable required");
    map_original(argv[1]);
    for(c=0;c<3;c++) {
        printf("rating,%u,%u,%u,%u\n",c,read32((void*)(uintptr_t)(0x468e80+c*12)),
               read32((void*)(uintptr_t)(0x468e84+c*12)),
               read32((void*)(uintptr_t)(0x468e88+c*12)));
        for(i=0;i<3;i++) {
            trial(c,engines[i],0,0,2);
            trial(c,engines[i],65536,4096,-2);
            trial(c,engines[i],65536,4096,2);
        }
    }
    return 0;
}
