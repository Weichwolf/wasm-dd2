#ifndef DD2_RENDER_COLOR_H
#define DD2_RENDER_COLOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Standard sRGB transfer, rounded to the nearest byte. Linear RGBA8 staging
 * retains SoftGL's limited dark-color precision; alpha is always linear. */
uint8_t dd2_color_srgb8_to_linear8(uint8_t value);
uint8_t dd2_color_linear8_to_srgb8(uint8_t value);
/* Output may equal input. Partial overlap is unsupported. No source mutation
 * occurs when the buffers differ; invalid pointers/extents fail before writes. */
bool dd2_color_present_rgba8(uint8_t *output, const uint8_t *linear, size_t bytes);

#endif
