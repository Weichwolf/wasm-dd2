#include "asset_fixture.h"
#include "assets/road.h"
#include "game/accidents.h"
#include "game/sound_events.h"
#include "physics/damage.h"
#include "physics/road_surface.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"
#include "surface_fixture.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    DD2_RECORDING_VERTICES = 6,
    DD2_RECORDING_LOWER_CORNERS = 4,
    DD2_RECORDING_FLOOR_EXTENT = 100000,
    DD2_RECORDING_GRID_MIDPOINT = 10
};
static const double dd2_recording_tolerance = 1e-8;
static const double dd2_recording_spacing = 10000;
static const double dd2_recording_free_x = 300000;
static const double dd2_recording_collision_x = 90373;
static const double dd2_recording_speed = 1000;
static const double dd2_recording_floor = 130;
static const double dd2_recording_impact_speed = 6000;
static const double dd2_recording_impulse = 3250;

typedef struct {
    dd2_road *road;
    dd2_road_surface *surface;
    dd2_vehicle_collision_storage *published;
    dd2_vehicle_collision_storage *working;
} dd2_recording_fixture;

typedef enum { DD2_RECORDING_FREE, DD2_RECORDING_PAIR } dd2_recording_case;

static bool dd2_recording_create(dd2_recording_fixture *test) {
    dd2_surface_test_fixture fixture = {0};
    dd2_surface_test_fixture_init(&fixture, 1);
    for (unsigned corner = 0; corner < DD2_RECORDING_VERTICES; ++corner) {
        uint8_t *vertex = fixture.vertices + ((size_t)corner * DD2_SURFACE_TEST_VERTEX_BYTES);
        dd2_test_write_le32(vertex,
                            (uint32_t)(((int32_t)(corner % 3) - 1) * DD2_RECORDING_FLOOR_EXTENT));
        dd2_test_write_le32(vertex + DD2_TEST_WORD_BYTES, 0);
        dd2_test_write_le32(
            vertex + (2U * (size_t)DD2_TEST_WORD_BYTES),
            (uint32_t)(corner < 3 ? -DD2_RECORDING_FLOOR_EXTENT : DD2_RECORDING_FLOOR_EXTENT));
    }
    test->road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    test->surface = dd2_road_surface_create(test->road);
    test->published = dd2_vehicle_collision_storage_create();
    test->working = dd2_vehicle_collision_storage_create();
    return test->surface != NULL && test->published != NULL && test->working != NULL;
}

static void dd2_recording_destroy(dd2_recording_fixture *test) {
    dd2_vehicle_collision_storage_destroy(test->published);
    dd2_vehicle_collision_storage_destroy(test->working);
    dd2_road_surface_destroy(test->surface);
    dd2_road_destroy(test->road);
}

static void dd2_recording_propose(dd2_vehicle *previous, dd2_vehicle *next,
                                  dd2_recording_case sample) {
    for (unsigned body = 0; body < DD2_VEHICLE_FLEET_LIMIT; ++body) {
        previous[body] =
            (dd2_vehicle){.position = {.x = ((double)body - (double)DD2_RECORDING_GRID_MIDPOINT) *
                                            dd2_recording_spacing,
                                       .y = dd2_recording_floor},
                          .velocity = {.y = -1},
                          .rotation = {.w = 1}};
    }
    if (sample == DD2_RECORDING_FREE) {
        previous[0] =
            (dd2_vehicle){.position = {.x = dd2_recording_free_x, .y = dd2_recording_spacing},
                          .velocity = {.x = dd2_recording_speed},
                          .rotation = {.w = 1}};
    } else {
        previous[0].position.x = dd2_recording_collision_x;
        previous[0].velocity.x = -dd2_recording_speed;
    }
    for (unsigned body = 0; body < DD2_VEHICLE_FLEET_LIMIT; ++body) {
        next[body] = previous[body];
        next[body].position.x += next[body].velocity.x * DD2_VEHICLE_STEP_SECONDS;
        next[body].position.y += next[body].velocity.y * DD2_VEHICLE_STEP_SECONDS;
    }
}

static bool dd2_recording_clear(const dd2_vehicle_collision_report *report) {
    return report->contacts == NULL && report->count == 0 && report->pair_contacts == 0 &&
           report->unresolved_sweeps == 0 && report->response_events == 0 &&
           report->impacts[0].contacts == 0 && report->impacts[0].impulse == 0;
}

static bool dd2_recording_validate(const dd2_vehicle *bodies,
                                   const dd2_vehicle_collision_report *report) {
    for (unsigned body = 0; body < DD2_VEHICLE_FLEET_LIMIT; ++body) {
        if (!dd2_vehicle_valid(&bodies[body])) {
            return false;
        }
        for (unsigned corner = 0; corner < DD2_RECORDING_LOWER_CORNERS; ++corner) {
            const dd2_vehicle_vector arm =
                dd2_vehicle_rotate(bodies[body].rotation, dd2_vehicle_body_corner(corner));
            if (bodies[body].position.y + arm.y < -dd2_recording_tolerance) {
                return false;
            }
        }
    }
    unsigned contacts[DD2_VEHICLE_FLEET_LIMIT] = {0};
    unsigned pairs = 0;
    double time = 0;
    for (unsigned index = 0; index < report->count; ++index) {
        const dd2_vehicle_contact contact = report->contacts[index];
        if (contact.time < time || contact.time > 1 || contact.first >= DD2_VEHICLE_FLEET_LIMIT) {
            return false;
        }
        time = contact.time;
        ++contacts[contact.first];
        if (contact.kind == DD2_VEHICLE_CONTACT_PAIR) {
            if (contact.second >= DD2_VEHICLE_FLEET_LIMIT) {
                return false;
            }
            ++contacts[contact.second];
            ++pairs;
        }
    }
    for (unsigned body = 0; body < DD2_VEHICLE_FLEET_LIMIT; ++body) {
        if (contacts[body] != report->impacts[body].contacts) {
            return false;
        }
    }
    if (pairs != report->pair_contacts) {
        return false;
    }
    return true;
}

static bool dd2_recording_vector(dd2_vehicle_vector left, dd2_vehicle_vector right) {
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

static bool dd2_recording_contact(const dd2_vehicle_contact *left,
                                  const dd2_vehicle_contact *right) {
    return dd2_recording_vector(left->point, right->point) &&
           dd2_recording_vector(left->normal, right->normal) &&
           dd2_recording_vector(left->local_points[0], right->local_points[0]) &&
           dd2_recording_vector(left->local_points[1], right->local_points[1]) &&
           left->time == right->time && left->normal_speed == right->normal_speed &&
           left->impulse == right->impulse && left->first == right->first &&
           left->second == right->second && left->obstacle == right->obstacle &&
           left->kind == right->kind;
}

static bool dd2_recording_motion(const dd2_recording_fixture *test, dd2_recording_case sample) {
    dd2_vehicle previous[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_vehicle next[DD2_VEHICLE_FLEET_LIMIT] = {0};
    dd2_recording_propose(previous, next, sample);
    dd2_vehicle_collision_report report = {0};
    if (!dd2_vehicle_collide_fleet_report(next, previous, DD2_VEHICLE_FLEET_LIMIT, test->surface,
                                          NULL, test->published, &report) ||
        report.count <= DD2_VEHICLE_CONTACT_LIMIT || report.count > DD2_VEHICLE_REPORT_LIMIT ||
        report.response_events == 0 || report.response_events > DD2_VEHICLE_EVENT_LIMIT) {
        return false;
    }
    if (sample == DD2_RECORDING_FREE) {
        if (fabs(next[0].position.x - dd2_recording_free_x -
                 (dd2_recording_speed * DD2_VEHICLE_STEP_SECONDS)) > dd2_recording_tolerance ||
            report.impacts[0].contacts != 0) {
            return false;
        }
    } else if (report.pair_contacts == 0 || report.impacts[0].pair_contacts == 0 ||
               next[0].position.x <= previous[DD2_VEHICLE_FLEET_LIMIT - 1].position.x) {
        return false;
    }
    if (!dd2_recording_validate(next, &report)) {
        return false;
    }
    /* Another buffer can fail without modifying an already borrowed view. */
    dd2_vehicle_contact *saved = malloc(report.count * sizeof(*saved));
    if (saved == NULL) {
        return false;
    }
    for (unsigned index = 0; index < report.count; ++index) {
        saved[index] = report.contacts[index];
    }
    dd2_vehicle_collision_report output = report;
    dd2_vehicle invalid = {0};
    bool rejected = !dd2_vehicle_collide_fleet_report(&invalid, &invalid, 1, test->surface, NULL,
                                                      test->working, &output) &&
                    dd2_recording_clear(&output) && invalid.rotation.w == 0 && invalid.steps == 0;
    for (unsigned index = 0; index < report.count; ++index) {
        rejected = dd2_recording_contact(&saved[index], &report.contacts[index]) && rejected;
    }
    free(saved);
    printf("Fleet recording case=%d records=%u events=%u pairs=%u x=%.17g rollback=%d\n",
           (int)sample, report.count, report.response_events, report.pair_contacts,
           next[0].position.x, (int)rejected);
    return rejected;
}

static bool dd2_recording_invalid(const dd2_recording_fixture *test) {
    dd2_vehicle vehicle = {.rotation = {.w = 1}};
    dd2_vehicle_collision_report report = {.count = 1};
    if (dd2_vehicle_collide_fleet_report(&vehicle, &vehicle, 1, NULL, NULL, NULL, &report) ||
        !dd2_recording_clear(&report) ||
        !dd2_vehicle_collide_fleet_report(&vehicle, &vehicle, 1, NULL, NULL, NULL, NULL)) {
        return false;
    }
    report.count = 1;
    return !dd2_vehicle_collide_fleet_report(&vehicle, &vehicle, 0, NULL, NULL, test->working,
                                             &report) &&
           dd2_recording_clear(&report);
}

static bool dd2_recording_consumers(dd2_vehicle_collision_report *report) {
    dd2_vehicle_damage damage[2] = {0};
    dd2_accident_driver scores[2] = {0};
    dd2_accident_observation cars[2] = {0};
    dd2_vehicle listener = {.rotation = {.w = 1}};
    dd2_sound_state sounds = {0};
    dd2_sound_batch batch = {0};
    if (!dd2_accidents_reset(scores, cars, 2)) {
        return false;
    }
    const bool damaged =
        dd2_damage_step(damage, (dd2_damage_frame){.contacts = report, .count = 2});
    const bool attributed = dd2_accidents_step(
        scores, (dd2_accident_frame){.contacts = report, .vehicles = cars, .count = 2});
    const bool audible = dd2_sound_events_step(
        &sounds, &batch,
        (dd2_sound_observation){.listener = &listener, .contacts = report, .count = 2});
    if (report->count > DD2_VEHICLE_REPORT_LIMIT || report->contacts == NULL ||
        report->contacts[report->count - 1].impulse < 0) {
        return !damaged && !attributed && !audible && damage[0].steps == 0 &&
               scores[0].steps == 0 && sounds.ticks == 0 && batch.count == 0;
    }
    return damaged && attributed && audible && dd2_damage_health(&damage[0]) == 1.0 / 2 &&
           scores[0].partner == 1 && scores[0].remaining != 0 && batch.count == 1;
}

static bool dd2_recording_late_consumers(void) {
    dd2_vehicle_contact *contacts = calloc(DD2_VEHICLE_REPORT_LIMIT, sizeof(*contacts));
    if (contacts == NULL) {
        return false;
    }
    for (unsigned index = 0; index < DD2_VEHICLE_REPORT_LIMIT; ++index) {
        contacts[index] = (dd2_vehicle_contact){.kind = DD2_VEHICLE_CONTACT_GROUND,
                                                .second = DD2_VEHICLE_NO_PARTNER,
                                                .normal = {.y = 1}};
    }
    contacts[DD2_VEHICLE_REPORT_LIMIT - 1] = (dd2_vehicle_contact){
        .kind = DD2_VEHICLE_CONTACT_PAIR,
        .second = 1,
        .normal = {.z = -1},
        .normal_speed = dd2_recording_impact_speed,
        .impulse = dd2_recording_impulse,
        .local_points = {{.x = -dd2_vehicle_body_corner(0).x, .z = dd2_vehicle_body_corner(0).z},
                         {.x = dd2_vehicle_body_corner(0).x, .z = -dd2_vehicle_body_corner(0).z}}};
    dd2_vehicle_collision_report report = {.contacts = contacts, .count = DD2_VEHICLE_REPORT_LIMIT};
    bool valid = dd2_recording_consumers(&report);
    contacts[DD2_VEHICLE_REPORT_LIMIT - 1].impulse = -1;
    valid = dd2_recording_consumers(&report) && valid;
    report.count = DD2_VEHICLE_REPORT_LIMIT + 1;
    valid = dd2_recording_consumers(&report) && valid;
    report.contacts = NULL;
    report.count = 1;
    valid = dd2_recording_consumers(&report) && valid;
    free(contacts);
    return valid;
}

int main(void) {
    dd2_recording_fixture test = {0};
    bool valid = dd2_recording_create(&test);
    valid = valid && dd2_recording_motion(&test, DD2_RECORDING_FREE) &&
            dd2_recording_motion(&test, DD2_RECORDING_PAIR) && dd2_recording_invalid(&test) &&
            dd2_recording_late_consumers();
    dd2_recording_destroy(&test);
    dd2_vehicle_collision_storage_destroy(NULL);
    printf("Owned fleet event recording: %s (report bytes=%zu)\n", valid ? "PASS" : "FAIL",
           sizeof(dd2_vehicle_collision_report));
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
