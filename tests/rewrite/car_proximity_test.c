#include "physics/car_contact.h"
#include "physics/vehicle.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_NEAR_AXES = 3, DD2_NEAR_CORNERS = 8, DD2_NEAR_POSES = 48 };
static const double dd2_near_half[DD2_NEAR_AXES] = {186, 130, 450};
static const double dd2_near_tolerance = 1e-7;
static const double dd2_near_margin = 0.00048828125;
static const double dd2_near_far = 100000;
static const double dd2_near_radius = 504;
static const double dd2_near_axis_tolerance = 1e-12;

typedef struct {
    unsigned axis;
    unsigned pose;
    double gap;
    double direction;
    double origin;
} dd2_near_face_case;

static double dd2_near_dot(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return (first.x * second.x) + (first.y * second.y) + (first.z * second.z);
}

static dd2_vehicle_vector dd2_near_scale(dd2_vehicle_vector value, double factor) {
    return (dd2_vehicle_vector){value.x * factor, value.y * factor, value.z * factor};
}

static dd2_vehicle_vector dd2_near_subtract(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return (dd2_vehicle_vector){first.x - second.x, first.y - second.y, first.z - second.z};
}

static dd2_vehicle_vector dd2_near_cross(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return (dd2_vehicle_vector){(first.y * second.z) - (first.z * second.y),
                                (first.z * second.x) - (first.x * second.z),
                                (first.x * second.y) - (first.y * second.x)};
}

static dd2_vehicle_rotation dd2_near_rotation(unsigned sample) {
    const double phase = (double)sample;
    dd2_vehicle_rotation rotation = {
        .x = sin(phase), .y = sin(phase / 2), .z = sin(phase / 3), .w = cos(phase / 4)};
    const double length = sqrt((rotation.x * rotation.x) + (rotation.y * rotation.y) +
                               (rotation.z * rotation.z) + (rotation.w * rotation.w));
    rotation.x /= length;
    rotation.y /= length;
    rotation.z /= length;
    rotation.w /= length;
    return rotation;
}

static void dd2_near_axes(dd2_vehicle_rotation rotation, dd2_vehicle_vector axes[DD2_NEAR_AXES]) {
    const dd2_vehicle_vector basis[DD2_NEAR_AXES] = {{.x = 1}, {.y = 1}, {.z = 1}};
    for (unsigned axis = 0; axis < DD2_NEAR_AXES; ++axis) {
        axes[axis] = dd2_vehicle_rotate(rotation, basis[axis]);
    }
}

static void dd2_near_corners(const dd2_vehicle *vehicle,
                             dd2_vehicle_vector corners[DD2_NEAR_CORNERS]) {
    for (unsigned corner = 0; corner < DD2_NEAR_CORNERS; ++corner) {
        const dd2_vehicle_vector local = {(corner & 1U ? 1 : -1) * dd2_near_half[0],
                                          (corner & 2U ? 1 : -1) * dd2_near_half[1],
                                          (corner & 4U ? 1 : -1) * dd2_near_half[2]};
        corners[corner] = dd2_vehicle_rotate(vehicle->rotation, local);
        corners[corner].x += vehicle->position.x;
        corners[corner].y += vehicle->position.y;
        corners[corner].z += vehicle->position.z;
    }
}

static double dd2_near_axis_depth(const dd2_vehicle_vector first[DD2_NEAR_CORNERS],
                                  const dd2_vehicle_vector second[DD2_NEAR_CORNERS],
                                  dd2_vehicle_vector axis) {
    double first_min = DBL_MAX;
    double first_max = -DBL_MAX;
    double second_min = DBL_MAX;
    double second_max = -DBL_MAX;
    for (unsigned corner = 0; corner < DD2_NEAR_CORNERS; ++corner) {
        const double first_value = dd2_near_dot(first[corner], axis);
        const double second_value = dd2_near_dot(second[corner], axis);
        first_min = fmin(first_min, first_value);
        first_max = fmax(first_max, first_value);
        second_min = fmin(second_min, second_value);
        second_max = fmax(second_max, second_value);
    }
    return fmin(first_max - second_min, second_max - first_min);
}

/* An exhaustive eight-corner projection oracle, separate from the runtime's
 * center/radius projection. Compare the signed minimum, including edge axes. */
static double dd2_near_oracle(const dd2_vehicle *first, const dd2_vehicle *second) {
    dd2_vehicle_vector first_corners[DD2_NEAR_CORNERS] = {0};
    dd2_vehicle_vector second_corners[DD2_NEAR_CORNERS] = {0};
    dd2_vehicle_vector first_axes[DD2_NEAR_AXES] = {0};
    dd2_vehicle_vector second_axes[DD2_NEAR_AXES] = {0};
    dd2_near_corners(first, first_corners);
    dd2_near_corners(second, second_corners);
    dd2_near_axes(first->rotation, first_axes);
    dd2_near_axes(second->rotation, second_axes);
    double minimum = DBL_MAX;
    for (unsigned axis = 0; axis < DD2_NEAR_AXES; ++axis) {
        minimum =
            fmin(minimum, dd2_near_axis_depth(first_corners, second_corners, first_axes[axis]));
        minimum =
            fmin(minimum, dd2_near_axis_depth(first_corners, second_corners, second_axes[axis]));
        for (unsigned other = 0; other < DD2_NEAR_AXES; ++other) {
            const dd2_vehicle_vector cross = dd2_near_cross(first_axes[axis], second_axes[other]);
            const double length = sqrt(dd2_near_dot(cross, cross));
            if (length > dd2_near_axis_tolerance) {
                minimum = fmin(minimum, dd2_near_axis_depth(first_corners, second_corners,
                                                            dd2_near_scale(cross, 1 / length)));
            }
        }
    }
    return minimum;
}

static bool dd2_near_clear(dd2_car_contact contact) {
    return contact.time == 0 && contact.penetration == 0 && contact.normal.x == 0 &&
           contact.normal.y == 0 && contact.normal.z == 0 && contact.point.x == 0 &&
           contact.point.y == 0 && contact.point.z == 0 && !contact.unresolved;
}

static bool dd2_near_analytic(dd2_near_face_case sample) {
    dd2_vehicle first = {0};
    if (!dd2_vehicle_reset(&first, (dd2_vehicle_spawn){.position = {sample.origin, sample.origin,
                                                                    sample.origin}})) {
        return false;
    }
    first.rotation = dd2_near_rotation(sample.pose);
    dd2_vehicle second = first;
    dd2_vehicle_vector axes[DD2_NEAR_AXES] = {0};
    dd2_near_axes(first.rotation, axes);
    const dd2_vehicle_vector shift = dd2_near_scale(
        axes[sample.axis], sample.direction * ((2 * dd2_near_half[sample.axis]) + sample.gap));
    second.position.x += shift.x;
    second.position.y += shift.y;
    second.position.z += shift.z;
    dd2_car_contact contact = {0};
    if (!dd2_car_contact_proximity(
            &(dd2_car_neighborhood){.first = &first, .second = &second, .margin = dd2_near_margin},
            &contact) ||
        contact.time != 0 || contact.unresolved ||
        fabs(contact.penetration + sample.gap) > dd2_near_tolerance) {
        return false;
    }
    const dd2_vehicle_vector normal_error =
        dd2_near_subtract(contact.normal, dd2_near_scale(axes[sample.axis], -sample.direction));
    const dd2_vehicle_vector center = {.x = first.position.x + (shift.x / 2),
                                       .y = first.position.y + (shift.y / 2),
                                       .z = first.position.z + (shift.z / 2)};
    const dd2_vehicle_vector point_error = dd2_near_subtract(contact.point, center);
    if (sqrt(dd2_near_dot(normal_error, normal_error)) > dd2_near_tolerance ||
        sqrt(dd2_near_dot(point_error, point_error)) > dd2_near_tolerance) {
        return false;
    }
    /* A support neighborhood must not turn stationary touching or separating
     * bodies into primary sweep events. Overlaps retain the existing sweep. */
    const bool swept = dd2_car_contact_sweep(&first, &first, &second, &second, &contact);
    return sample.gap < -dd2_near_tolerance ? swept : !swept;
}

static bool dd2_near_face_samples(dd2_near_face_case sample) {
    const double gaps[] = {-dd2_near_margin / 2, 0, dd2_near_margin / 2};
    const unsigned poses[] = {0, 1, 7};
    for (unsigned pose = 0; pose < sizeof(poses) / sizeof(poses[0]); ++pose) {
        for (unsigned gap = 0; gap < sizeof(gaps) / sizeof(gaps[0]); ++gap) {
            sample.pose = poses[pose];
            sample.gap = gaps[gap];
            if (!dd2_near_analytic(sample)) {
                printf("Proximity face axis=%u direction=%.17g origin=%.17g pose=%u gap=%u\n",
                       sample.axis, sample.direction, sample.origin, pose, gap);
                return false;
            }
        }
    }
    return true;
}

static bool dd2_near_faces(void) {
    for (unsigned axis = 0; axis < DD2_NEAR_AXES; ++axis) {
        for (unsigned sign = 0; sign < 2; ++sign) {
            for (unsigned origin = 0; origin < 2; ++origin) {
                if (!dd2_near_face_samples(
                        (dd2_near_face_case){.axis = axis,
                                             .direction = sign == 0 ? -1 : 1,
                                             .origin = origin == 0 ? 0 : dd2_near_far})) {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool dd2_near_general(void) {
    const double margins[] = {0, dd2_near_margin, 1, dd2_near_radius};
    const double offsets[] = {0, 200, 370, 500, 900, 1400};
    for (unsigned pose = 0; pose < DD2_NEAR_POSES; ++pose) {
        for (unsigned origin = 0; origin < 2; ++origin) {
            dd2_vehicle first = {0};
            if (!dd2_vehicle_reset(&first, (dd2_vehicle_spawn){0})) {
                return false;
            }
            first.rotation = dd2_near_rotation(pose);
            first.position = origin == 0
                                 ? (dd2_vehicle_vector){0}
                                 : (dd2_vehicle_vector){dd2_near_far, -dd2_near_far, dd2_near_far};
            dd2_vehicle second = first;
            second.rotation = dd2_near_rotation(DD2_NEAR_POSES - pose);
            for (unsigned distance = 0; distance < sizeof(offsets) / sizeof(offsets[0]);
                 ++distance) {
                second.position.x = first.position.x + offsets[distance];
                second.position.y = first.position.y + (offsets[distance] / 3);
                second.position.z = first.position.z - (offsets[distance] / 2);
                const double expected = dd2_near_oracle(&first, &second);
                for (unsigned margin = 0; margin < sizeof(margins) / sizeof(margins[0]); ++margin) {
                    dd2_car_contact contact = {.time = 1, .unresolved = true};
                    const bool found = dd2_car_contact_proximity(
                        &(dd2_car_neighborhood){
                            .first = &first, .second = &second, .margin = margins[margin]},
                        &contact);
                    if (found != (expected >= -margins[margin]) ||
                        (!found && !dd2_near_clear(contact)) ||
                        (found && (contact.time != 0 || contact.unresolved ||
                                   fabs(contact.penetration - expected) > dd2_near_tolerance ||
                                   fabs(dd2_near_dot(contact.normal, contact.normal) - 1) >
                                       dd2_near_tolerance))) {
                        printf("Proximity oracle pose=%u origin=%u distance=%u margin=%u "
                               "depth=%.17g\n",
                               pose, origin, distance, margin, expected);
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

static bool dd2_near_invalid(void) {
    dd2_vehicle first = {0};
    if (!dd2_vehicle_reset(&first, (dd2_vehicle_spawn){0})) {
        return false;
    }
    dd2_vehicle second = first;
    const union {
        uint64_t bits;
        double number;
    } infinity = {.bits = UINT64_C(0x7ff0000000000000)},
      negative_infinity = {.bits = UINT64_C(0xfff0000000000000)},
      not_a_number = {.bits = UINT64_C(0x7ff8000000000001)};
    const double invalid[] = {-1, dd2_near_radius + 1, infinity.number, negative_infinity.number,
                              not_a_number.number};
    for (unsigned sample = 0; sample < sizeof(invalid) / sizeof(invalid[0]); ++sample) {
        dd2_car_contact hit = {.time = 1, .penetration = 1, .unresolved = true};
        if (dd2_car_contact_proximity(&(dd2_car_neighborhood){.first = &first,
                                                              .second = &second,
                                                              .margin = invalid[sample]},
                                      &hit) ||
            !dd2_near_clear(hit)) {
            printf("Rejected margin sample=%u output depth=%.17g\n", sample, hit.penetration);
            return false;
        }
    }
    dd2_car_contact hit = {.time = 1};
    if (dd2_car_contact_proximity(NULL, &hit) || !dd2_near_clear(hit)) {
        return false;
    }
    if (dd2_car_contact_proximity(
            &(dd2_car_neighborhood){.first = NULL, .second = &second, .margin = 0}, &hit) ||
        !dd2_near_clear(hit) ||
        dd2_car_contact_proximity(
            &(dd2_car_neighborhood){.first = &first, .second = NULL, .margin = 0}, &hit) ||
        !dd2_near_clear(hit) ||
        dd2_car_contact_proximity(
            &(dd2_car_neighborhood){.first = &first, .second = &second, .margin = 0}, NULL)) {
        puts("Null proximity arguments failed");
        return false;
    }
    second.position.x = not_a_number.number;
    if (dd2_car_contact_proximity(
            &(dd2_car_neighborhood){.first = &first, .second = &second, .margin = 0}, &hit) ||
        !dd2_near_clear(hit)) {
        puts("Nonfinite proximity position failed");
        return false;
    }
    second = first;
    second.velocity.z = infinity.number;
    if (dd2_car_contact_proximity(
            &(dd2_car_neighborhood){.first = &first, .second = &second, .margin = 0}, &hit) ||
        !dd2_near_clear(hit)) {
        puts("Nonfinite proximity velocity failed");
        return false;
    }
    second = first;
    second.rotation.w = 0;
    return !dd2_car_contact_proximity(
               &(dd2_car_neighborhood){.first = &first, .second = &second, .margin = 0}, &hit) &&
           dd2_near_clear(hit);
}

static bool dd2_near_boundaries(void) {
    dd2_vehicle first = {0};
    if (!dd2_vehicle_reset(&first, (dd2_vehicle_spawn){0})) {
        return false;
    }
    dd2_vehicle second = first;
    second.position.x = (2 * dd2_near_half[0]) + dd2_near_margin;
    unsigned char before[2][sizeof(dd2_vehicle)] = {0};
    const unsigned char *first_bytes = (const unsigned char *)&first;
    const unsigned char *second_bytes = (const unsigned char *)&second;
    for (unsigned byte = 0; byte < sizeof(dd2_vehicle); ++byte) {
        before[0][byte] = first_bytes[byte];
        before[1][byte] = second_bytes[byte];
    }
    dd2_car_contact hit = {0};
    if (!dd2_car_contact_proximity(
            &(dd2_car_neighborhood){.first = &first, .second = &second, .margin = dd2_near_margin},
            &hit) ||
        hit.penetration != -dd2_near_margin || hit.normal.x != -1 ||
        hit.point.x != second.position.x / 2 ||
        memcmp((const unsigned char *)&first, before[0], sizeof(first)) != 0 ||
        memcmp((const unsigned char *)&second, before[1], sizeof(second)) != 0) {
        return false;
    }
    if (!dd2_car_contact_proximity(
            &(dd2_car_neighborhood){.first = &second, .second = &first, .margin = dd2_near_margin},
            &hit) ||
        hit.penetration != -dd2_near_margin || hit.normal.x != 1 ||
        hit.point.x != second.position.x / 2) {
        return false;
    }
    second.position.x += dd2_near_margin;
    return !dd2_car_contact_proximity(&(dd2_car_neighborhood){.first = &first,
                                                              .second = &second,
                                                              .margin = dd2_near_margin},
                                      &hit) &&
           dd2_near_clear(hit);
}

int main(void) {
    if (!dd2_near_invalid()) {
        puts("car SAT proximity invalid-input rejection: FAIL");
        return EXIT_FAILURE;
    }
    if (!dd2_near_boundaries()) {
        puts("car SAT proximity boundaries/read-only inputs: FAIL");
        return EXIT_FAILURE;
    }
    if (!dd2_near_faces() || !dd2_near_general()) {
        puts("car SAT proximity geometry: FAIL");
        return EXIT_FAILURE;
    }
    puts("car SAT proximity: PASS (108 analytic faces, 2304 corner-projection queries)");
    return EXIT_SUCCESS;
}
