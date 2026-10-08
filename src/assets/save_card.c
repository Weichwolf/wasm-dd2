#include "assets/save_card.h"

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_SAVE_CARD_NAME_OFFSET = 4 };
struct dd2_save_card {
    uint8_t bytes[DD2_SAVE_CARD_BYTES];
};

static const uint8_t *dd2_save_card_header(const dd2_save_card *card, unsigned physical) {
    return &card->bytes[(size_t)physical * DD2_SAVE_CARD_HEADER_BYTES];
}
static bool dd2_save_card_name_length(const char *name, size_t *length) {
    if (name == NULL) {
        return false;
    }
    for (size_t index = 0; index <= DD2_SAVE_CARD_NAME_LIMIT; ++index) {
        if (name[index] == '\0') {
            *length = index;
            return true;
        }
    }
    return false;
}
static unsigned dd2_save_card_physical(const dd2_save_card *card, unsigned logical) {
    unsigned found = 0;
    for (unsigned physical = 0; physical < DD2_SAVE_CARD_SLOTS; ++physical) {
        if (dd2_read_le32(dd2_save_card_header(card, physical)) != 0) {
            if (found == logical) {
                return physical;
            }
            ++found;
        }
    }
    return DD2_SAVE_CARD_SLOTS;
}

dd2_save_card_result dd2_save_card_open(dd2_byte_view bytes, dd2_save_card **out) {
    if (out == NULL || bytes.data == NULL || bytes.size != DD2_SAVE_CARD_BYTES) {
        return DD2_SAVE_CARD_INVALID;
    }
    for (unsigned physical = 0; physical < DD2_SAVE_CARD_SLOTS; ++physical) {
        const uint8_t *header = &bytes.data[(size_t)physical * DD2_SAVE_CARD_HEADER_BYTES];
        const uint32_t occupied = dd2_read_le32(header);
        if (occupied > 1 || (occupied != 0 && memchr(&header[DD2_SAVE_CARD_NAME_OFFSET], 0,
                                                     DD2_SAVE_CARD_NAME_LIMIT + 1) == NULL)) {
            return DD2_SAVE_CARD_INVALID;
        }
    }
    dd2_save_card *card = malloc(sizeof(*card));
    if (card == NULL) {
        return DD2_SAVE_CARD_NO_MEMORY;
    }
    for (size_t index = 0; index < sizeof(card->bytes); ++index) {
        card->bytes[index] = bytes.data[index];
    }
    *out = card;
    return DD2_SAVE_CARD_OK;
}
void dd2_save_card_destroy(dd2_save_card *card) {
    free(card);
}
unsigned dd2_save_card_count(const dd2_save_card *card) {
    unsigned count = 0;
    if (card != NULL) {
        for (unsigned physical = 0; physical < DD2_SAVE_CARD_SLOTS; ++physical) {
            count += (unsigned)(dd2_read_le32(dd2_save_card_header(card, physical)) != 0);
        }
    }
    return count;
}
bool dd2_save_card_get(const dd2_save_card *card, unsigned logical_slot, dd2_save_card_entry *out) {
    if (out == NULL) {
        return false;
    }
    *out = (dd2_save_card_entry){0};
    if (card == NULL || logical_slot >= DD2_SAVE_CARD_SLOTS) {
        return false;
    }
    const unsigned physical = dd2_save_card_physical(card, logical_slot);
    if (physical == DD2_SAVE_CARD_SLOTS) {
        return false;
    }
    const uint8_t *name = &dd2_save_card_header(card, physical)[DD2_SAVE_CARD_NAME_OFFSET];
    out->physical_slot = physical;
    for (size_t index = 0; index <= DD2_SAVE_CARD_NAME_LIMIT; ++index) {
        out->name[index] = (char)name[index];
        if (name[index] == 0) {
            break;
        }
    }
    out->payload =
        (dd2_byte_view){.data = &card->bytes[(size_t)(physical + 1) * DD2_SAVE_CARD_BLOCK_BYTES],
                        .size = DD2_SAVE_CARD_BLOCK_BYTES};
    return true;
}
dd2_byte_view dd2_save_card_image(const dd2_save_card *card) {
    return card == NULL ? (dd2_byte_view){0}
                        : (dd2_byte_view){.data = card->bytes, .size = sizeof(card->bytes)};
}
static void dd2_save_card_clear(dd2_save_card *card, unsigned physical) {
    uint8_t *header = &card->bytes[(size_t)physical * DD2_SAVE_CARD_HEADER_BYTES];
    for (unsigned index = 0; index <= DD2_SAVE_CARD_NAME_OFFSET; ++index) {
        header[index] = 0;
    }
}
dd2_save_card_result dd2_save_card_put(dd2_save_card *card, unsigned logical_slot, const char *name,
                                       dd2_byte_view payload) {
    size_t length = 0;
    if (card == NULL || logical_slot > DD2_SAVE_CARD_SLOTS ||
        !dd2_save_card_name_length(name, &length) || payload.data == NULL ||
        payload.size != DD2_SAVE_CARD_BLOCK_BYTES) {
        return DD2_SAVE_CARD_INVALID;
    }
    const unsigned replaced = dd2_save_card_physical(card, logical_slot);
    unsigned target = DD2_SAVE_CARD_SLOTS;
    for (unsigned physical = 0; physical < DD2_SAVE_CARD_SLOTS; ++physical) {
        const uint8_t *header = dd2_save_card_header(card, physical);
        if (physical == replaced || dd2_read_le32(header) == 0) {
            if (target == DD2_SAVE_CARD_SLOTS) {
                target = physical;
            }
        } else if (strcmp((const char *)&header[DD2_SAVE_CARD_NAME_OFFSET], name) == 0) {
            return DD2_SAVE_CARD_DUPLICATE;
        }
    }
    if (target == DD2_SAVE_CARD_SLOTS) {
        return DD2_SAVE_CARD_FULL;
    }
    /* Stage borrowed inputs before clearing any header or replacing a block. */
    uint8_t staged_payload[DD2_SAVE_CARD_BLOCK_BYTES];
    char staged_name[DD2_SAVE_CARD_NAME_LIMIT + 1];
    for (size_t index = 0; index < sizeof(staged_payload); ++index) {
        staged_payload[index] = payload.data[index];
    }
    for (size_t index = 0; index <= length; ++index) {
        staged_name[index] = name[index];
    }
    if (replaced != DD2_SAVE_CARD_SLOTS) {
        dd2_save_card_clear(card, replaced);
    }
    uint8_t *header = &card->bytes[(size_t)target * DD2_SAVE_CARD_HEADER_BYTES];
    header[0] = 1;
    for (unsigned index = 1; index < DD2_SAVE_CARD_NAME_OFFSET; ++index) {
        header[index] = 0;
    }
    for (size_t index = 0; index <= length; ++index) {
        header[DD2_SAVE_CARD_NAME_OFFSET + index] = (uint8_t)staged_name[index];
    }
    for (size_t index = 0; index < sizeof(staged_payload); ++index) {
        card->bytes[((size_t)(target + 1) * DD2_SAVE_CARD_BLOCK_BYTES) + index] =
            staged_payload[index];
    }
    return DD2_SAVE_CARD_OK;
}
dd2_save_card_result dd2_save_card_delete(dd2_save_card *card, unsigned logical_slot) {
    if (card == NULL || logical_slot >= DD2_SAVE_CARD_SLOTS) {
        return DD2_SAVE_CARD_INVALID;
    }
    const unsigned physical = dd2_save_card_physical(card, logical_slot);
    if (physical == DD2_SAVE_CARD_SLOTS) {
        return DD2_SAVE_CARD_EMPTY;
    }
    dd2_save_card_clear(card, physical);
    return DD2_SAVE_CARD_OK;
}
