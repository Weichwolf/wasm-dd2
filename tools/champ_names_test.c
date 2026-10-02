/* Run the real patched engine's naming/text builders, with explicit fixtures.
 * This is not a replacement for live championship/menu tests. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __EMSCRIPTEN__
#include <sys/mman.h>
#endif
void Setup_Driver_Names(void);
void FUN_00453d58(void);
static void require(int ok,const char *reason){
 if(!ok){fprintf(stderr,"Championship builder test failed: %s\n",reason);exit(1);}
}
static void load_image(const char *path){
 FILE *file=fopen(path,"rb");
 require(file!=NULL,"open image");
 require(fread((void*)0x400000,1,0x580400,file)==0x580400 && fgetc(file)==EOF,"exact engine image size");
 fclose(file);
}
int main(int argc,char **argv){
 const int counts[4]={1,2,5,10};
 FILE *output;int scenario,i;
 require(argc==3 || argc==5,"image/output and optional reference/score output paths");
#ifndef __EMSCRIPTEN__
 require(mmap((void*)0x400000,0x580400,PROT_READ|PROT_WRITE,
  MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)!=(void*)-1,"map engine image");
#endif
 load_image(argv[1]);output=fopen(argv[2],"wb");require(output!=NULL,"open fixture output");
 for(scenario=0;scenario<4;scenario++){
  memset((void*)0x93dee0,0,20*54);memset((void*)0x93e318,0,10*12);
  *(int*)0x467658=counts[scenario];
  for(i=0;i<counts[scenario];i++)sprintf((char*)(0x93e318+i*12),"PLAYER%d",i+1);
  Setup_Driver_Names();require(fwrite((void*)0x93dee0,54,20,output)==20,"write name records");
 }
 require(fclose(output)==0,"close fixture output");
 if(argc==5){
  load_image(argv[3]);output=fopen(argv[4],"wb");require(output!=NULL,"open score output");
  for(scenario=0;scenario<4;scenario++){
   *(int*)0x46ad00=scenario;FUN_00453d58();
   for(i=0;i<5;i++){
    char name[26]={0},points[16]={0};
    require(strlen((char*)(0x940290+i*26))<26 && strlen((char*)(0x940240+i*16))<16,"bounded score strings");
    strcpy(name,(char*)(0x940290+i*26));strcpy(points,(char*)(0x940240+i*16));
    require(fwrite(name,1,26,output)==26 && fwrite(points,1,16,output)==16,"write scores");
   }
  }
  require(fclose(output)==0,"close score output");
 }
 puts("Real engine builders: names for 1/2/5/10 humans and optional reference score records generated");
 return 0;
}
