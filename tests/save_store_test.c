#include "assets/bytes.h"
#include "assets/save_card.h"
#include "platform/save_backend.h"
#include "platform/save_store.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Delayed backend exercises owner acknowledgment/lifetime. Actual Native disk
 * and actual Chromium IndexedDB have separate production-backend checks. */
struct dd2_save_backend {
    dd2_save_store_result result;
    unsigned delay;
    bool existed;
};
static uint8_t dd2_store_test_disk[DD2_SAVE_CARD_BYTES];
static uint8_t dd2_store_test_before[DD2_SAVE_CARD_BYTES];
static uint8_t dd2_store_test_payload[DD2_SAVE_CARD_BLOCK_BYTES];
static bool dd2_store_test_exists;
static dd2_save_store_result dd2_store_test_failure = DD2_SAVE_STORE_OK;
static void dd2_store_test_copy(uint8_t *destination, size_t size, const uint8_t *source) {
    for (size_t index = 0; index < size; ++index) {
        destination[index] = source[index];
    }
}
dd2_save_backend *dd2_save_backend_create(void) {
    return calloc(1, sizeof(dd2_save_backend));
}
void dd2_save_backend_destroy(dd2_save_backend *backend) {
    free(backend);
}
void dd2_save_backend_close(dd2_save_backend *backend) {
    backend->result = DD2_SAVE_STORE_IDLE;
}
void dd2_save_backend_read(dd2_save_backend *backend) {
    backend->result = DD2_SAVE_STORE_OK;
    backend->existed = dd2_store_test_exists;
    backend->delay = 1;
}
void dd2_save_backend_open(dd2_save_backend *backend, const char *location) {
    if (location != NULL && location[0] != '\0') {
        dd2_save_backend_read(backend);
    } else {
        backend->result = DD2_SAVE_STORE_INVALID;
    }
}
void dd2_save_backend_write(dd2_save_backend *backend, dd2_byte_view expected, bool existed,
                            dd2_byte_view replacement) {
    backend->result = dd2_store_test_failure;
    if (existed != dd2_store_test_exists ||
        (existed && memcmp(expected.data, dd2_store_test_disk, expected.size) != 0)) {
        backend->result = DD2_SAVE_STORE_CONFLICT;
    }
    if (backend->result == DD2_SAVE_STORE_OK || backend->result == DD2_SAVE_STORE_INDETERMINATE) {
        dd2_store_test_copy(dd2_store_test_disk, replacement.size, replacement.data);
        dd2_store_test_exists = true;
    }
    backend->delay = 1;
}
dd2_save_store_result dd2_save_backend_poll(dd2_save_backend *backend) {
    if (backend->delay != 0) {
        --backend->delay;
        return DD2_SAVE_STORE_PENDING;
    }
    return backend->result;
}
dd2_byte_view dd2_save_backend_image(const dd2_save_backend *backend) {
    return backend->result == DD2_SAVE_STORE_IDLE
               ? (dd2_byte_view){0}
               : (dd2_byte_view){.data = dd2_store_test_disk, .size = sizeof(dd2_store_test_disk)};
}
bool dd2_save_backend_exists(const dd2_save_backend *backend) {
    return backend->existed;
}
static void dd2_store_test_require(bool valid, const char *message) {
    if (!valid) {
        if (fputs(message, stderr) == EOF) {
            abort();
        }
        exit(EXIT_FAILURE);
    }
}
static dd2_byte_view dd2_store_test_view(void) {
    return (dd2_byte_view){.data = dd2_store_test_payload, .size = sizeof(dd2_store_test_payload)};
}
static void dd2_store_test_pending(dd2_save_store *store) {
    dd2_store_test_require(!dd2_save_store_close(store) && !dd2_save_store_destroy(store) &&
                               !dd2_save_store_reload(store) &&
                               !dd2_save_store_put(store, 0, "X", dd2_store_test_view()) &&
                               !dd2_save_store_delete(store, 0) &&
                               dd2_save_store_poll(store) == DD2_SAVE_STORE_PENDING,
                           "pending owner was changed/destroyed\n");
}
static void dd2_store_test_reload(dd2_save_store *store, unsigned count) {
    dd2_store_test_require(dd2_save_store_reload(store), "reload rejected\n");
    dd2_store_test_pending(store);
    dd2_store_test_require(dd2_save_store_poll(store) == DD2_SAVE_STORE_OK &&
                               dd2_save_store_state(store) == DD2_SAVE_STORE_READY &&
                               dd2_save_card_count(dd2_save_store_view(store)) == count,
                           "complete validated reload differs\n");
}
static void dd2_store_test_failed(dd2_save_store *store) {
    const dd2_byte_view image = dd2_save_card_image(dd2_save_store_view(store));
    dd2_store_test_copy(dd2_store_test_before, image.size, image.data);
    dd2_store_test_failure = DD2_SAVE_STORE_IO_ERROR;
    dd2_store_test_require(dd2_save_store_put(store, 1, "B", dd2_store_test_view()),
                           "candidate write rejected\n");
    dd2_store_test_pending(store);
    dd2_store_test_require(
        dd2_save_store_poll(store) == DD2_SAVE_STORE_IO_ERROR &&
            dd2_save_store_state(store) == DD2_SAVE_STORE_READY &&
            memcmp(dd2_save_card_image(dd2_save_store_view(store)).data, dd2_store_test_before,
                   sizeof(dd2_store_test_before)) == 0 &&
            memcmp(dd2_store_test_disk, dd2_store_test_before, sizeof(dd2_store_test_before)) == 0,
        "known failed write changed memory/disk\n");
    dd2_store_test_failure = DD2_SAVE_STORE_INDETERMINATE;
    dd2_store_test_require(dd2_save_store_put(store, 1, "B", dd2_store_test_view()),
                           "indeterminate write rejected\n");
    dd2_store_test_pending(store);
    dd2_store_test_require(dd2_save_store_poll(store) == DD2_SAVE_STORE_INDETERMINATE &&
                               dd2_save_store_state(store) == DD2_SAVE_STORE_NEEDS_RELOAD &&
                               !dd2_save_store_delete(store, 0) &&
                               memcmp(dd2_save_card_image(dd2_save_store_view(store)).data,
                                      dd2_store_test_before, sizeof(dd2_store_test_before)) == 0,
                           "ambiguous write published/retried\n");
    dd2_store_test_failure = DD2_SAVE_STORE_OK;
    dd2_store_test_reload(store, 2);
}
static void dd2_store_test_invalid(dd2_save_store *store) {
    const dd2_byte_view image = dd2_save_card_image(dd2_save_store_view(store));
    dd2_store_test_copy(dd2_store_test_before, image.size, image.data);
    dd2_store_test_disk[0] = 2;
    dd2_store_test_require(dd2_save_store_reload(store), "corrupt read begin rejected\n");
    dd2_store_test_pending(store);
    dd2_store_test_require(dd2_save_store_poll(store) == DD2_SAVE_STORE_INVALID &&
                               dd2_save_store_state(store) == DD2_SAVE_STORE_NEEDS_RELOAD &&
                               !dd2_save_store_put(store, 0, "C", dd2_store_test_view()) &&
                               memcmp(dd2_save_card_image(dd2_save_store_view(store)).data,
                                      dd2_store_test_before, sizeof(dd2_store_test_before)) == 0,
                           "corrupt reload changed accepted card\n");
    dd2_store_test_copy(dd2_store_test_disk, sizeof(dd2_store_test_disk), dd2_store_test_before);
    dd2_store_test_reload(store, 2);
}
int main(void) {
    dd2_store_test_require(dd2_save_store_destroy(NULL) && !dd2_save_store_close(NULL) &&
                               dd2_save_store_poll(NULL) == DD2_SAVE_STORE_INVALID,
                           "null lifecycle differs\n");
    dd2_save_store *store = dd2_save_store_create();
    dd2_store_test_require(store != NULL && !dd2_save_store_open(store, NULL) &&
                               dd2_save_store_poll(store) == DD2_SAVE_STORE_INVALID &&
                               dd2_save_store_state(store) == DD2_SAVE_STORE_CLOSED &&
                               dd2_save_store_open(store, "delayed-test"),
                           "owner open differs\n");
    dd2_store_test_pending(store);
    dd2_store_test_require(dd2_save_store_poll(store) == DD2_SAVE_STORE_OK &&
                               dd2_save_card_count(dd2_save_store_view(store)) == 0,
                           "missing storage is not empty\n");
    for (size_t index = 0; index < sizeof(dd2_store_test_payload); ++index) {
        dd2_store_test_payload[index] = (uint8_t)index;
    }
    dd2_store_test_require(dd2_save_store_put(store, 0, "A", dd2_store_test_view()) &&
                               dd2_save_card_count(dd2_save_store_view(store)) == 0,
                           "unconfirmed candidate published\n");
    dd2_store_test_payload[0] = 1;
    dd2_store_test_pending(store);
    dd2_save_card_entry entry = {0};
    dd2_store_test_require(dd2_save_store_poll(store) == DD2_SAVE_STORE_OK &&
                               dd2_save_card_get(dd2_save_store_view(store), 0, &entry) &&
                               entry.payload.data[0] == 0,
                           "borrowed input was not staged\n");
    dd2_store_test_require(!dd2_save_store_put(store, 1, "A", entry.payload) &&
                               dd2_save_store_poll(store) == DD2_SAVE_STORE_DUPLICATE &&
                               !dd2_save_store_delete(store, 1) &&
                               dd2_save_store_poll(store) == DD2_SAVE_STORE_NOT_FOUND,
                           "rejected card mutation differs\n");
    dd2_store_test_failed(store);
    dd2_store_test_invalid(store);
    dd2_store_test_require(dd2_save_store_delete(store, 0), "delete begin differs\n");
    dd2_store_test_pending(store);
    dd2_store_test_require(dd2_save_store_poll(store) == DD2_SAVE_STORE_OK &&
                               dd2_save_card_count(dd2_save_store_view(store)) == 1 &&
                               dd2_save_store_close(store) &&
                               dd2_save_store_open(store, "delayed-test"),
                           "committed delete/close differs\n");
    dd2_store_test_pending(store);
    dd2_store_test_require(dd2_save_store_poll(store) == DD2_SAVE_STORE_OK &&
                               dd2_save_card_count(dd2_save_store_view(store)) == 1 &&
                               dd2_save_store_destroy(store),
                           "reopen/destruction differs\n");
    return EXIT_SUCCESS;
}
