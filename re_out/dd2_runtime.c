#include <stdio.h>
#include <stdlib.h>
#include <string.h>
unsigned char* g_image = (unsigned char*)0x400000;
extern void dd2_relocate(void);
extern int __WinMain();
void dd2_load_image(const char* path){
    FILE* f=fopen(path,"rb"); if(!f){ return; }
    fseek(f,0,SEEK_END); long n=ftell(f); fseek(f,0,SEEK_SET);
    unsigned char* tmp=malloc(n); fread(tmp,1,n,f); fclose(f);
    memcpy((void*)0x400000, tmp, n);   /* image at fixed VA so raw-VA pointers resolve */
    free(tmp);
}
int main(){ dd2_load_image("dd2_image.bin"); dd2_relocate(); return __WinMain(0,0,0,1); }
