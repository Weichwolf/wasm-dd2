#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

enum { DD2_FILE_MAX_BYTES = 64 * 1024 * 1024 };

void dd2_file_release(dd2_file *file) {
    if (file != NULL) {
        free(file->data);
        *file = (dd2_file){0};
    }
}

bool dd2_file_read(const char *path, dd2_file *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_file){0};
    if (path == NULL) {
        return false;
    }
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    bool loaded = false;
    if (fseek(file, 0, SEEK_END) == 0) {
        const long length = ftell(file);
        if (length > 0 && length <= DD2_FILE_MAX_BYTES && fseek(file, 0, SEEK_SET) == 0) {
            result->size = (size_t)length;
            result->data = malloc(result->size);
            loaded =
                result->data != NULL && fread(result->data, 1, result->size, file) == result->size;
        }
    }
    const bool closed = fclose(file) == 0;
    if (!loaded || !closed) {
        dd2_file_release(result);
        return false;
    }
    return true;
}
