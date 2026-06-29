#ifndef X86_INTRIN_H
#define X86_INTRIN_H
/* ============================================================================
 * x86_intrin.h  —  map x86 instruction semantics to portable C.
 *
 * PURPOSE (per project direction): the deliverable is a C + SDL3 program; native
 * x86 is the real target and WASM is just an emcc build target. Most of the engine
 * is Ghidra's decompiled C. The handful of functions Ghidra CANNOT cleanly decompile
 * — the GTE fixed-point math and the register-ABI helpers it mangles into `unaff_EBX`
 * — are transcribed from Ghidra's x86 ASM into C *using these intrinsics*.
 *
 * This is NOT a CPU emulator (no register file, no MEM array, no P-code interpreter).
 * Each macro is the plain-C equivalent of one x86 instruction's value semantics, so a
 * transcribed function reads like the disassembly yet compiles to native AND wasm32.
 *
 * Only model the bits the engine actually relies on: 32-bit two's-complement ints,
 * arithmetic vs logical shifts, 64-bit IMUL intermediates (Q-format math + perspective
 * divide), and the few GTE "register" carriers Ghidra dropped. Flags are added only
 * where a transcribed function genuinely branches on them.
 * ========================================================================== */
#include <stdint.h>

typedef int32_t  i32;
typedef uint32_t u32;
typedef int64_t  i64;
typedef uint64_t u64;
typedef int16_t  i16;

/* ---- pointer<->VA: the image lives at its original VAs (0x400000..) in both targets
 * (wasm: memcpy'd there with GLOBAL_BASE above; native: mmap MAP_FIXED at 0x400000). ---- */
#define VA(t, a)   ((t *)(uintptr_t)(a))

/* ---- arithmetic vs logical shift (x86 SAR vs SHR); shifts are well-defined here ---- */
#define SAR32(a, n)  ((i32)(a) >> (n))            /* SAR r/m32, n  (sign-replicating) */
#define SHR32(a, n)  ((u32)(a) >> (n))            /* SHR r/m32, n  (zero-filling)     */
#define SHL32(a, n)  ((i32)((u32)(a) << (n)))     /* SHL/SAL r/m32, n                 */

/* ---- IMUL: 32x32 producing a 64-bit intermediate (EDX:EAX). The GTE keeps full
 * precision then shifts. `IMULHI` takes the high dword (EDX) as a signed divide proxy. ---- */
#define IMUL64(a, b)   ((i64)(i32)(a) * (i64)(i32)(b))           /* IMUL -> EDX:EAX (signed)   */
#define MUL64(a, b)    ((u64)(u32)(a) * (u64)(u32)(b))           /* MUL  -> EDX:EAX (unsigned) */
#define IMULHI(a, b)   ((i32)(IMUL64(a, b) >> 32))               /* EDX after IMUL             */

/* ---- the GTE workhorse: Q12 fixed-point multiply (IMUL r32 then SAR 12). Operands are
 * matrix shorts and vector shorts; the 32-bit product never overflows the engine's range,
 * matching x86's `imul eax, r/m ; sar eax, 0Ch`. Use Q12_64 when full 64-bit is required. ---- */
#define Q12_MUL(a, b)   SAR32((i32)(a) * (i32)(b), 12)
#define Q_MUL(a, b, q)  SAR32((i32)(a) * (i32)(b), (q))
#define Q12_MUL64(a, b) ((i32)(IMUL64(a, b) >> 12))   /* when the product can exceed 31 bits */

/* ---- SUBPIECE / register sub-fields (AL/AX inside EAX) without a register file ---- */
#define LO16(x)   ((u16)(u32)(x))
#define LO8(x)    ((u8)(u32)(x))
#define HI16(x)   ((u16)((u32)(x) >> 16))
typedef uint16_t u16;
typedef uint8_t  u8;

/* ---- the GTE register-ABI carriers Ghidra dropped (it read them as unaff_ESI/EDI/EBP/EBX).
 * The GTE primitives FUN_00413fd2 / 00414055 / 00413f45 pass pointers between each other in
 * these regs; the callers (GTERPS/GTERPT/...) set them before each call. Declaring them here
 * makes that recovered ABI explicit and shared across the GTE translation unit. ---- */
extern i32 _g_esi, _g_edi, _g_ebp, _g_ebx;   /* defined once in the GTE unit */
extern i32 __flg;                            /* GTE flag/accumulator latch (FLAG-like) */

#endif /* X86_INTRIN_H */
