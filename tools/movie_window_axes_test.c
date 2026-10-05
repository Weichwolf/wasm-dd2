/* Exact old/current movie window bytes: all AVI source frames and clipped
 * transforms. No old/reference data enters the production renderer. */
#include "dd2_avi.h"
#include "dd2_movie_platform.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
extern int dd2_movie_render_window_before(const uint8_t*,unsigned,unsigned,const int32_t*,uint32_t*);
static uint32_t before[640*480],after[640*480];
static uint64_t count,before_ns,after_ns;
static void require(int ok,const char* why){if(!ok){fprintf(stderr,"movie window axes: %s\n",why);exit(1);}}
static uint64_t now_ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;}
static void exact(const uint8_t* rgb,unsigned width,unsigned height,const int32_t* rect){
    uint64_t start=now_ns();
    require(!dd2_movie_render_window_before(rgb,width,height,rect,before),"before render");
    before_ns+=now_ns()-start;start=now_ns();
    require(!dd2_movie_render_window(rgb,width,height,rect,after),"current render");
    after_ns+=now_ns()-start;
    require(!memcmp(before,after,sizeof(after)),"changed actual window bytes");count++;
}
int main(int argc,char** argv){
    static const int32_t rectangles[][4]={
        {0,48,640,384},{0,0,320,192},{-97,-41,517,309},{529,331,147,173},
        {0,0,1,1},{-1,-1,3,3},{0,0,641,481},{-640,-480,640,480},
        {2147483647,2147483647,2147483647,2147483647},
        {-2147483647-1,-2147483647-1,2147483647,2147483647},
        {-2147483647,-2147483647,2147483647,2147483647},
        {0,0,2147483647,2147483647},{-160,-96,320,192},{17,29,97,73}};
    uint8_t* rgb;unsigned i,j,frames=0;uint32_t state=1;
    require(argc==2,"original AVI path required");
    rgb=malloc(320*192*3);require(rgb!=NULL,"synthetic RGB allocation");
    for(i=0;i<320*192*3;i++){state=state*1664525u+1013904223u;rgb[i]=(uint8_t)(state>>24);}
    for(i=0;i<sizeof(rectangles)/sizeof(rectangles[0]);i++){
        exact(rgb,320,192,rectangles[i]);exact(rgb,1,1,rectangles[i]);exact(rgb,7,5,rectangles[i]);
    }
    for(i=0;i<32;i++){
        int32_t rect[4];
        for(j=0;j<4;j++){state=state*1664525u+1013904223u;rect[j]=(int32_t)(state%1024);}
        rect[0]-=512;rect[1]-=512;rect[2]++;rect[3]++;
        exact(rgb,320,192,rect);
    }
    free(rgb);
    {
        FILE* file=fopen(argv[1],"rb");long length=0;uint8_t* bytes;DD2AVI* movie;const DD2AVIInfo* info;
        require(file!=NULL,"original AVI open");
        require(!fseek(file,0,SEEK_END) && (length=ftell(file))>0 && !fseek(file,0,SEEK_SET),"AVI extent");
        bytes=malloc((size_t)length);require(bytes!=NULL,"AVI allocation");
        require(fread(bytes,1,(size_t)length,file)==(size_t)length && !fclose(file),"AVI read");
        movie=dd2_avi_open(bytes,(size_t)length);free(bytes);require(movie!=NULL,"production AVI decoder");
        info=dd2_avi_info(movie);require(info->width==320 && info->height==192,"original source dimensions");
        for(i=0;i<info->frames;i++){
            const uint8_t* pixels=dd2_avi_frame(movie,i);require(pixels!=NULL,"every AVI source frame");
            exact(pixels,info->width,info->height,rectangles[0]);frames++;
        }
        dd2_avi_close(movie);
    }
    after[1000]^=1;require(memcmp(before,after,sizeof(after))!=0,"changed display bit rejected");
    {
        int32_t invalid[4]={0,0,0,384};uint8_t pixel[3]={0,0,0};
        require(dd2_movie_render_window(pixel,1,1,invalid,after)==-1,"zero width rejected");
        invalid[2]=640;invalid[3]=-1;
        require(dd2_movie_render_window(pixel,1,1,invalid,after)==-1,"negative height rejected");
    }
    printf("{\"avi_frames\":%u,\"synthetic_cases\":74,\"exact_frames\":%llu,\"exact_argb_bytes\":%llu,"
           "\"before_ms\":%.3f,\"after_ms\":%.3f,\"changed_bit_rejected\":true,\"invalid_rectangles_rejected\":true}\n",
           frames,(unsigned long long)count,(unsigned long long)(count*sizeof(after)),before_ns/1000000.0,after_ns/1000000.0);
    return 0;
}
