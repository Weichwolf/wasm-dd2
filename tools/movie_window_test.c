/* Decode original AVI inputs and render requested frame/rectangle inputs with
 * production code. No captured pixels or engine state feed this component. */
#include "dd2_avi.h"
#include "dd2_movie_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
static int fail(const char *why){fprintf(stderr,"movie window component: %s\n",why);return 1;}
int main(int argc,char **argv){
    FILE *file;long length;uint8_t *bytes;DD2AVI *movie;
    const DD2AVIInfo *info;uint32_t request[5],header[4],*argb;
    if(argc!=2)return fail("arguments");
    file=fopen(argv[1],"rb");if(!file)return fail("original AVI input");
    if(fseek(file,0,SEEK_END)||(length=ftell(file))<=0||fseek(file,0,SEEK_SET))return fail("AVI extent");
    bytes=malloc((size_t)length);if(!bytes)return fail("AVI allocation");
    if(fread(bytes,1,(size_t)length,file)!=(size_t)length)return fail("AVI read");
    fclose(file);movie=dd2_avi_open(bytes,(size_t)length);free(bytes);
    if(!movie)return fail("production AVI open");
    info=dd2_avi_info(movie);
    if(info->width!=320||info->height!=192)return fail("source dimensions");
    argb=malloc(640*480*4);if(!argb)return fail("frame allocation");
    header[0]=0x50324444u;header[1]=info->width;header[2]=info->height;header[3]=info->frames;
    if(fwrite(header,sizeof(header),1,stdout)!=1||fflush(stdout))return fail("source header");
    while(fread(request,sizeof(request),1,stdin)==1){
        const uint8_t *rgb=dd2_avi_frame(movie,request[0]);
        if(!rgb||dd2_movie_render_window(rgb,info->width,info->height,(const int32_t*)(request+1),argb))return fail("production movie frame");
        if(fwrite(rgb,1,320*192*3,stdout)!=320*192*3||
           fwrite(argb,1,640*480*4,stdout)!=640*480*4||fflush(stdout))return fail("frame output");
    }
    if(!feof(stdin))return fail("incomplete request");
    free(argb);dd2_avi_close(movie);return 0;
}
