/* Cinepak source decoder for the original DD2 AVI assets.
 * Packet/codebook/vector structure and RGB arithmetic checked against Wine
 * 10's public Win32 ICCVID codec, dlls/iccvid/iccvid.c:
 * https://github.com/wine-mirror/wine/blob/wine-10.0/dlls/iccvid/iccvid.c
 * In particular, chroma U rounds upward when halved for green; C's signed
 * division alone differs for positive odd U. This is source-frame decoding,
 * separate from the MCI transport, output scaling and movie audio. */
#include "dd2_cinepak.h"
#include <stdlib.h>
#include <string.h>

struct DD2Cinepak {
    unsigned width,height;
    uint8_t *pixels;
    uint8_t book[32][2][256][4][3];
};
typedef struct { const uint8_t *at,*end;uint32_t flags;unsigned bits;int error; } Reader;
static unsigned take(Reader* reader,unsigned bytes) {
    unsigned value=0;
    if((size_t)(reader->end-reader->at)<bytes){reader->error=1;return 0;}
    while(bytes--)value=value*256+*reader->at++;
    return value;
}
static unsigned bit(Reader* reader) {
    unsigned value;
    if(!reader->bits){reader->flags=take(reader,4);reader->bits=32;}
    value=reader->flags>>31;reader->flags<<=1;reader->bits--;
    return value;
}
static uint8_t clip(int value) { return value<0 ? 0 : value>255 ? 255 : value; }
DD2Cinepak* dd2_cinepak_create(unsigned width,unsigned height) {
    DD2Cinepak* decoder;
    if(!width || !height || width>65535 || height>65535
            || (size_t)width>SIZE_MAX/height/3)return NULL;
    decoder=calloc(1,sizeof(*decoder));if(!decoder)return NULL;
    decoder->pixels=calloc((size_t)width*height,3);
    if(!decoder->pixels){free(decoder);return NULL;}
    decoder->width=width;decoder->height=height;return decoder;
}
void dd2_cinepak_destroy(DD2Cinepak* decoder) {
    if(decoder){free(decoder->pixels);free(decoder);}
}
const uint8_t* dd2_cinepak_pixels(const DD2Cinepak* decoder) { return decoder->pixels; }
static int codebook(DD2Cinepak* decoder,unsigned strip,unsigned kind,Reader* reader) {
    unsigned index,q,component;
    int u,v,half,y[4];
    int sparse=(kind&0x100)!=0,gray=(kind&0x400)!=0;
    unsigned bank=(kind&0x200)!=0;
    for(index=0;index<256 && reader->at<reader->end;index++) {
        if(sparse && !bit(reader))continue;
        for(q=0;q<4;q++)y[q]=take(reader,1);
        u=v=0;
        if(!gray) {
            component=take(reader,1);u=component<128 ? (int)component : (int)component-256;
            component=take(reader,1);v=component<128 ? (int)component : (int)component-256;
        }
        half=u>=0 ? (u+1)/2 : u/2;
        for(q=0;q<4;q++) {
            decoder->book[strip][bank][index][q][0]=clip(y[q]+2*v);
            decoder->book[strip][bank][index][q][1]=clip(y[q]-half-v);
            decoder->book[strip][bank][index][q][2]=clip(y[q]+2*u);
        }
        if(reader->error)return -1;
    }
    return reader->error || reader->at!=reader->end ? -1 : 0;
}
static void block(DD2Cinepak* decoder,unsigned strip,unsigned x,unsigned y,
                  unsigned indices[4],unsigned four) {
    unsigned row,col,quadrant,sample;
    const uint8_t* color;
    for(row=0;row<4 && y+row<decoder->height;row++)
        for(col=0;col<4 && x+col<decoder->width;col++) {
            quadrant=(row/2)*2+col/2;
            sample=four ? (row%2)*2+col%2 : quadrant;
            color=decoder->book[strip][four ? 0 : 1][indices[four ? quadrant : 0]][sample];
            memcpy(decoder->pixels+((size_t)(y+row)*decoder->width+x+col)*3,color,3);
        }
}
static int vectors(DD2Cinepak* decoder,unsigned strip,unsigned kind,Reader* reader,
                    unsigned top,unsigned bottom) {
    unsigned x,y,four,indices[4],n;
    for(y=top;y<bottom;y+=4)for(x=0;x<decoder->width;x+=4) {
        if(kind==0x3100 && !bit(reader)) {
            if(reader->error)return -1;
            continue;
        }
        four=kind!=0x3200 && bit(reader);
        for(n=0;n<(four ? 4u : 1u);n++)indices[n]=take(reader,1);
        if(reader->error)return -1;
        block(decoder,strip,x,y,indices,four);
    }
    /* The declared vector chunk can include bytes after its last tile,
     * including the AVI encoder's odd-payload pad. ICCVID skips that tail. */
    return reader->error ? -1 : 0;
}
int dd2_cinepak_decode(DD2Cinepak* decoder,const uint8_t* packet,size_t bytes) {
    Reader frame;
    unsigned flags,length,width,height,strips,strip,bottom=0;
    if(!decoder)return -1;
    if(!bytes)return 0; /* Empty AVI packets hold the previous decoded frame. */
    if(!packet || bytes<10)return -1;
    frame=(Reader){packet,packet+bytes,0,0,0};
    flags=take(&frame,1);length=take(&frame,3);
    width=take(&frame,2);height=take(&frame,2);strips=take(&frame,2);
    if(width!=decoder->width || height!=decoder->height || strips>32
            || length<10 || length>bytes || bytes-length>(length&1))return -1;
    frame.end=packet+length;
    for(strip=0;strip<strips;strip++) {
        Reader content;
        const uint8_t* begin=frame.at;
        unsigned id,size,top=bottom,y0,x0,y1,x1;
        if((size_t)(frame.end-begin)<12)return -1;
        id=take(&frame,2);size=take(&frame,2);
        y0=take(&frame,2);x0=take(&frame,2);y1=take(&frame,2);x1=take(&frame,2);
        if((id!=0x1000 && id!=0x1100) || size<12 || size>(size_t)(frame.end-begin)
                || y0 || x0 || x1!=width || y1>height-bottom)return -1;
        bottom+=y1;
        if(strip && !(flags&1))memcpy(decoder->book[strip],decoder->book[strip-1],sizeof(decoder->book[strip]));
        content=(Reader){frame.at,begin+size,0,0,0};frame.at=content.end;
        while(content.at<content.end) {
            Reader chunk;
            unsigned kind,chunk_bytes;
            if((size_t)(content.end-content.at)<4)return -1;
            kind=take(&content,2);chunk_bytes=take(&content,2);
            if(chunk_bytes<4 || chunk_bytes-4>(size_t)(content.end-content.at))return -1;
            chunk=(Reader){content.at,content.at+chunk_bytes-4,0,0,0};content.at=chunk.end;
            if(kind>=0x2000 && kind<=0x2700 && !(kind&255)) {
                if(codebook(decoder,strip,kind,&chunk))return -1;
            } else if(kind==0x3000 || kind==0x3100 || kind==0x3200) {
                if(vectors(decoder,strip,kind,&chunk,top,bottom))return -1;
            } else return -1;
        }
    }
    return frame.at!=frame.end || bottom!=height ? -1 : 0;
}
