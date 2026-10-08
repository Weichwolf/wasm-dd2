#include "physics/car_contact.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static dd2_vehicle_collision_storage *dd2_test_storage;

enum { DD2_BUDGET_BODIES = 2 };
static const double dd2_budget_turn = 0.32;
static const double dd2_budget_separation = 910;
static const double dd2_budget_height = 5000;
static const double dd2_budget_extent = 450;
static const double dd2_budget_side = 186;
static const double dd2_budget_tolerance = 1e-6;

/* This target compiles the real sweep with one refinement window. The
 * independent contact condition proves the reported bound precedes impact. */
static bool dd2_budget_test(void) {
    dd2_vehicle before[DD2_BUDGET_BODIES] = {0};
    if (!dd2_vehicle_reset(&before[0], (dd2_vehicle_spawn){.position = {.y = dd2_budget_height}}) ||
        !dd2_vehicle_reset(&before[1],
                           (dd2_vehicle_spawn){
                               .position = {.y = dd2_budget_height, .z = dd2_budget_separation}})) {
        return false;
    }
    dd2_vehicle after[DD2_BUDGET_BODIES] = {before[0], before[1]};
    after[0].rotation =
        (dd2_vehicle_rotation){.y = sin(dd2_budget_turn / 2), .w = cos(dd2_budget_turn / 2)};
    dd2_car_contact contact = {0};
    if (!dd2_car_contact_sweep(&before[0], &after[0], &before[1], &after[1], &contact) ||
        !contact.unresolved || contact.time < 0 || contact.time >= 1 || contact.penetration != 0 ||
        contact.normal.x != 0 || contact.normal.y != 0 || contact.normal.z != 0) {
        return false;
    }
    const double quaternion_y = contact.time * sin(dd2_budget_turn / 2);
    const double quaternion_w = 1 - (contact.time * (1 - cos(dd2_budget_turn / 2)));
    const double angle = 2 * atan2(quaternion_y, quaternion_w);
    const double extent = (dd2_budget_extent * cos(angle)) + (dd2_budget_side * sin(angle));
    if (extent + dd2_budget_extent >= dd2_budget_separation) {
        return false;
    }
    dd2_vehicle_collision_report report = {0};
    if (!dd2_vehicle_collide_fleet_report(after, before, DD2_BUDGET_BODIES, NULL, NULL,
                                          dd2_test_storage, &report) ||
        report.unresolved_sweeps != 1 || report.count != 0 || report.pair_contacts != 0) {
        return false;
    }
    for (unsigned body = 0; body < DD2_BUDGET_BODIES; ++body) {
        if (report.impacts[body].contacts != 0 || report.impacts[body].impulse != 0 ||
            after[body].steps != before[body].steps || after[body].velocity.x != 0 ||
            after[body].velocity.y != 0 || after[body].velocity.z != 0 ||
            after[body].angular_velocity.x != 0 || after[body].angular_velocity.y != 0 ||
            after[body].angular_velocity.z != 0 ||
            after[body].position.z != before[body].position.z) {
            return false;
        }
    }
    const double factor = 1 / hypot(quaternion_y, quaternion_w);
    return fabs(after[0].rotation.y - (quaternion_y * factor)) < dd2_budget_tolerance &&
           fabs(after[0].rotation.w - (quaternion_w * factor)) < dd2_budget_tolerance;
}

static int dd2_test_run(void) {
    if (!dd2_budget_test()) {
        puts("unresolved car sweep: FAIL");
        return EXIT_FAILURE;
    }
    puts("unresolved car sweep: PASS");
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
