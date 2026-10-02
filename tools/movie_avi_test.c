/* Production AVI source interface exercised with complete original files,
 * sequential decoding, nonsequential seeks and caller-buffer ownership. */
#include "dd2_avi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
static int fail(const char* why) { fprintf(stderr,"movie AVI: %s\n",why);return 1; }
int main(int argc,char** argv) {
    FILE* file;
    long bytes;
    uint8_t* data;
    DD2AVI* movie;
    const DD2AVIInfo* info;
    size_t pixels;
    unsigned frame,seeks[7],n;
    if(argc!=5)return fail("arguments");
    file=fopen(argv[1],"rb");if(!file)return fail("input");
    if(fseek(file,0,SEEK_END) || (bytes=ftell(file))<0 || fseek(file,0,SEEK_SET))return fail("length");
    data=malloc(bytes ? (size_t)bytes : 1);if(!data)return fail("allocation");
    if(fread(data,1,bytes,file)!=(size_t)bytes)return fail("read");
    fclose(file);
    movie=dd2_avi_open(data,bytes);
    /* A caller may immediately reuse/free its input buffer. */
    memset(data,0,bytes);free(data);
    if(!movie)return fail("invalid container");
    info=dd2_avi_info(movie);pixels=(size_t)info->width*info->height*3;
    file=fopen(argv[2],"wb");if(!file)return fail("video output");
    for(frame=0;frame<info->frames;frame++) {
        const uint8_t* rgb=dd2_avi_frame(movie,frame);
        if(!rgb || fwrite(rgb,1,pixels,file)!=pixels)return fail("source frame");
    }
    fclose(file);
    if(dd2_avi_frame(movie,info->frames) || dd2_avi_frame(movie,UINT_MAX))return fail("accepted invalid frame");
    file=fopen(argv[3],"wb");if(!file)return fail("audio output");
    if(fwrite(dd2_avi_pcm(movie),sizeof(int16_t)*info->pcm_channels,info->pcm_frames,file)!=info->pcm_frames)
        return fail("source PCM");
    fclose(file);
    file=fopen(argv[4],"wb");if(!file)return fail("seek output");
    seeks[0]=info->frames-1;seeks[1]=0;seeks[2]=1;seeks[3]=info->frames/2;
    seeks[4]=info->frames/2;seeks[5]=info->frames-2;seeks[6]=2;
    for(n=0;n<7;n++) {
        const uint8_t* rgb=dd2_avi_frame(movie,seeks[n]);
        if(!rgb || fwrite(rgb,1,pixels,file)!=pixels)return fail("seek frame");
    }
    fclose(file);
    printf("{\"width\":%u,\"height\":%u,\"frames\":%u,\"empty_frames\":%u,"
           "\"video_scale\":%u,\"video_rate\":%u,\"video_start\":%u,"
           "\"audio_scale\":%u,\"audio_rate\":%u,\"audio_start\":%u,\"audio_initial_frames\":%u,"
           "\"pcm_rate\":%u,\"pcm_channels\":%u,\"pcm_frames\":%lu,"
           "\"seeks\":[%u,%u,%u,%u,%u,%u,%u]}\n",
           info->width,info->height,info->frames,info->empty_frames,
           info->video_scale,info->video_rate,info->video_start,
           info->audio_scale,info->audio_rate,info->audio_start,info->audio_initial_frames,
           info->pcm_rate,info->pcm_channels,(unsigned long)info->pcm_frames,
           seeks[0],seeks[1],seeks[2],seeks[3],seeks[4],seeks[5],seeks[6]);
    dd2_avi_close(movie);return 0;
}
