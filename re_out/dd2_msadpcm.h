#ifndef DD2_MSADPCM_H
#define DD2_MSADPCM_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    unsigned channels,rate,block_bytes,samples_per_block,coefficients;
    int16_t coefficient[256][2];
} DD2MSADPCM;
int dd2_msadpcm_format(DD2MSADPCM*,const uint8_t*,size_t);
/* Decode complete blocks to interleaved signed PCM16. The output capacity
 * and written count are frames, not individual channel samples. */
int dd2_msadpcm_decode(const DD2MSADPCM*,const uint8_t*,size_t,int16_t*,size_t,size_t*);
#endif
