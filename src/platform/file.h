#ifndef DD2_PLATFORM_FILE_H
#define DD2_PLATFORM_FILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *data;
    size_t size;
} dd2_file;

/* Owns a bounded complete file read (native or browser virtual filesystem).
 * Output must be empty. Failure clears it; release is null-safe. */
bool dd2_file_read(const char *path, dd2_file *result);
void dd2_file_release(dd2_file *file);

#endif
