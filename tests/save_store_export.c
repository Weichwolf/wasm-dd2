#include "assets/bytes.h"
#include "assets/save_card.h"
#include "platform/file.h"
#include "platform/save_store.h"

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Linker wrappers retain production backend source and actual filesystem I/O.
 * Named assembly aliases are fixture linkage, never game address emulation. */
ssize_t dd2_store_real_write(int descriptor, const void *bytes,
                             size_t size) __asm__("__real_write");
ssize_t dd2_store_fault_write(int descriptor, const void *bytes,
                              size_t size) __asm__("__wrap_write");
ssize_t dd2_store_real_read(int descriptor, void *bytes, size_t size) __asm__("__real_read");
ssize_t dd2_store_fault_read(int descriptor, void *bytes, size_t size) __asm__("__wrap_read");
int dd2_store_real_fsync(int descriptor) __asm__("__real_fsync");
int dd2_store_fault_fsync(int descriptor) __asm__("__wrap_fsync");
int dd2_store_real_close(int descriptor) __asm__("__real_close");
int dd2_store_fault_close(int descriptor) __asm__("__wrap_close");
int dd2_store_real_rename(int old_directory, const char *old_name, int new_directory,
                          const char *new_name) __asm__("__real_renameat");
int dd2_store_fault_rename(int old_directory, const char *old_name, int new_directory,
                           const char *new_name) __asm__("__wrap_renameat");
enum {
    DD2_STORE_FAULT_NONE = 0,
    DD2_STORE_FAULT_SHORT = 1,
    DD2_STORE_FAULT_PARTIAL = 2,
    DD2_STORE_FAULT_ZERO = 3,
    DD2_STORE_FAULT_FILE_SYNC = 4,
    DD2_STORE_FAULT_RENAME = 5,
    DD2_STORE_FAULT_DIRECTORY_SYNC = 6,
    DD2_STORE_FAULT_CLOSE = 7,
    DD2_STORE_FAULT_READ_INTERRUPT = 8,
    DD2_STORE_FAULT_STOP_FILE = 9,
    DD2_STORE_FAULT_STOP_DIRECTORY = 10,
    DD2_STORE_FAULT_READ = 12,
    DD2_STORE_EXPORT_ARGUMENTS = 8,
    DD2_STORE_EXPORT_NAME = 5,
    DD2_STORE_EXPORT_PAYLOAD = 6,
    DD2_STORE_EXPORT_FAULT = 7,
    DD2_STORE_EXPORT_BASE = 10,
    DD2_STORE_EXPORT_SHORT_BYTES = 97,
    DD2_STORE_EXPORT_PARTIAL_BYTES = 17,
    DD2_STORE_EXPORT_PATH_BYTES = 4096
};
static unsigned dd2_store_fault_mode;
static unsigned dd2_store_fault_writes;
static unsigned dd2_store_fault_reads;
static bool dd2_store_fault_synced;
ssize_t dd2_store_fault_write(int descriptor, const void *bytes, size_t size) {
    const unsigned step = dd2_store_fault_writes++;
    if (dd2_store_fault_mode == DD2_STORE_FAULT_ZERO) {
        return 0;
    }
    if ((dd2_store_fault_mode == DD2_STORE_FAULT_SHORT ||
         dd2_store_fault_mode == DD2_STORE_FAULT_READ_INTERRUPT) &&
        step == 0) {
        errno = EINTR;
        return -1;
    }
    if (dd2_store_fault_mode == DD2_STORE_FAULT_PARTIAL) {
        if (step != 0) {
            errno = ENOSPC;
            return -1;
        }
        size = size > DD2_STORE_EXPORT_PARTIAL_BYTES ? DD2_STORE_EXPORT_PARTIAL_BYTES : size;
    } else if (dd2_store_fault_mode == DD2_STORE_FAULT_SHORT ||
               dd2_store_fault_mode == DD2_STORE_FAULT_READ_INTERRUPT) {
        size = size > DD2_STORE_EXPORT_SHORT_BYTES ? DD2_STORE_EXPORT_SHORT_BYTES : size;
    }
    return dd2_store_real_write(descriptor, bytes, size);
}
ssize_t dd2_store_fault_read(int descriptor, void *bytes, size_t size) {
    if (dd2_store_fault_mode == DD2_STORE_FAULT_READ) {
        errno = EIO;
        return -1;
    }
    if (dd2_store_fault_mode == DD2_STORE_FAULT_READ_INTERRUPT && dd2_store_fault_reads++ == 0) {
        errno = EINTR;
        return -1;
    }
    return dd2_store_real_read(descriptor, bytes, size);
}
static void dd2_store_fault_stop(void) {
    if (fputs("checkpoint\n", stdout) == EOF || fflush(stdout) != 0 || raise(SIGSTOP) != 0) {
        abort();
    }
}
int dd2_store_fault_fsync(int descriptor) {
    struct stat metadata;
    if (fstat(descriptor, &metadata) != 0) {
        return -1;
    }
    const bool directory = S_ISDIR(metadata.st_mode);
    if ((directory && dd2_store_fault_mode == DD2_STORE_FAULT_STOP_DIRECTORY) ||
        (!directory && dd2_store_fault_mode == DD2_STORE_FAULT_STOP_FILE)) {
        dd2_store_fault_stop();
    }
    if ((directory && dd2_store_fault_mode == DD2_STORE_FAULT_DIRECTORY_SYNC) ||
        (!directory && dd2_store_fault_mode == DD2_STORE_FAULT_FILE_SYNC)) {
        errno = EIO;
        return -1;
    }
    if (!directory && dd2_store_fault_mode == DD2_STORE_FAULT_CLOSE) {
        dd2_store_fault_synced = true;
    }
    return dd2_store_real_fsync(descriptor);
}
int dd2_store_fault_close(int descriptor) {
    const int result = dd2_store_real_close(descriptor);
    if (dd2_store_fault_synced && dd2_store_fault_mode == DD2_STORE_FAULT_CLOSE) {
        dd2_store_fault_synced = false;
        errno = EIO;
        return -1;
    }
    return result;
}
int dd2_store_fault_rename(int old_directory, const char *old_name, int new_directory,
                           const char *new_name) {
    if (dd2_store_fault_mode == DD2_STORE_FAULT_RENAME) {
        errno = EACCES;
        return -1;
    }
    return dd2_store_real_rename(old_directory, old_name, new_directory, new_name);
}
static bool dd2_store_export_unsigned(const char *argument, unsigned *value) {
    char *end = NULL;
    errno = 0;
    const unsigned long parsed = strtoul(argument, &end, DD2_STORE_EXPORT_BASE);
    if (errno != 0 || end == argument || *end != '\0' || parsed > UINT_MAX) {
        return false;
    }
    *value = (unsigned)parsed;
    return true;
}
static bool dd2_store_export_write(const char *path, dd2_byte_view bytes) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    const bool written = fwrite(bytes.data, 1, bytes.size, file) == bytes.size;
    const int closed = fclose(file);
    return written && closed == 0;
}
static bool dd2_store_export_stale(const char *directory, dd2_byte_view bytes) {
    char path[DD2_STORE_EXPORT_PATH_BYTES];
    const size_t length = strlen(directory);
    const char suffix[] = "/SaveGames";
    if (length >= sizeof(path) - sizeof(suffix)) {
        return false;
    }
    for (size_t index = 0; index < length; ++index) {
        path[index] = directory[index];
    }
    for (size_t index = 0; index < sizeof(suffix); ++index) {
        path[length + index] = suffix[index];
    }
    uint8_t image[DD2_SAVE_CARD_BYTES];
    for (size_t index = 0; index < sizeof(image); ++index) {
        image[index] = bytes.data[index];
    }
    image[sizeof(image) - 1] ^= 1;
    return dd2_store_export_write(path, (dd2_byte_view){.data = image, .size = sizeof(image)});
}
static bool dd2_store_export_operation(dd2_save_store *store, char **argv, unsigned logical) {
    const char *operation = argv[3];
    if (strcmp(operation, "inspect") == 0) {
        return true;
    }
    if (strcmp(operation, "lock") == 0) {
        dd2_save_store *second = dd2_save_store_create();
        const bool conflict = second != NULL && dd2_save_store_open(second, argv[1]) &&
                              dd2_save_store_poll(second) == DD2_SAVE_STORE_CONFLICT;
        const bool destroyed = dd2_save_store_destroy(second);
        return conflict && destroyed;
    }
    if (strcmp(operation, "delete") == 0) {
        return dd2_save_store_delete(store, logical);
    }
    dd2_file payload = {0};
    bool accepted = dd2_file_read(argv[DD2_STORE_EXPORT_PAYLOAD], &payload);
    if (accepted && strcmp(operation, "stale-put") == 0) {
        accepted = dd2_store_export_stale(argv[1], dd2_save_card_image(dd2_save_store_view(store)));
    }
    if (accepted && (strcmp(operation, "put") == 0 || strcmp(operation, "recover") == 0 ||
                     strcmp(operation, "stale-put") == 0)) {
        accepted = dd2_save_store_put(store, logical, argv[DD2_STORE_EXPORT_NAME],
                                      (dd2_byte_view){.data = payload.data, .size = payload.size});
    } else {
        accepted = false;
    }
    dd2_file_release(&payload);
    return accepted;
}
static bool dd2_store_export_run(char **argv) {
    unsigned logical = 0;
    unsigned fault = 0;
    if (!dd2_store_export_unsigned(argv[4], &logical) ||
        !dd2_store_export_unsigned(argv[DD2_STORE_EXPORT_FAULT], &fault)) {
        return false;
    }
    dd2_save_store *store = dd2_save_store_create();
    if (store == NULL) {
        return false;
    }
    bool valid = dd2_save_store_open(store, argv[1]);
    dd2_save_store_result result = dd2_save_store_poll(store);
    bool accepted = false;
    bool unpublished = false;
    if (valid && result == DD2_SAVE_STORE_OK) {
        const dd2_save_card *before = dd2_save_store_view(store);
        dd2_store_fault_mode = fault;
        accepted = dd2_store_export_operation(store, argv, logical);
        unpublished = dd2_save_store_view(store) == before;
        result = dd2_save_store_poll(store);
    }
    const dd2_save_store_result first = result;
    if (valid && strcmp(argv[3], "recover") == 0 && result == DD2_SAVE_STORE_INDETERMINATE) {
        dd2_store_fault_mode = DD2_STORE_FAULT_NONE;
        valid = dd2_save_store_reload(store);
        result = dd2_save_store_poll(store);
    }
    const dd2_byte_view image = dd2_save_card_image(dd2_save_store_view(store));
    if (valid && image.data != NULL) {
        valid = dd2_store_export_write(argv[2], image);
    }
    printf("{\"accepted\":%d,\"unpublished\":%d,\"first\":%d,\"result\":%d,\"phase\":%d,\"count\":%"
           "u}\n",
           (int)accepted, (int)unpublished, (int)first, (int)result,
           (int)dd2_save_store_state(store), dd2_save_card_count(dd2_save_store_view(store)));
    const bool destroyed = dd2_save_store_destroy(store);
    return valid && destroyed;
}
int main(int argc, char **argv) {
    const char prefix[] = "/tmp/wasm-dd2/";
    return argc == DD2_STORE_EXPORT_ARGUMENTS &&
                   strncmp(argv[1], prefix, sizeof(prefix) - 1) == 0 &&
                   strncmp(argv[2], prefix, sizeof(prefix) - 1) == 0 && dd2_store_export_run(argv)
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
