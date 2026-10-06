#ifndef DD2_ASSETS_LZ_H
#define DD2_ASSETS_LZ_H

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>

/* Decode a complete original object stream, including its little-endian size
 * word. Overlapping backward copies are intentional. Invalid/truncated streams
 * clear written but may have changed output bytes. Source/output must not overlap. */
bool dd2_lz_decode(dd2_byte_view source, dd2_byte_buffer output, size_t *written);

#endif
