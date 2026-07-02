/* Real Win32 file shims over emscripten libc: the decompiled MSVC CRT file layer (sopen, FUN_004572f8...)
   reads Dirinfo assets through these. Handle = FILE*; fd table mirrors FUN_004572f8's _DAT_00908590[fd*4]. */
#include <stdio.h>

/* Windows/wine open files CASE-INSENSITIVELY; the engine passes original-case names
   ("DIRINFO", "SaveGames", ...) while the shipped files are mixed-case on a case-sensitive
   Linux FS. dd2_fopen_ci = exact fopen, then case-variant probes of the LAST path component
   (as-shipped variants: exact / Capitalized / lower / UPPER). No opendir: on the wasm build
   libc opendir()'s open() resolves to the DECOMPILED MSVC CRT sopen (symbol shadowing) and
   traps; plain fopen probes stay on the known-good path. Keeps the engine-visible name
   buffers (e.g. 0x74ef18) byte-identical to the reference. */
#include <string.h>
#include <ctype.h>
FILE* dd2_fopen_ci(const char* name, const char* mode){
  FILE* f=fopen(name,mode);
  if(f) return f;
  char buf[512];
  size_t len=strlen(name);
  if(len>=sizeof(buf)) return 0;
  memcpy(buf,name,len+1);
  char* comp=buf; char* q;
  for(q=buf;*q;q++) if(*q=='\\'||*q=='/') comp=q+1;
  size_t cl=strlen(comp);
  if(!cl) return 0;
  size_t i;
  /* Capitalized */
  comp[0]=(char)toupper((unsigned char)comp[0]);
  for(i=1;i<cl;i++) comp[i]=(char)tolower((unsigned char)comp[i]);
  if((f=fopen(buf,mode))) return f;
  /* lower */
  comp[0]=(char)tolower((unsigned char)comp[0]);
  if((f=fopen(buf,mode))) return f;
  /* UPPER */
  for(i=0;i<cl;i++) comp[i]=(char)toupper((unsigned char)comp[i]);
  if((f=fopen(buf,mode))) return f;
  return 0;
}

static int g_fdtbl[4096];
static int g_fdcnt = 0;
int CreateFileA(const char* name,int a,int b,int c,int d,int e,int f){ FILE* fp=fopen(name,"rb"); return fp?(int)(long)fp:-1; }
int __NTAddFileHandle(int handle){ *(int*)0x908590=(int)(long)g_fdtbl; int fd=g_fdcnt++; g_fdtbl[fd*4]=handle; return fd; }
int SetFilePointer(int h,int dist,void* hi,int method){ FILE* fp=(FILE*)(long)h; if(!fp||fp==(FILE*)-1)return -1; fseek(fp,dist,method); return (int)ftell(fp); }
int ReadFile(int h,void* buf,int n,int* nread,void* o){ FILE* fp=(FILE*)(long)h; if(!fp||fp==(FILE*)-1)return 0; int r=fread(buf,1,n,fp); if(nread)*nread=r; return 1; }
int WriteFile(int h,const void* buf,int n,int* nwr,void* o){ if(nwr)*nwr=n; return 1; }
int CloseHandle(int h){ FILE* fp=(FILE*)(long)h; if(fp&&fp!=(FILE*)-1)fclose(fp); return 1; }
int GetFileType(int h){ return 1; }  /* FILE_TYPE_DISK */

/* Game CRT buffered-file seek -> plain fseek on the emscripten FILE* (file layer is shimmed, not decompiled).
   FUN_00415160 does fopen()+FUN_0045607b(seek)+fread()+fclose(), all emscripten libc -> consistent. */
#include <stdio.h>
int FUN_0045607b(void* file, long offset, int whence){ return fseek((FILE*)file, offset, whence); }
int dd2_asset_off;
int dd2_asset_size;
