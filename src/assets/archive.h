#ifndef DD2_ASSETS_ARCHIVE_H
#define DD2_ASSETS_ARCHIVE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct dd2_archive dd2_archive;

enum { DD2_ASSET_NAME_BYTES = 18 };

typedef enum {
    DD2_ARCHIVE_OK,
    DD2_ARCHIVE_INVALID_ARGUMENT,
    DD2_ARCHIVE_TRUNCATED_DIRECTORY,
    DD2_ARCHIVE_INVALID_NAME,
    DD2_ARCHIVE_DUPLICATE_NAME,
    DD2_ARCHIVE_INVALID_EXTENT,
    DD2_ARCHIVE_MISSING_TERMINATOR,
    DD2_ARCHIVE_OUT_OF_MEMORY
} dd2_archive_status;

typedef struct {
    char name[DD2_ASSET_NAME_BYTES];
    const uint8_t *bytes;
    size_t size;
} dd2_asset;

/* The archive borrows immutable bytes, which must outlive it and every asset
 * view. Opening validates the complete directory before exposing any entries.
 * Names use the original uppercase spelling and backslash separators. Lookup
 * is exact, as in the original; no filesystem paths are opened here. */
dd2_archive_status dd2_archive_open(const uint8_t *bytes, size_t size, dd2_archive **result);
void dd2_archive_close(dd2_archive *archive);
size_t dd2_archive_count(const dd2_archive *archive);
bool dd2_archive_entry(const dd2_archive *archive, size_t index, dd2_asset *result);
bool dd2_archive_find(const dd2_archive *archive, const char *name, dd2_asset *result);

#endif
