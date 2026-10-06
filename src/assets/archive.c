#include "assets/archive.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_DIRECTORY_BYTES = 0x2808,
    DD2_DIRECTORY_ROW_BYTES = 24,
    DD2_DIRECTORY_ROWS = DD2_DIRECTORY_BYTES / DD2_DIRECTORY_ROW_BYTES,
    DD2_SECTOR_BYTES = 2048,
    DD2_FIRST_PAYLOAD_SECTOR = 5,
    DD2_NAME_BYTES = 18,
    DD2_SIZE_OFFSET = 20,
    DD2_BYTE_SHIFT = 8,
    DD2_ASCII_FIRST = 32,
    DD2_ASCII_LAST = 126
};

struct dd2_archive {
    size_t count;
    dd2_asset entries[DD2_DIRECTORY_ROWS];
};

static uint32_t dd2_read_u32(const uint8_t *bytes) {
    uint32_t value = 0;
    for (unsigned index = 0; index < sizeof(value); ++index) {
        value |= (uint32_t)bytes[index] << (index * DD2_BYTE_SHIFT);
    }
    return value;
}

static bool dd2_archive_name(const uint8_t *row, char *name) {
    for (size_t index = 0; index < DD2_NAME_BYTES; ++index) {
        if (row[index] == 0) {
            name[index] = '\0';
            return true;
        }
        if (row[index] < DD2_ASCII_FIRST || row[index] > DD2_ASCII_LAST) {
            return false;
        }
        name[index] = (char)row[index];
    }
    return false;
}

static dd2_archive_status dd2_archive_directory(dd2_archive *archive, const uint8_t *bytes,
                                                size_t size) {
    for (size_t index = 0; index < DD2_DIRECTORY_ROWS; ++index) {
        const uint8_t *row = bytes + (index * DD2_DIRECTORY_ROW_BYTES);
        if (row[0] == 0) {
            return DD2_ARCHIVE_OK;
        }
        dd2_asset *asset = &archive->entries[index];
        if (!dd2_archive_name(row, asset->name)) {
            return DD2_ARCHIVE_INVALID_NAME;
        }
        for (size_t previous = 0; previous < index; ++previous) {
            if (strcmp(asset->name, archive->entries[previous].name) == 0) {
                return DD2_ARCHIVE_DUPLICATE_NAME;
            }
        }
        const size_t sector =
            (size_t)row[DD2_NAME_BYTES] | ((size_t)row[DD2_NAME_BYTES + 1] << DD2_BYTE_SHIFT);
        const size_t offset = sector * DD2_SECTOR_BYTES;
        asset->size = dd2_read_u32(row + DD2_SIZE_OFFSET);
        if (sector < DD2_FIRST_PAYLOAD_SECTOR || offset > size || asset->size > size - offset) {
            return DD2_ARCHIVE_INVALID_EXTENT;
        }
        asset->bytes = bytes + offset;
        archive->count = index + 1;
    }
    return DD2_ARCHIVE_MISSING_TERMINATOR;
}

dd2_archive_status dd2_archive_open(const uint8_t *bytes, size_t size, dd2_archive **result) {
    if (result == NULL) {
        return DD2_ARCHIVE_INVALID_ARGUMENT;
    }
    *result = NULL;
    if (bytes == NULL) {
        return DD2_ARCHIVE_INVALID_ARGUMENT;
    }
    if (size < DD2_DIRECTORY_BYTES) {
        return DD2_ARCHIVE_TRUNCATED_DIRECTORY;
    }
    dd2_archive *archive = calloc(1, sizeof(*archive));
    if (archive == NULL) {
        return DD2_ARCHIVE_OUT_OF_MEMORY;
    }
    const dd2_archive_status status = dd2_archive_directory(archive, bytes, size);
    if (status != DD2_ARCHIVE_OK) {
        free(archive);
        return status;
    }
    *result = archive;
    return DD2_ARCHIVE_OK;
}

void dd2_archive_close(dd2_archive *archive) {
    free(archive);
}

size_t dd2_archive_count(const dd2_archive *archive) {
    return archive != NULL ? archive->count : 0;
}

bool dd2_archive_entry(const dd2_archive *archive, size_t index, dd2_asset *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_asset){0};
    if (archive == NULL || index >= archive->count) {
        return false;
    }
    *result = archive->entries[index];
    return true;
}

bool dd2_archive_find(const dd2_archive *archive, const char *name, dd2_asset *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_asset){0};
    if (archive == NULL || name == NULL) {
        return false;
    }
    for (size_t index = 0; index < archive->count; ++index) {
        if (strcmp(name, archive->entries[index].name) == 0) {
            *result = archive->entries[index];
            return true;
        }
    }
    return false;
}
