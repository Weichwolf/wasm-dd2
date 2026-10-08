#include "assets/bytes.h"
#include "assets/save_card.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_SAVE_CARD_TEST_NAME = 4,
    DD2_SAVE_CARD_TEST_PATTERN = 251,
    DD2_SAVE_CARD_TEST_REPLACE_SLOT = 7
};
static uint8_t dd2_save_card_test_source[DD2_SAVE_CARD_BYTES];
static uint8_t dd2_save_card_test_before[DD2_SAVE_CARD_BYTES];
static uint8_t dd2_save_card_test_payload[DD2_SAVE_CARD_BLOCK_BYTES];

static void dd2_save_card_test_require(bool valid, const char *message) {
    if (!valid) {
        if (fputs(message, stderr) == EOF) {
            abort();
        }
        exit(EXIT_FAILURE);
    }
}
static dd2_byte_view dd2_save_card_test_view(void) {
    return (dd2_byte_view){.data = dd2_save_card_test_source,
                           .size = sizeof(dd2_save_card_test_source)};
}
static dd2_save_card *dd2_save_card_test_open(void) {
    dd2_save_card *card = NULL;
    dd2_save_card_test_require(dd2_save_card_open(dd2_save_card_test_view(), &card) ==
                                   DD2_SAVE_CARD_OK,
                               "open exact original extent");
    return card;
}
static void dd2_save_card_test_fixture(void) {
    for (size_t index = 0; index < sizeof(dd2_save_card_test_source); ++index) {
        dd2_save_card_test_source[index] = (uint8_t)(index % DD2_SAVE_CARD_TEST_PATTERN);
    }
    for (unsigned slot = 0; slot < DD2_SAVE_CARD_SLOTS; ++slot) {
        for (unsigned byte = 0; byte < DD2_SAVE_CARD_TEST_NAME; ++byte) {
            dd2_save_card_test_source[((size_t)slot * DD2_SAVE_CARD_HEADER_BYTES) + byte] = 0;
        }
    }
    for (size_t index = 0; index < sizeof(dd2_save_card_test_payload); ++index) {
        dd2_save_card_test_payload[index] = (uint8_t)((index + 1) % DD2_SAVE_CARD_TEST_PATTERN);
    }
}
static void dd2_save_card_test_snapshot(const dd2_save_card *card) {
    const dd2_byte_view image = dd2_save_card_image(card);
    for (size_t index = 0; index < image.size; ++index) {
        dd2_save_card_test_before[index] = image.data[index];
    }
}
static void dd2_save_card_test_unchanged(const dd2_save_card *card) {
    const dd2_byte_view image = dd2_save_card_image(card);
    dd2_save_card_test_require(image.size == sizeof(dd2_save_card_test_before) &&
                                   memcmp(image.data, dd2_save_card_test_before, image.size) == 0,
                               "transactional exact rollback");
}
static void dd2_save_card_test_read(const dd2_save_card *card, unsigned logical, unsigned physical,
                                    const char *name) {
    dd2_save_card_entry entry = {0};
    dd2_save_card_test_require(
        dd2_save_card_get(card, logical, &entry) && entry.physical_slot == physical &&
            strcmp(entry.name, name) == 0 && entry.payload.size == DD2_SAVE_CARD_BLOCK_BYTES,
        "compact logical-to-physical entry and complete payload");
}
static void dd2_save_card_test_bounds(dd2_save_card *card) {
    for (size_t size = 0; size < DD2_SAVE_CARD_BYTES; ++size) {
        dd2_save_card *out = card;
        dd2_save_card_test_require(
            dd2_save_card_open((dd2_byte_view){.data = dd2_save_card_test_source, .size = size},
                               &out) == DD2_SAVE_CARD_INVALID &&
                out == card,
            "every truncated extent preserves existing owner");
    }
    dd2_save_card *out = card;
    dd2_save_card_test_require(
        dd2_save_card_open(
            (dd2_byte_view){.data = dd2_save_card_test_source, .size = DD2_SAVE_CARD_BYTES + 1},
            &out) == DD2_SAVE_CARD_INVALID &&
            out == card && dd2_save_card_open((dd2_byte_view){0}, &out) == DD2_SAVE_CARD_INVALID &&
            dd2_save_card_open(dd2_save_card_test_view(), NULL) == DD2_SAVE_CARD_INVALID,
        "oversized and missing inputs");
    for (unsigned physical = 0; physical < DD2_SAVE_CARD_SLOTS; ++physical) {
        uint8_t *header = &dd2_save_card_test_source[(size_t)physical * DD2_SAVE_CARD_HEADER_BYTES];
        header[0] = 2;
        dd2_save_card_test_require(dd2_save_card_open(dd2_save_card_test_view(), &out) ==
                                           DD2_SAVE_CARD_INVALID &&
                                       out == card,
                                   "invalid occupancy in each physical header");
        header[0] = 1;
        for (unsigned byte = 0; byte <= DD2_SAVE_CARD_NAME_LIMIT; ++byte) {
            header[DD2_SAVE_CARD_TEST_NAME + byte] = 'A';
        }
        dd2_save_card_test_require(dd2_save_card_open(dd2_save_card_test_view(), &out) ==
                                           DD2_SAVE_CARD_INVALID &&
                                       out == card,
                                   "unterminated name in each physical header");
        header[0] = 0;
    }
    dd2_save_card_test_unchanged(card);
}
static void dd2_save_card_test_invalid(dd2_save_card *card) {
    const dd2_byte_view payload = {.data = dd2_save_card_test_payload,
                                   .size = sizeof(dd2_save_card_test_payload)};
    dd2_save_card_test_snapshot(card);
    dd2_save_card_test_require(
        dd2_save_card_put(card, DD2_SAVE_CARD_SLOTS + 1, "A", payload) == DD2_SAVE_CARD_INVALID &&
            dd2_save_card_put(card, 0, "ABCDEFGHI", payload) == DD2_SAVE_CARD_INVALID &&
            dd2_save_card_put(card, 0, NULL, payload) == DD2_SAVE_CARD_INVALID &&
            dd2_save_card_put(card, 0, "A", (dd2_byte_view){0}) == DD2_SAVE_CARD_INVALID &&
            dd2_save_card_put(card, 0, "A",
                              (dd2_byte_view){.data = payload.data, .size = payload.size - 1}) ==
                DD2_SAVE_CARD_INVALID &&
            dd2_save_card_put(card, 0, "A",
                              (dd2_byte_view){.data = payload.data, .size = payload.size + 1}) ==
                DD2_SAVE_CARD_INVALID &&
            dd2_save_card_put(NULL, 0, "A", payload) == DD2_SAVE_CARD_INVALID &&
            dd2_save_card_delete(card, DD2_SAVE_CARD_SLOTS) == DD2_SAVE_CARD_INVALID &&
            dd2_save_card_delete(NULL, 0) == DD2_SAVE_CARD_INVALID &&
            dd2_save_card_delete(card, 0) == DD2_SAVE_CARD_EMPTY,
        "invalid mutations and empty deletion");
    dd2_save_card_test_unchanged(card);
    dd2_save_card_entry entry = {.physical_slot = 1, .name = "A", .payload = payload};
    dd2_save_card_test_require(
        !dd2_save_card_get(card, 0, &entry) && entry.payload.data == NULL &&
            entry.name[0] == '\0' && entry.physical_slot == 0 &&
            !dd2_save_card_get(NULL, 0, &entry) &&
            !dd2_save_card_get(card, DD2_SAVE_CARD_SLOTS, &entry) &&
            !dd2_save_card_get(card, 0, NULL) && dd2_save_card_count(NULL) == 0 &&
            dd2_save_card_image(NULL).data == NULL && dd2_save_card_image(NULL).size == 0,
        "empty and NULL borrowed queries");
}
static void dd2_save_card_test_first_put(const dd2_save_card *card) {
    const dd2_byte_view image = dd2_save_card_image(card);
    for (size_t index = 0; index < image.size; ++index) {
        uint8_t wanted = dd2_save_card_test_before[index];
        if (index == 0) {
            wanted = 1;
        } else if (index == DD2_SAVE_CARD_TEST_NAME) {
            wanted = 'A';
        } else if (index == DD2_SAVE_CARD_TEST_NAME + 1) {
            wanted = 0;
        } else if (index >= DD2_SAVE_CARD_BLOCK_BYTES &&
                   index < (size_t)2 * DD2_SAVE_CARD_BLOCK_BYTES) {
            wanted = dd2_save_card_test_payload[index - DD2_SAVE_CARD_BLOCK_BYTES];
        }
        dd2_save_card_test_require(image.data[index] == wanted,
                                   "put preserves unrelated headers, name tails and unused blocks");
    }
}
static void dd2_save_card_test_population(dd2_save_card *card) {
    const dd2_byte_view payload = {.data = dd2_save_card_test_payload,
                                   .size = sizeof(dd2_save_card_test_payload)};
    dd2_save_card_test_snapshot(card);
    for (unsigned logical = 0; logical < DD2_SAVE_CARD_SLOTS; ++logical) {
        char name[] = "A";
        name[0] = (char)((unsigned)'A' + logical);
        dd2_save_card_test_require(
            dd2_save_card_put(card, DD2_SAVE_CARD_NEW_ENTRY, name, payload) == DD2_SAVE_CARD_OK,
            "populate all fifteen physical blocks");
        dd2_save_card_test_read(card, logical, logical, name);
        if (logical == 0) {
            dd2_save_card_test_first_put(card);
        }
    }
    dd2_save_card_test_snapshot(card);
    dd2_save_card_test_require(
        dd2_save_card_count(card) == DD2_SAVE_CARD_SLOTS &&
            dd2_save_card_put(card, DD2_SAVE_CARD_NEW_ENTRY, "Z", payload) == DD2_SAVE_CARD_FULL &&
            dd2_save_card_put(card, DD2_SAVE_CARD_TEST_REPLACE_SLOT, "A", payload) ==
                DD2_SAVE_CARD_DUPLICATE,
        "full append and duplicate replacement preserve all bytes");
    dd2_save_card_test_unchanged(card);
    dd2_save_card_test_require(dd2_save_card_put(card, DD2_SAVE_CARD_TEST_REPLACE_SLOT, "ABCDEFGH",
                                                 payload) == DD2_SAVE_CARD_OK,
                               "replacement on a full card with maximum name");
    dd2_save_card_test_read(card, DD2_SAVE_CARD_TEST_REPLACE_SLOT, DD2_SAVE_CARD_TEST_REPLACE_SLOT,
                            "ABCDEFGH");
}
static void dd2_save_card_test_compaction(dd2_save_card *card) {
    dd2_save_card_test_snapshot(card);
    dd2_save_card_test_require(dd2_save_card_delete(card, 0) == DD2_SAVE_CARD_OK,
                               "delete first physical entry");
    dd2_save_card_test_read(card, 0, 1, "B");
    const dd2_byte_view deleted = dd2_save_card_image(card);
    for (size_t index = 0; index < deleted.size; ++index) {
        dd2_save_card_test_require(
            deleted.data[index] ==
                (index <= DD2_SAVE_CARD_TEST_NAME ? 0 : dd2_save_card_test_before[index]),
            "only occupancy and first name byte cleared");
    }
    dd2_save_card_entry entry = {0};
    dd2_save_card_test_require(dd2_save_card_get(card, 0, &entry) &&
                                   dd2_save_card_put(card, 0, "Z", entry.payload) ==
                                       DD2_SAVE_CARD_OK,
                               "move logical replacement to earlier free physical block");
    dd2_save_card_test_read(card, 0, 0, "Z");
    dd2_save_card_test_read(card, 1, 2, "C");
    dd2_save_card_test_snapshot(card);
    const dd2_byte_view image = dd2_save_card_image(card);
    dd2_save_card_test_require(
        dd2_save_card_put(card, 0, (const char *)&image.data[DD2_SAVE_CARD_TEST_NAME],
                          (dd2_byte_view){.data = image.data, .size = DD2_SAVE_CARD_BLOCK_BYTES}) ==
                DD2_SAVE_CARD_OK &&
            dd2_save_card_get(card, 0, &entry) &&
            memcmp(entry.payload.data, dd2_save_card_test_before, entry.payload.size) == 0 &&
            strcmp(entry.name, "Z") == 0,
        "borrowed name and header-overlapping payload staged before mutation");
    dd2_save_card_test_require(
        dd2_save_card_put(card, DD2_SAVE_CARD_NEW_ENTRY, "", entry.payload) == DD2_SAVE_CARD_OK,
        "preserve original empty-name support");
    dd2_save_card_test_read(card, 1, 1, "");
}
int main(void) {
    dd2_save_card_test_fixture();
    dd2_save_card *card = dd2_save_card_test_open();
    dd2_save_card_test_snapshot(card);
    dd2_save_card_test_bounds(card);
    dd2_save_card_test_invalid(card);
    dd2_save_card_test_population(card);
    dd2_save_card_test_compaction(card);
    const dd2_byte_view image = dd2_save_card_image(card);
    dd2_save_card *reopened = NULL;
    dd2_save_card_test_require(
        dd2_save_card_open(image, &reopened) == DD2_SAVE_CARD_OK &&
            memcmp(dd2_save_card_image(reopened).data, image.data, image.size) == 0,
        "exact image round trip including reserved bytes");
    dd2_save_card_destroy(card);
    dd2_save_card_test_read(reopened, 0, 0, "Z");
    dd2_save_card_test_read(reopened, 1, 1, "");
    dd2_save_card_destroy(reopened);
    dd2_save_card_destroy(NULL);
    puts("Save card bounds, ownership, physical mapping, mutation and rollback checks passed.");
    return EXIT_SUCCESS;
}
