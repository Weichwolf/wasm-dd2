#ifndef DD2_ASSETS_SAVE_CARD_H
#define DD2_ASSETS_SAVE_CARD_H

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>

/* Original Windows SaveGames container. Payload interpretation belongs to
 * settings/championship/replay codecs; this owner preserves complete blocks. */
enum {
    DD2_SAVE_CARD_SLOTS = 15,
    DD2_SAVE_CARD_NEW_ENTRY = DD2_SAVE_CARD_SLOTS,
    DD2_SAVE_CARD_NAME_LIMIT = 8,
    DD2_SAVE_CARD_HEADER_BYTES = 512,
    DD2_SAVE_CARD_BLOCK_BYTES = 8192,
    DD2_SAVE_CARD_BYTES = 131072
};
typedef struct dd2_save_card dd2_save_card;
typedef struct {
    unsigned physical_slot;
    char name[DD2_SAVE_CARD_NAME_LIMIT + 1];
    dd2_byte_view payload;
} dd2_save_card_entry;
typedef enum {
    DD2_SAVE_CARD_OK,
    DD2_SAVE_CARD_INVALID,
    DD2_SAVE_CARD_NO_MEMORY,
    DD2_SAVE_CARD_EMPTY,
    DD2_SAVE_CARD_FULL,
    DD2_SAVE_CARD_DUPLICATE
} dd2_save_card_result;

/* Owns an exact copy. Valid supported headers contain little-endian occupancy
 * zero/one and occupied names terminated within nine bytes; unused headers,
 * name tails, reserved bytes and complete opaque payloads are preserved.
 * Failure preserves *out. Success creates a new allocation; any previous owner
 * remains the caller's responsibility. Callers serialize access to each card.
 * NULL output/input is invalid; destroy accepts NULL. */
dd2_save_card_result dd2_save_card_open(dd2_byte_view bytes, dd2_save_card **out);
void dd2_save_card_destroy(dd2_save_card *card);
unsigned dd2_save_card_count(const dd2_save_card *card);
/* Logical entries compact occupied physical headers in ascending order.
 * Empty/out-of-range/NULL queries clear the output; borrowed payloads remain
 * valid until successful put/delete or destruction. */
bool dd2_save_card_get(const dd2_save_card *card, unsigned logical_slot, dd2_save_card_entry *out);
/* Borrow the complete image for a platform adapter's staged atomic write.
 * An in-memory success is not a successful durable save. */
dd2_byte_view dd2_save_card_image(const dd2_save_card *card);
/* Replace a selected occupied logical entry or create in any empty logical
 * slot (0..14), or append with NEW_ENTRY. After logically removing a replacement,
 * use the first free
 * physical header, as the original does. Other occupied names must be unique.
 * Names are at most eight bytes, including the original's empty-name case;
 * input must be a readable C string. Payload is exactly one complete block.
 * Inputs may borrow this card's image. Every failure preserves all bytes;
 * caller publishes/persists the candidate only after adapter success. */
dd2_save_card_result dd2_save_card_put(dd2_save_card *card, unsigned logical_slot, const char *name,
                                       dd2_byte_view payload);
/* Clear occupancy and the first name byte; retain the header tail and payload. */
dd2_save_card_result dd2_save_card_delete(dd2_save_card *card, unsigned logical_slot);

#endif
