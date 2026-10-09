#include "asset_fixture.h"
#include "assets/barriers.h"
#include "assets/road.h"
#include "fleet_remainder_fixture.h"
#include "physics/barrier_world.h"
#include "physics/body_geometry.h"
#include "physics/car_contact.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"
#include "surface_fixture.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    DD2_REMAINDER_FREE,
    DD2_REMAINDER_FLOOR_CONVOY,
    DD2_REMAINDER_CROSSING,
    DD2_REMAINDER_WALL_CONVOY
} dd2_remainder_case;
enum {
    DD2_REMAINDER_FLOOR_EXTENT = 100000,
    DD2_REMAINDER_CONVOY_FIRST = 1,
    DD2_REMAINDER_CONVOY_LAST = 3,
    DD2_REMAINDER_PAIR_FIRST = 4,
    DD2_REMAINDER_PAIR_SECOND = 5,
    DD2_REMAINDER_CHAIN_FIRST = 7,
    DD2_REMAINDER_CHAIN_LAST = 12
};
static const double dd2_remainder_tolerance = 1e-6;
static const double dd2_remainder_gap = 45;
static const double dd2_remainder_convoy_origin = -30000;
static const double dd2_remainder_spacing = (2 * (double)DD2_BODY_HALF_WIDTH) + 2;
static const double dd2_remainder_speed = 10000;
static const double dd2_remainder_wall_height = 190;
static const double dd2_remainder_cross_height = 1000;
static const double dd2_remainder_cross_first = -36000;
static const double dd2_remainder_cross_second = -35528;

typedef struct {
    dd2_road *road;
    dd2_road_surface *surface;
    dd2_barriers *barriers;
    dd2_barrier_world *world;
    dd2_vehicle_collision_storage *storage;
} dd2_remainder_fixture;

static bool dd2_remainder_close(double actual, double expected) {
    return fabs(actual - expected) < dd2_remainder_tolerance;
}
static bool dd2_remainder_create(dd2_remainder_fixture *test) {
    dd2_surface_test_fixture fixture = {0};
    dd2_surface_test_fixture_init(&fixture, 1);
    for (unsigned corner = 0; corner < DD2_SURFACE_TEST_VERTICES; ++corner) {
        uint8_t *vertex = fixture.vertices + ((size_t)corner * DD2_SURFACE_TEST_VERTEX_BYTES);
        dd2_test_write_le32(vertex,
                            (uint32_t)(((int32_t)(corner % DD2_SURFACE_TEST_ROW_VERTICES) - 1) *
                                       DD2_REMAINDER_FLOOR_EXTENT));
        dd2_test_write_le32(vertex + DD2_TEST_WORD_BYTES, 0);
        dd2_test_write_le32(vertex + DD2_SURFACE_TEST_Z_OFFSET,
                            (uint32_t)(corner < DD2_SURFACE_TEST_ROW_VERTICES
                                           ? -DD2_REMAINDER_FLOOR_EXTENT
                                           : DD2_REMAINDER_FLOOR_EXTENT));
    }
    test->road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    test->surface = dd2_road_surface_create(test->road);
    test->barriers = dd2_barriers_create(test->road, 1);
    test->world = dd2_barrier_world_create(test->barriers);
    test->storage = dd2_vehicle_collision_storage_create();
    return test->surface != NULL && test->world != NULL && test->storage != NULL;
}
static void dd2_remainder_destroy(dd2_remainder_fixture *test) {
    dd2_vehicle_collision_storage_destroy(test->storage);
    dd2_barrier_world_destroy(test->world);
    dd2_barriers_destroy(test->barriers);
    dd2_road_surface_destroy(test->surface);
    dd2_road_destroy(test->road);
}
static void dd2_remainder_convoy(dd2_vehicle *before, dd2_vehicle *after,
                                 dd2_remainder_case sample) {
    const bool wall = sample == DD2_REMAINDER_WALL_CONVOY;
    const double origin =
        wall ? (double)DD2_REMAINDER_FLOOR_EXTENT - (double)DD2_BODY_HALF_WIDTH - dd2_remainder_gap
             : dd2_remainder_convoy_origin;
    for (unsigned body = DD2_REMAINDER_CONVOY_FIRST; body <= DD2_REMAINDER_CONVOY_LAST; ++body) {
        before[body] = (dd2_vehicle){
            .position = {.x = origin -
                              ((double)(body - DD2_REMAINDER_CONVOY_FIRST) * dd2_remainder_spacing),
                         .y = wall ? dd2_remainder_wall_height
                                   : (double)DD2_BODY_HALF_HEIGHT + dd2_remainder_gap,
                         .z = dd2_remainder_convoy_origin},
            .velocity = {.x = dd2_remainder_speed,
                         .y = !wall && body == DD2_REMAINDER_CONVOY_FIRST ? -dd2_remainder_speed
                                                                          : 0},
            .rotation = {.w = 1}};
        after[body] = before[body];
        after[body].position.x += before[body].velocity.x * DD2_VEHICLE_STEP_SECONDS;
        after[body].position.y += before[body].velocity.y * DD2_VEHICLE_STEP_SECONDS;
    }
}
static void dd2_remainder_crossing(dd2_vehicle *before, dd2_vehicle *after) {
    before[DD2_REMAINDER_PAIR_FIRST] = (dd2_vehicle){.position = {.x = dd2_remainder_cross_first,
                                                                  .y = dd2_remainder_cross_height,
                                                                  .z = dd2_remainder_convoy_origin},
                                                     .velocity = {.x = 2 * dd2_remainder_speed},
                                                     .rotation = {.w = 1}};
    before[DD2_REMAINDER_PAIR_SECOND] = before[DD2_REMAINDER_PAIR_FIRST];
    before[DD2_REMAINDER_PAIR_SECOND].position.x = dd2_remainder_cross_second;
    before[DD2_REMAINDER_PAIR_SECOND].velocity.x = -2 * dd2_remainder_speed;
    for (unsigned body = DD2_REMAINDER_PAIR_FIRST; body <= DD2_REMAINDER_PAIR_SECOND; ++body) {
        after[body] = before[body];
        after[body].position.x += before[body].velocity.x * DD2_VEHICLE_STEP_SECONDS;
    }
}
static bool dd2_remainder_protected(dd2_remainder_case sample, const dd2_vehicle *before,
                                    const dd2_vehicle *after, double time) {
    if (sample == DD2_REMAINDER_FLOOR_CONVOY || sample == DD2_REMAINDER_WALL_CONVOY) {
        /* The leader's later world collision freezes it. Neither follower's
         * original parallel motion collides, but freezing each predecessor
         * obstructs the next follower. Both propagation passes are required. */
        for (unsigned body = DD2_REMAINDER_CONVOY_FIRST; body <= DD2_REMAINDER_CONVOY_LAST;
             ++body) {
            if (!dd2_remainder_close(after[body].position.x,
                                     before[body].position.x + (before[body].velocity.x *
                                                                DD2_VEHICLE_STEP_SECONDS * time))) {
                return false;
            }
        }
        return after[DD2_REMAINDER_CONVOY_FIRST].position.y >= (double)DD2_BODY_HALF_HEIGHT;
    }
    if (sample == DD2_REMAINDER_CROSSING) {
        for (unsigned body = DD2_REMAINDER_PAIR_FIRST; body <= DD2_REMAINDER_PAIR_SECOND; ++body) {
            if (!dd2_remainder_close(after[body].position.x,
                                     before[body].position.x + (before[body].velocity.x *
                                                                DD2_VEHICLE_STEP_SECONDS * time))) {
                return false;
            }
        }
        dd2_car_contact contact = {0};
        return !dd2_car_contact_proximity(
            &(dd2_car_neighborhood){.first = &after[DD2_REMAINDER_PAIR_FIRST],
                                    .second = &after[DD2_REMAINDER_PAIR_SECOND],
                                    .margin = 0},
            &contact);
    }
    return true;
}
static bool dd2_remainder_test(const dd2_remainder_fixture *test, dd2_remainder_case sample) {
    dd2_vehicle before[DD2_VEHICLE_FLEET_LIMIT];
    dd2_vehicle after[DD2_VEHICLE_FLEET_LIMIT];
    for (unsigned body = 0; body < DD2_VEHICLE_FLEET_LIMIT; ++body) {
        before[body] = dd2_remainder_previous[body];
        after[body] = dd2_remainder_proposed[body];
    }
    if (sample == DD2_REMAINDER_FLOOR_CONVOY || sample == DD2_REMAINDER_WALL_CONVOY) {
        dd2_remainder_convoy(before, after, sample);
    } else if (sample == DD2_REMAINDER_CROSSING) {
        dd2_remainder_crossing(before, after);
    }
    const dd2_vehicle proposal = after[0];
    dd2_vehicle_collision_report report = {0};
    if (!dd2_vehicle_collide_fleet_report(after, before, DD2_VEHICLE_FLEET_LIMIT, test->surface,
                                          sample == DD2_REMAINDER_WALL_CONVOY ? test->world : NULL,
                                          test->storage, &report)) {
        return false;
    }
    double last_time = 0;
    for (unsigned index = 0; index < report.count; ++index) {
        const dd2_vehicle_contact *contact = &report.contacts[index];
        if (contact->kind != DD2_VEHICLE_CONTACT_PAIR ||
            contact->first < DD2_REMAINDER_CHAIN_FIRST ||
            contact->first > DD2_REMAINDER_CHAIN_LAST ||
            contact->second < DD2_REMAINDER_CHAIN_FIRST ||
            contact->second > DD2_REMAINDER_CHAIN_LAST) {
            return false;
        }
        last_time = fmax(last_time, contact->time);
    }
    const bool full = dd2_remainder_close(after[0].position.x, proposal.position.x) &&
                      dd2_remainder_close(after[0].position.y, proposal.position.y) &&
                      dd2_remainder_close(after[0].position.z, proposal.position.z) &&
                      after[0].velocity.x == proposal.velocity.x &&
                      after[0].velocity.y == proposal.velocity.y &&
                      after[0].velocity.z == proposal.velocity.z;
    const bool protected_motion = dd2_remainder_protected(sample, before, after, last_time);
    printf("{\"case\":%u,\"events\":%u,\"records\":%u,\"last_time\":%.17g,"
           "\"free_x_error\":%.17g,\"protected\":%u}\n",
           (unsigned)sample, report.response_events, report.count, last_time,
           after[0].position.x - proposal.position.x, (unsigned)protected_motion);
    return full && protected_motion && report.response_events == DD2_VEHICLE_EVENT_LIMIT;
}
int main(void) {
    dd2_remainder_fixture test = {0};
    bool passed = dd2_remainder_create(&test);
    const dd2_remainder_case cases[] = {DD2_REMAINDER_FREE, DD2_REMAINDER_FLOOR_CONVOY,
                                        DD2_REMAINDER_CROSSING, DD2_REMAINDER_WALL_CONVOY};
    for (size_t index = 0; passed && index < sizeof(cases) / sizeof(cases[0]); ++index) {
        passed = dd2_remainder_test(&test, cases[index]);
    }
    dd2_remainder_destroy(&test);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
