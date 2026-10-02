/* Explicit 64-significand-bit arithmetic for the observed 32-bit Wine x87
 * FIR mixer. Native x87 and WASM f64 otherwise round their running sums at
 * different precisions. Finite normalized values only; the audio filter uses
 * neither infinities nor x87 subnormals. Each operation rounds to nearest/even.
 * Value = (-1)^negative * significand * 2^exponent. */
#ifndef DD2_SOUND_WIDE_H
#define DD2_SOUND_WIDE_H
#include <stdint.h>
#include <string.h>
typedef struct { uint64_t significand; int exponent, negative; } DD2Wide;
typedef struct { uint64_t hi, lo; } DD2Pair;

static DD2Wide dd2_wide_round(DD2Pair p,int exponent,int negative){
    DD2Wide result={0,0,0};
    int shift;
    if(!p.hi){
        if(!p.lo)return result;
        p.hi=p.lo;p.lo=0;exponent-=64;
    }
    shift=__builtin_clzll(p.hi);
    if(shift){p.hi=(p.hi<<shift)|(p.lo>>(64-shift));p.lo<<=shift;exponent-=shift;}
    if(p.lo>UINT64_C(0x8000000000000000) ||
       (p.lo==UINT64_C(0x8000000000000000) && (p.hi&1))){
        p.hi++;
        if(!p.hi){p.hi=UINT64_C(0x8000000000000000);exponent++;}
    }
    result.significand=p.hi;result.exponent=exponent;result.negative=negative;
    return result;
}

static DD2Wide dd2_wide_float(float input){
    DD2Wide result={0,0,0};uint32_t bits,mantissa;int exp,shift;
    memcpy(&bits,&input,4);exp=(bits>>23)&255;mantissa=bits&0x7fffff;
    if(!exp && !mantissa)return result;
    result.negative=bits>>31;
    if(exp){result.significand=(uint64_t)(mantissa|0x800000)<<40;result.exponent=exp-127-63;}
    else {
        shift=__builtin_clz(mantissa)-8;
        result.significand=(uint64_t)mantissa<<(40+shift);result.exponent=-126-63-shift;
    }
    return result;
}

static DD2Pair dd2_pair_product(uint64_t a,uint64_t b){
    uint64_t a0=(uint32_t)a,a1=a>>32,b0=(uint32_t)b,b1=b>>32;
    uint64_t lower=a0*b0,middle=a1*b0+(lower>>32),upper=middle>>32;
    DD2Pair result;
    middle=(uint32_t)middle+a0*b1;
    result.hi=a1*b1+upper+(middle>>32);
    result.lo=(middle<<32)|(uint32_t)lower;
    return result;
}

static DD2Wide dd2_wide_mul(DD2Wide a,DD2Wide b){
    return dd2_wide_round(dd2_pair_product(a.significand,b.significand),
                          a.exponent+b.exponent+64,a.negative^b.negative);
}

static DD2Wide dd2_wide_add(DD2Wide a,DD2Wide b){
    DD2Pair p,q;int distance,carry,borrow;
    DD2Wide swap;
    if(!a.significand)return b;
    if(!b.significand)return a;
    if(a.exponent<b.exponent || (a.exponent==b.exponent && a.significand<b.significand)){
        swap=a;a=b;b=swap;
    }
    distance=a.exponent-b.exponent;
    p.hi=a.significand;p.lo=0;q.hi=0;q.lo=0;
    if(!distance)q.hi=b.significand;
    else if(distance<64){q.hi=b.significand>>distance;q.lo=b.significand<<(64-distance);}
    else if(distance==64)q.lo=b.significand;
    else if(distance<128){
        q.lo=b.significand>>(distance-64);
        if(b.significand<<(128-distance))q.lo|=1;
    }else q.lo=1; /* sticky bit for all discarded nonzero bits */
    if(a.negative==b.negative){
        p.lo=q.lo;p.hi+=q.hi;carry=p.hi<q.hi;
        if(carry){p.lo=(p.hi<<63)|(p.lo>>1)|(p.lo&1);p.hi=(p.hi>>1)|UINT64_C(0x8000000000000000);a.exponent++;}
    }else{
        borrow=p.lo<q.lo;p.lo-=q.lo;p.hi=p.hi-q.hi-borrow;
    }
    return dd2_wide_round(p,a.exponent,a.negative);
}

/* An integer ratio is divided at x87 precision before rem is rounded to f32.
 * Four base-2^32 long-division limbs avoid target-specific __int128 support. */
static DD2Wide dd2_wide_ratio(uint64_t numerator,uint32_t denominator){
    DD2Wide result={0,0,0};DD2Pair scaled;
    uint32_t limbs[4],quotient[4];uint64_t remainder=0,part;
    int power,shift,i;
    if(!numerator)return result;
    power=(63-__builtin_clzll(numerator))-(31-__builtin_clz(denominator));
    if(power>=0){if(numerator<((uint64_t)denominator<<power))power--;}
    else if((numerator<<(-power))<denominator)power--;
    shift=63-power;
    if(shift>=64){scaled.hi=numerator<<(shift-64);scaled.lo=0;}
    else if(shift){scaled.hi=numerator>>(64-shift);scaled.lo=numerator<<shift;}
    else {scaled.hi=0;scaled.lo=numerator;}
    limbs[0]=scaled.hi>>32;limbs[1]=(uint32_t)scaled.hi;
    limbs[2]=scaled.lo>>32;limbs[3]=(uint32_t)scaled.lo;
    for(i=0;i<4;i++){
        part=(remainder<<32)|limbs[i];quotient[i]=(uint32_t)(part/denominator);remainder=part%denominator;
    }
    result.significand=((uint64_t)quotient[2]<<32)|quotient[3];
    result.exponent=power-63;
    if(remainder*2>denominator || (remainder*2==denominator && (result.significand&1))){
        result.significand++;
        if(!result.significand){result.significand=UINT64_C(0x8000000000000000);result.exponent++;}
    }
    return result;
}

static float dd2_wide_to_float(DD2Wide input){
    uint32_t bits=0;uint64_t mantissa,remainder,half;float result;int power,shift;
    if(input.significand){
        power=input.exponent+63;shift=40;
        if(power<-126)shift+=-126-power;
        if(shift<64){
            mantissa=input.significand>>shift;
            remainder=input.significand&((UINT64_C(1)<<shift)-1);half=UINT64_C(1)<<(shift-1);
            if(remainder>half || (remainder==half && (mantissa&1)))mantissa++;
        }else if(shift==64)mantissa=input.significand>UINT64_C(0x8000000000000000);
        else mantissa=0;
        if(power>=-126){
            if(mantissa==0x1000000){mantissa>>=1;power++;}
            bits=power>127?0x7f800000:((uint32_t)(power+127)<<23)|((uint32_t)mantissa&0x7fffff);
        }else bits=(uint32_t)mantissa;
        bits|=(uint32_t)input.negative<<31;
    }
    memcpy(&result,&bits,4);return result;
}
#endif
