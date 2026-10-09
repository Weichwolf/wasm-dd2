#include "assets/bounds.h"
#include "render/frustum.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static bool dd2_frustum_test_orthographic(void) {
    const float identity[DD2_FRUSTUM_MATRIX] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    const dd2_frustum frustum = dd2_frustum_create(identity, identity);
    const dd2_bounds center = {.minimum = {-1, -1, -1}, .maximum = {1, 1, 1}};
    if (!frustum.valid || !dd2_frustum_visible(&frustum, &center)) {
        return false;
    }
    for (size_t axis = 0; axis < 3; ++axis) {
        for (int direction = -1; direction <= 1; direction += 2) {
            dd2_bounds box = {.minimum = {0, 0, 0}, .maximum = {0, 0, 0}};
            box.minimum[axis] = (float)direction * 2;
            box.maximum[axis] = box.minimum[axis];
            if (dd2_frustum_visible(&frustum, &box)) {
                return false;
            }
            box.minimum[axis] = -2;
            box.maximum[axis] = 2;
            if (!dd2_frustum_visible(&frustum, &box)) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_frustum_test_perspective(void) {
    // Near 1, far 3, symmetric 90-degree perspective, column-major.
    const float projection[DD2_FRUSTUM_MATRIX] = {1, 0, 0,  0,  0, 1, 0,  0,
                                                  0, 0, -2, -1, 0, 0, -3, 0};
    float view[DD2_FRUSTUM_MATRIX] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    dd2_frustum frustum = dd2_frustum_create(projection, view);
    dd2_bounds box = {.minimum = {0, 0, -2}, .maximum = {0, 0, -2}};
    if (!dd2_frustum_visible(&frustum, &box)) {
        return false;
    }
    const float rejected[] = {2, -.5F, -4};
    for (size_t index = 0; index < sizeof(rejected) / sizeof(rejected[0]); ++index) {
        box.minimum[2] = rejected[index];
        box.maximum[2] = rejected[index];
        if (dd2_frustum_visible(&frustum, &box)) {
            return false;
        }
    }
    const size_t translation = 14;
    view[translation] = -2;
    frustum = dd2_frustum_create(projection, view);
    box = (dd2_bounds){.minimum = {0, 0, 0}, .maximum = {0, 0, 0}};
    return dd2_frustum_visible(&frustum, &box);
}

static bool dd2_frustum_test_fail_open(void) {
    float identity[DD2_FRUSTUM_MATRIX] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    dd2_frustum frustum = dd2_frustum_create(identity, identity);
    dd2_bounds box = {.minimum = {2, 0, 0}, .maximum = {1, 0, 0}};
    if (!dd2_frustum_visible(&frustum, &box) || !dd2_frustum_visible(NULL, &box)) {
        return false;
    }
    const uint32_t nan = UINT32_C(0x7fc12345);
    const unsigned char *source = (const unsigned char *)&nan;
    unsigned char *destination = (unsigned char *)&identity[0];
    for (size_t byte = 0; byte < sizeof(nan); ++byte) {
        destination[byte] = source[byte];
    }
    frustum = dd2_frustum_create(identity, identity);
    return !frustum.valid && dd2_frustum_visible(&frustum, &box);
}

int main(void) {
    return dd2_frustum_test_orthographic() && dd2_frustum_test_perspective() &&
                   dd2_frustum_test_fail_open()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
