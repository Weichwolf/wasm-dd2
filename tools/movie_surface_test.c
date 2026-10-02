/* Real Win32 StretchDIBits RGB32 -> RGB565 -> display RGB versus the movie
 * surface conversion. Input is an already verified 320x192 RGB24 frame. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include "dd2_movie_platform.h"
#endif
static void require(int ok,const char* reason){if(!ok){fprintf(stderr,"movie surface: %s\n",reason);exit(1);}}
int main(int argc,char** argv){
    uint8_t* rgb=malloc(320*192*3);
    uint32_t* argb=malloc(640*480*4);
    int32_t rectangles[][4]={{0,48,640,384},{0,0,320,192},{11,7,640,384},{-7,-3,640,384}};
    FILE *file,*out;
    unsigned n;
#ifdef _WIN32
    struct {BITMAPINFOHEADER h;DWORD masks[3];} dst={0};
    BITMAPINFO input={0},expanded={0};
    HDC screen=GetDC(NULL),dc=CreateCompatibleDC(screen);
    uint16_t* pixels;
    uint32_t* bgr=malloc(320*192*4);
    HBITMAP bitmap;HGDIOBJ old;
    unsigned x,y;
#endif
    require(argc==3 && rgb && argb,"arguments/allocations");
    file=fopen(argv[1],"rb");require(file!=NULL,"input");
    require(fread(rgb,1,320*192*3,file)==320*192*3 && fgetc(file)==EOF,"complete RGB source");fclose(file);
    out=fopen(argv[2],"wb");require(out!=NULL,"output");
#ifdef _WIN32
    require(screen && dc && bgr,"Win32 objects");
    dst.h.biSize=sizeof(dst.h);dst.h.biWidth=640;dst.h.biHeight=-480;
    dst.h.biPlanes=1;dst.h.biBitCount=16;dst.h.biCompression=BI_BITFIELDS;
    dst.masks[0]=0xf800;dst.masks[1]=0x07e0;dst.masks[2]=0x001f;
    bitmap=CreateDIBSection(dc,(BITMAPINFO*)&dst,DIB_RGB_COLORS,(void**)&pixels,NULL,0);
    require(bitmap!=NULL,"RGB565 DIB");
    input.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);input.bmiHeader.biWidth=320;input.bmiHeader.biHeight=192;
    input.bmiHeader.biPlanes=1;input.bmiHeader.biBitCount=32;input.bmiHeader.biCompression=BI_RGB;
    expanded.bmiHeader=input.bmiHeader;expanded.bmiHeader.biWidth=640;expanded.bmiHeader.biHeight=-480;
    for(y=0;y<192;y++)for(x=0;x<320;x++) {
        const uint8_t* p=rgb+(y*320+x)*3;
        bgr[(191-y)*320+x]=(uint32_t)p[0]<<16|(uint32_t)p[1]<<8|p[2];
    }
#endif
    for(n=0;n<sizeof(rectangles)/sizeof(rectangles[0]);n++){
#ifdef _WIN32
        int32_t* r=rectangles[n];
        memset(pixels,0,640*480*2);old=SelectObject(dc,bitmap);
        require(StretchDIBits(dc,r[0],r[1],r[2],r[3],0,0,320,192,bgr,&input,DIB_RGB_COLORS,SRCCOPY)!=(int)GDI_ERROR,"actual StretchDIBits");
        GdiFlush();SelectObject(dc,old);
        require(GetDIBits(dc,bitmap,0,480,argb,&expanded,DIB_RGB_COLORS)==480,"actual display expansion");
        for(x=0;x<640*480;x++)argb[x]|=0xff000000u;
#else
        require(!dd2_movie_render(rgb,320,192,rectangles[n],argb),"portable movie surface");
#endif
        require(fwrite(argb,4,640*480,out)==640*480,"write full display frame");
    }
    require(!fclose(out),"close display frames");
#ifdef _WIN32
    DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(NULL,screen);free(bgr);
#endif
    free(rgb);free(argb);puts("{\"rectangles\":4,\"argb_bytes\":4915200}");return 0;
}
