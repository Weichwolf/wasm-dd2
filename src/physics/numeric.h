#ifndef DD2_PHYSICS_NUMERIC_H
#define DD2_PHYSICS_NUMERIC_H

#include <float.h>
#include <stdbool.h>
#include <stdint.h>

enum { DD2_BINARY64_MANTISSA = 53, DD2_BINARY64_EXPONENT = 1024 };
_Static_assert(sizeof(double) == sizeof(uint64_t) && DBL_MANT_DIG == DD2_BINARY64_MANTISSA &&
                   DBL_MAX_EXP == DD2_BINARY64_EXPONENT,
               "Native/WASM physics requires IEEE binary64");

/* A floating argument/bitcast can also be folded under -ffast-math. Read object
 * bytes into an integer without floating operations or a floating parameter.
 * Character accesses are permitted by C11 aliasing rules. */
static inline bool dd2_numeric_finite(const double *value) {
    uint64_t representation = 0;
    unsigned char *target = (unsigned char *)&representation;
    const unsigned char *source = (const unsigned char *)value;
    for (unsigned index = 0; index < sizeof(representation); ++index) {
        target[index] = source[index];
    }
    const uint64_t exponent = UINT64_C(0x7ff0000000000000);
    return (representation & exponent) != exponent;
}

#endif
