#ifndef DD2_CINEPAK_H
#define DD2_CINEPAK_H
#include <stddef.h>
#include <stdint.h>
typedef struct DD2Cinepak DD2Cinepak;
DD2Cinepak* dd2_cinepak_create(unsigned width,unsigned height);
void dd2_cinepak_destroy(DD2Cinepak* decoder);
/* Sequential compressed frames, including empty AVI hold packets. Output is
 * tightly packed, top-down RGB24, retained across interframes. Returns 0 on
 * success and -1 on invalid input; discard the context after a decode error. */
int dd2_cinepak_decode(DD2Cinepak* decoder,const uint8_t* packet,size_t bytes);
const uint8_t* dd2_cinepak_pixels(const DD2Cinepak* decoder);
#endif
