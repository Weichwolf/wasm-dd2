/* Fixed PE32 mapping for isolated original/reconstructed engine fixtures.
 * Load the verified file bytes; never modify original machine code. */
#ifndef DD2_PE_FIXTURE_H
#define DD2_PE_FIXTURE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
#define BASE 0x400000u
#define SIZE 0x590000u
static void require(int ok,const char *message) {
    if (!ok) { fprintf(stderr,"replay fixture: %s\n",message); exit(1); }
}
static uint32_t read32(const void *p) { uint32_t n; memcpy(&n,p,4); return n; }
static uint16_t read16(const void *p) { uint16_t n; memcpy(&n,p,2); return n; }
static void put32(unsigned a,uint32_t n) { memcpy((void*)(uintptr_t)a,&n,4); }
static void put16(unsigned a,uint16_t n) { memcpy((void*)(uintptr_t)a,&n,2); }
static void load_original(const char *path) {
    FILE *f=fopen(path,"rb"); unsigned char *pe; long length;
    unsigned h,opt,table,count,i;
    require(f!=NULL,"open supported original executable");
    require(fseek(f,0,SEEK_END)==0,"seek original"); length=ftell(f);
    require(length>256 && fseek(f,0,SEEK_SET)==0,"original size");
    pe=malloc((size_t)length); require(pe!=NULL,"allocate PE input");
    require(fread(pe,1,(size_t)length,f)==(size_t)length,"read PE input"); fclose(f);
    h=read32(pe+0x3c); require(h+24u<(unsigned)length,"PE header bounds");
    require(memcmp(pe+h,"PE\0\0",4)==0 && read16(pe+h+4)==0x14c,"PE32 x86");
    opt=h+24; require(read32(pe+opt+28)==BASE && read32(pe+opt+56)==SIZE,"supported fixed image layout");
    count=read16(pe+h+6); table=opt+read16(pe+h+20);
    require(table+count*40u<=(unsigned)length,"section table bounds");
    for (i=0;i<count;i++) {
        unsigned char *section=pe+table+i*40;
        unsigned address=read32(section+12),bytes=read32(section+16),offset=read32(section+20);
        require(address<=SIZE && bytes<=SIZE-address,"mapped section bounds");
        /* Watcom's BSS declares its allocation in SizeOfRawData with no file
         * payload. The Windows loader zeroes uninitialized-data sections. */
        if (read32(section+36)&0x80u) continue;
        require(offset<=(unsigned)length && bytes<=(unsigned)length-offset,"file section bounds");
        memcpy((void*)(uintptr_t)(BASE+address),pe+offset,bytes);
    }
    free(pe);
}
static void map_original(const char *path) {
#ifndef __EMSCRIPTEN__
    require(mmap((void*)BASE,SIZE,PROT_READ|PROT_WRITE|PROT_EXEC,
                 MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,
            "map isolated engine image");
#endif
    load_original(path);
}
#endif
