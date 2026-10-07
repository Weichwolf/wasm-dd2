#ifndef DD2_TEST_CONTACT_PATCH_FIXTURE_H
#define DD2_TEST_CONTACT_PATCH_FIXTURE_H

#include "physics/vehicle.h"

/* Original arena 8, physical step 2017, first contact between slots 3 and 19.
 * Captured at c540948 before the fleet advanced to that contact. These typed
 * trajectories reproduce an empty softened front patch; they are not an
 * original-engine or complete-arena acceptance fixture. */
static const dd2_vehicle dd2_patch_start[2] = {
    {
        .position = {.x = 7364.760966421662, .y = -668.4293099334246, .z = -7565.106579574731},
        .velocity = {.x = 1025.2920292225738, .y = 127.74639968156886, .z = -534.2311178701076},
        .rotation = {.x = 0.02967437007456285,
                     .y = 0.19049151205153225,
                     .z = 0.041749724809284995,
                     .w = 0.9803516593933771},
        .angular_velocity = {.x = -0.031625710713745384,
                             .y = 0.8532046877689221,
                             .z = 0.09263785017063085},
    },
    {
        .position = {.x = 6769.545761196612, .y = -737.9784246061652, .z = -7270.452407664308},
        .velocity = {.x = 1103.402228907084, .y = 105.06285915738371, .z = -346.61249828778165},
        .rotation = {.x = 0.004203390552093338,
                     .y = -0.7616116800137392,
                     .z = -0.05043504805577873,
                     .w = -0.6460543988722062},
        .angular_velocity = {.x = 0.015836790092263936,
                             .y = 0.11087805762336704,
                             .z = 0.03996876704945715},
    },
};

static const dd2_vehicle dd2_patch_end[2] = {
    {
        .position = {.x = 7369.865274119356, .y = -667.7933380223371, .z = -7567.766192572366},
        .velocity = {.x = 1025.2920292225738, .y = 127.74639968156886, .z = -534.2311178701076},
        .rotation = {.x = 0.029641868154629762,
                     .y = 0.19258326690186942,
                     .z = 0.04189767389108689,
                     .w = 0.9799375642785731},
        .angular_velocity = {.x = -0.031625710713745384,
                             .y = 0.8532046877689221,
                             .z = 0.09263785017063085},
    },
    {
        .position = {.x = 6775.038932244605, .y = -737.4553802973691, .z = -7272.1779812498835},
        .velocity = {.x = 1103.402228907084, .y = 105.06285915738371, .z = -346.61249828778165},
        .rotation = {.x = 0.004239775237137969,
                     .y = -0.761787549535311,
                     .z = -0.05053050541657456,
                     .w = -0.6458393156980071},
        .angular_velocity = {.x = 0.015836790092263936,
                             .y = 0.11087805762336704,
                             .z = 0.03996876704945715},
    },
};

#endif
