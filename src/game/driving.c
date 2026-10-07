#include "game/driving.h"

#include "ai/driver.h"
#include "assets/barriers.h"
#include "assets/road.h"
#include "game/accidents.h"
#include "game/course.h"
#include "game/laps.h"
#include "game/race.h"
#include "game/recovery.h"
#include "game/sound_events.h"
#include "game/starting_grid.h"
#include "physics/barrier_world.h"
#include "physics/damage.h"
#include "physics/numeric.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum { DD2_DRIVING_RACING_LEVELS = 7, DD2_DRIVING_LEVELS = 11, DD2_DRIVING_SETTLE_STEPS = 200 };
static const double dd2_driving_full_turn = 6.28318530717958647693;
static const double dd2_driving_ride_height = 190;
static const double dd2_driving_wheel_radius = 60;
static const double dd2_driving_max_frame = 0.25;
static const double dd2_driving_time_tolerance = 1e-12;
static const double dd2_driving_heading_tolerance = 1e-10;
static const double dd2_driving_progress_spacing = 32;

struct dd2_driving {
    const dd2_road *road;
    dd2_road_surface *surface;
    dd2_barriers *barriers;
    dd2_barrier_world *barrier_world;
    dd2_course *course;
    dd2_course_rules course_rules;
    unsigned count;
    dd2_race race;
    bool racing;
    uint64_t collisions;
    dd2_grid_start starts[DD2_VEHICLE_FLEET_LIMIT];
    dd2_vehicle vehicles[DD2_VEHICLE_FLEET_LIMIT];
    dd2_ai_driver drivers[DD2_VEHICLE_FLEET_LIMIT];
    dd2_vehicle_damage damages[DD2_VEHICLE_FLEET_LIMIT];
    dd2_recovery_driver recovery[DD2_VEHICLE_FLEET_LIMIT];
    dd2_accident_driver accidents[DD2_VEHICLE_FLEET_LIMIT];
    dd2_lap_driver laps[DD2_VEHICLE_FLEET_LIMIT];
    dd2_vehicle_collision_report contacts;
    dd2_sound_state sounds;
    dd2_sound_batch sound_events;
    bool opponents;
    bool damage_enabled;
    uint64_t pair_collisions;
    double accumulator;
    double wheel_rolls[DD2_VEHICLE_FLEET_LIMIT];
};

static void dd2_driving_observe(const dd2_vehicle *vehicles, const dd2_vehicle_damage *damages,
                                const dd2_accident_driver *accidents,
                                dd2_accident_observation *observations, unsigned count) {
    for (unsigned slot = 0; slot < count; ++slot) {
        const dd2_vehicle_vector forward =
            dd2_vehicle_rotate(vehicles[slot].rotation, (dd2_vehicle_vector){.z = 1});
        /* A near-vertical forward axis has no horizontal heading. Preserve the
         * last observation rather than manufacture a spin at its singularity. */
        observations[slot] = (dd2_accident_observation){
            .heading = hypot(forward.x, forward.z) > dd2_driving_heading_tolerance
                           ? atan2(forward.x, forward.z)
                           : accidents[slot].heading,
            .retired = damages[slot].retired};
    }
}

static bool dd2_driving_align(dd2_vehicle *vehicle, const dd2_driving *driving, unsigned slot) {
    dd2_road_contact contact = {0};
    if (!dd2_road_surface_sample(
            driving->surface,
            (dd2_surface_query){.point = {.x = driving->starts[slot].spawn.position.x,
                                          .z = driving->starts[slot].spawn.position.z},
                                .min_height = driving->starts[slot].spawn.position.y -
                                              (2 * dd2_driving_ride_height),
                                .max_height = driving->starts[slot].spawn.position.y,
                                .preferred_cell = driving->starts[slot].cell},
            &contact, NULL)) {
        return false;
    }
    /* Shortest rotation from world up to the source contact normal, followed
     * by the configured yaw. Keep the typed original spawn as reset evidence. */
    const double length = sqrt(2 * (1 + contact.normal[1]));
    const dd2_vehicle_rotation tilt = {.x = contact.normal[2] / length,
                                       .z = -contact.normal[0] / length,
                                       .w = (1 + contact.normal[1]) / length};
    const dd2_vehicle_rotation yaw = vehicle->rotation;
    vehicle->rotation = (dd2_vehicle_rotation){.x = (tilt.x * yaw.w) - (tilt.z * yaw.y),
                                               .y = tilt.w * yaw.y,
                                               .z = (tilt.x * yaw.y) + (tilt.z * yaw.w),
                                               .w = tilt.w * yaw.w};
    vehicle->position.y = contact.height + (dd2_driving_ride_height / contact.normal[1]);
    return true;
}

static bool dd2_driving_settle(const dd2_driving *driving, dd2_vehicle *vehicles) {
    for (unsigned slot = 0; slot < driving->count; ++slot) {
        if (!dd2_vehicle_reset(&vehicles[slot], driving->starts[slot].spawn) ||
            !dd2_driving_align(&vehicles[slot], driving, slot)) {
            return false;
        }
    }
    for (unsigned step = 0; step < DD2_DRIVING_SETTLE_STEPS; ++step) {
        dd2_vehicle previous[DD2_VEHICLE_FLEET_LIMIT] = {0};
        for (unsigned slot = 0; slot < driving->count; ++slot) {
            previous[slot] = vehicles[slot];
            if (!dd2_vehicle_step(&vehicles[slot], driving->road, driving->surface,
                                  (dd2_vehicle_control){.brake = 1})) {
                return false;
            }
        }
        if (!dd2_vehicle_collide_fleet(vehicles, previous, driving->count, driving->surface,
                                       driving->barrier_world, NULL, NULL)) {
            return false;
        }
    }
    return true;
}

bool dd2_driving_reset(dd2_driving *driving) {
    if (driving == NULL) {
        return false;
    }
    dd2_vehicle vehicles[DD2_VEHICLE_FLEET_LIMIT] = {0};
    if (!dd2_driving_settle(driving, vehicles)) {
        return false;
    }
    dd2_ai_driver drivers[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_lap_driver laps[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_accident_driver accidents[DD2_VEHICLE_FLEET_LIMIT] = {0};
    const dd2_vehicle_damage damages[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_accident_observation observations[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_driving_observe(vehicles, damages, accidents, observations, driving->count);
    if (!dd2_accidents_reset(accidents, observations, driving->count)) {
        return false;
    }
    for (unsigned slot = 0; slot < driving->count; ++slot) {
        if (driving->course != NULL &&
            !dd2_laps_reset(&laps[slot], driving->course, driving->starts[slot].cell)) {
            return false;
        }
        if (!dd2_ai_driver_reset(&drivers[slot], (dd2_ai_start){.road = driving->road,
                                                                .cell = driving->starts[slot].cell,
                                                                .slot = slot,
                                                                .count = driving->count})) {
            return false;
        }
        if (driving->racing && driving->race.rules.mode == DD2_RACE_TOTAL_DESTRUCTION) {
            drivers[slot].target = 0;
        }
    }
    dd2_race race = {0};
    if (driving->racing && !dd2_race_reset(&race, driving->race.rules,
                                           (dd2_race_observation){.laps = laps,
                                                                  .damage = damages,
                                                                  .accidents = accidents,
                                                                  .count = driving->count})) {
        return false;
    }
    driving->race = race;
    for (unsigned slot = 0; slot < DD2_VEHICLE_FLEET_LIMIT; ++slot) {
        driving->vehicles[slot] = vehicles[slot];
        driving->drivers[slot] = drivers[slot];
        driving->damages[slot] = (dd2_vehicle_damage){0};
        driving->recovery[slot] = (dd2_recovery_driver){0};
        driving->accidents[slot] = accidents[slot];
        driving->laps[slot] = laps[slot];
        driving->wheel_rolls[slot] = 0;
    }
    driving->accumulator = 0;
    driving->contacts = (dd2_vehicle_collision_report){0};
    driving->sounds = (dd2_sound_state){.waiting_for_go = driving->racing};
    driving->sound_events = (dd2_sound_batch){0};
    driving->collisions = 0;
    driving->pair_collisions = 0;
    return true;
}

static bool dd2_driving_grid_valid(const unsigned *slot_for_driver) {
    if (slot_for_driver == NULL) {
        return true;
    }
    bool used[DD2_VEHICLE_FLEET_LIMIT] = {false};
    for (unsigned driver = 0; driver < DD2_VEHICLE_FLEET_LIMIT; ++driver) {
        const unsigned slot = slot_for_driver[driver];
        if (slot >= DD2_VEHICLE_FLEET_LIMIT || used[slot]) {
            return false;
        }
        used[slot] = true;
    }
    return true;
}

static bool dd2_driving_make_grid(dd2_driving *driving, unsigned level,
                                  const unsigned *slot_for_driver) {
    dd2_grid_start physical[DD2_VEHICLE_FLEET_LIMIT] = {0};
    if (!dd2_starting_grid(driving->road, driving->surface, level, DD2_VEHICLE_FLEET_LIMIT,
                           physical)) {
        return false;
    }
    for (unsigned driver = 0; driver < DD2_VEHICLE_FLEET_LIMIT; ++driver) {
        const unsigned slot = slot_for_driver != NULL ? slot_for_driver[driver] : driver;
        driving->starts[driver] = physical[slot];
    }
    return true;
}

dd2_driving *dd2_driving_create(const dd2_road *road, unsigned level) {
    return dd2_driving_create_grid(road, level, NULL);
}

dd2_driving *dd2_driving_create_grid(const dd2_road *road, unsigned level,
                                     const unsigned *slot_for_driver) {
    if (!dd2_driving_grid_valid(slot_for_driver) || road == NULL || level == 0 ||
        level > DD2_DRIVING_LEVELS ||
        ((level <= DD2_DRIVING_RACING_LEVELS) != (dd2_road_strip_count(road) != 0))) {
        return NULL;
    }
    dd2_driving *driving = calloc(1, sizeof(*driving));
    if (driving == NULL) {
        return NULL;
    }
    driving->road = road;
    driving->count = DD2_VEHICLE_FLEET_LIMIT;
    driving->opponents = true;
    driving->damage_enabled = true;
    driving->surface = dd2_road_surface_create(road);
    if (level <= DD2_DRIVING_RACING_LEVELS) {
        dd2_course_rules rules = {0};
        if (!dd2_course_original_rules(level, &rules)) {
            dd2_driving_destroy(driving);
            return NULL;
        }
        driving->course_rules = rules;
        driving->course = dd2_course_create(road, rules);
        if (driving->course == NULL) {
            dd2_driving_destroy(driving);
            return NULL;
        }
    }
    driving->barriers = dd2_barriers_create(road, level);
    driving->barrier_world = dd2_barrier_world_create(driving->barriers);
    if (driving->surface == NULL || driving->barrier_world == NULL ||
        !dd2_driving_make_grid(driving, level, slot_for_driver) || !dd2_driving_reset(driving)) {
        dd2_driving_destroy(driving);
        return NULL;
    }
    return driving;
}

void dd2_driving_destroy(dd2_driving *driving) {
    if (driving != NULL) {
        dd2_barrier_world_destroy(driving->barrier_world);
        dd2_barriers_destroy(driving->barriers);
        dd2_road_surface_destroy(driving->surface);
        dd2_course_destroy(driving->course);
        free(driving);
    }
}

static bool dd2_driving_progress(const dd2_driving *driving, dd2_lap_driver *lap,
                                 const dd2_vehicle *previous, const dd2_vehicle *vehicle,
                                 bool retired) {
    /* Sample actual center motion densely enough to visit narrow source strips.
     * The local support-height window keeps bridge levels separate. Unsupported
     * samples cannot advance checkpoints; no global nearest-road projection.
     * Exceptional displacements beyond the bounded trace earn no progress. */
    const double distance = hypot(vehicle->position.x - previous->position.x,
                                  vehicle->position.z - previous->position.z);
    uint32_t cells[DD2_LAP_TRACE_LIMIT] = {0};
    if (distance > dd2_driving_progress_spacing * (double)DD2_LAP_TRACE_LIMIT) {
        cells[0] = DD2_ROAD_NO_STRIP;
        return dd2_laps_step(lap, driving->course,
                             (dd2_lap_observation){.cells = cells, .count = 1, .retired = retired});
    }
    const unsigned samples = (unsigned)fmax(1, ceil(distance / dd2_driving_progress_spacing));
    uint32_t preferred = lap->cell;
    for (unsigned sample = 0; sample < samples; ++sample) {
        const double fraction = (double)(sample + 1) / (double)samples;
        const double height =
            previous->position.y + (fraction * (vehicle->position.y - previous->position.y));
        dd2_road_contact contact = {0};
        const bool found = dd2_road_surface_sample(
            driving->surface,
            (dd2_surface_query){
                .point = {.x = previous->position.x +
                               (fraction * (vehicle->position.x - previous->position.x)),
                          .z = previous->position.z +
                               (fraction * (vehicle->position.z - previous->position.z))},
                .min_height = height - (2 * dd2_driving_ride_height),
                .max_height = height,
                .preferred_cell = preferred},
            &contact, NULL);
        cells[sample] = found ? contact.cell : DD2_ROAD_NO_STRIP;
        if (found) {
            preferred = contact.cell;
        }
    }
    return dd2_laps_step(
        lap, driving->course,
        (dd2_lap_observation){.cells = cells, .count = samples, .retired = retired});
}

static bool dd2_driving_step(const dd2_driving *driving, dd2_vehicle *vehicles,
                             dd2_ai_driver *drivers, dd2_vehicle_damage *damages,
                             dd2_accident_driver *accidents, dd2_lap_driver *laps,
                             dd2_recovery_driver *recovery, dd2_vehicle_control player,
                             const dd2_race *race, dd2_vehicle_collision_report *report) {
    dd2_vehicle previous[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_vehicle_control controls[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned slot = 0; slot < driving->count; ++slot) {
        previous[slot] = vehicles[slot];
        controls[slot] = (dd2_vehicle_control){.brake = 1};
    }
    controls[0] = player;
    if (race != NULL && (race->phase == DD2_RACE_COASTING || race->drivers[0].finish_place != 0)) {
        controls[0] = (dd2_vehicle_control){.brake = 1};
    }
    for (unsigned slot = 1; slot < driving->count && driving->opponents; ++slot) {
        const dd2_ai_observation observation = {
            .road = driving->road,
            .surface = driving->surface,
            .vehicles = previous,
            .count = driving->count,
            .slot = slot,
            .pursue_player = race != NULL && race->rules.mode == DD2_RACE_TOTAL_DESTRUCTION};
        if (race != NULL && race->drivers[slot].finish_place != 0) {
            continue;
        }
        if (!dd2_ai_driver_step(&drivers[slot], &observation, &controls[slot])) {
            return false;
        }
    }
    for (unsigned slot = 0; slot < driving->count; ++slot) {
        if (driving->damage_enabled) {
            controls[slot] = dd2_damage_control(&damages[slot], controls[slot]);
        }
        if (!dd2_vehicle_step(&vehicles[slot], driving->road, driving->surface, controls[slot])) {
            return false;
        }
    }
    if (!dd2_vehicle_collide_fleet_report(vehicles, previous, driving->count, driving->surface,
                                          driving->barrier_world, report) ||
        (driving->damage_enabled &&
         !dd2_damage_step(damages,
                          (dd2_damage_frame){.contacts = report, .count = driving->count}))) {
        return false;
    }
    dd2_accident_observation observations[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_driving_observe(vehicles, damages, accidents, observations, driving->count);
    for (unsigned slot = 0; slot < driving->count && driving->course != NULL; ++slot) {
        if (!dd2_driving_progress(driving, &laps[slot], &previous[slot], &vehicles[slot],
                                  damages[slot].retired)) {
            return false;
        }
    }
    return dd2_accidents_step(accidents, (dd2_accident_frame){.contacts = report,
                                                              .vehicles = observations,
                                                              .count = driving->count}) &&
           dd2_recovery_step(recovery, vehicles,
                             (dd2_recovery_frame){.road = driving->road,
                                                  .surface = driving->surface,
                                                  .world = driving->barrier_world,
                                                  .damage = damages,
                                                  .count = driving->count});
}

static bool dd2_driving_hold_step(dd2_race *race, dd2_race_observation observation,
                                  dd2_sound_state *sounds, dd2_sound_batch *events,
                                  dd2_sound_observation sound) {
    const bool countdown = race->phase == DD2_RACE_COUNTDOWN;
    return dd2_race_step(race, observation) &&
           (!countdown || dd2_sound_events_step(sounds, events, sound));
}

bool dd2_driving_advance(dd2_driving *driving, dd2_driving_frame frame) {
    if (driving == NULL || !dd2_numeric_finite(&frame.seconds) || frame.seconds < 0 ||
        frame.seconds > dd2_driving_max_frame || !dd2_numeric_finite(&frame.control.throttle) ||
        fabs(frame.control.throttle) > 1 || !dd2_numeric_finite(&frame.control.brake) ||
        frame.control.brake < 0 || frame.control.brake > 1 ||
        !dd2_numeric_finite(&frame.control.steer) || fabs(frame.control.steer) > 1) {
        return false;
    }
    dd2_vehicle vehicles[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_ai_driver drivers[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_vehicle_damage damages[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_recovery_driver recovery[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_accident_driver accidents[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_lap_driver laps[DD2_VEHICLE_FLEET_LIMIT] = {0};
    double rolls[DD2_VEHICLE_FLEET_LIMIT] = {0};
    for (unsigned slot = 0; slot < driving->count; ++slot) {
        vehicles[slot] = driving->vehicles[slot];
        drivers[slot] = driving->drivers[slot];
        damages[slot] = driving->damages[slot];
        recovery[slot] = driving->recovery[slot];
        accidents[slot] = driving->accidents[slot];
        laps[slot] = driving->laps[slot];
        rolls[slot] = driving->wheel_rolls[slot];
    }
    dd2_sound_state sounds = driving->sounds;
    dd2_sound_batch sound_events = {0};
    dd2_race race = driving->race;
    double accumulator = driving->accumulator + frame.seconds;
    uint64_t pairs = driving->pair_collisions;
    dd2_vehicle_collision_report report = {0};
    uint64_t collisions = driving->collisions;
    const unsigned steps =
        (unsigned)floor((accumulator + dd2_driving_time_tolerance) / DD2_VEHICLE_STEP_SECONDS);
    for (unsigned step = 0; step < steps; ++step) {
        const dd2_race_observation observation = {.laps = laps,
                                                  .damage = damages,
                                                  .accidents = accidents,
                                                  .recovery = recovery,
                                                  .count = driving->count};
        if (driving->racing &&
            (race.phase == DD2_RACE_COUNTDOWN || race.phase == DD2_RACE_RESULTS)) {
            report = (dd2_vehicle_collision_report){0};
            if (!dd2_driving_hold_step(&race, observation, &sounds, &sound_events,
                                       (dd2_sound_observation){.listener = &vehicles[0],
                                                               .contacts = &report,
                                                               .race = &race,
                                                               .count = driving->count})) {
                return false;
            }
            continue;
        }
        if (!dd2_driving_step(driving, vehicles, drivers, damages, accidents, laps, recovery,
                              frame.control, driving->racing ? &race : NULL, &report) ||
            (driving->racing && !dd2_race_step(&race, observation)) ||
            UINT64_MAX - collisions < report.impacts[0].contacts ||
            UINT64_MAX - pairs < report.impacts[0].pair_contacts) {
            return false;
        }
        if (!dd2_sound_events_step(&sounds, &sound_events,
                                   (dd2_sound_observation){.listener = &vehicles[0],
                                                           .contacts = &report,
                                                           .race = driving->racing ? &race : NULL,
                                                           .count = driving->count})) {
            return false;
        }
        collisions += report.impacts[0].contacts;
        pairs += report.impacts[0].pair_contacts;
        for (unsigned slot = 0; slot < driving->count; ++slot) {
            const dd2_vehicle *vehicle = &vehicles[slot];
            const dd2_vehicle_vector forward =
                dd2_vehicle_rotate(vehicle->rotation, (dd2_vehicle_vector){.z = 1});
            const double speed = (vehicle->velocity.x * forward.x) +
                                 (vehicle->velocity.y * forward.y) +
                                 (vehicle->velocity.z * forward.z);
            rolls[slot] =
                fmod(rolls[slot] + (speed * DD2_VEHICLE_STEP_SECONDS / dd2_driving_wheel_radius),
                     dd2_driving_full_turn);
        }
    }
    accumulator = fmax(0, accumulator - ((double)steps * DD2_VEHICLE_STEP_SECONDS));
    for (unsigned slot = 0; slot < driving->count; ++slot) {
        driving->vehicles[slot] = vehicles[slot];
        driving->drivers[slot] = drivers[slot];
        driving->damages[slot] = damages[slot];
        driving->recovery[slot] = recovery[slot];
        driving->accidents[slot] = accidents[slot];
        driving->laps[slot] = laps[slot];
        driving->wheel_rolls[slot] = rolls[slot];
    }
    driving->race = race;
    driving->sounds = sounds;
    driving->sound_events = sound_events;
    driving->pair_collisions = pairs;
    driving->contacts = report;
    driving->accumulator = accumulator;
    driving->collisions = collisions;
    return true;
}

void dd2_driving_suspend(dd2_driving *driving) {
    if (driving != NULL) {
        driving->accumulator = 0;
    }
}

void dd2_driving_set_opponents(dd2_driving *driving, bool enabled) {
    if (driving != NULL) {
        driving->opponents = enabled;
        driving->accumulator = 0;
    }
}
bool dd2_driving_opponents(const dd2_driving *driving) {
    return driving != NULL && driving->opponents;
}
const dd2_ai_driver *dd2_driving_drivers(const dd2_driving *driving) {
    return driving != NULL ? driving->drivers : NULL;
}
const dd2_vehicle_collision_report *dd2_driving_contact_report(const dd2_driving *driving) {
    return driving != NULL ? &driving->contacts : NULL;
}
const dd2_sound_batch *dd2_driving_sound_events(const dd2_driving *driving) {
    return driving != NULL ? &driving->sound_events : NULL;
}

void dd2_driving_set_damage(dd2_driving *driving, bool enabled) {
    if (driving != NULL) {
        driving->damage_enabled = enabled;
        driving->accumulator = 0;
    }
}
bool dd2_driving_damage_enabled(const dd2_driving *driving) {
    return driving != NULL && driving->damage_enabled;
}
const dd2_vehicle_damage *dd2_driving_damage(const dd2_driving *driving) {
    return driving != NULL ? driving->damages : NULL;
}
const dd2_recovery_driver *dd2_driving_recovery(const dd2_driving *driving) {
    return driving != NULL ? driving->recovery : NULL;
}
const dd2_accident_driver *dd2_driving_accidents(const dd2_driving *driving) {
    return driving != NULL ? driving->accidents : NULL;
}
const dd2_course *dd2_driving_course(const dd2_driving *driving) {
    return driving == NULL ? NULL : driving->course;
}
const dd2_lap_driver *dd2_driving_laps(const dd2_driving *driving) {
    return driving == NULL || driving->course == NULL ? NULL : driving->laps;
}

const dd2_vehicle *dd2_driving_vehicle(const dd2_driving *driving) {
    return driving != NULL ? &driving->vehicles[0] : NULL;
}

double dd2_driving_wheel_roll(const dd2_driving *driving) {
    return driving != NULL ? driving->wheel_rolls[0] : 0;
}

const dd2_vehicle_spawn *dd2_driving_start(const dd2_driving *driving) {
    return driving != NULL ? &driving->starts[0].spawn : NULL;
}

uint64_t dd2_driving_collisions(const dd2_driving *driving) {
    return driving != NULL ? driving->collisions : 0;
}

unsigned dd2_driving_vehicle_count(const dd2_driving *driving) {
    return driving != NULL ? driving->count : 0;
}
const dd2_vehicle *dd2_driving_vehicles(const dd2_driving *driving) {
    return driving != NULL ? driving->vehicles : NULL;
}
const double *dd2_driving_wheel_rolls(const dd2_driving *driving) {
    return driving != NULL ? driving->wheel_rolls : NULL;
}
uint64_t dd2_driving_pair_collisions(const dd2_driving *driving) {
    return driving != NULL ? driving->pair_collisions : 0;
}
const dd2_vehicle_spawn *dd2_driving_grid_start(const dd2_driving *driving, unsigned slot) {
    return driving != NULL && slot < driving->count ? &driving->starts[slot].spawn : NULL;
}

bool dd2_driving_set_race(dd2_driving *driving, bool enabled, dd2_race_mode mode) {
    if (driving == NULL ||
        (enabled && mode != DD2_RACE_WRECKING && mode != DD2_RACE_STOCKCAR &&
         mode != DD2_RACE_TIME_TRIAL && mode != DD2_RACE_TOTAL_DESTRUCTION) ||
        (enabled && (mode == DD2_RACE_STOCKCAR || mode == DD2_RACE_TIME_TRIAL) &&
         driving->course == NULL) ||
        (enabled && mode == DD2_RACE_TOTAL_DESTRUCTION && driving->course != NULL)) {
        return false;
    }
    dd2_course *course = NULL;
    if (driving->course != NULL) {
        dd2_course_rules rules = driving->course_rules;
        if (enabled && mode == DD2_RACE_TIME_TRIAL) {
            rules.laps = 0;
        }
        course = dd2_course_create(driving->road, rules);
        if (course == NULL) {
            return false;
        }
    }
    const dd2_race previous = driving->race;
    dd2_course *previous_course = driving->course;
    const unsigned previous_count = driving->count;
    const bool was_racing = driving->racing;
    driving->course = course;
    driving->count = enabled && mode == DD2_RACE_TIME_TRIAL ? 1 : DD2_VEHICLE_FLEET_LIMIT;
    driving->racing = enabled;
    driving->race.rules = (dd2_race_rules){.mode = mode,
                                           .count = driving->count,
                                           .length = dd2_course_length(course),
                                           .laps = dd2_course_laps(course)};
    if (!dd2_driving_reset(driving)) {
        driving->race = previous;
        driving->racing = was_racing;
        driving->course = previous_course;
        driving->count = previous_count;
        dd2_course_destroy(course);
        return false;
    }
    dd2_course_destroy(previous_course);
    return true;
}

const dd2_race *dd2_driving_race(const dd2_driving *driving) {
    return driving == NULL || !driving->racing ? NULL : &driving->race;
}

bool dd2_driving_withdraw(dd2_driving *driving) {
    if (driving == NULL || !driving->racing || !dd2_race_withdraw(&driving->race)) {
        return false;
    }
    driving->accumulator = 0;
    driving->contacts = (dd2_vehicle_collision_report){0};
    driving->sound_events = (dd2_sound_batch){0};
    return true;
}
