#ifndef DD2_RENDER_FRUSTUM_H
#define DD2_RENDER_FRUSTUM_H

#include "assets/bounds.h"

#include <stdbool.h>

enum { DD2_FRUSTUM_PLANES = 6, DD2_FRUSTUM_COMPONENTS = 4, DD2_FRUSTUM_MATRIX = 16 };

typedef struct {
    double planes[DD2_FRUSTUM_PLANES][DD2_FRUSTUM_COMPONENTS];
    bool valid;
} dd2_frustum;

/* Column-major GL projection/modelview matrices, including the actual object
 * pose. Tests model-local bounds against all six clip planes before submission.
 * Intersecting/touching bounds remain visible. A conservative float-rounding
 * margin protects raster edge cases; nonfinite matrices/bounds fail open. */
dd2_frustum dd2_frustum_create(const float projection[DD2_FRUSTUM_MATRIX],
                               const float modelview[DD2_FRUSTUM_MATRIX]);
bool dd2_frustum_visible(const dd2_frustum *frustum, const dd2_bounds *bounds);

#endif
