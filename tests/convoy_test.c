#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static dd2_vehicle_collision_storage *dd2_test_storage;

enum { DD2_CONVOY_CLOCK = 128 };
static const double dd2_convoy_height = 5000;
static const double dd2_convoy_front_spacing = 900;
static const double dd2_convoy_side_spacing = 372;
static const double dd2_convoy_speed = 1000;
static const double dd2_convoy_clearance = 0.0002;
static const double dd2_convoy_tolerance = 1e-8;

typedef struct {
    unsigned count;
    double overlap;
    double yaw;
    bool side;
} dd2_convoy_case;

static bool dd2_convoy_case_test(dd2_convoy_case sample) {
    const double sine = sin(sample.yaw);
    const double cosine = cos(sample.yaw);
    const dd2_vehicle_vector row = {.x = sample.side ? cosine : sine,
                                    .z = sample.side ? -sine : cosine};
    const dd2_vehicle_vector tangent = {.x = row.z, .z = -row.x};
    const double spacing =
        (sample.side ? dd2_convoy_side_spacing : dd2_convoy_front_spacing) - sample.overlap;
    dd2_vehicle before[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_vehicle next[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned body = 0; body < sample.count; ++body) {
        if (!dd2_vehicle_reset(&before[body],
                               (dd2_vehicle_spawn){.position = {.x = row.x * spacing * body,
                                                                .y = dd2_convoy_height,
                                                                .z = row.z * spacing * body},
                                                   .yaw = sample.yaw})) {
            return false;
        }
        before[body].steps = DD2_CONVOY_CLOCK;
        before[body].velocity = (dd2_vehicle_vector){.x = tangent.x * dd2_convoy_speed,
                                                     .z = tangent.z * dd2_convoy_speed};
        next[body] = before[body];
        next[body].position.x += next[body].velocity.x * DD2_VEHICLE_STEP_SECONDS;
        next[body].position.z += next[body].velocity.z * DD2_VEHICLE_STEP_SECONDS;
    }
    dd2_vehicle_collision_report report = {0};
    if (!dd2_vehicle_collide_fleet_report(next, before, sample.count, NULL, NULL, dd2_test_storage,
                                          &report) ||
        report.unresolved_sweeps != 0 || report.count != sample.count - 1 ||
        report.pair_contacts != report.count) {
        return false;
    }
    /* Equally spaced, equal-mass neighbors have an independent least-norm
     * separation: symmetric offsets about the field center. Common tangential
     * motion must be fully retained, with no physical impulse or clock advance. */
    for (unsigned body = 0; body < sample.count; ++body) {
        const double offset = (sample.overlap + dd2_convoy_clearance) *
                              ((double)body - ((double)(sample.count - 1) / 2));
        const double expected_x = before[body].position.x + (row.x * offset) +
                                  (before[body].velocity.x * DD2_VEHICLE_STEP_SECONDS);
        const double expected_z = before[body].position.z + (row.z * offset) +
                                  (before[body].velocity.z * DD2_VEHICLE_STEP_SECONDS);
        if (fabs(next[body].position.x - expected_x) > dd2_convoy_tolerance ||
            fabs(next[body].position.z - expected_z) > dd2_convoy_tolerance ||
            next[body].position.y != before[body].position.y ||
            next[body].velocity.x != before[body].velocity.x ||
            next[body].velocity.y != before[body].velocity.y ||
            next[body].velocity.z != before[body].velocity.z ||
            next[body].angular_velocity.x != 0 || next[body].angular_velocity.y != 0 ||
            next[body].angular_velocity.z != 0 || next[body].steps != DD2_CONVOY_CLOCK) {
            return false;
        }
    }
    for (unsigned index = 0; index < report.count; ++index) {
        const dd2_vehicle_contact contact = report.contacts[index];
        if (contact.impulse != 0 || contact.normal_speed != 0 || contact.time != 0 ||
            contact.kind != DD2_VEHICLE_CONTACT_PAIR || contact.first != index ||
            contact.second != index + 1) {
            return false;
        }
    }
    return true;
}

static bool dd2_convoy_sizes(double yaw) {
    const double overlaps[] = {0.001, 0.01, 0.1, 1};
    for (unsigned count = 2; count <= DD2_VEHICLE_FLEET_LIMIT; ++count) {
        for (unsigned index = 0; index < sizeof(overlaps) / sizeof(overlaps[0]); ++index) {
            for (unsigned side = 0; side < 2; ++side) {
                const dd2_convoy_case sample = {
                    .count = count, .overlap = overlaps[index], .yaw = yaw, .side = side != 0};
                if (!dd2_convoy_case_test(sample)) {
                    printf("Convoy count=%u overlap=%.17g yaw=%.17g side=%u\n", count,
                           sample.overlap, yaw, side);
                    return false;
                }
            }
        }
    }
    return true;
}

static int dd2_test_run(void) {
    const double headings[] = {0, 0.3, 1.1};
    for (unsigned index = 0; index < sizeof(headings) / sizeof(headings[0]); ++index) {
        if (!dd2_convoy_sizes(headings[index])) {
            puts("collective convoy repair: FAIL");
            return EXIT_FAILURE;
        }
    }
    puts("collective convoy repair: PASS (456 complete fixed steps)");
    return EXIT_SUCCESS;
}

int main(void) {
    dd2_test_storage = dd2_vehicle_collision_storage_create();
    if (dd2_test_storage == NULL) {
        return EXIT_FAILURE;
    }
    const int result = dd2_test_run();
    dd2_vehicle_collision_storage_destroy(dd2_test_storage);
    return result;
}
