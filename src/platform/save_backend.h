#ifndef DD2_PLATFORM_SAVE_BACKEND_H
#define DD2_PLATFORM_SAVE_BACKEND_H

#include "assets/bytes.h"
#include "platform/save_store.h"

#include <stdbool.h>

typedef struct dd2_save_backend dd2_save_backend;
/* Private backend. All reads are complete physical images. Open/read/write
 * results are collected by the owner; callers do not reuse pending backends. */
dd2_save_backend *dd2_save_backend_create(void);
void dd2_save_backend_destroy(dd2_save_backend *backend);
void dd2_save_backend_open(dd2_save_backend *backend, const char *location);
void dd2_save_backend_read(dd2_save_backend *backend);
void dd2_save_backend_write(dd2_save_backend *backend, dd2_byte_view expected, bool existed,
                            dd2_byte_view replacement);
dd2_save_store_result dd2_save_backend_poll(dd2_save_backend *backend);
dd2_byte_view dd2_save_backend_image(const dd2_save_backend *backend);
bool dd2_save_backend_exists(const dd2_save_backend *backend);
void dd2_save_backend_close(dd2_save_backend *backend);

#endif
