#include "ai/driver.h"
#include "ai/path.h"
#include "asset_fixture.h"
#include "assets/road.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "surface_fixture.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_AI_TEST_STALL_STEPS = 350,
    DD2_AI_TEST_PROGRESS_STEPS = 300,
    DD2_AI_TEST_MOVING_STEPS = 1000,
    DD2_AI_TEST_PASSING_STEPS = 140
};
static const double dd2_ai_test_tolerance = 1e-8;
static const double dd2_ai_test_height = 190;
static const double dd2_ai_test_lane = 500;
static const double dd2_ai_test_distance = 3000;
static const double dd2_ai_test_wrap = 9000;
static const double dd2_ai_test_advertised_speed = 2000;
static const double dd2_ai_test_creep_divisor = 10;
static const double dd2_ai_test_oscillation = 20;
static const double dd2_ai_test_curvature = 0.0007853981633974483096;

static dd2_road *dd2_ai_test_road(void) {
    dd2_surface_test_fixture fixture;
    dd2_surface_test_fixture_init(&fixture, 4);
    static const int32_t centers[4][2] = {{0, 0}, {0, 2000}, {2000, 2000}, {2000, 0}};
    for (unsigned strip = 0; strip < 4; ++strip) {
        for (unsigned corner = 0; corner < DD2_SURFACE_TEST_VERTICES; ++corner) {
            uint8_t *vertex =
                fixture.vertices + (((size_t)strip * DD2_SURFACE_TEST_VERTICES + corner) *
                                    DD2_SURFACE_TEST_VERTEX_BYTES);
            const int32_t xpos =
                centers[strip][0] +
                (((int32_t)(corner % DD2_SURFACE_TEST_ROW_VERTICES) - 1) * DD2_SURFACE_TEST_SIDE);
            const int32_t zpos = centers[strip][1] + (corner < DD2_SURFACE_TEST_ROW_VERTICES
                                                          ? -DD2_SURFACE_TEST_SIDE
                                                          : DD2_SURFACE_TEST_SIDE);
            dd2_test_write_le32(vertex, (uint32_t)xpos);
            dd2_test_write_le32(vertex + DD2_TEST_WORD_BYTES, 0);
            dd2_test_write_le32(vertex + DD2_SURFACE_TEST_Z_OFFSET, (uint32_t)zpos);
        }
    }
    return dd2_road_create(&fixture.level, DD2_ROAD_RACING);
}

static bool dd2_ai_test_path(const dd2_road *road) {
    dd2_ai_path_query query = {.cell = 0, .lane = 1.0 / 2, .distance = dd2_ai_test_distance};
    dd2_ai_path_sample sample = {0};
    bool valid = dd2_ai_path(road, query, &sample) &&
                 fabs(sample.point.x - (double)DD2_SURFACE_TEST_SIDE) < dd2_ai_test_tolerance &&
                 fabs(sample.point.z - (2 * DD2_SURFACE_TEST_SIDE)) < dd2_ai_test_tolerance &&
                 fabs(sample.direction.x - 1) < dd2_ai_test_tolerance &&
                 fabs(sample.curvature - dd2_ai_test_curvature) < dd2_ai_test_tolerance;
    query.distance = dd2_ai_test_wrap;
    valid = valid && dd2_ai_path(road, query, &sample) &&
            fabs(sample.point.x) < dd2_ai_test_tolerance &&
            fabs(sample.point.z - (double)DD2_SURFACE_TEST_SIDE) < dd2_ai_test_tolerance &&
            sample.strip == 0;
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff8000000000001)};
    query.lane = invalid.number;
    return valid && !dd2_ai_path(road, query, &sample) && sample.width == 0;
}

/* Mirrored clear passing lanes with an obstructed base path. Heading-only
 * traffic misses the obstacle and starts merging back into it. A remaining
 * body-heading obstacle still prevents merging; both paths must be clear. */
static bool dd2_ai_test_passing_return(const dd2_road *road, const dd2_road_surface *surface,
                                       bool mirrored) {
    const double side = mirrored ? -1 : 1;
    const double shifted_lane = mirrored ? 0.57 : 0.43;
    dd2_vehicle vehicles[2] = {0};
    dd2_ai_driver driver = {0};
    if (!dd2_vehicle_reset(&vehicles[0],
                           (dd2_vehicle_spawn){.position = {.x = side * dd2_ai_test_lane,
                                                            .y = dd2_ai_test_height}}) ||
        !dd2_vehicle_reset(&vehicles[1],
                           (dd2_vehicle_spawn){.position = {.x = -side * dd2_ai_test_lane,
                                                            .y = dd2_ai_test_height,
                                                            .z = dd2_ai_test_lane}}) ||
        !dd2_ai_driver_reset(
            &driver,
            (dd2_ai_start){.road = road, .cell = mirrored ? 1U : 0U, .slot = 0, .count = 2})) {
        return false;
    }
    driver.lane = shifted_lane;
    const dd2_ai_observation observation = {
        .road = road, .surface = surface, .vehicles = vehicles, .count = 2, .slot = 0};
    dd2_vehicle_control control = {0};
    if (!dd2_ai_driver_step(&driver, &observation, &control) || control.throttle <= 0 ||
        fabs(driver.lane - shifted_lane) >= dd2_ai_test_tolerance) {
        return false;
    }
    vehicles[1].position.x = vehicles[0].position.x;
    vehicles[1].position.z = 3 * dd2_ai_test_lane;
    /* The mirrored heading guard chooses its preferred outer candidate;
     * allow the original rate-limited traversal across the clear base lane. */
    for (unsigned step = 0; step < DD2_AI_TEST_PASSING_STEPS; ++step) {
        if (!dd2_ai_driver_step(&driver, &observation, &control) || control.throttle <= 0) {
            return false;
        }
    }
    if (mirrored ? driver.lane <= driver.base_lane
                 : fabs(driver.lane - shifted_lane) >= dd2_ai_test_tolerance) {
        return false;
    }
    const double passing_lane = driver.lane;
    vehicles[1].position.z = -dd2_ai_test_lane;
    return dd2_ai_driver_step(&driver, &observation, &control) && control.throttle > 0 &&
           fabs(driver.lane - driver.base_lane) < fabs(passing_lane - driver.base_lane);
}

static bool dd2_ai_test_driver(const dd2_road *road, const dd2_road_surface *surface) {
    dd2_vehicle vehicles[2] = {0};
    if (!dd2_vehicle_reset(
            &vehicles[0],
            (dd2_vehicle_spawn){.position = {.x = dd2_ai_test_lane, .y = dd2_ai_test_height}}) ||
        !dd2_vehicle_reset(
            &vehicles[1],
            (dd2_vehicle_spawn){.position = {.x = -dd2_ai_test_lane, .y = dd2_ai_test_height}})) {
        return false;
    }
    dd2_ai_driver driver = {0};
    if (!dd2_ai_driver_reset(&driver,
                             (dd2_ai_start){.road = road, .cell = 1, .slot = 0, .count = 2})) {
        return false;
    }
    dd2_ai_observation observation = {
        .road = road, .surface = surface, .vehicles = vehicles, .count = 2, .slot = 0};
    dd2_vehicle_control control = {0};
    bool valid = dd2_ai_driver_step(&driver, &observation, &control) && control.throttle > 0 &&
                 fabs(control.steer) < dd2_ai_test_tolerance && driver.steps == 1;
    for (unsigned step = 0; step < DD2_AI_TEST_STALL_STEPS && valid; ++step) {
        valid = dd2_ai_driver_step(&driver, &observation, &control);
    }
    valid = valid && control.throttle < 0 && driver.reverse_steps > 0;
    const dd2_ai_driver saved = driver;
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff8000000000001)};
    observation.slot = 1;
    observation.pursue_player = true;
    if (dd2_ai_driver_step(&driver, &observation, &control) || driver.steps != saved.steps) {
        return false;
    }
    observation.slot = 0;
    observation.pursue_player = false;
    vehicles[1].position.x = invalid.number;
    return valid && !dd2_ai_driver_step(&driver, &observation, &control) &&
           saved.cell == driver.cell && saved.lane == driver.lane &&
           saved.base_lane == driver.base_lane && saved.target == driver.target &&
           saved.stuck_steps == driver.stuck_steps && saved.reverse_steps == driver.reverse_steps &&
           saved.steps == driver.steps && saved.progress_steps == driver.progress_steps &&
           saved.progress_origin.x == driver.progress_origin.x &&
           saved.progress_origin.y == driver.progress_origin.y &&
           saved.progress_origin.z == driver.progress_origin.z && control.throttle == 0 &&
           control.steer == 0;
}

/* A car that cannot move during reverse still needs a new forward attempt.
 * A reverse-speed counter must not rearm reverse on its expiration tick. */
static bool dd2_ai_test_stopped_reverse(const dd2_road *road, const dd2_road_surface *surface) {
    dd2_vehicle vehicle = {0};
    dd2_ai_driver driver = {0};
    if (!dd2_vehicle_reset(&vehicle, (dd2_vehicle_spawn){.position = {.y = dd2_ai_test_height}}) ||
        !dd2_ai_driver_reset(&driver,
                             (dd2_ai_start){.road = road, .cell = 0, .slot = 0, .count = 1})) {
        return false;
    }
    const dd2_ai_observation observation = {
        .road = road, .surface = surface, .vehicles = &vehicle, .count = 1, .slot = 0};
    dd2_vehicle_control control = {0};
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        for (unsigned step = 1; step <= DD2_AI_TEST_PROGRESS_STEPS; ++step) {
            if (!dd2_ai_driver_step(&driver, &observation, &control) ||
                (step < DD2_AI_TEST_PROGRESS_STEPS &&
                 (control.throttle <= 0 || driver.reverse_steps != 0 ||
                  driver.stuck_steps != step || driver.progress_steps != step))) {
                return false;
            }
        }
        if (control.throttle >= 0 || driver.reverse_steps != DD2_AI_TEST_PROGRESS_STEPS - 1 ||
            driver.stuck_steps != 0 || driver.progress_steps != 0) {
            return false;
        }
        for (unsigned step = 1; step < DD2_AI_TEST_PROGRESS_STEPS; ++step) {
            if (!dd2_ai_driver_step(&driver, &observation, &control) || control.throttle >= 0 ||
                driver.reverse_steps != DD2_AI_TEST_PROGRESS_STEPS - 1 - step ||
                driver.progress_steps != 0) {
                return false;
            }
        }
    }
    return driver.reverse_steps == 0;
}

typedef enum {
    DD2_AI_TEST_STATIONARY,
    DD2_AI_TEST_MOVING,
    DD2_AI_TEST_CREEPING,
    DD2_AI_TEST_OSCILLATING,
    DD2_AI_TEST_MOTIONS
} dd2_ai_test_motion;

typedef struct {
    dd2_ai_test_motion motion;
    unsigned step;
} dd2_ai_test_sample;

static double dd2_ai_test_position(dd2_ai_test_sample sample) {
    switch (sample.motion) {
    case DD2_AI_TEST_MOVING:
        return (double)sample.step / 2;
    case DD2_AI_TEST_CREEPING:
        return (double)sample.step / dd2_ai_test_creep_divisor;
    case DD2_AI_TEST_OSCILLATING:
        return (sample.step & 1U) == 0 ? -dd2_ai_test_oscillation : dd2_ai_test_oscillation;
    default:
        return 0;
    }
}

static bool dd2_ai_test_reverse_expiry(dd2_ai_driver *driver, const dd2_ai_observation *observation,
                                       dd2_vehicle_control control) {
    if (control.throttle >= 0 || driver->reverse_steps != DD2_AI_TEST_PROGRESS_STEPS - 1 ||
        driver->progress_steps != 0) {
        return false;
    }
    for (unsigned step = 1; step < DD2_AI_TEST_PROGRESS_STEPS; ++step) {
        if (!dd2_ai_driver_step(driver, observation, &control) || control.throttle >= 0 ||
            driver->progress_steps != 0) {
            return false;
        }
    }
    return driver->reverse_steps == 0 && dd2_ai_driver_step(driver, observation, &control) &&
           control.throttle >= 0 && driver->progress_steps == 1;
}

static bool dd2_ai_test_progress_invalid(dd2_ai_driver driver,
                                         const dd2_ai_observation *observation) {
    const dd2_ai_driver saved = driver;
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff8000000000001)};
    dd2_vehicle_control control = {0};
    driver.progress_origin.x = invalid.number;
    if (dd2_ai_driver_step(&driver, observation, &control) || driver.steps != saved.steps ||
        driver.progress_steps != saved.progress_steps || control.throttle != 0) {
        return false;
    }
    driver = saved;
    driver.progress_steps = DD2_AI_TEST_PROGRESS_STEPS + 1;
    if (dd2_ai_driver_step(&driver, observation, &control) || driver.steps != saved.steps ||
        driver.progress_steps != DD2_AI_TEST_PROGRESS_STEPS + 1 || control.throttle != 0) {
        return false;
    }
    return dd2_ai_driver_reset(
               &driver,
               (dd2_ai_start){.road = observation->road, .cell = 0, .slot = 0, .count = 2}) &&
           driver.progress_steps == 0 && driver.progress_origin.x == 0 &&
           driver.progress_origin.y == 0 && driver.progress_origin.z == 0;
}

/* A contact-limited car may advertise forward speed while its accepted pose
 * barely moves. Exercise that independently from the zero-speed stall case. */
static bool dd2_ai_test_progress_case(const dd2_road *road, const dd2_road_surface *surface,
                                      dd2_ai_test_motion motion) {
    dd2_vehicle vehicles[2] = {0};
    if (!dd2_vehicle_reset(&vehicles[0],
                           (dd2_vehicle_spawn){.position = {.y = dd2_ai_test_height}}) ||
        !dd2_vehicle_reset(
            &vehicles[1],
            (dd2_vehicle_spawn){.position = {.y = dd2_ai_test_height, .z = dd2_ai_test_wrap}})) {
        return false;
    }
    vehicles[0].velocity.z = dd2_ai_test_advertised_speed;
    dd2_ai_driver driver = {0};
    if (!dd2_ai_driver_reset(&driver,
                             (dd2_ai_start){.road = road, .cell = 0, .slot = 0, .count = 2})) {
        return false;
    }
    const dd2_ai_observation observation = {
        .road = road, .surface = surface, .vehicles = vehicles, .count = 2, .slot = 0};
    dd2_vehicle_control control = {0};
    const unsigned limit =
        motion == DD2_AI_TEST_MOVING ? DD2_AI_TEST_MOVING_STEPS : DD2_AI_TEST_PROGRESS_STEPS;
    for (unsigned step = 0; step < limit; ++step) {
        vehicles[0].position.z =
            dd2_ai_test_position((dd2_ai_test_sample){.motion = motion, .step = step});
        if (!dd2_ai_driver_step(&driver, &observation, &control) || driver.stuck_steps != 0 ||
            (step + 1 < DD2_AI_TEST_PROGRESS_STEPS && control.throttle < 0)) {
            return false;
        }
        if (motion == DD2_AI_TEST_MOVING && (control.throttle < 0 || driver.reverse_steps != 0)) {
            return false;
        }
    }
    return (motion == DD2_AI_TEST_MOVING ||
            dd2_ai_test_reverse_expiry(&driver, &observation, control)) &&
           dd2_ai_test_progress_invalid(driver, &observation);
}

static bool dd2_ai_test_progress(void) {
    dd2_road *road = dd2_ai_test_road();
    dd2_road_surface *surface = dd2_road_surface_create(road);
    bool valid = road != NULL && surface != NULL;
    for (unsigned motion = 0; motion < DD2_AI_TEST_MOTIONS && valid; ++motion) {
        valid = dd2_ai_test_progress_case(road, surface, (dd2_ai_test_motion)motion);
    }
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    return valid;
}

int main(void) {
    dd2_road *road = dd2_ai_test_road();
    dd2_road_surface *surface = dd2_road_surface_create(road);
    const bool valid = road != NULL && surface != NULL && dd2_ai_test_path(road) &&
                       dd2_ai_test_driver(road, surface) &&
                       dd2_ai_test_passing_return(road, surface, false) &&
                       dd2_ai_test_passing_return(road, surface, true) &&
                       dd2_ai_test_stopped_reverse(road, surface) && dd2_ai_test_progress();
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    puts(valid ? "AI guidance/control/recovery validation: PASS" : "AI validation: FAIL");
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
