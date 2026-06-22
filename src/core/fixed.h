// Q12 fixed-point math for the faithful DD2 physics/GTE (per docs/spec/02,04).
// Conventions reconstructed from the decompile: positions/velocities Q12 (1.0 = 4096),
// angles are 0..0xFFF = full circle (0x1000 = 360deg, 0x800 = 180deg), products >>12.
// Original implementation (standard fixed-point); used to make the sim integer-deterministic
// like the original rather than float/free-body.
#ifndef DD_FIXED_H
#define DD_FIXED_H
#include <stdint.h>

typedef int32_t fx;            // Q12 fixed-point
#define FX_ONE   4096
#define FX_SHIFT 12

static inline fx  fx_from_int(int v){ return (fx)(v << FX_SHIFT); }
static inline int fx_to_int(fx v){ return v >> FX_SHIFT; }
static inline fx  fx_mul(fx a, fx b){ return (fx)(((int64_t)a * b) >> FX_SHIFT); }
static inline fx  fx_div(fx a, fx b){ return b ? (fx)(((int64_t)a << FX_SHIFT) / b) : 0; }

// Angle: 0..0xFFF = full circle. Wrap to signed [-0x800, 0x800).
#define ANG_FULL  0x1000
#define ANG_HALF  0x800
static inline int ang_wrap(int a){ return ((a + ANG_HALF) & 0xFFF) - ANG_HALF; }

// Q12 sine table over the 0x1000-step circle (256-entry, interpolated). cos = sin(a + 0x400).
extern const int16_t fx_sintab[256];   // defined in fixed.c; values in [-4096,4096]
static inline fx fx_sin(int ang){
    ang &= 0xFFF;
    int i = (ang >> 4) & 0xFF;          // 0x1000/256 = 0x10 per entry
    int f = ang & 0xF;
    int a = fx_sintab[i], b = fx_sintab[(i + 1) & 0xFF];
    return a + ((b - a) * f >> 4);      // linear interp between table entries
}
static inline fx fx_cos(int ang){ return fx_sin(ang + 0x400); }

// clamp helper (steering rate ±0x200, etc.)
static inline int iclamp(int v, int lo, int hi){ return v < lo ? lo : (v > hi ? hi : v); }

#endif
