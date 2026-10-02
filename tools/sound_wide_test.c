/* Compare the explicit significand arithmetic with actual 32-bit x87 results;
 * emit every result so WASM is checked against the independently run CPU. */
#include <stdio.h>
#include <stdlib.h>
#include "dd2_sound_wide.h"
static uint32_t seed=0x9abcdef0u;
static uint32_t next_value(void){seed=seed*1664525u+1013904223u;return seed;}
static float random_float(void){
    uint32_t bits=(next_value()&0x807fffff)|((next_value()%150+25)<<23);
    float value;memcpy(&value,&bits,4);return value;
}
static FILE *output;
static unsigned operations;
static void check(float actual,long double expected){
    uint32_t bits;memcpy(&bits,&actual,4);
#ifndef __EMSCRIPTEN__
    { volatile float cpu=(float)expected;
      uint32_t oracle;memcpy(&oracle,(const void*)&cpu,4);
      if(bits!=oracle){fprintf(stderr,"x87 mismatch %u: %08x != %08x\n",operations,bits,oracle);exit(1);} }
#else
    (void)expected;
#endif
    if(fwrite(&bits,4,1,output)!=1)exit(1);
    operations++;
}
int main(int argc,char **argv){
    unsigned i,r;const unsigned rates[]={22050,44100,48000};DD2Wide total={0,0,0};
#ifndef __EMSCRIPTEN__
    volatile long double reference=0;
    unsigned short control;
    __asm__("fnstcw %0":"=m"(control));
    if((control&0xf00)!=0x300){fprintf(stderr,"Requires x87 64-bit precision, nearest/even\n");return 1;}
#endif
    if(argc!=2 || !(output=fopen(argv[1],"wb")))return 1;
    for(i=0;i<100000;i++){
        float a=random_float(),b=random_float();
        DD2Wide x=dd2_wide_float(a),y=dd2_wide_float(b),product=dd2_wide_mul(x,y);
        uint64_t numerator=(uint64_t)next_value()*120;
        uint32_t denominator=next_value()%200000+1;
#ifndef __EMSCRIPTEN__
        volatile long double multiplication=(long double)a*b,addition=(long double)a+b;
        volatile long double ratio=(long double)numerator/denominator;
        reference+=multiplication;
        check(dd2_wide_to_float(product),multiplication);
        check(dd2_wide_to_float(dd2_wide_add(x,y)),addition);
        check(dd2_wide_to_float(dd2_wide_ratio(numerator,denominator)),ratio);
        total=dd2_wide_add(total,product);check(dd2_wide_to_float(total),reference);
#else
        check(dd2_wide_to_float(product),0);
        check(dd2_wide_to_float(dd2_wide_add(x,y)),0);
        check(dd2_wide_to_float(dd2_wide_ratio(numerator,denominator)),0);
        total=dd2_wide_add(total,product);check(dd2_wide_to_float(total),0);
#endif
        if(i%257==256){total.significand=0;
#ifndef __EMSCRIPTEN__
            reference=0;
#endif
        }
    }
    /* Exhaust every possible device-rate fractional remainder at three FIR
     * indices. This also checks tiny ratios and x87 cancellation before rem's
     * f32 store, rather than relying on the random large-numerator coverage. */
    for(r=0;r<3;r++)for(i=0;i<3*rates[r];i++){
        unsigned integer=i/rates[r]*100000,phase=i%rates[r];
        uint64_t numerator=(uint64_t)integer*rates[r]+phase;
        DD2Wide ratio=dd2_wide_ratio(numerator,rates[r]),negative=ratio;
        DD2Wide upper=dd2_wide_float((float)(integer+1));
        negative.negative=1;
#ifndef __EMSCRIPTEN__
        volatile long double cpu_ratio=(long double)numerator/rates[r];
        volatile long double cpu_rem=(long double)(integer+1)-cpu_ratio;
        check(dd2_wide_to_float(ratio),cpu_ratio);
        check(dd2_wide_to_float(dd2_wide_add(upper,negative)),cpu_rem);
#else
        check(dd2_wide_to_float(ratio),0);
        check(dd2_wide_to_float(dd2_wide_add(upper,negative)),0);
#endif
    }
    fclose(output);printf("%u exact results\n",operations);return 0;
}
