/* Microsoft ADPCM source decoding for DD2's AVI audio.
 * Predictors, initial sample order and signed /256 rounding checked against
 * actual Wine Win32 ACM msadp32.acm; reference arithmetic:
 * https://github.com/wine-mirror/wine/blob/wine-10.0/dlls/msadp32.acm/msadp32.c
 * This produces source PCM, before movie transport or output resampling. */
#include "dd2_msadpcm.h"
#include <limits.h>
#include <string.h>
static unsigned u16(const uint8_t* data) { return data[0]|(unsigned)data[1]<<8; }
static int s16(const uint8_t* data) {
    unsigned value=u16(data);return value<32768 ? (int)value : (int)value-65536;
}
static unsigned u32(const uint8_t* data) {
    return u16(data)|(unsigned)u16(data+2)<<16;
}
int dd2_msadpcm_format(DD2MSADPCM* format,const uint8_t* data,size_t bytes) {
    unsigned count,i,channels,block,samples,extra;
    if(!format || !data || bytes<22 || u16(data)!=2 || u16(data+14)!=4)return -1;
    channels=u16(data+2);block=u16(data+12);extra=u16(data+16);
    samples=u16(data+18);count=u16(data+20);
    if((channels!=1 && channels!=2) || !u32(data+4) || !count || count>256
            || extra<4+count*4 || bytes<18u+extra || block<7*channels || samples<2
            || (samples-2)*channels>(block-7*channels)*2)return -1;
    memset(format,0,sizeof(*format));
    format->channels=channels;format->rate=u32(data+4);format->block_bytes=block;
    format->samples_per_block=samples;format->coefficients=count;
    for(i=0;i<count;i++) {
        format->coefficient[i][0]=s16(data+22+i*4);
        format->coefficient[i][1]=s16(data+24+i*4);
    }
    return 0;
}
static int sample(unsigned nibble,int* delta,int* previous,int* older,const int16_t* coefficient) {
    static const unsigned adaptation[16]={230,230,230,230,307,409,512,614,768,614,512,409,307,230,230,230};
    int signed_nibble=nibble<8 ? (int)nibble : (int)nibble-16;
    int64_t prediction=((int64_t)*previous*coefficient[0]+(int64_t)*older*coefficient[1])/256;
    int64_t value=prediction+(int64_t)signed_nibble* *delta;
    int64_t next=(int64_t)*delta*adaptation[nibble]/256;
    if(next>INT_MAX)return -1;
    *older=*previous;
    *previous=value<-32768 ? -32768 : value>32767 ? 32767 : (int)value;
    *delta=next<16 ? 16 : (int)next;
    return 0;
}
int dd2_msadpcm_decode(const DD2MSADPCM* format,const uint8_t* data,size_t bytes,
                        int16_t* output,size_t capacity,size_t* written) {
    size_t blocks,index,total;
    unsigned channels,c,n,code[2];
    int previous[2],older[2],delta[2];
    if(written)*written=0;
    if(!format || !written || (format->channels!=1 && format->channels!=2)
            || !format->coefficients || format->coefficients>256 || !format->block_bytes
            || format->samples_per_block<2)return -1;
    channels=format->channels;
    if(format->block_bytes<7*channels || (format->samples_per_block-2)*channels>(format->block_bytes-7*channels)*2
            || bytes%format->block_bytes || (bytes && (!data || !output)))return -1;
    blocks=bytes/format->block_bytes;
    if(blocks>SIZE_MAX/format->samples_per_block)return -1;
    total=blocks*format->samples_per_block;
    if(total>capacity || total>SIZE_MAX/channels)return -1;
    for(index=0;index<blocks;index++,data+=format->block_bytes) {
        for(c=0;c<channels;c++) {
            code[c]=data[c];if(code[c]>=format->coefficients)return -1;
            delta[c]=s16(data+channels+c*2);
            previous[c]=s16(data+channels*3+c*2);
            older[c]=s16(data+channels*5+c*2);
            output[c]=older[c];output[channels+c]=previous[c];
        }
        for(n=0;n<(format->samples_per_block-2)*channels;n++) {
            unsigned byte=data[channels*7+n/2];
            unsigned nibble=(byte>>(n%2 ? 0 : 4))&15;
            c=n%channels;
            if(sample(nibble,&delta[c],&previous[c],&older[c],format->coefficient[code[c]]))return -1;
            output[channels*2+n]=previous[c];
        }
        output+=format->samples_per_block*channels;
    }
    *written=total;return 0;
}
