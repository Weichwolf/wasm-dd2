#ifndef DD2_PLATFORM_SAVE_STORE_H
#define DD2_PLATFORM_SAVE_STORE_H

#include "assets/bytes.h"
#include "assets/save_card.h"

#include <stdbool.h>

typedef struct dd2_save_store dd2_save_store;
typedef enum {
    DD2_SAVE_STORE_IDLE,
    DD2_SAVE_STORE_PENDING,
    DD2_SAVE_STORE_OK,
    DD2_SAVE_STORE_INVALID,
    DD2_SAVE_STORE_NO_MEMORY,
    DD2_SAVE_STORE_IO_ERROR,
    DD2_SAVE_STORE_CONFLICT,
    DD2_SAVE_STORE_INDETERMINATE,
    DD2_SAVE_STORE_FULL,
    DD2_SAVE_STORE_DUPLICATE,
    DD2_SAVE_STORE_NOT_FOUND
} dd2_save_store_result;
typedef enum {
    DD2_SAVE_STORE_CLOSED,
    DD2_SAVE_STORE_OPENING,
    DD2_SAVE_STORE_READY,
    DD2_SAVE_STORE_WRITING,
    DD2_SAVE_STORE_RELOADING,
    DD2_SAVE_STORE_NEEDS_RELOAD
} dd2_save_store_phase;

/* Main-thread owner. Native location is an existing dedicated writable folder;
 * reserve SaveGames, SaveGames.lock and .SaveGames.pending there. Browser
 * location is a dedicated IndexedDB database name. Asset/provisioning paths are
 * never defaults for game persistence. Missing storage opens as an empty card
 * without writing a card until the first successful mutation. */
dd2_save_store *dd2_save_store_create(void);
bool dd2_save_store_open(dd2_save_store *store, const char *location);
/* Begin stages all borrowed inputs. True means accepted, not saved. One request
 * at a time; a rejected busy request leaves the pending result intact. Poll
 * publishes only completed successful candidates. Close/destroy refuse pending
 * owners, so callers must retain them until terminal completion. */
bool dd2_save_store_put(dd2_save_store *store, unsigned logical, const char *name,
                        dd2_byte_view payload);
bool dd2_save_store_delete(dd2_save_store *store, unsigned logical);
bool dd2_save_store_reload(dd2_save_store *store);
dd2_save_store_result dd2_save_store_poll(dd2_save_store *store);
dd2_save_store_phase dd2_save_store_state(const dd2_save_store *store);
/* Borrowed last accepted snapshot, even during a write or reload failure.
 * Successful publication/reload or close invalidates it. NEEDS_RELOAD forbids
 * writes until a complete validated read succeeds. Payload codecs and playable
 * session validation belong to the consuming game/frontend. */
const dd2_save_card *dd2_save_store_view(const dd2_save_store *store);
bool dd2_save_store_close(dd2_save_store *store);
bool dd2_save_store_destroy(dd2_save_store *store);

#endif
