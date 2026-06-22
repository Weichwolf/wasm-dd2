// Ghidra-decompilation compatibility layer: types + macros so re_out/dd2_decomp.c can compile.
// First milestone of "refactor that C into a clean, compilable codebase" (toward bit-identical repro).
#ifndef GHIDRA_COMPAT_H
#define GHIDRA_COMPAT_H
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// Win32 types Ghidra emits (the PC port uses DDRAW/DSOUND/WINMM).
typedef uint32_t DWORD; typedef uint16_t WORD; typedef uint8_t BYTE;
typedef int32_t  LONG;  typedef int32_t  BOOL; typedef uint32_t UINT;
typedef void*    HANDLE; typedef void* HWND; typedef void* HDC; typedef char* LPSTR; typedef const char* LPCSTR;

typedef uint8_t  undefined;
typedef uint8_t  undefined1;
typedef uint16_t undefined2;
typedef uint32_t undefined4;
typedef uint64_t undefined8;
typedef uint8_t  byte;
typedef uint16_t ushort;
typedef uint32_t uint;
typedef uint32_t ulong;
typedef int64_t  longlong;
typedef uint64_t ulonglong;
typedef void     code;   // function-pointer target

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
