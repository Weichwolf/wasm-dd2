#ifndef DD2_TEST_CONTENT_FILE_FIXTURE_H
#define DD2_TEST_CONTENT_FILE_FIXTURE_H

#include "assets/bytes.h"
#include "assets/model.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static bool dd2_content_test_read(const char *root, const char *resource, dd2_file *file) {
    enum { DD2_CONTENT_TEST_PATH_BYTES = 4096 };
    char path[DD2_CONTENT_TEST_PATH_BYTES] = {0};
    const size_t prefix = strlen(root);
    const size_t name = strlen(resource);
    if (prefix + name + 2 > sizeof(path)) {
        return false;
    }
    for (size_t index = 0; index < prefix; ++index) {
        path[index] = root[index];
    }
    path[prefix] = '/';
    for (size_t index = 0; index < name; ++index) {
        path[prefix + index + 1] = resource[index];
    }
    return dd2_file_read(path, file);
}

static dd2_model *dd2_content_test_model(void *user, const char *resource) {
    dd2_file file = {0};
    if (!dd2_content_test_read(user, resource, &file)) {
        return NULL;
    }
    dd2_model *model = dd2_model_create((dd2_byte_view){file.data, file.size});
    dd2_file_release(&file);
    return model;
}

#endif
