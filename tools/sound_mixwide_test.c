/* Compare the production FIR operations with the independently checked
 * integer x87 model, including caller control-word restoration. */
#include <stdio.h>
#include <stdlib.h>
#include "dd2_sound_mixwide.h"
static uint32_t seed=0x9abcdef0u;
static uint32_t next_value(void){seed=seed*1664525u+1013904223u;return seed;}
static float random_float(void){
    uint32_t bits=(next_value()&0x807fffff)|((next_value()%150+25)<<23);
    float sample;memcpy(&sample,&bits,4);return sample;
}
static unsigned operations;
static FILE* output;
static void check(float actual,float expected){
    uint32_t a,b;memcpy(&a,&actual,4);memcpy(&b,&expected,4);
    if(a!=b){fprintf(stderr,"FIR arithmetic mismatch %u: %08x != %08x\n",operations,a,b);exit(1);}
    if(fwrite(&a,4,1,output)!=1)exit(1);
    operations++;
}
int main(int argc,char** argv){
    unsigned i;
    DD2Wide total={0,0,0};DD2MixWide accumulated=DD2_MIX_ZERO;
#ifndef __EMSCRIPTEN__
    unsigned short initial;
    __asm__ __volatile__("fnstcw %0":"=m"(initial));
#endif
    if(argc!=2 || !(output=fopen(argv[1],"wb")))return 1;
    for(i=0;i<100000;i++){
        float a=random_float(),b=random_float();
        uint64_t numerator=((uint64_t)next_value()<<32)|next_value();
        uint32_t denominator=next_value()%200000+1;
        DD2Wide x=dd2_wide_float(a),y=dd2_wide_float(b),product=dd2_wide_mul(x,y);
        unsigned short previous;
        DD2MixWide mx,my;
#ifndef __EMSCRIPTEN__
        /* The engine normally selects 53 bits. Also exercise every rounding
         * mode and all three supported precision settings, then restore it. */
        unsigned short caller=(unsigned short)(0x7f|((i%4)<<10)|
            ((i%3==0?0:i%3==1?2:3)<<8)),restored;
        __asm__ __volatile__("fldcw %0"::"m"(caller):"memory");
#endif
        previous=dd2_mix_begin();mx=dd2_mix_float(a);my=dd2_mix_float(b);
        check(dd2_mix_to_float(dd2_mix_mul(mx,my)),dd2_wide_to_float(product));
        check(dd2_mix_to_float(dd2_mix_add(mx,my)),dd2_wide_to_float(dd2_wide_add(x,y)));
        x.negative^=1;
        check(dd2_mix_to_float(dd2_mix_negate(mx)),dd2_wide_to_float(x));
        check(dd2_mix_to_float(dd2_mix_ratio(numerator,denominator)),
              dd2_wide_to_float(dd2_wide_ratio(numerator,denominator)));
        accumulated=dd2_mix_add(accumulated,dd2_mix_mul(mx,my));
        total=dd2_wide_add(total,product);
        check(dd2_mix_to_float(accumulated),dd2_wide_to_float(total));
        if(i<12){
            /* A 53-bit intermediate loses this increment. Subtraction makes
             * that precision error observable even after the Float32 spill. */
            uint32_t small_bits=0x24800000u;
            volatile float small,unit=1.0f,negative_unit=-1.0f;
            memcpy((void*)&small,&small_bits,4);
            check(dd2_mix_to_float(dd2_mix_add(
                dd2_mix_add(dd2_mix_float(unit),dd2_mix_float(small)),
                dd2_mix_float(negative_unit))),small);
        }
        dd2_mix_end(previous);
#ifndef __EMSCRIPTEN__
        __asm__ __volatile__("fnstcw %0":"=m"(restored)::"memory");
        if(restored!=caller){fprintf(stderr,"FIR changed caller control word\n");return 1;}
#endif
        if(i%257==256){total.significand=0;previous=dd2_mix_begin();
            accumulated=dd2_mix_float(0.0f);dd2_mix_end(previous);}
    }
#ifndef __EMSCRIPTEN__
    __asm__ __volatile__("fldcw %0"::"m"(initial):"memory");
#endif
    fclose(output);printf("%u exact production FIR results\n",operations);return 0;
}
