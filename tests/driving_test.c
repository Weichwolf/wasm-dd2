#include "ai/driver.h"
#include "asset_fixture.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "game/accidents.h"
#include "game/driving.h"
#include "game/league.h"
#include "game/race.h"
#include "game/recovery.h"
#include "game/sound_events.h"
#include "physics/damage.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_DRIVE_TEST_SIDE = 32,
    DD2_DRIVE_TEST_VERTICES = DD2_DRIVE_TEST_SIDE * DD2_DRIVE_TEST_SIDE,
    DD2_DRIVE_TEST_VERTEX_BYTES = 12,
    DD2_DRIVE_TEST_LANE_BYTES = 14,
    DD2_DRIVE_TEST_Z_OFFSET = 8,
    DD2_DRIVE_TEST_SPACING = 2000,
    DD2_DRIVE_TEST_ORIGIN = 32000,
    DD2_DRIVE_TEST_ARENA = 8,
    DD2_DRIVE_TEST_LAST_ARENA = 11,
    DD2_DRIVE_TEST_FRAMES = 400,
    DD2_DRIVE_TEST_PARTS = 5,
    DD2_DRIVE_TEST_RESET_STEPS = 200
};
static const double dd2_drive_test_frame_seconds = 0.025;
static const double dd2_drive_test_partial_seconds = 0.004;
static const double dd2_drive_test_resume_seconds = 0.001;
static const double dd2_drive_test_tolerance = 1e-8;
static const double dd2_drive_test_start = -12000;
static const double dd2_drive_test_quarter_turn = 1.57079632679489661923;

static dd2_road *dd2_drive_test_road(void) {
    uint8_t vertices[DD2_DRIVE_TEST_VERTICES * DD2_DRIVE_TEST_VERTEX_BYTES] = {0};
    uint8_t source[DD2_DRIVE_TEST_VERTICES * DD2_DRIVE_TEST_LANE_BYTES] = {0};
    for (unsigned index = 0; index < DD2_DRIVE_TEST_VERTICES; ++index) {
        uint8_t *vertex = vertices + ((size_t)index * DD2_DRIVE_TEST_VERTEX_BYTES);
        const int32_t xpos = ((int32_t)(index % DD2_DRIVE_TEST_SIDE) * DD2_DRIVE_TEST_SPACING) -
                             DD2_DRIVE_TEST_ORIGIN;
        const int32_t zpos = ((int32_t)(index / DD2_DRIVE_TEST_SIDE) * DD2_DRIVE_TEST_SPACING) -
                             DD2_DRIVE_TEST_ORIGIN;
        dd2_test_write_le32(vertex, (uint32_t)xpos);
        dd2_test_write_le32(vertex + DD2_DRIVE_TEST_Z_OFFSET, (uint32_t)zpos);
    }
    dd2_level_data level = {.vertex_count = DD2_DRIVE_TEST_VERTICES};
    level.sections[DD2_LEVEL_ROAD_VERTICES] =
        (dd2_byte_view){.data = vertices, .size = sizeof(vertices)};
    level.sections[DD2_LEVEL_ROAD_STRIPS] = (dd2_byte_view){.data = source, .size = sizeof(source)};
    return dd2_road_create(&level, DD2_ROAD_ARENA);
}

static bool dd2_drive_test_same(const dd2_vehicle *first, const dd2_vehicle *second) {
    return first->steps == second->steps &&
           fabs(first->position.x - second->position.x) < dd2_drive_test_tolerance &&
           fabs(first->position.y - second->position.y) < dd2_drive_test_tolerance &&
           fabs(first->position.z - second->position.z) < dd2_drive_test_tolerance &&
           fabs(first->velocity.x - second->velocity.x) < dd2_drive_test_tolerance &&
           fabs(first->velocity.y - second->velocity.y) < dd2_drive_test_tolerance &&
           fabs(first->velocity.z - second->velocity.z) < dd2_drive_test_tolerance &&
           fabs(first->rotation.x - second->rotation.x) < dd2_drive_test_tolerance &&
           fabs(first->rotation.y - second->rotation.y) < dd2_drive_test_tolerance &&
           fabs(first->rotation.z - second->rotation.z) < dd2_drive_test_tolerance &&
           fabs(first->rotation.w - second->rotation.w) < dd2_drive_test_tolerance;
}

static bool dd2_drive_test_vector(dd2_vehicle_vector first, dd2_vehicle_vector second) {
    return fabs(first.x - second.x) < dd2_drive_test_tolerance &&
           fabs(first.y - second.y) < dd2_drive_test_tolerance &&
           fabs(first.z - second.z) < dd2_drive_test_tolerance;
}

static bool dd2_drive_test_contacts(const dd2_driving *first, const dd2_driving *second) {
    const dd2_vehicle_collision_report *left = dd2_driving_contact_report(first);
    const dd2_vehicle_collision_report *right = dd2_driving_contact_report(second);
    if (left->count != right->count || left->pair_contacts != right->pair_contacts) {
        return false;
    }
    for (unsigned index = 0; index < left->count; ++index) {
        const dd2_vehicle_contact left_contact = left->contacts[index];
        const dd2_vehicle_contact right_contact = right->contacts[index];
        if (left_contact.kind != right_contact.kind || left_contact.first != right_contact.first ||
            left_contact.second != right_contact.second ||
            left_contact.obstacle != right_contact.obstacle ||
            fabs(left_contact.time - right_contact.time) > dd2_drive_test_tolerance ||
            fabs(left_contact.normal_speed - right_contact.normal_speed) >
                dd2_drive_test_tolerance ||
            fabs(left_contact.impulse - right_contact.impulse) > dd2_drive_test_tolerance ||
            !dd2_drive_test_vector(left_contact.point, right_contact.point) ||
            !dd2_drive_test_vector(left_contact.normal, right_contact.normal) ||
            !dd2_drive_test_vector(left_contact.local_points[0], right_contact.local_points[0]) ||
            !dd2_drive_test_vector(left_contact.local_points[1], right_contact.local_points[1])) {
            return false;
        }
    }
    return true;
}

static bool dd2_drive_test_field(const dd2_driving *first, const dd2_driving *second) {
    const dd2_ai_driver *first_drivers = dd2_driving_drivers(first);
    const dd2_ai_driver *second_drivers = dd2_driving_drivers(second);
    for (unsigned slot = 0; slot < dd2_driving_vehicle_count(first); ++slot) {
        const dd2_ai_driver driver = first_drivers[slot];
        const dd2_ai_driver other = second_drivers[slot];
        const dd2_vehicle_damage damage = dd2_driving_damage(first)[slot];
        const dd2_vehicle_damage other_damage = dd2_driving_damage(second)[slot];
        const dd2_recovery_driver recovery = dd2_driving_recovery(first)[slot];
        const dd2_recovery_driver other_recovery = dd2_driving_recovery(second)[slot];
        if (recovery.rest_steps != other_recovery.rest_steps ||
            recovery.recoveries != other_recovery.recoveries ||
            recovery.overturned != other_recovery.overturned) {
            return false;
        }
        const dd2_accident_driver score = dd2_driving_accidents(first)[slot];
        const dd2_accident_driver other_score = dd2_driving_accidents(second)[slot];
        if (score.points != other_score.points || score.destructions != other_score.destructions ||
            score.partner != other_score.partner || score.remaining != other_score.remaining ||
            score.quarters != other_score.quarters || score.retired != other_score.retired ||
            score.steps != other_score.steps ||
            fabs(score.heading - other_score.heading) > dd2_drive_test_tolerance ||
            fabs(score.rotation - other_score.rotation) > dd2_drive_test_tolerance) {
            return false;
        }
        if (damage.steps != other_damage.steps || damage.retired != other_damage.retired) {
            return false;
        }
        for (unsigned region = 0; region < DD2_DAMAGE_REGIONS; ++region) {
            if (fabs(damage.regions[region] - other_damage.regions[region]) >
                dd2_drive_test_tolerance) {
                return false;
            }
        }
        if (!dd2_drive_test_same(&dd2_driving_vehicles(first)[slot],
                                 &dd2_driving_vehicles(second)[slot]) ||
            fabs(dd2_driving_wheel_rolls(first)[slot] - dd2_driving_wheel_rolls(second)[slot]) >
                dd2_drive_test_tolerance ||
            driver.cell != other.cell || driver.lane != other.lane ||
            driver.base_lane != other.base_lane || driver.target != other.target ||
            driver.stuck_steps != other.stuck_steps ||
            driver.reverse_steps != other.reverse_steps || driver.steps != other.steps ||
            driver.progress_steps != other.progress_steps ||
            driver.progress_origin.x != other.progress_origin.x ||
            driver.progress_origin.y != other.progress_origin.y ||
            driver.progress_origin.z != other.progress_origin.z) {
            return false;
        }
    }
    return dd2_drive_test_contacts(first, second);
}

static bool dd2_drive_test_clear_contacts(dd2_driving *driving) {
    const dd2_vehicle_collision_report *report = dd2_driving_contact_report(driving);
    const unsigned count = report->count;
    const unsigned pairs = report->pair_contacts;
    const dd2_vehicle_contact contact = report->contacts[0];
    return !dd2_driving_advance(driving, (dd2_driving_frame){.seconds = -1}) &&
           report->count == count && report->pair_contacts == pairs &&
           report->contacts[0].impulse == contact.impulse &&
           dd2_driving_advance(driving, (dd2_driving_frame){0}) && report->count == 0 &&
           report->pair_contacts == 0 && report->impacts[0].contacts == 0;
}

static bool dd2_drive_test_frames(dd2_driving *first, dd2_driving *second) {
    const dd2_vehicle initial = *dd2_driving_vehicle(first);
    const dd2_vehicle_control control = {.throttle = 1, .steer = 0.1};
    bool cleared_contacts = false;
    for (unsigned frame = 0; frame < DD2_DRIVE_TEST_FRAMES; ++frame) {
        if (!dd2_driving_advance(first, (dd2_driving_frame){.seconds = dd2_drive_test_frame_seconds,
                                                            .control = control})) {
            return false;
        }
        for (unsigned part = 0; part < DD2_DRIVE_TEST_PARTS; ++part) {
            if (!dd2_driving_advance(
                    second,
                    (dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS, .control = control})) {
                return false;
            }
        }
        if (!cleared_contacts && dd2_driving_contact_report(first)->count != 0) {
            if (!dd2_drive_test_clear_contacts(first) || !dd2_drive_test_clear_contacts(second)) {
                return false;
            }
            cleared_contacts = true;
        }
    }
    if (!cleared_contacts || !dd2_drive_test_field(first, second) ||
        fabs(dd2_driving_wheel_roll(first) - dd2_driving_wheel_roll(second)) >
            dd2_drive_test_tolerance ||
        dd2_driving_vehicle(first)->steps !=
            DD2_DRIVE_TEST_RESET_STEPS + (DD2_DRIVE_TEST_FRAMES * DD2_DRIVE_TEST_PARTS)) {
        return false;
    }
    if (!dd2_driving_reset(first) || !dd2_driving_reset(second) ||
        !dd2_drive_test_field(first, second) || dd2_driving_contact_report(first)->count != 0 ||
        dd2_driving_damage(first)[0].steps != 0 || dd2_driving_accidents(first)[0].steps != 0 ||
        dd2_driving_accidents(first)[0].points != 0 ||
        dd2_driving_accidents(first)[0].remaining != 0 ||
        dd2_damage_health(&dd2_driving_damage(first)[0]) != 1 ||
        !dd2_drive_test_same(&initial, dd2_driving_vehicle(first))) {
        return false;
    }
    /* A partial frame followed by pause must not leak into resumed simulation. */
    if (!dd2_driving_advance(first, (dd2_driving_frame){.seconds = dd2_drive_test_partial_seconds,
                                                        .control = control})) {
        return false;
    }
    dd2_driving_suspend(first);
    return dd2_driving_advance(first, (dd2_driving_frame){.seconds = dd2_drive_test_resume_seconds,
                                                          .control = control}) &&
           dd2_drive_test_same(&initial, dd2_driving_vehicle(first));
}

static bool dd2_drive_test_rejection(dd2_driving *driving) {
    const dd2_vehicle initial = *dd2_driving_vehicle(driving);
    const union {
        uint64_t bits;
        double number;
    } invalid = {.bits = UINT64_C(0x7ff8000000000001)};
    const dd2_driving_frame bad[] = {{.seconds = invalid.number},
                                     {.seconds = -1},
                                     {.seconds = 1},
                                     {.control = {.throttle = invalid.number}},
                                     {.control = {.brake = -1}},
                                     {.control = {.steer = 2}}};
    for (size_t index = 0; index < sizeof(bad) / sizeof(bad[0]); ++index) {
        if (dd2_driving_advance(driving, bad[index]) ||
            !dd2_drive_test_same(&initial, dd2_driving_vehicle(driving))) {
            return false;
        }
    }
    return true;
}

static bool dd2_drive_test_total(dd2_driving *driving) {
    if (!dd2_driving_set_race(driving, true, DD2_RACE_TOTAL_DESTRUCTION) ||
        dd2_driving_vehicle_count(driving) != DD2_VEHICLE_FLEET_LIMIT ||
        dd2_driving_course(driving) != NULL) {
        return false;
    }
    for (unsigned tick = 0; tick < DD2_RACE_START_STEPS + DD2_DRIVE_TEST_FRAMES; ++tick) {
        if (!dd2_driving_advance(driving,
                                 (dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS})) {
            return false;
        }
        for (unsigned slot = 1; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
            if (dd2_driving_drivers(driving)[slot].target != 0) {
                return false;
            }
        }
    }
    if (dd2_driving_race(driving)->survival != DD2_DRIVE_TEST_FRAMES ||
        !dd2_driving_withdraw(driving) || !dd2_driving_reset(driving) ||
        dd2_driving_race(driving)->survival != 0) {
        return false;
    }
    return dd2_driving_set_race(driving, false, DD2_RACE_WRECKING) &&
           dd2_driving_drivers(driving)[1].target == 1 + DD2_VEHICLE_FLEET_LIMIT / 2;
}
static bool dd2_drive_test_pursuit(const dd2_road *road) {
    enum { DD2_PURSUIT_CARS = 3, DD2_PURSUIT_TICKS = 1200, DD2_PURSUIT_RETARGET = 400 };
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_vehicle vehicles[DD2_PURSUIT_CARS] = {0};
    const dd2_vehicle_spawn starts[] = {{.position = {.y = 190, .z = 5000}},
                                        {.position = {.y = 190}},
                                        {.position = {.y = 190, .z = 2000}}};
    bool valid = surface != NULL;
    for (unsigned slot = 0; slot < DD2_PURSUIT_CARS && valid; ++slot) {
        valid = dd2_vehicle_reset(&vehicles[slot], starts[slot]);
    }
    dd2_ai_driver driver = {.cell = 0, .target = 2};
    dd2_ai_observation observation = {.road = road,
                                      .surface = surface,
                                      .vehicles = vehicles,
                                      .count = DD2_PURSUIT_CARS,
                                      .slot = 1,
                                      .pursue_player = true};
    dd2_vehicle_control control = {0};
    for (unsigned tick = 0; tick < DD2_PURSUIT_TICKS && valid; ++tick) {
        valid = dd2_ai_driver_step(&driver, &observation, &control) && driver.target == 0;
    }
    /* Ordinary arena AI selects the closer opponent at the same retarget tick. */
    observation.pursue_player = false;
    driver.steps = DD2_PURSUIT_RETARGET;
    valid = valid && dd2_ai_driver_step(&driver, &observation, &control) && driver.target == 2;
    observation.pursue_player = true;
    vehicles[1].rotation = (dd2_vehicle_rotation){.x = 1};
    valid = valid && dd2_ai_driver_step(&driver, &observation, &control) && driver.target == 0 &&
            control.brake == 1;
    observation.slot = 0;
    const uint64_t steps = driver.steps;
    valid = valid && !dd2_ai_driver_step(&driver, &observation, &control) &&
            driver.steps == steps && control.throttle == 0 && control.brake == 0;
    dd2_road_surface_destroy(surface);
    return valid;
}

static bool dd2_drive_test_sound_advance(dd2_driving *driving, double seconds,
                                         dd2_sound_event events[4], unsigned *count) {
    if (!dd2_driving_advance(driving, (dd2_driving_frame){.seconds = seconds})) {
        return false;
    }
    const dd2_sound_batch *batch = dd2_driving_sound_events(driving);
    if (*count + batch->count > 4) {
        return false;
    }
    for (unsigned index = 0; index < batch->count; ++index) {
        events[(*count)++] = batch->events[index];
    }
    return true;
}

static bool dd2_drive_test_sound_frames(dd2_driving *first, dd2_driving *second) {
    enum {
        DD2_SOUND_FRAME_PARTS = 50,
        DD2_SOUND_FRAME_COUNT = 8,
        DD2_SOUND_EXPECTED_CUES = 4,
        DD2_SOUND_FIRST_TICK = 36,
        DD2_SOUND_SECOND_TICK = 156,
        DD2_SOUND_THIRD_TICK = 276
    };
    const unsigned expected_ticks[DD2_SOUND_EXPECTED_CUES] = {
        DD2_SOUND_FIRST_TICK, DD2_SOUND_SECOND_TICK, DD2_SOUND_THIRD_TICK, DD2_RACE_START_STEPS};
    const dd2_sound_cue expected_cues[DD2_SOUND_EXPECTED_CUES] = {DD2_SOUND_THREE, DD2_SOUND_TWO,
                                                                  DD2_SOUND_ONE, DD2_SOUND_GO};
    dd2_sound_event coarse[DD2_SOUND_EXPECTED_CUES] = {0};
    dd2_sound_event fine[DD2_SOUND_EXPECTED_CUES] = {0};
    unsigned coarse_count = 0;
    unsigned fine_count = 0;
    if (!dd2_driving_set_race(first, true, DD2_RACE_WRECKING) ||
        !dd2_driving_set_race(second, true, DD2_RACE_WRECKING)) {
        return false;
    }
    for (unsigned frame = 0; frame < DD2_SOUND_FRAME_COUNT; ++frame) {
        if (!dd2_drive_test_sound_advance(first,
                                          DD2_VEHICLE_STEP_SECONDS * (double)DD2_SOUND_FRAME_PARTS,
                                          coarse, &coarse_count)) {
            return false;
        }
        for (unsigned part = 0; part < DD2_SOUND_FRAME_PARTS; ++part) {
            if (!dd2_drive_test_sound_advance(second, DD2_VEHICLE_STEP_SECONDS, fine,
                                              &fine_count)) {
                return false;
            }
        }
    }
    if (coarse_count != DD2_SOUND_EXPECTED_CUES || fine_count != coarse_count) {
        return false;
    }
    for (unsigned index = 0; index < coarse_count; ++index) {
        if (coarse[index].tick != expected_ticks[index] ||
            fine[index].tick != expected_ticks[index] ||
            coarse[index].cue != expected_cues[index] || fine[index].cue != expected_cues[index]) {
            return false;
        }
    }
    return !dd2_driving_advance(first, (dd2_driving_frame){.seconds = 1}) &&
           dd2_driving_sound_events(first)->count == 1 && dd2_driving_reset(first) &&
           dd2_driving_sound_events(first)->count == 0;
}

enum { DD2_GRID_TEST_FRAMES = 100, DD2_GRID_TEST_ELAPSED_STEPS = 100 };

static bool dd2_drive_test_grid_positions(const dd2_driving *identity, const dd2_driving *field,
                                          const unsigned *slots) {
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        const dd2_vehicle_spawn *expected = dd2_driving_grid_start(identity, slots[driver]);
        const dd2_vehicle_spawn *actual = dd2_driving_grid_start(field, driver);
        if (expected == NULL || actual == NULL ||
            !dd2_drive_test_vector(expected->position, actual->position) ||
            expected->yaw != actual->yaw ||
            !dd2_vehicle_valid(&dd2_driving_vehicles(field)[driver]) ||
            dd2_driving_vehicles(field)[driver].steps != DD2_DRIVE_TEST_RESET_STEPS) {
            return false;
        }
    }
    return true;
}

static bool dd2_drive_test_grid_lifecycle(dd2_driving *field) {
    if (!dd2_driving_set_race(field, true, DD2_RACE_TOTAL_DESTRUCTION) ||
        dd2_driving_vehicle(field) != &dd2_driving_vehicles(field)[0]) {
        return false;
    }
    for (unsigned driver = 1; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        if (dd2_driving_drivers(field)[driver].target != 0) {
            return false;
        }
    }
    for (unsigned frame = 0; frame < DD2_GRID_TEST_FRAMES; ++frame) {
        if (!dd2_driving_advance(field, (dd2_driving_frame){.seconds = dd2_drive_test_frame_seconds,
                                                            .control = {.throttle = 1}})) {
            return false;
        }
    }
    return dd2_driving_race(field)->phase == DD2_RACE_RUNNING &&
           dd2_driving_race(field)->elapsed == DD2_GRID_TEST_ELAPSED_STEPS &&
           dd2_driving_withdraw(field) && dd2_driving_race(field)->phase == DD2_RACE_RESULTS &&
           dd2_driving_reset(field) && dd2_driving_race(field)->phase == DD2_RACE_COUNTDOWN;
}

static bool dd2_drive_test_grid_season(const dd2_road *road, const dd2_driving *identity,
                                       const dd2_league *league) {
    unsigned slots[DD2_LEAGUE_DRIVERS] = {0};
    if (!dd2_league_grid(league, slots)) {
        return false;
    }
    dd2_driving *field = dd2_driving_create_grid(road, DD2_DRIVE_TEST_ARENA, slots);
    if (field == NULL) {
        return false;
    }
    bool passed = dd2_drive_test_grid_positions(identity, field, slots);
    const dd2_vehicle_spawn human = *dd2_driving_start(field);
    for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS; ++driver) {
        slots[driver] = DD2_LEAGUE_DRIVERS;
    }
    passed = passed && dd2_driving_create_grid(road, DD2_DRIVE_TEST_ARENA, slots) == NULL &&
             dd2_driving_reset(field) &&
             dd2_drive_test_vector(dd2_driving_start(field)->position, human.position) &&
             dd2_drive_test_grid_lifecycle(field) &&
             dd2_drive_test_vector(dd2_driving_start(field)->position, human.position);
    dd2_driving_destroy(field);
    return passed;
}

static bool dd2_drive_test_league_grid(const dd2_road *road) {
    dd2_driving *identity = dd2_driving_create(road, DD2_DRIVE_TEST_ARENA);
    if (identity == NULL) {
        return false;
    }
    dd2_league league = {0};
    bool passed = dd2_league_reset(&league);
    for (unsigned season = 0; season < DD2_LEAGUE_DIVISIONS && passed; ++season) {
        unsigned points[DD2_LEAGUE_DRIVERS] = {0};
        points[0] = DD2_LEAGUE_RACE_POINT_LIMIT;
        passed = dd2_drive_test_grid_season(road, identity, &league) &&
                 dd2_league_add_points(&league, points) && dd2_league_sort(&league);
        if (season + 1 < DD2_LEAGUE_DIVISIONS) {
            passed = passed && dd2_league_standing(&league, 0) == DD2_LEAGUE_PROMOTED &&
                     dd2_league_transfer(&league) && dd2_league_clear_points(&league);
        } else {
            passed = passed && dd2_league_standing(&league, 0) == DD2_LEAGUE_CHAMPION;
        }
    }
    unsigned duplicates[DD2_LEAGUE_DRIVERS] = {0};
    passed = passed && dd2_driving_create_grid(road, DD2_DRIVE_TEST_ARENA, duplicates) == NULL &&
             dd2_driving_create_grid(NULL, DD2_DRIVE_TEST_ARENA, NULL) == NULL;
    dd2_driving_destroy(identity);
    puts(passed ? "Stable-ID league grids/reset/player/pursuit: PASS"
                : "Stable-ID league grid: FAIL");
    return passed;
}

int main(void) {
    dd2_road *road = dd2_drive_test_road();
    dd2_driving *first = dd2_driving_create(road, DD2_DRIVE_TEST_ARENA);
    dd2_driving *second = dd2_driving_create(road, DD2_DRIVE_TEST_ARENA);
    dd2_driving *last = dd2_driving_create(road, DD2_DRIVE_TEST_LAST_ARENA);
    bool passed = first != NULL && second != NULL && last != NULL &&
                  dd2_driving_damage_enabled(first) && dd2_drive_test_league_grid(road);
    if (passed) {
        const dd2_vehicle_spawn *spawn = dd2_driving_start(first);
        const dd2_vehicle_spawn *other = dd2_driving_start(last);
        passed = spawn->position.x == 0 && spawn->position.z == dd2_drive_test_start &&
                 spawn->yaw == 0 && other->position.x == dd2_drive_test_start &&
                 other->position.z == 0 && other->yaw == dd2_drive_test_quarter_turn &&
                 dd2_drive_test_frames(first, second) && dd2_drive_test_rejection(first) &&
                 dd2_drive_test_pursuit(road) && dd2_drive_test_total(first) &&
                 dd2_drive_test_sound_frames(first, second);
    }
    dd2_driving *bad = dd2_driving_create(road, 1);
    passed = passed && bad == NULL && !dd2_driving_advance(NULL, (dd2_driving_frame){0}) &&
             dd2_driving_contact_report(NULL) == NULL;
    dd2_driving_destroy(bad);
    dd2_driving_destroy(first);
    dd2_driving_destroy(second);
    dd2_driving_destroy(last);
    dd2_road_destroy(road);
    puts(passed ? "driving timing/reset/start validation: PASS" : "driving validation: FAIL");
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
