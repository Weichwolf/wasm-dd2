#include "assets/archive.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_TEST_BYTES = 16384,
    DD2_HEADER_BYTES = 0x2808,
    DD2_ROW_BYTES = 24,
    DD2_SECTOR_BYTES = 2048,
    DD2_NAME_BYTES = 18,
    DD2_SIZE_OFFSET = 20,
    DD2_FIRST_SECTOR = 5,
    DD2_SECOND_SECTOR = 6,
    DD2_END_SECTOR = 8,
    DD2_FIRST_SIZE = 12,
    DD2_SECOND_SIZE = 16,
    DD2_TEST_PATTERN = 0xa5,
    DD2_DECIMAL_BASE = 10,
    DD2_BYTE_SHIFT = 8,
    DD2_TEST_MAX_FILE_BYTES = 64 * 1024 * 1024
};

static void dd2_test_row(uint8_t *row, unsigned sector, const char *name, uint32_t size) {
    for (size_t index = 0; index < DD2_ROW_BYTES; ++index) {
        row[index] = 0;
    }
    for (size_t index = 0; index < DD2_NAME_BYTES && name[index] != '\0'; ++index) {
        row[index] = (uint8_t)name[index];
    }
    row[DD2_NAME_BYTES] = (uint8_t)sector;
    row[DD2_NAME_BYTES + 1] = (uint8_t)(sector >> DD2_BYTE_SHIFT);
    for (unsigned index = 0; index < sizeof(size); ++index) {
        row[DD2_SIZE_OFFSET + index] = (uint8_t)(size >> (index * DD2_BYTE_SHIFT));
    }
}

static bool dd2_expect_status(dd2_archive_status expected, const uint8_t *bytes, size_t size) {
    dd2_archive *archive = NULL;
    const dd2_archive_status actual = dd2_archive_open(bytes, size, &archive);
    const bool passed = actual == expected && (actual == DD2_ARCHIVE_OK || archive == NULL);
    dd2_archive_close(archive);
    if (!passed) {
        puts("archive status mismatch");
    }
    return passed;
}

static bool dd2_test_views(uint8_t *bytes) {
    dd2_test_row(bytes, DD2_FIRST_SECTOR, "LEV1\\LEVEL.DAT", DD2_FIRST_SIZE);
    dd2_test_row(bytes + DD2_ROW_BYTES, DD2_SECOND_SECTOR, "VAGS\\BANK1.SBK", DD2_SECOND_SIZE);
    for (size_t index = 0; index < DD2_FIRST_SIZE; ++index) {
        bytes[((size_t)DD2_FIRST_SECTOR * DD2_SECTOR_BYTES) + index] = DD2_TEST_PATTERN;
    }
    dd2_archive *archive = NULL;
    if (dd2_archive_open(bytes, DD2_TEST_BYTES, &archive) != DD2_ARCHIVE_OK) {
        return false;
    }
    dd2_asset asset = {0};
    const bool passed =
        dd2_archive_count(archive) == 2 && dd2_archive_entry(archive, 0, &asset) &&
        strcmp(asset.name, "LEV1\\LEVEL.DAT") == 0 && asset.size == DD2_FIRST_SIZE &&
        asset.bytes == bytes + ((size_t)DD2_FIRST_SECTOR * DD2_SECTOR_BYTES) &&
        asset.bytes[0] == DD2_TEST_PATTERN &&
        dd2_archive_find(archive, "VAGS\\BANK1.SBK", &asset) && asset.size == DD2_SECOND_SIZE &&
        asset.bytes == bytes + ((size_t)DD2_SECOND_SECTOR * DD2_SECTOR_BYTES) &&
        !dd2_archive_find(archive, "lev1\\level.dat", &asset) && asset.bytes == NULL &&
        !dd2_archive_find(archive, "LEV1/LEVEL.DAT", &asset) &&
        !dd2_archive_find(archive, "LEV1\\LEVEL.DA", &asset) &&
        !dd2_archive_find(archive, NULL, &asset) && !dd2_archive_find(NULL, "A", &asset) &&
        !dd2_archive_find(archive, "LEV1\\LEVEL.DAT", NULL) &&
        !dd2_archive_entry(archive, 2, &asset) && asset.size == 0 &&
        !dd2_archive_entry(archive, SIZE_MAX, &asset) && !dd2_archive_entry(archive, 0, NULL) &&
        !dd2_archive_entry(NULL, 0, &asset);
    dd2_archive_close(archive);
    return passed;
}

static bool dd2_test_bad_rows(uint8_t *bytes) {
    dd2_test_row(bytes + DD2_ROW_BYTES, DD2_SECOND_SECTOR, "LEV1\\LEVEL.DAT", DD2_SECOND_SIZE);
    if (!dd2_expect_status(DD2_ARCHIVE_DUPLICATE_NAME, bytes, DD2_TEST_BYTES)) {
        return false;
    }
    for (size_t index = 0; index < DD2_ROW_BYTES; ++index) {
        bytes[DD2_ROW_BYTES + index] = 0;
    }
    for (size_t index = 0; index < DD2_NAME_BYTES; ++index) {
        bytes[index] = 'A';
    }
    if (!dd2_expect_status(DD2_ARCHIVE_INVALID_NAME, bytes, DD2_TEST_BYTES)) {
        return false;
    }
    dd2_test_row(bytes, DD2_FIRST_SECTOR, "A", DD2_FIRST_SIZE);
    bytes[0] = UINT8_MAX;
    if (!dd2_expect_status(DD2_ARCHIVE_INVALID_NAME, bytes, DD2_TEST_BYTES)) {
        return false;
    }
    bytes[0] = 1;
    if (!dd2_expect_status(DD2_ARCHIVE_INVALID_NAME, bytes, DD2_TEST_BYTES)) {
        return false;
    }
    dd2_test_row(bytes, 4, "A", DD2_FIRST_SIZE);
    if (!dd2_expect_status(DD2_ARCHIVE_INVALID_EXTENT, bytes, DD2_TEST_BYTES)) {
        return false;
    }
    dd2_test_row(bytes, UINT16_MAX, "A", DD2_FIRST_SIZE);
    if (!dd2_expect_status(DD2_ARCHIVE_INVALID_EXTENT, bytes, DD2_TEST_BYTES)) {
        return false;
    }
    dd2_test_row(bytes, DD2_END_SECTOR, "A", 1);
    if (!dd2_expect_status(DD2_ARCHIVE_INVALID_EXTENT, bytes, DD2_TEST_BYTES)) {
        return false;
    }
    dd2_test_row(bytes, DD2_FIRST_SECTOR, "A", UINT32_MAX);
    if (!dd2_expect_status(DD2_ARCHIVE_INVALID_EXTENT, bytes, DD2_TEST_BYTES)) {
        return false;
    }
    dd2_test_row(bytes, DD2_END_SECTOR, "A", 0);
    return dd2_expect_status(DD2_ARCHIVE_OK, bytes, DD2_TEST_BYTES);
}

static bool dd2_test_directory(uint8_t *bytes) {
    if (!dd2_expect_status(DD2_ARCHIVE_INVALID_ARGUMENT, NULL, DD2_TEST_BYTES) ||
        !dd2_expect_status(DD2_ARCHIVE_TRUNCATED_DIRECTORY, bytes, 0) ||
        !dd2_expect_status(DD2_ARCHIVE_TRUNCATED_DIRECTORY, bytes, DD2_HEADER_BYTES - 1) ||
        dd2_archive_open(bytes, DD2_TEST_BYTES, NULL) != DD2_ARCHIVE_INVALID_ARGUMENT ||
        dd2_archive_count(NULL) != 0) {
        return false;
    }
    dd2_archive_close(NULL);
    if (!dd2_expect_status(DD2_ARCHIVE_OK, bytes, DD2_TEST_BYTES) || !dd2_test_views(bytes) ||
        !dd2_test_bad_rows(bytes)) {
        return false;
    }
    for (size_t index = 0; index < DD2_HEADER_BYTES / DD2_ROW_BYTES; ++index) {
        char name[DD2_NAME_BYTES] = {0};
        name[0] = 'A';
        size_t value = index;
        size_t digit = 1;
        do {
            name[digit] = (char)('0' + (value % DD2_DECIMAL_BASE));
            value /= DD2_DECIMAL_BASE;
            ++digit;
        } while (value != 0);
        dd2_test_row(bytes + (index * DD2_ROW_BYTES), DD2_SECOND_SECTOR, name, 1);
    }
    return dd2_expect_status(DD2_ARCHIVE_MISSING_TERMINATOR, bytes, DD2_TEST_BYTES);
}

static uint64_t dd2_asset_fingerprint(const dd2_asset *asset) {
    uint64_t value = UINT64_C(14695981039346656037);
    const uint64_t prime = UINT64_C(1099511628211);
    for (size_t index = 0; index < asset->size; ++index) {
        value = (value ^ asset->bytes[index]) * prime;
    }
    return value;
}

static bool dd2_print_name(const char *name) {
    if (putchar('"') == EOF) {
        return false;
    }
    for (const char *character = name; *character != '\0'; ++character) {
        if (*character == '"' || *character == '\\') {
            if (putchar('\\') == EOF) {
                return false;
            }
        }
        if (putchar(*character) == EOF) {
            return false;
        }
    }
    if (putchar('"') == EOF) {
        return false;
    }
    return true;
}

static bool dd2_inspect_bytes(const uint8_t *bytes, size_t size) {
    dd2_archive *archive = NULL;
    if (dd2_archive_open(bytes, size, &archive) != DD2_ARCHIVE_OK) {
        return false;
    }
    puts("[");
    bool passed = true;
    for (size_t index = 0; index < dd2_archive_count(archive); ++index) {
        dd2_asset asset = {0};
        dd2_asset found = {0};
        if (!dd2_archive_entry(archive, index, &asset) ||
            !dd2_archive_find(archive, asset.name, &found) || found.bytes != asset.bytes ||
            found.size != asset.size) {
            passed = false;
            break;
        }
        if (printf("%s{\"name\":", index == 0 ? "" : ",") < 0 || !dd2_print_name(asset.name) ||
            printf(",\"size\":%zu,\"offset\":%zu,\"fnv1a64\":\"%016" PRIx64 "\"}\n", asset.size,
                   (size_t)(asset.bytes - bytes), dd2_asset_fingerprint(&asset)) < 0) {
            passed = false;
            break;
        }
    }
    puts("]");
    dd2_archive_close(archive);
    return passed;
}

static bool dd2_inspect_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    bool passed = false;
    if (fseek(file, 0, SEEK_END) == 0) {
        const long length = ftell(file);
        if (length >= DD2_HEADER_BYTES && length <= DD2_TEST_MAX_FILE_BYTES &&
            fseek(file, 0, SEEK_SET) == 0) {
            uint8_t *bytes = malloc((size_t)length);
            if (bytes != NULL) {
                if (fread(bytes, 1, (size_t)length, file) == (size_t)length) {
                    passed = dd2_inspect_bytes(bytes, (size_t)length);
                }
                free(bytes);
            }
        }
    }
    return fclose(file) == 0 && passed;
}

int main(int argc, char **argv) {
    if (argc == 2) {
        return dd2_inspect_file(argv[1]) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (argc != 1) {
        return EXIT_FAILURE;
    }
    uint8_t *bytes = calloc(DD2_TEST_BYTES, 1);
    if (bytes == NULL) {
        return EXIT_FAILURE;
    }
    const bool passed = dd2_test_directory(bytes);
    free(bytes);
    if (!passed) {
        return EXIT_FAILURE;
    }
    puts("{\"scope\":\"rewrite archive bounds, names and borrowed views\",\"pass\":true}");
    return EXIT_SUCCESS;
}
