/* Real Win32 file shims over emscripten libc: the decompiled MSVC CRT file layer (sopen, FUN_004572f8...)
   reads Dirinfo assets through these. Handle = FILE*; fd table mirrors FUN_004572f8's _DAT_00908590[fd*4]. */
#include <stdio.h>
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
