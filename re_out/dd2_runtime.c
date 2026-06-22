#include "ghidra_compat.h"
#include <stdio.h>
#include <stdlib.h>
unsigned char* g_image = 0;   // loaded image of dd2.exe at base 0x400000 (data segment)
void dd2_load_image(const char* path){
    FILE* f = fopen(path,"rb"); if(!f){ g_image=calloc(0x540000,1); return; }
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    g_image = malloc(n>0x540000?n:0x540000); fread(g_image,1,n,f); fclose(f);
}
