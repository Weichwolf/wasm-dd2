/* Exercise the real engine aliases and check every adjacent byte. */
#include <stdio.h>
#include <string.h>
#include "dd2_symbols.h"

static unsigned char image[0x300];
#undef GIMG
#define GIMG(va) (image + ((unsigned)(va) - 0x792400u))

int main(void) {
    unsigned char expected[sizeof image];
    unsigned offsets[8] = {0x10a,0x136,0x162,0x18e,0x1ba,0x1e6,0x212,0x23e};
    unsigned i, pass, counter;
    for (pass = 0; pass < 2; pass++) {
        unsigned char lane_value = pass ? 255 : 1;
        memset(image, 0x5a, sizeof image);
        counter = 99;
        memcpy(image + 0x240, &counter, sizeof counter);
        memcpy(expected, image, sizeof image);
        for (i = 0; i < 8; i++) expected[offsets[i]] = lane_value;
        DAT_0079250a = lane_value; DAT_00792536 = lane_value;
        DAT_00792562 = lane_value; DAT_0079258e = lane_value;
        DAT_007925ba = lane_value; DAT_007925e6 = lane_value;
        DAT_00792612 = lane_value; DAT_0079263e = lane_value;
        if (memcmp(image, expected, sizeof image)) {
            memcpy(&counter, image + 0x240, sizeof counter);
            printf("neighbor field corruption; grounded_count=%u\n", counter);
            return 1;
        }
    }
    puts("eight exact BYTE stores; adjacent FD fields and grounded_count preserved");
    return 0;
}
