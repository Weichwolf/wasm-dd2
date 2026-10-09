#include "archive_fixture.h"
#include "assets/car.h"
#include "assets/car_class.h"
#include "assets/model.h"
#include "assets/track.h"
#include "assets/world.h"
#include "game/championship.h"
#include "game/championship_session.h"
#include "game/driving.h"
#include "game/league.h"
#include "game/race.h"
#include "physics/damage.h"
#include "physics/vehicle.h"
#include "platform/content.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_TRACK_EXPORT_ARGUMENTS = 4, DD2_TRACK_EXPORT_STEPS = 120 };
static const double dd2_track_export_seconds = 0.005;
static const double dd2_track_export_steer = 0.15;

typedef struct {
    dd2_track_provider provider;
    bool fail;
    bool wrong_number;
} dd2_track_export_provider;

static dd2_track *dd2_track_export_load(const void *user, unsigned number) {
    const dd2_track_export_provider *source = user;
    return source->fail ? NULL
                        : dd2_track_load(source->provider, number + (unsigned)source->wrong_number);
}

static bool dd2_track_export_championship(dd2_track_provider provider, dd2_race_mode mode) {
    dd2_track_export_provider source = {.provider = provider};
    dd2_championship_session *session = dd2_championship_session_create_provider(
        (dd2_track_provider){.load = dd2_track_export_load, .user = &source}, mode,
        DD2_CAR_AMATEUR);
    if (session == NULL) {
        return false;
    }
    const dd2_track *old_track = dd2_championship_session_track(session);
    const dd2_driving *old_driving = dd2_championship_session_driving(session);
    const dd2_championship before = *dd2_championship_session_state(session);
    bool valid = dd2_driving_vehicle_count(old_driving) == DD2_CAR_DRIVERS &&
                 dd2_driving_class(old_driving, 0) == DD2_CAR_AMATEUR &&
                 dd2_track_number(old_track) == dd2_championship_session_level(session);
    for (unsigned fault = 0; fault < 2 && valid; ++fault) {
        source.fail = fault == 0;
        source.wrong_number = fault == 1;
        dd2_championship_transition *failed = dd2_championship_session_prepare_restart(session);
        valid = failed == NULL && dd2_championship_session_track(session) == old_track &&
                dd2_championship_session_driving(session) == old_driving &&
                dd2_championship_session_state(session)->ticket == before.ticket &&
                dd2_championship_session_state(session)->phase == before.phase &&
                dd2_championship_session_state(session)->history_count == before.history_count &&
                dd2_championship_valid(dd2_championship_session_state(session));
        for (unsigned driver = 0; driver < DD2_LEAGUE_DRIVERS && valid; ++driver) {
            const dd2_league_driver current =
                dd2_championship_session_state(session)->league.drivers[driver];
            const dd2_league_driver original = before.league.drivers[driver];
            valid = current.points == original.points && current.division == original.division &&
                    current.rank == original.rank;
        }
        dd2_championship_transition_destroy(failed);
    }
    source.fail = false;
    source.wrong_number = false;
    dd2_championship_transition *candidate = dd2_championship_session_prepare_restart(session);
    dd2_championship_transition *stale = dd2_championship_session_prepare_restart(session);
    if (candidate == NULL || stale == NULL) {
        valid = false;
    } else {
        valid = valid && dd2_championship_session_track(session) == old_track &&
                dd2_championship_transition_track(candidate) != old_track &&
                dd2_championship_session_commit(session, candidate);
        if (valid) {
            candidate = NULL;
            valid =
                !dd2_championship_transition_current(session, stale) &&
                !dd2_championship_session_commit(session, stale) &&
                dd2_driving_class(dd2_championship_session_driving(session), 0) == DD2_CAR_AMATEUR;
        }
    }
    dd2_championship_transition_destroy(candidate);
    dd2_championship_transition_destroy(stale);
    valid = valid &&
            dd2_championship_session_advance(
                session, (dd2_driving_frame){.seconds = dd2_track_export_seconds}) &&
            dd2_championship_session_abort(session);
    dd2_championship_session_destroy(session);
    return valid;
}

static void dd2_track_export_snapshot(const dd2_driving *driving) {
    const unsigned count = dd2_driving_vehicle_count(driving);
    const dd2_vehicle *vehicles = dd2_driving_vehicles(driving);
    const dd2_vehicle_damage *damage = dd2_driving_damage(driving);
    printf("{\"pair_collisions\":%llu,\"vehicles\":[",
           (unsigned long long)dd2_driving_pair_collisions(driving));
    for (unsigned index = 0; index < count; ++index) {
        const dd2_vehicle *vehicle = &vehicles[index];
        printf("%s{\"steps\":%llu,\"body\":[%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,"
               "%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g],\"wheels\":[",
               index == 0 ? "" : ",", (unsigned long long)vehicle->steps, vehicle->position.x,
               vehicle->position.y, vehicle->position.z, vehicle->velocity.x, vehicle->velocity.y,
               vehicle->velocity.z, vehicle->rotation.x, vehicle->rotation.y, vehicle->rotation.z,
               vehicle->rotation.w, vehicle->angular_velocity.x, vehicle->angular_velocity.y,
               vehicle->angular_velocity.z, vehicle->steering);
        for (unsigned wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
            const dd2_vehicle_wheel *state = &vehicle->wheels[wheel];
            printf("%s[%.17g,%.17g,%.17g,%.17g,%.17g,%u,%u,%u]", wheel == 0 ? "" : ",",
                   state->point.x, state->point.y, state->point.z, state->compression, state->load,
                   (unsigned)state->grounded, state->contact.cell, state->contact.triangle);
        }
        printf("],\"engine_health\":%.17g}", dd2_damage_health(&damage[index]));
    }
    printf("]}");
}

int main(int argc, char **argv) {
    const char codes[] = "123456789AB";
    if (argc != DD2_TRACK_EXPORT_ARGUMENTS || strlen(argv[3]) != 1 ||
        strchr(codes, argv[3][0]) == NULL ||
        (strcmp(argv[1], "prepared") != 0 && strcmp(argv[1], "reference") != 0)) {
        return EXIT_FAILURE;
    }
    const bool prepared = strcmp(argv[1], "prepared") == 0;
    dd2_archive_fixture fixture = {0};
    if (!prepared && !dd2_archive_fixture_open(argv[2], &fixture)) {
        return EXIT_FAILURE;
    }
    const dd2_track_provider provider = prepared ? dd2_content_track_provider(argv[2])
                                                 : dd2_track_reference_provider(fixture.archive);
    const unsigned number = (unsigned)(strchr(codes, argv[3][0]) - codes) + 1;
    dd2_track *track = dd2_track_load(provider, number);
    dd2_driving *driving =
        dd2_driving_create_class(dd2_track_road(track), number, DD2_CAR_AMATEUR, NULL);
    bool valid = track != NULL && driving != NULL;
    if (valid && number == 1) {
        valid = dd2_track_export_championship(provider, DD2_RACE_WRECKING) &&
                dd2_track_export_championship(provider, DD2_RACE_STOCKCAR);
    }
    if (!valid) {
        dd2_driving_destroy(driving);
        dd2_track_destroy(track);
        dd2_archive_fixture_close(&fixture);
        return EXIT_FAILURE;
    }
    printf("{\"level\":%u,\"resources\":%zu,\"instances\":%zu,\"models\":[", number,
           dd2_world_resource_count(dd2_track_world(track)),
           dd2_world_instance_count(dd2_track_world(track)));
    for (unsigned kind = 0; kind < DD2_TRACK_MODEL_COUNT; ++kind) {
        printf("%s%zu", kind == 0 ? "" : ",",
               dd2_model_index_count(dd2_track_prepared_model(track, (dd2_track_model_kind)kind)) /
                   3);
    }
    printf("],\"snapshots\":[");
    dd2_track_export_snapshot(driving);
    for (unsigned step = 0; step < DD2_TRACK_EXPORT_STEPS && valid; ++step) {
        valid = dd2_driving_advance(
            driving,
            (dd2_driving_frame){.seconds = dd2_track_export_seconds,
                                .control = {.throttle = 1, .steer = dd2_track_export_steer}});
    }
    printf(",");
    dd2_track_export_snapshot(driving);
    printf("],\"valid\":%s}\n", valid ? "true" : "false");
    dd2_driving_destroy(driving);
    dd2_track_destroy(track);
    dd2_archive_fixture_close(&fixture);
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
