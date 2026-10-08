#include "platform/save_store.h"

#include "assets/bytes.h"
#include "assets/save_card.h"
#include "platform/save_backend.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

struct dd2_save_store {
    dd2_save_backend *backend;
    dd2_save_card *card;
    dd2_save_card *candidate;
    dd2_save_store_phase phase;
    dd2_save_store_result result;
    bool existed;
};
static bool dd2_save_store_pending(const dd2_save_store *store) {
    return store->phase == DD2_SAVE_STORE_OPENING || store->phase == DD2_SAVE_STORE_WRITING ||
           store->phase == DD2_SAVE_STORE_RELOADING;
}
static dd2_save_store_result dd2_save_store_card_result(dd2_save_card_result result) {
    switch (result) {
    case DD2_SAVE_CARD_OK:
        return DD2_SAVE_STORE_OK;
    case DD2_SAVE_CARD_INVALID:
        return DD2_SAVE_STORE_INVALID;
    case DD2_SAVE_CARD_NO_MEMORY:
        return DD2_SAVE_STORE_NO_MEMORY;
    case DD2_SAVE_CARD_EMPTY:
        return DD2_SAVE_STORE_NOT_FOUND;
    case DD2_SAVE_CARD_FULL:
        return DD2_SAVE_STORE_FULL;
    case DD2_SAVE_CARD_DUPLICATE:
        return DD2_SAVE_STORE_DUPLICATE;
    }
    return DD2_SAVE_STORE_INVALID;
}
dd2_save_store *dd2_save_store_create(void) {
    dd2_save_store *store = calloc(1, sizeof(*store));
    if (store != NULL) {
        store->backend = dd2_save_backend_create();
        if (store->backend == NULL) {
            free(store);
            return NULL;
        }
    }
    return store;
}
bool dd2_save_store_open(dd2_save_store *store, const char *location) {
    if (store == NULL || store->phase != DD2_SAVE_STORE_CLOSED) {
        return false;
    }
    if (location == NULL || location[0] == '\0') {
        store->result = DD2_SAVE_STORE_INVALID;
        return false;
    }
    store->phase = DD2_SAVE_STORE_OPENING;
    store->result = DD2_SAVE_STORE_PENDING;
    dd2_save_backend_open(store->backend, location);
    return true;
}
static bool dd2_save_store_candidate(dd2_save_store *store) {
    if (store == NULL || store->phase != DD2_SAVE_STORE_READY) {
        return false;
    }
    store->result = dd2_save_store_card_result(
        dd2_save_card_open(dd2_save_card_image(store->card), &store->candidate));
    return store->result == DD2_SAVE_STORE_OK;
}
static bool dd2_save_store_write(dd2_save_store *store, dd2_save_card_result result) {
    if (result != DD2_SAVE_CARD_OK) {
        store->result = dd2_save_store_card_result(result);
        dd2_save_card_destroy(store->candidate);
        store->candidate = NULL;
        return false;
    }
    store->phase = DD2_SAVE_STORE_WRITING;
    store->result = DD2_SAVE_STORE_PENDING;
    dd2_save_backend_write(store->backend, dd2_save_card_image(store->card), store->existed,
                           dd2_save_card_image(store->candidate));
    return true;
}
bool dd2_save_store_put(dd2_save_store *store, unsigned logical, const char *name,
                        dd2_byte_view payload) {
    return dd2_save_store_candidate(store) &&
           dd2_save_store_write(store, dd2_save_card_put(store->candidate, logical, name, payload));
}
bool dd2_save_store_delete(dd2_save_store *store, unsigned logical) {
    return dd2_save_store_candidate(store) &&
           dd2_save_store_write(store, dd2_save_card_delete(store->candidate, logical));
}
bool dd2_save_store_reload(dd2_save_store *store) {
    if (store == NULL ||
        (store->phase != DD2_SAVE_STORE_READY && store->phase != DD2_SAVE_STORE_NEEDS_RELOAD)) {
        return false;
    }
    store->phase = DD2_SAVE_STORE_RELOADING;
    store->result = DD2_SAVE_STORE_PENDING;
    dd2_save_backend_read(store->backend);
    return true;
}
static dd2_save_store_result dd2_save_store_read_complete(dd2_save_store *store,
                                                          dd2_save_store_result result) {
    dd2_save_card *loaded = NULL;
    if (result == DD2_SAVE_STORE_OK) {
        result = dd2_save_store_card_result(
            dd2_save_card_open(dd2_save_backend_image(store->backend), &loaded));
    }
    if (result == DD2_SAVE_STORE_OK) {
        dd2_save_card_destroy(store->card);
        store->card = loaded;
        store->existed = dd2_save_backend_exists(store->backend);
        store->phase = DD2_SAVE_STORE_READY;
    } else if (store->phase == DD2_SAVE_STORE_OPENING) {
        dd2_save_backend_close(store->backend);
        store->phase = DD2_SAVE_STORE_CLOSED;
    } else {
        store->phase = DD2_SAVE_STORE_NEEDS_RELOAD;
    }
    return result;
}
dd2_save_store_result dd2_save_store_poll(dd2_save_store *store) {
    if (store == NULL) {
        return DD2_SAVE_STORE_INVALID;
    }
    if (!dd2_save_store_pending(store)) {
        return store->result;
    }
    const dd2_save_store_result result = dd2_save_backend_poll(store->backend);
    if (result == DD2_SAVE_STORE_PENDING) {
        return result;
    }
    if (store->phase != DD2_SAVE_STORE_WRITING) {
        store->result = dd2_save_store_read_complete(store, result);
        return store->result;
    }
    store->result = result;
    store->phase = DD2_SAVE_STORE_READY;
    if (result == DD2_SAVE_STORE_OK) {
        dd2_save_card_destroy(store->card);
        store->card = store->candidate;
        store->candidate = NULL;
        store->existed = true;
    } else if (result == DD2_SAVE_STORE_CONFLICT || result == DD2_SAVE_STORE_INDETERMINATE ||
               result == DD2_SAVE_STORE_INVALID) {
        store->phase = DD2_SAVE_STORE_NEEDS_RELOAD;
    }
    dd2_save_card_destroy(store->candidate);
    store->candidate = NULL;
    return store->result;
}
dd2_save_store_phase dd2_save_store_state(const dd2_save_store *store) {
    return store == NULL ? DD2_SAVE_STORE_CLOSED : store->phase;
}
const dd2_save_card *dd2_save_store_view(const dd2_save_store *store) {
    return store == NULL ? NULL : store->card;
}
bool dd2_save_store_close(dd2_save_store *store) {
    if (store == NULL) {
        return false;
    }
    if (dd2_save_store_pending(store)) {
        return false;
    }
    dd2_save_backend_close(store->backend);
    dd2_save_card_destroy(store->card);
    store->card = NULL;
    store->phase = DD2_SAVE_STORE_CLOSED;
    store->result = DD2_SAVE_STORE_IDLE;
    store->existed = false;
    return true;
}
bool dd2_save_store_destroy(dd2_save_store *store) {
    if (store == NULL) {
        return true;
    }
    if (!dd2_save_store_close(store)) {
        return false;
    }
    dd2_save_backend_destroy(store->backend);
    free(store);
    return true;
}
