#include "render/frustum.h"

#include "assets/bounds.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const double dd2_frustum_rounding_margin = 64 * FLT_EPSILON;
enum { DD2_FRUSTUM_AXES = 3 };
_Static_assert(sizeof(float) == sizeof(uint32_t), "Frustum finite checks require binary32 floats");

static bool dd2_frustum_finite(const float *value) {
    uint32_t representation = 0;
    unsigned char *target = (unsigned char *)&representation;
    const unsigned char *source = (const unsigned char *)value;
    for (size_t byte = 0; byte < sizeof(representation); ++byte) {
        target[byte] = source[byte];
    }
    return (representation & UINT32_C(0x7f800000)) != UINT32_C(0x7f800000);
}

dd2_frustum dd2_frustum_create(const float projection[DD2_FRUSTUM_MATRIX],
                               const float modelview[DD2_FRUSTUM_MATRIX]) {
    dd2_frustum result = {0};
    if (projection == NULL || modelview == NULL) {
        return result;
    }
    for (size_t index = 0; index < DD2_FRUSTUM_MATRIX; ++index) {
        if (!dd2_frustum_finite(&projection[index]) || !dd2_frustum_finite(&modelview[index])) {
            return result;
        }
    }
    double clip[DD2_FRUSTUM_MATRIX] = {0};
    for (size_t column = 0; column < DD2_FRUSTUM_COMPONENTS; ++column) {
        for (size_t row = 0; row < DD2_FRUSTUM_COMPONENTS; ++row) {
            for (size_t inner = 0; inner < DD2_FRUSTUM_COMPONENTS; ++inner) {
                clip[(column * DD2_FRUSTUM_COMPONENTS) + row] +=
                    (double)projection[(inner * DD2_FRUSTUM_COMPONENTS) + row] *
                    (double)modelview[(column * DD2_FRUSTUM_COMPONENTS) + inner];
            }
        }
    }
    for (size_t plane = 0; plane < DD2_FRUSTUM_PLANES; ++plane) {
        const double sign = (plane % 2) == 0 ? 1 : -1;
        for (size_t column = 0; column < DD2_FRUSTUM_COMPONENTS; ++column) {
            result.planes[plane][column] =
                clip[(column * DD2_FRUSTUM_COMPONENTS) + DD2_FRUSTUM_AXES] +
                (sign * clip[(column * DD2_FRUSTUM_COMPONENTS) + (plane / 2)]);
        }
    }
    result.valid = true;
    return result;
}

bool dd2_frustum_visible(const dd2_frustum *frustum, const dd2_bounds *bounds) {
    if (frustum == NULL || bounds == NULL || !frustum->valid) {
        return true;
    }
    for (size_t axis = 0; axis < DD2_FRUSTUM_AXES; ++axis) {
        if (!dd2_frustum_finite(&bounds->minimum[axis]) ||
            !dd2_frustum_finite(&bounds->maximum[axis]) ||
            bounds->minimum[axis] > bounds->maximum[axis]) {
            return true;
        }
    }
    for (size_t plane = 0; plane < DD2_FRUSTUM_PLANES; ++plane) {
        const double *normal = frustum->planes[plane];
        double support = normal[DD2_FRUSTUM_AXES];
        double magnitude = fabs(support) + 1;
        for (size_t axis = 0; axis < DD2_FRUSTUM_AXES; ++axis) {
            const double corner = normal[axis] >= 0 ? bounds->maximum[axis] : bounds->minimum[axis];
            support += normal[axis] * corner;
            magnitude += fabs(normal[axis]) * fmax(fabs((double)bounds->minimum[axis]),
                                                   fabs((double)bounds->maximum[axis]));
        }
        if (support < -(dd2_frustum_rounding_margin * magnitude)) {
            return false;
        }
    }
    return true;
}
