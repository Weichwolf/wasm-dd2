#ifndef DD2_PLATFORM_SAVE_LOCATION_H
#define DD2_PLATFORM_SAVE_LOCATION_H

#include "platform/save_store.h"

#include <stdbool.h>

/* Explicit frontend default: SDL's dedicated per-user writable Native folder
 * (created independently of game assets), or the browser profile database.
 * Storage acceptance still needs polling. Never defaults to provisioned data. */
bool dd2_save_store_open_user(dd2_save_store *store);

#endif
