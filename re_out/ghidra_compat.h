// Ghidra-decompilation compatibility layer: types + macros so re_out/dd2_decomp.c can compile.
// First milestone of "refactor that C into a clean, compilable codebase" (toward bit-identical repro).
#ifndef GHIDRA_COMPAT_H
#define GHIDRA_COMPAT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
// (decompiled MSVC CRT functions that touch FILE internals are excluded from the build; libc provides stdio)

// x86 calling-convention keywords (no-ops on a flat target)
#define __cdecl
#define __stdcall
#define __fastcall
#define __thiscall

// Win32 types Ghidra emits (the PC port uses DDRAW/DSOUND/WINMM).
typedef uint32_t DWORD; typedef uint16_t WORD; typedef uint8_t BYTE;
typedef int32_t  LONG;  typedef int32_t  BOOL; typedef uint32_t UINT;
typedef void*    HANDLE; typedef void* HWND; typedef void* HDC; typedef char* LPSTR; typedef const char* LPCSTR;
typedef uint32_t MMRESULT; typedef uint32_t MCIERROR; typedef uint32_t* LPDWORD; typedef void* HLOCAL;
typedef uint32_t SIZE_T; typedef int32_t LRESULT; typedef void* LPVOID; typedef char* LPCH; typedef uint32_t UINT_PTR;
typedef void* HMODULE; typedef void* LPSECURITY_ATTRIBUTES; typedef uint32_t DWORD_PTR; typedef void* LPOVERLAPPED;
typedef uint32_t MCIDEVICEID; typedef void* PINPUT_RECORD; typedef void* LPTIMECALLBACK; typedef void* LPTHREAD_START_ROUTINE;
typedef void* LPJOYINFO; typedef void* LPJOYCAPSA; typedef void* _StartAddress; typedef void* HINSTANCE; typedef void* HKEY;
typedef void* LPWORD; typedef void* LPMMTIME; typedef void* LPWAVEFORMATEX; typedef void* LPHWAVEOUT; typedef void* HWAVEOUT;
struct _exception { int type; char* name; double arg1, arg2, retval; };
typedef void *HGLOBAL,*HMENU,*HBRUSH,*HICON,*HCURSOR,*HPALETTE,*HGDIOBJ,*HFONT,*HBITMAP,*HRGN,*HRSRC,*HGLRC,*HACCEL,*HMETAFILE,*HWAVEIN,*HMIDIOUT,*LPMSG,*LPPAINTSTRUCT,*FARPROC,*WNDPROC,*LPCRITICAL_SECTION;
typedef uint32_t WPARAM,LPARAM,COLORREF,ATOM,HFILE,HRESULT,WAVEHDR;
// Win32 structs the decompiled shim-layer code touches (functions get replaced by SDL3/WebGL shims)
typedef struct { uint32_t style; void* lpfnWndProc; int cbClsExtra,cbWndExtra; void *hInstance,*hIcon,*hCursor,*hbrBackground; char *lpszMenuName,*lpszClassName; } WNDCLASSA, WNDCLASS;
typedef struct { int left,top,right,bottom; } RECT, *LPRECT;
typedef struct { int x,y; } POINT;
typedef struct { uint32_t dwSize,dwFlags; int dwWidth,dwHeight; void* lpSurface; uint32_t dw[16]; } DDSURFACEDESC;

typedef uint8_t  undefined;
typedef uint8_t  undefined1;
typedef uint16_t undefined2;
typedef uint32_t undefined4;
typedef uint32_t undefined3;
typedef int32_t int3; typedef uint32_t uint3;
typedef int64_t int5,int6,int7; typedef uint64_t uint5,uint6,uint7;
typedef uint64_t undefined5;
typedef uint64_t undefined6;
typedef uint64_t undefined7;
typedef uint64_t undefined8;
typedef long double float10;
typedef struct { uint8_t b[10]; } unkbyte10;
typedef uint8_t  byte;
#ifndef __USE_MISC   /* when the system (sys/types via __USE_MISC) defines these, don't clash */
typedef uint16_t ushort;
typedef uint32_t uint;
typedef uint32_t ulong;
#endif
typedef int64_t  longlong;
typedef uint64_t ulonglong;
typedef int      code();   // function type: code* is a callable function pointer

// CONCAT: build a wider value from parts (little-endian byte/halfword concat as Ghidra emits).
#define CONCAT11(a,b)  (((uint16_t)(uint8_t)(a)<<8)|(uint8_t)(b))
#define CONCAT22(a,b)  (((uint32_t)(uint16_t)(a)<<16)|(uint16_t)(b))
#define CONCAT44(a,b)  (((uint64_t)(uint32_t)(a)<<32)|(uint32_t)(b))
#define CONCAT13(a,b)  (((uint32_t)(uint8_t)(a)<<24)|((b)&0xffffff))
#define CONCAT31(a,b)  (((uint32_t)((a)&0xffffff)<<8)|(uint8_t)(b))
#define CONCAT12(a,b)  (((uint32_t)(uint8_t)(a)<<16)|(uint16_t)(b))
#define CONCAT21(a,b)  (((uint32_t)(uint16_t)(a)<<8)|(uint8_t)(b))
// SUB: extract a narrower value at a byte offset.
#define SUB41(x,o)     ((uint8_t)((uint32_t)(x)>>((o)*8)))
#define SUB42(x,o)     ((uint16_t)((uint32_t)(x)>>((o)*8)))
#define SUB84(x,o)     ((uint32_t)((uint64_t)(x)>>((o)*8)))
#define ZEXT14(x)      ((uint32_t)(uint8_t)(x))
#define ZEXT24(x)      ((uint32_t)(uint16_t)(x))
#define ZEXT48(x)      ((uint64_t)(uint32_t)(x))
#define SEXT14(x)      ((int32_t)(int8_t)(x))
#define SEXT24(x)      ((int32_t)(int16_t)(x))

#endif
