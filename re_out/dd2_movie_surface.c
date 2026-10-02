#include "dd2_movie_platform.h"
int dd2_movie_render(const uint8_t* rgb,unsigned width,unsigned height,const int32_t* rect,uint32_t* argb) {
    int x,y;
    if(!rgb || !width || !height || !rect || !argb || rect[2]<=0 || rect[3]<=0)return -1;
    for(y=0;y<480;y++)for(x=0;x<640;x++) {
        int64_t dx=(int64_t)x-rect[0],dy=(int64_t)y-rect[1];
        uint32_t color=0xff000000u;
        if(dx>=0 && dy>=0 && dx<rect[2] && dy<rect[3]) {
            size_t sx=(uint64_t)dx*width/(unsigned)rect[2];
            size_t sy=(uint64_t)dy*height/(unsigned)rect[3];
            const uint8_t* p=rgb+(sy*width+sx)*3;
            unsigned r=p[0]>>3,g=p[1]>>2,b=p[2]>>3;
            color|=((r<<3)|(r>>2))<<16;
            color|=((g<<2)|(g>>4))<<8;
            color|=(b<<3)|(b>>2);
        }
        argb[y*640+x]=color;
    }
    return 0;
}
