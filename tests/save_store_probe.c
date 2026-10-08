#include "assets/bytes.h"
#include "assets/save_card.h"
#include "platform/save_store.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum { DD2_SAVE_PROBE_OWNERS = 2 };
static dd2_save_store *dd2_save_probe_owners[DD2_SAVE_PROBE_OWNERS];
int dd2_save_probe_open(unsigned owner, const char *location);
int dd2_save_probe_poll(unsigned owner);
int dd2_save_probe_phase(unsigned owner);
int dd2_save_probe_put(unsigned owner, unsigned logical, const char *name, const uint8_t *bytes,
                       size_t size);
int dd2_save_probe_delete(unsigned owner, unsigned logical);
int dd2_save_probe_reload(unsigned owner);
int dd2_save_probe_close(unsigned owner);
unsigned dd2_save_probe_count(unsigned owner);
uintptr_t dd2_save_probe_image(unsigned owner);
size_t dd2_save_probe_size(unsigned owner);
static dd2_save_store *dd2_save_probe_owner(unsigned owner) {
    return owner < DD2_SAVE_PROBE_OWNERS ? dd2_save_probe_owners[owner] : NULL;
}
int dd2_save_probe_open(unsigned owner, const char *location) {
    if (owner >= DD2_SAVE_PROBE_OWNERS) {
        return 0;
    }
    if (dd2_save_probe_owners[owner] == NULL) {
        dd2_save_probe_owners[owner] = dd2_save_store_create();
    }
    return (int)dd2_save_store_open(dd2_save_probe_owners[owner], location);
}
int dd2_save_probe_poll(unsigned owner) {
    return (int)dd2_save_store_poll(dd2_save_probe_owner(owner));
}
int dd2_save_probe_phase(unsigned owner) {
    return (int)dd2_save_store_state(dd2_save_probe_owner(owner));
}
int dd2_save_probe_put(unsigned owner, unsigned logical, const char *name, const uint8_t *bytes,
                       size_t size) {
    return (int)dd2_save_store_put(dd2_save_probe_owner(owner), logical, name,
                                   (dd2_byte_view){.data = bytes, .size = size});
}
int dd2_save_probe_delete(unsigned owner, unsigned logical) {
    return (int)dd2_save_store_delete(dd2_save_probe_owner(owner), logical);
}
int dd2_save_probe_reload(unsigned owner) {
    return (int)dd2_save_store_reload(dd2_save_probe_owner(owner));
}
int dd2_save_probe_close(unsigned owner) {
    if (owner >= DD2_SAVE_PROBE_OWNERS || !dd2_save_store_destroy(dd2_save_probe_owners[owner])) {
        return 0;
    }
    dd2_save_probe_owners[owner] = NULL;
    return 1;
}
unsigned dd2_save_probe_count(unsigned owner) {
    return dd2_save_card_count(dd2_save_store_view(dd2_save_probe_owner(owner)));
}
uintptr_t dd2_save_probe_image(unsigned owner) {
    return (uintptr_t)dd2_save_card_image(dd2_save_store_view(dd2_save_probe_owner(owner))).data;
}
size_t dd2_save_probe_size(unsigned owner) {
    return dd2_save_card_image(dd2_save_store_view(dd2_save_probe_owner(owner))).size;
}
int main(void) {
    return EXIT_SUCCESS;
}
