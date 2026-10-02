/* Actual Wine Win32 ICCVID versus the portable source decoder. Input contains
 * the sequential original AVI compressed packets; output is canonical RGB24.
 * Normalizing Win32's bottom-up BGR DIB only changes storage layout, not values. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <vfw.h>
#else
#include "dd2_cinepak.h"
#endif
static void require(int ok,const char* why) {
    if(!ok){fprintf(stderr,"movie codec: %s\n",why);exit(1);}
}
static uint32_t read32(FILE* file) {
    unsigned char bytes[4];
    require(fread(bytes,1,4,file)==4,"read bundle field");
    return bytes[0]|(uint32_t)bytes[1]<<8|(uint32_t)bytes[2]<<16|(uint32_t)bytes[3]<<24;
}
int main(int argc,char** argv) {
    FILE *bundle,*capture;
    unsigned width,height,frames,i,empty=0;
    size_t bytes;
#ifdef _WIN32
    HIC codec;
    BITMAPINFOHEADER input={0},output={0};
    unsigned char *dib,*rgb;
#else
    DD2Cinepak* codec;
#endif
    require(argc==3,"bundle and output paths required");
    bundle=fopen(argv[1],"rb");require(bundle!=NULL,"open bundle");
    capture=fopen(argv[2],"wb");require(capture!=NULL,"open decoded output");
    width=read32(bundle);height=read32(bundle);frames=read32(bundle);
    require(width && height && width<=65535 && height<=65535 && (size_t)width<=SIZE_MAX/height/3,"valid dimensions");
    bytes=(size_t)width*height*3;
#ifdef _WIN32
    require(width%4==0,"fixture's unpadded Win32 RGB24 rows");
    input.biSize=sizeof(input);input.biWidth=width;input.biHeight=height;
    input.biPlanes=1;input.biBitCount=24;input.biCompression=mmioFOURCC('c','v','i','d');
    output=input;output.biCompression=BI_RGB;output.biSizeImage=bytes;
    dib=calloc(1,bytes);rgb=malloc(bytes);require(dib && rgb,"allocate DIB/RGB");
    codec=ICOpen(ICTYPE_VIDEO,input.biCompression,ICMODE_DECOMPRESS);require(codec!=NULL,"open actual ICCVID");
    require(ICDecompressQuery(codec,&input,&output)==ICERR_OK,"query actual RGB24 codec");
    require(ICDecompressBegin(codec,&input,&output)==ICERR_OK,"begin actual codec");
#else
    codec=dd2_cinepak_create(width,height);require(codec!=NULL,"create portable decoder");
#endif
    for(i=0;i<frames;i++) {
        uint32_t length=read32(bundle);
        unsigned char* packet=malloc(length ? length : 1);
        require(packet!=NULL && fread(packet,1,length,bundle)==length,"read original compressed packet");
        if(!length)empty++;
#ifdef _WIN32
        if(length) {
            input.biSizeImage=length;
            require(ICDecompress(codec,packet[0]&1 ? ICDECOMPRESS_NOTKEYFRAME : 0,
                                &input,packet,&output,dib)==ICERR_OK,"actual codec frame");
        }
        {
            unsigned x,y;
            for(y=0;y<height;y++)for(x=0;x<width;x++) {
                const unsigned char* p=dib+((size_t)(height-1-y)*width+x)*3;
                unsigned char* q=rgb+((size_t)y*width+x)*3;
                q[0]=p[2];q[1]=p[1];q[2]=p[0];
            }
        }
        require(fwrite(rgb,1,bytes,capture)==bytes,"write actual RGB frame");
#else
        if(dd2_cinepak_decode(codec,packet,length)) {
            fprintf(stderr,"movie codec: rejected frame %u (%u compressed bytes)\n",i,length);return 1;
        }
        require(fwrite(dd2_cinepak_pixels(codec),1,bytes,capture)==bytes,"write portable RGB frame");
#endif
        free(packet);
    }
    require(fgetc(bundle)==EOF,"bundle ends after declared frames");
    require(fclose(capture)==0 && fclose(bundle)==0,"close streams");
#ifdef _WIN32
    require(ICDecompressEnd(codec)==ICERR_OK,"end actual codec");
    require(ICClose(codec)==ICERR_OK,"close actual codec");free(dib);free(rgb);
#else
    dd2_cinepak_destroy(codec);
#endif
    printf("{\"width\":%u,\"height\":%u,\"frames\":%u,\"empty_packets\":%u}\n",width,height,frames,empty);
    return 0;
}
