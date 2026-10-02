/* Real Win32 ACM source conversion versus the portable MSADPCM decoder.
 * Bundle: uint32 format size, compressed size; then original WAVEFORMATEX
 * and concatenated complete original AVI audio packets. Output is PCM16LE. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <mmreg.h>
#include <msacm.h>
#else
#include "dd2_msadpcm.h"
#endif
static void require(int ok,const char* why) {
    if(!ok){fprintf(stderr,"movie audio: %s\n",why);exit(1);}
}
static uint32_t read32(FILE* file) {
    unsigned char bytes[4];require(fread(bytes,1,4,file)==4,"read bundle field");
    return bytes[0]|(uint32_t)bytes[1]<<8|(uint32_t)bytes[2]<<16|(uint32_t)bytes[3]<<24;
}
int main(int argc,char** argv) {
    FILE *bundle,*capture;
    unsigned format_bytes,source_bytes,channels,rate,frames;
    unsigned char *format,*source;
    int16_t* pcm;
    size_t pcm_bytes;
#ifdef _WIN32
    HACMSTREAM stream;
    WAVEFORMATEX destination={0};
    ACMSTREAMHEADER header={0};
    DWORD size;
#else
    DD2MSADPCM description;
    size_t written;
#endif
    require(argc==3,"bundle and PCM paths required");
    bundle=fopen(argv[1],"rb");require(bundle!=NULL,"open bundle");
    format_bytes=read32(bundle);source_bytes=read32(bundle);
    require(format_bytes>=22,"complete source format");
    format=malloc(format_bytes);source=malloc(source_bytes);
    require(format && source,"allocate source format/data");
    require(fread(format,1,format_bytes,bundle)==format_bytes && fread(source,1,source_bytes,bundle)==source_bytes
            && fgetc(bundle)==EOF && fclose(bundle)==0,"read complete original bundle");
#ifdef _WIN32
    destination.wFormatTag=WAVE_FORMAT_PCM;destination.nChannels=((WAVEFORMATEX*)format)->nChannels;
    destination.nSamplesPerSec=((WAVEFORMATEX*)format)->nSamplesPerSec;destination.wBitsPerSample=16;
    destination.nBlockAlign=destination.nChannels*2;
    destination.nAvgBytesPerSec=destination.nSamplesPerSec*destination.nBlockAlign;
    require(acmStreamOpen(&stream,0,(WAVEFORMATEX*)format,&destination,0,0,0,ACM_STREAMOPENF_NONREALTIME)==0,"open actual ACM");
    require(acmStreamSize(stream,source_bytes,&size,ACM_STREAMSIZEF_SOURCE)==0,"actual converted size");
    pcm=malloc(size);require(pcm!=NULL,"allocate actual PCM");
    header.cbStruct=sizeof(header);header.pbSrc=source;header.cbSrcLength=source_bytes;
    header.pbDst=(unsigned char*)pcm;header.cbDstLength=size;
    require(acmStreamPrepareHeader(stream,&header,0)==0,"prepare actual ACM conversion");
    require(acmStreamConvert(stream,&header,ACM_STREAMCONVERTF_START|ACM_STREAMCONVERTF_END|ACM_STREAMCONVERTF_BLOCKALIGN)==0,"actual ACM conversion");
    require(header.cbSrcLengthUsed==source_bytes,"actual ACM consumes every source byte");
    pcm_bytes=header.cbDstLengthUsed;channels=destination.nChannels;rate=destination.nSamplesPerSec;
    frames=pcm_bytes/destination.nBlockAlign;
    require(acmStreamUnprepareHeader(stream,&header,0)==0 && acmStreamClose(stream,0)==0,"close actual ACM");
#else
    require(dd2_msadpcm_format(&description,format,format_bytes)==0,"parse original ADPCM format");
    channels=description.channels;rate=description.rate;
    frames=source_bytes/description.block_bytes*description.samples_per_block;pcm_bytes=(size_t)frames*channels*2;
    pcm=malloc(pcm_bytes);require(pcm!=NULL,"allocate portable PCM");
    require(dd2_msadpcm_decode(&description,source,source_bytes,pcm,frames,&written)==0 && written==frames,"decode complete portable PCM");
#endif
    capture=fopen(argv[2],"wb");require(capture!=NULL,"open PCM output");
    require(fwrite(pcm,1,pcm_bytes,capture)==pcm_bytes && fclose(capture)==0,"write complete PCM output");
    printf("{\"rate\":%u,\"channels\":%u,\"frames\":%u,\"bytes\":%u}\n",rate,channels,frames,(unsigned)pcm_bytes);
    free(format);free(source);free(pcm);return 0;
}
