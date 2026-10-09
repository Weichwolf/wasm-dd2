#include "assets/track.h"
#include "assets/world.h"
#include "game/application.h"
#include "game/championship.h"
#include "game/driving.h"
#include "game/race.h"
#include "physics/vehicle.h"
#include "render/renderer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_PREPARED_APP_WIDTH = 640,
    DD2_PREPARED_APP_HEIGHT = 360,
    DD2_PREPARED_APP_SAMPLES = 4,
    DD2_PREPARED_APP_PATH_BYTES = 4096,
    DD2_PREPARED_APP_FRAMES = 12,
    DD2_PREPARED_APP_FLEET = 20
};
static const double dd2_prepared_app_seconds = 0.025;

static bool dd2_prepared_app_image(void) {
    const dd2_application_image image = dd2_application_image_view();
    return image.pixels != NULL && image.viewport.width == DD2_PREPARED_APP_WIDTH &&
           image.viewport.height == DD2_PREPARED_APP_HEIGHT &&
           image.viewport.samples == DD2_PREPARED_APP_SAMPLES &&
           image.viewport.output == DD2_RENDER_LINEAR_TO_SRGB;
}

static bool dd2_prepared_app_track(int level) {
    if (!dd2_application_select_level(level) || !dd2_application_set_driving(0) ||
        !dd2_application_show_car(0) || !dd2_application_present() || !dd2_prepared_app_image()) {
        return false;
    }
    const dd2_track *track = dd2_application_track_view();
    const dd2_world *world = dd2_track_world(track);
    if (world == NULL || dd2_track_level(track) != NULL || dd2_track_scene(track) != NULL ||
        dd2_track_car(track) != NULL || dd2_track_textures(track) != NULL ||
        !dd2_application_show_car(1) || !dd2_application_present() ||
        !dd2_application_show_car(0) || !dd2_application_set_driving(1) ||
        dd2_application_vehicle_count() != DD2_PREPARED_APP_FLEET || !dd2_application_present()) {
        return false;
    }
    for (unsigned frame = 0; frame < DD2_PREPARED_APP_FRAMES; ++frame) {
        if (!dd2_application_advance((dd2_driving_frame){.seconds = dd2_prepared_app_seconds,
                                                         .control = {.throttle = 1}})) {
            return false;
        }
    }
    const dd2_vehicle *vehicle = dd2_driving_vehicle(dd2_application_driving_view());
    const uint64_t steps = vehicle->steps;
    if (!dd2_application_present() || !dd2_application_set_paused(1) ||
        !dd2_application_advance((dd2_driving_frame){.seconds = dd2_prepared_app_seconds}) ||
        dd2_driving_vehicle(dd2_application_driving_view())->steps != steps ||
        !dd2_application_set_driving(0)) {
        return false;
    }
    return true;
}

static bool dd2_prepared_app_championship(int mode) {
    if (!dd2_application_start_championship(mode) ||
        dd2_application_championship_phase() != DD2_CHAMPIONSHIP_RACING ||
        dd2_application_current_level() != 1 || !dd2_application_present() ||
        !dd2_application_advance((dd2_driving_frame){.seconds = dd2_prepared_app_seconds}) ||
        !dd2_application_restart_championship() || dd2_application_race_steps() != 0 ||
        !dd2_application_present() || !dd2_application_exit_championship()) {
        return false;
    }
    return dd2_application_current_level() == DD2_TRACK_COUNT &&
           dd2_application_championship_phase() == -1 && dd2_application_present();
}

int main(int argc, char **argv) {
    if (argc != 2 || strlen(argv[1]) >= DD2_PREPARED_APP_PATH_BYTES) {
        return EXIT_FAILURE;
    }
    char root[DD2_PREPARED_APP_PATH_BYTES] = {0};
    for (size_t index = 0; argv[1][index] != '\0'; ++index) {
        root[index] = argv[1][index];
    }
    bool passed = !dd2_application_open_prepared(NULL, 1) &&
                  !dd2_application_open_prepared("", 1) &&
                  !dd2_application_open_prepared("/missing-dd2-content", 1) &&
                  dd2_application_current_level() == 0 && dd2_application_open_prepared(root, 1) &&
                  !dd2_application_open_prepared(root, 2);
    root[0] = '\0';
    for (int level = 1; level <= DD2_TRACK_COUNT && passed; ++level) {
        passed = dd2_prepared_app_track(level);
    }
    passed = passed && !dd2_application_select_level(0) &&
             dd2_application_current_level() == DD2_TRACK_COUNT &&
             dd2_prepared_app_championship(DD2_RACE_WRECKING) &&
             dd2_prepared_app_championship(DD2_RACE_STOCKCAR) && dd2_application_close() &&
             dd2_application_current_level() == 0 && dd2_application_image_view().pixels == NULL;
    const bool closed = dd2_application_close() != 0;
    puts(passed && closed ? "Prepared application lifecycle passed."
                          : "Prepared application lifecycle failed.");
    return passed && closed ? EXIT_SUCCESS : EXIT_FAILURE;
}
