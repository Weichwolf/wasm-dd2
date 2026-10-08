#include "platform/save_backend.h"

#include "assets/bytes.h"
#include "assets/save_card.h"
#include "platform/save_store.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __EMSCRIPTEN__

#include <emscripten/em_js.h>
#include <stdint.h>
#include <stdlib.h>

struct dd2_save_backend {
    int identifier;
    int existed;
    bool active;
    dd2_save_store_result result;
    uint8_t image[DD2_SAVE_CARD_BYTES];
};
EM_JS(int, dd2_save_web_create, (void), {
    try {
        return Module.dd2SaveBackend.create();
    } catch (error) {
        return 0;
    }
})
EM_JS(int, dd2_save_web_open, (int identifier, const char *location), {
    try {
        return Module.dd2SaveBackend.open(identifier, UTF8ToString(location));
    } catch (error) {
        return 0;
    }
})
EM_JS(int, dd2_save_web_read, (int identifier), { return Module.dd2SaveBackend.read(identifier); })
EM_JS(int, dd2_save_web_write,
      (int identifier, const uint8_t *previous, const uint8_t *replacement, int existed, int size),
      {
          try {
              return Module.dd2SaveBackend.write(
                  identifier, HEAPU8.slice(previous, previous + size),
                  HEAPU8.slice(replacement, replacement + size), existed != 0);
          } catch (error) {
              return 0;
          }
      })
EM_JS(int, dd2_save_web_poll, (int identifier, uint8_t *image, int *existed, int size), {
    const record = Module.dd2SaveBackend.poll(identifier);
    if (!record)
        return 5;
    if (record.status == 2 && record.data) {
        if (record.data.length != size)
            return 3;
        HEAPU8.set(record.data, image);
        HEAP32[existed >> 2] = record.exists ? 1 : 0;
    }
    return record.status;
})
EM_JS(void, dd2_save_web_close, (int identifier), { Module.dd2SaveBackend.close(identifier); })

dd2_save_backend *dd2_save_backend_create(void) {
    return calloc(1, sizeof(dd2_save_backend));
}
void dd2_save_backend_close(dd2_save_backend *backend) {
    if (backend->identifier != 0) {
        dd2_save_web_close(backend->identifier);
        backend->identifier = 0;
    }
    backend->active = false;
    backend->result = DD2_SAVE_STORE_IDLE;
}
void dd2_save_backend_destroy(dd2_save_backend *backend) {
    if (backend != NULL) {
        dd2_save_backend_close(backend);
        free(backend);
    }
}
void dd2_save_backend_open(dd2_save_backend *backend, const char *location) {
    if (backend->identifier == 0) {
        backend->identifier = dd2_save_web_create();
    }
    backend->result = DD2_SAVE_STORE_NO_MEMORY;
    backend->active =
        backend->identifier > 0 && dd2_save_web_open(backend->identifier, location) != 0;
}
void dd2_save_backend_read(dd2_save_backend *backend) {
    backend->result = DD2_SAVE_STORE_IO_ERROR;
    backend->active = backend->identifier > 0 && dd2_save_web_read(backend->identifier) != 0;
}
void dd2_save_backend_write(dd2_save_backend *backend, dd2_byte_view expected, bool existed,
                            dd2_byte_view replacement) {
    backend->result = DD2_SAVE_STORE_NO_MEMORY;
    backend->active = dd2_save_web_write(backend->identifier, expected.data, replacement.data,
                                         (int)existed, DD2_SAVE_CARD_BYTES) != 0;
}
dd2_save_store_result dd2_save_backend_poll(dd2_save_backend *backend) {
    if (backend->active) {
        const int result = dd2_save_web_poll(backend->identifier, backend->image, &backend->existed,
                                             DD2_SAVE_CARD_BYTES);
        backend->result = result >= DD2_SAVE_STORE_IDLE && result <= DD2_SAVE_STORE_NOT_FOUND
                              ? (dd2_save_store_result)result
                              : DD2_SAVE_STORE_IO_ERROR;
        backend->active = backend->result == DD2_SAVE_STORE_PENDING;
    }
    return backend->result;
}
dd2_byte_view dd2_save_backend_image(const dd2_save_backend *backend) {
    return (dd2_byte_view){.data = backend->image, .size = sizeof(backend->image)};
}
bool dd2_save_backend_exists(const dd2_save_backend *backend) {
    return backend->existed != 0;
}

#else

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

enum { DD2_SAVE_BACKEND_PERMISSIONS = 0600 };
static const char dd2_save_backend_filename[] = "SaveGames";
static const char dd2_save_backend_lockname[] = "SaveGames.lock";
static const char dd2_save_backend_pending[] = ".SaveGames.pending";
struct dd2_save_backend {
    int directory;
    int lock;
    dd2_save_store_result result;
    bool existed;
    uint8_t image[DD2_SAVE_CARD_BYTES];
};
static bool dd2_save_backend_release(int *descriptor) {
    const int owned = *descriptor;
    *descriptor = -1;
    return owned < 0 || close(owned) == 0;
}
void dd2_save_backend_close(dd2_save_backend *backend) {
    const bool unlocked = dd2_save_backend_release(&backend->lock);
    const bool closed = dd2_save_backend_release(&backend->directory);
    backend->result = unlocked && closed ? DD2_SAVE_STORE_IDLE : DD2_SAVE_STORE_IO_ERROR;
}
dd2_save_backend *dd2_save_backend_create(void) {
    dd2_save_backend *backend = calloc(1, sizeof(*backend));
    if (backend != NULL) {
        backend->directory = -1;
        backend->lock = -1;
    }
    return backend;
}
void dd2_save_backend_destroy(dd2_save_backend *backend) {
    if (backend != NULL) {
        dd2_save_backend_close(backend);
        free(backend);
    }
}
static bool dd2_save_backend_discard_pending(dd2_save_backend *backend) {
    struct stat pending;
    if (fstatat(backend->directory, dd2_save_backend_pending, &pending, AT_SYMLINK_NOFOLLOW) != 0) {
        return errno == ENOENT;
    }
    /* The exclusive lease proves no cooperating pending writer is live.
     * Unlink this one reserved path without following a symlink. */
    return (S_ISREG(pending.st_mode) || S_ISLNK(pending.st_mode)) &&
           unlinkat(backend->directory, dd2_save_backend_pending, 0) == 0;
}
static dd2_save_store_result dd2_save_backend_read_file(dd2_save_backend *backend, int file) {
    struct stat metadata;
    if (fstat(file, &metadata) != 0) {
        return DD2_SAVE_STORE_IO_ERROR;
    }
    if (!S_ISREG(metadata.st_mode) || metadata.st_size != DD2_SAVE_CARD_BYTES) {
        return DD2_SAVE_STORE_INVALID;
    }
    size_t offset = 0;
    while (offset < sizeof(backend->image)) {
        const ssize_t bytes = read(file, &backend->image[offset], sizeof(backend->image) - offset);
        if (bytes < 0 && errno == EINTR) {
            continue;
        }
        if (bytes <= 0) {
            return DD2_SAVE_STORE_IO_ERROR;
        }
        offset += (size_t)bytes;
    }
    uint8_t extra = 0;
    ssize_t end;
    do {
        end = read(file, &extra, sizeof(extra));
    } while (end < 0 && errno == EINTR);
    return end == 0 ? DD2_SAVE_STORE_OK : DD2_SAVE_STORE_IO_ERROR;
}
void dd2_save_backend_read(dd2_save_backend *backend) {
    backend->result = DD2_SAVE_STORE_IO_ERROR;
    if (backend->directory < 0 || backend->lock < 0 || !dd2_save_backend_discard_pending(backend)) {
        return;
    }
    int file = openat(backend->directory, dd2_save_backend_filename,
                      O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
    if (file < 0) {
        if (errno == ENOENT) {
            for (size_t index = 0; index < sizeof(backend->image); ++index) {
                backend->image[index] = 0;
            }
            backend->existed = false;
            backend->result = DD2_SAVE_STORE_OK;
        } else if (errno == ELOOP) {
            backend->result = DD2_SAVE_STORE_INVALID;
        }
        return;
    }
    backend->result = dd2_save_backend_read_file(backend, file);
    if (!dd2_save_backend_release(&file)) {
        backend->result = DD2_SAVE_STORE_IO_ERROR;
    }
    if (backend->result == DD2_SAVE_STORE_OK) {
        backend->existed = true;
    }
}
void dd2_save_backend_open(dd2_save_backend *backend, const char *location) {
    backend->result = DD2_SAVE_STORE_IO_ERROR;
    backend->directory = open(location, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (backend->directory < 0) {
        return;
    }
    backend->lock = openat(backend->directory, dd2_save_backend_lockname,
                           O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, DD2_SAVE_BACKEND_PERMISSIONS);
    if (backend->lock < 0) {
        return;
    }
    struct stat metadata;
    if (fstat(backend->lock, &metadata) != 0 || !S_ISREG(metadata.st_mode)) {
        return;
    }
    if (flock(backend->lock, LOCK_EX | LOCK_NB) != 0) {
        backend->result = errno == EWOULDBLOCK ? DD2_SAVE_STORE_CONFLICT : DD2_SAVE_STORE_IO_ERROR;
        return;
    }
    dd2_save_backend_read(backend);
}
static bool dd2_save_backend_sync(int descriptor) {
    int result;
    do {
        result = fsync(descriptor);
    } while (result != 0 && errno == EINTR);
    return result == 0;
}
static bool dd2_save_backend_write_file(int file, dd2_byte_view replacement) {
    size_t offset = 0;
    while (offset < replacement.size) {
        const ssize_t written = write(file, &replacement.data[offset], replacement.size - offset);
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            return false;
        }
        offset += (size_t)written;
    }
    return dd2_save_backend_sync(file);
}
void dd2_save_backend_write(dd2_save_backend *backend, dd2_byte_view expected, bool existed,
                            dd2_byte_view replacement) {
    dd2_save_backend_read(backend);
    if (backend->result != DD2_SAVE_STORE_OK) {
        return;
    }
    if (backend->existed != existed ||
        (existed && memcmp(backend->image, expected.data, expected.size) != 0)) {
        backend->result = DD2_SAVE_STORE_CONFLICT;
        return;
    }
    backend->result = DD2_SAVE_STORE_IO_ERROR;
    int file =
        openat(backend->directory, dd2_save_backend_pending,
               O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, DD2_SAVE_BACKEND_PERMISSIONS);
    if (file < 0) {
        return;
    }
    const bool written = dd2_save_backend_write_file(file, replacement);
    const bool closed = dd2_save_backend_release(&file);
    if (!written || !closed) {
        if (unlinkat(backend->directory, dd2_save_backend_pending, 0) != 0) {
            backend->result = DD2_SAVE_STORE_IO_ERROR;
        }
        return;
    }
    if (renameat(backend->directory, dd2_save_backend_pending, backend->directory,
                 dd2_save_backend_filename) != 0) {
        if (unlinkat(backend->directory, dd2_save_backend_pending, 0) != 0) {
            backend->result = DD2_SAVE_STORE_IO_ERROR;
        }
        return;
    }
    /* Replacement is already visible. A failed directory sync cannot be
     * described as unchanged disk or as a successful durable save. */
    backend->existed = true;
    backend->result = dd2_save_backend_sync(backend->directory) ? DD2_SAVE_STORE_OK
                                                                : DD2_SAVE_STORE_INDETERMINATE;
}
dd2_save_store_result dd2_save_backend_poll(dd2_save_backend *backend) {
    return backend->result;
}
dd2_byte_view dd2_save_backend_image(const dd2_save_backend *backend) {
    return (dd2_byte_view){.data = backend->image, .size = sizeof(backend->image)};
}
bool dd2_save_backend_exists(const dd2_save_backend *backend) {
    return backend->existed;
}

#endif
