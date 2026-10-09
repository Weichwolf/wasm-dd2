#include "assets/archive.h"
#include "assets/car_class.h"
#include "assets/track.h"
#include "game/championship_session.h"
#include "game/driving.h"
#include "game/race.h"
#include "physics/damage.h"
#include "physics/vehicle.h"
#include "physics/vehicle_collision.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

enum { DD2_CLASS_EXPORT_ACTIVE = 500, DD2_CLASS_EXPORT_RACING_LEVELS = 7 };

static bool dd2_class_export_identity(const dd2_driving *driving, dd2_car_class car_class) {
    if (dd2_driving_class(driving, 0) != car_class ||
        dd2_driving_class(NULL, 0) != DD2_CAR_CLASSES ||
        dd2_driving_class(driving, DD2_VEHICLE_FLEET_LIMIT) != DD2_CAR_CLASSES) {
        return false;
    }
    for (unsigned driver = 1; driver < DD2_VEHICLE_FLEET_LIMIT; ++driver) {
        if (dd2_driving_class(driving, driver) != DD2_CAR_PRO) {
            return false;
        }
    }
    return true;
}

static bool dd2_class_export_prefix(dd2_driving *driving, dd2_car_class car_class, unsigned level,
                                    unsigned mode) {
    const dd2_vehicle original = *dd2_driving_vehicle(driving);
    for (unsigned step = 0; step < DD2_CLASS_EXPORT_ACTIVE; ++step) {
        if (!dd2_driving_advance(driving, (dd2_driving_frame){.seconds = DD2_VEHICLE_STEP_SECONDS,
                                                              .control = {.throttle = 1}})) {
            return false;
        }
        for (unsigned driver = 0; driver < dd2_driving_vehicle_count(driving); ++driver) {
            if (!dd2_vehicle_valid(&dd2_driving_vehicles(driving)[driver])) {
                return false;
            }
        }
    }
    const dd2_vehicle *vehicle = dd2_driving_vehicle(driving);
    if (printf("{\"level\":%u,\"class\":%u,\"mode\":%u,\"steps\":%llu,"
               "\"position\":[%.17g,%.17g,%.17g],\"engine\":%.17g}\n",
               level, (unsigned)car_class, mode, (unsigned long long)vehicle->steps,
               vehicle->position.x, vehicle->position.y, vehicle->position.z,
               dd2_damage_health(dd2_driving_damage(driving))) < 0 ||
        !dd2_class_export_identity(driving, car_class) || !dd2_driving_reset(driving)) {
        return false;
    }
    vehicle = dd2_driving_vehicle(driving);
    return dd2_class_export_identity(driving, car_class) && vehicle->steps == original.steps &&
           vehicle->position.x == original.position.x &&
           vehicle->position.y == original.position.y &&
           vehicle->position.z == original.position.z &&
           dd2_damage_health(dd2_driving_damage(driving)) == 1;
}

static bool dd2_class_export_level(const dd2_archive *archive, unsigned level) {
    dd2_track *track = dd2_track_create(archive, level);
    if (track == NULL) {
        return false;
    }
    bool valid = true;
    for (unsigned index = 0; valid && index < DD2_CAR_CLASSES; ++index) {
        const dd2_car_class car_class = (dd2_car_class)index;
        dd2_driving *driving =
            dd2_driving_create_class(dd2_track_road(track), level, car_class, NULL);
        valid = driving != NULL && dd2_class_export_identity(driving, car_class) &&
                dd2_class_export_prefix(driving, car_class, level, 0);
        for (unsigned mode = 0; valid && mode <= DD2_RACE_TOTAL_DESTRUCTION; ++mode) {
            const bool supported =
                mode == DD2_RACE_WRECKING ||
                ((level <= DD2_CLASS_EXPORT_RACING_LEVELS) != (mode == DD2_RACE_TOTAL_DESTRUCTION));
            if (supported) {
                valid = dd2_driving_set_race(driving, true, (dd2_race_mode)mode) &&
                        dd2_class_export_prefix(driving, car_class, level, mode + 1);
            }
        }
        dd2_driving_destroy(driving);
    }
    dd2_track_destroy(track);
    return valid;
}

static bool dd2_class_export_championship(const dd2_archive *archive) {
    for (unsigned index = 0; index < DD2_CAR_CLASSES; ++index) {
        for (unsigned mode = 0; mode <= DD2_RACE_STOCKCAR; ++mode) {
            const dd2_car_class car_class = (dd2_car_class)index;
            dd2_championship_session *session =
                dd2_championship_session_create_class(archive, (dd2_race_mode)mode, car_class);
            bool valid =
                session != NULL &&
                dd2_class_export_identity(dd2_championship_session_driving(session), car_class);
            dd2_championship_transition *restart =
                valid ? dd2_championship_session_prepare_restart(session) : NULL;
            valid = valid && restart != NULL &&
                    dd2_class_export_identity(dd2_championship_transition_driving(restart),
                                              car_class) &&
                    dd2_championship_session_commit(session, restart);
            if (!valid) {
                dd2_championship_transition_destroy(restart);
            }
            dd2_championship_session_destroy(session);
            if (!valid) {
                return false;
            }
        }
    }
    return true;
}

int main(int argc, char **argv) {
    dd2_file file = {0};
    if (argc != 2 || !dd2_file_read(argv[1], &file)) {
        return EXIT_FAILURE;
    }
    dd2_archive *archive = NULL;
    bool valid = dd2_archive_open(file.data, file.size, &archive) == DD2_ARCHIVE_OK &&
                 dd2_class_export_championship(archive);
    for (unsigned level = 1; valid && level <= DD2_TRACK_COUNT; ++level) {
        valid = dd2_class_export_level(archive, level);
    }
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return valid ? EXIT_SUCCESS : EXIT_FAILURE;
}
