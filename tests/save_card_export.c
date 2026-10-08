#include "assets/bytes.h"
#include "assets/save_card.h"
#include "platform/file.h"

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_SAVE_CARD_EXPORT_ARGUMENTS = 7,
    DD2_SAVE_CARD_EXPORT_BASE = 10,
    DD2_SAVE_CARD_EXPORT_NAME_ARGUMENT = 5,
    DD2_SAVE_CARD_EXPORT_PAYLOAD_ARGUMENT = 6
};
static bool dd2_save_card_export_write(const char *path, dd2_byte_view bytes) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    const bool written = fwrite(bytes.data, 1, bytes.size, file) == bytes.size;
    const int closed = fclose(file);
    return written && closed == 0;
}
static bool dd2_save_card_export_slot(const char *argument, unsigned *slot) {
    char *end = NULL;
    errno = 0;
    const unsigned long value = strtoul(argument, &end, DD2_SAVE_CARD_EXPORT_BASE);
    if (errno != 0 || end == argument || *end != '\0' || value > UINT_MAX) {
        return false;
    }
    *slot = (unsigned)value;
    return true;
}
static bool dd2_save_card_export_run(char **argv) {
    dd2_file image = {0};
    dd2_file payload = {0};
    dd2_save_card *card = NULL;
    unsigned slot = 0;
    bool valid = dd2_save_card_export_slot(argv[4], &slot) && dd2_file_read(argv[1], &image) &&
                 dd2_save_card_open((dd2_byte_view){.data = image.data, .size = image.size},
                                    &card) == DD2_SAVE_CARD_OK;
    dd2_save_card_result result = DD2_SAVE_CARD_OK;
    if (valid && strcmp(argv[3], "put") == 0) {
        valid = dd2_file_read(argv[DD2_SAVE_CARD_EXPORT_PAYLOAD_ARGUMENT], &payload);
        if (valid) {
            result = dd2_save_card_put(card, slot, argv[DD2_SAVE_CARD_EXPORT_NAME_ARGUMENT],
                                       (dd2_byte_view){.data = payload.data, .size = payload.size});
        }
    } else if (valid && strcmp(argv[3], "delete") == 0) {
        result = dd2_save_card_delete(card, slot);
    } else if (valid && strcmp(argv[3], "inspect") != 0 && strcmp(argv[3], "read") != 0) {
        valid = false;
    }
    if (valid) {
        dd2_byte_view bytes = dd2_save_card_image(card);
        if (strcmp(argv[3], "read") == 0) {
            dd2_save_card_entry entry = {0};
            valid = dd2_save_card_get(card, slot, &entry);
            bytes = entry.payload;
        }
        valid = valid && dd2_save_card_export_write(argv[2], bytes);
    }
    if (valid) {
        printf("{\"result\":%d,\"entries\":[", (int)result);
        for (unsigned logical = 0; logical < dd2_save_card_count(card); ++logical) {
            dd2_save_card_entry entry = {0};
            valid = dd2_save_card_get(card, logical, &entry);
            if (!valid) {
                break;
            }
            printf("%s{\"physical\":%u,\"name_hex\":\"", logical == 0 ? "" : ",",
                   entry.physical_slot);
            for (size_t byte = 0; byte < sizeof(entry.name); ++byte) {
                printf("%02x", (unsigned)(unsigned char)entry.name[byte]);
            }
            printf("\",\"magic\":%u}", (unsigned)dd2_read_le16(entry.payload.data));
        }
        puts("]}");
    }
    dd2_save_card_destroy(card);
    dd2_file_release(&payload);
    dd2_file_release(&image);
    return valid;
}
int main(int argc, char **argv) {
    return argc == DD2_SAVE_CARD_EXPORT_ARGUMENTS && dd2_save_card_export_run(argv) ? EXIT_SUCCESS
                                                                                    : EXIT_FAILURE;
}
