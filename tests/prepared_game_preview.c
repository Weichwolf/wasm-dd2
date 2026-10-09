#include "assets/car_class.h"
#include "assets/track.h"
#include "game/course.h"
#include "game/driving.h"
#include "game/race.h"
#include "physics/vehicle.h"
#include "platform/content.h"
#include "render/camera.h"
#include "render/driving_draw.h"
#include "render/race_draw.h"
#include "render/renderer.h"
#include "render/track_draw.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_PREVIEW_ARGUMENTS = 5,
    DD2_PREVIEW_WIDTH = 640,
    DD2_PREVIEW_HEIGHT = 360,
    DD2_PREVIEW_SAMPLES = 4,
    DD2_PREVIEW_RGBA_CHANNELS = 4,
    DD2_PREVIEW_RGB_CHANNELS = 3,
    DD2_PREVIEW_MIN_PIXELS = 100
};
static const double dd2_preview_drive_seconds = 0.025;
static bool dd2_preview_image(dd2_renderer *renderer, const char *path) {
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    if (pixels == NULL) {
        return false;
    }
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    const char header[] = "P6\n640 360\n255\n";
    bool passed = fwrite(header, 1, sizeof(header) - 1, file) == sizeof(header) - 1;
    size_t visible = 0;
    uint8_t row[DD2_PREVIEW_WIDTH * DD2_PREVIEW_RGB_CHANNELS] = {0};
    for (size_t y_pos = 0; y_pos < DD2_PREVIEW_HEIGHT && passed; ++y_pos) {
        for (size_t x_pos = 0; x_pos < DD2_PREVIEW_WIDTH; ++x_pos) {
            const size_t source = (((DD2_PREVIEW_HEIGHT - y_pos - 1) * DD2_PREVIEW_WIDTH) + x_pos) *
                                  DD2_PREVIEW_RGBA_CHANNELS;
            for (size_t channel = 0; channel < DD2_PREVIEW_RGB_CHANNELS; ++channel) {
                row[(x_pos * DD2_PREVIEW_RGB_CHANNELS) + channel] = pixels[source + channel];
            }
            visible += pixels[source] != 0 || pixels[source + 1] != 0 || pixels[source + 2] != 0;
        }
        passed = fwrite(row, 1, sizeof(row), file) == sizeof(row);
    }
    const bool closed = fclose(file) == 0;
    printf("{\"scope\":\"prepared game presentation\",\"visible_pixels\":%zu}\n", visible);
    return passed && closed && visible >= DD2_PREVIEW_MIN_PIXELS;
}

static bool dd2_prepared_preview_arguments(int argc, char **argv) {
    const char allowed[] = "/tmp/wasm-dd2/";
    const char codes[] = "123456789AB";
    if (argc != DD2_PREVIEW_ARGUMENTS || strncmp(argv[2], allowed, sizeof(allowed) - 1) != 0 ||
        strstr(argv[2], "..") != NULL || strlen(argv[3]) != 1 ||
        strchr(codes, argv[3][0]) == NULL) {
        return false;
    }
    const char *modes[] = {"world",       "car",          "start",       "drive",
                           "race-start",  "race-results", "trial-start", "trial-results",
                           "total-start", "total-results"};
    for (size_t index = 0; index < sizeof(modes) / sizeof(modes[0]); ++index) {
        if (strcmp(argv[4], modes[index]) == 0) {
            return true;
        }
    }
    return false;
}

static bool dd2_prepared_preview_inspect(dd2_track_draw *draw, const dd2_track *track,
                                         dd2_render_options viewport, const char *mode) {
    const bool car = strcmp(mode, "car") == 0;
    if (!car && strcmp(mode, "world") != 0) {
        return true;
    }
    dd2_camera camera = {0};
    return dd2_track_draw_fit(track, &camera, car) &&
           dd2_track_draw_inspect(draw, &camera, car, viewport, DD2_CAR_ROOKIE) &&
           (!car || dd2_car_class_draw(DD2_CAR_ROOKIE, viewport));
}

int main(int argc, char **argv) {
    const char codes[] = "123456789AB";
    if (!dd2_prepared_preview_arguments(argc, argv)) {
        return EXIT_FAILURE;
    }
    const unsigned level = (unsigned)(strchr(codes, argv[3][0]) - codes) + 1;
    dd2_track *track = dd2_track_load(dd2_content_track_provider(argv[1]), level);
    dd2_driving *driving = dd2_driving_create(dd2_track_road(track), level);
    const dd2_render_options viewport = {.width = DD2_PREVIEW_WIDTH,
                                         .height = DD2_PREVIEW_HEIGHT,
                                         .samples = DD2_PREVIEW_SAMPLES,
                                         .output = DD2_RENDER_LINEAR_TO_SRGB};
    dd2_renderer *renderer = dd2_renderer_create(&viewport);
    dd2_track_draw *presentation =
        renderer != NULL ? dd2_track_draw_create(track, dd2_content_image, argv[1]) : NULL;
    bool passed = driving != NULL && renderer != NULL && presentation != NULL;
    if (strncmp(argv[4], "race-", sizeof("race-") - 1) == 0 ||
        strncmp(argv[4], "trial-", sizeof("trial-") - 1) == 0 ||
        strncmp(argv[4], "total-", sizeof("total-") - 1) == 0) {
        dd2_race_mode mode = DD2_RACE_WRECKING;
        if (strncmp(argv[4], "trial-", sizeof("trial-") - 1) == 0) {
            mode = DD2_RACE_TIME_TRIAL;
        } else if (argv[4][0] == 't') {
            mode = DD2_RACE_TOTAL_DESTRUCTION;
        }
        passed = passed && dd2_driving_set_race(driving, true, mode);
        if (strcmp(argv[4], "race-results") == 0 || strcmp(argv[4], "trial-results") == 0 ||
            strcmp(argv[4], "total-results") == 0) {
            passed = passed && dd2_driving_withdraw(driving);
        }
    }
    if (strcmp(argv[4], "drive") == 0) {
        enum { DD2_PREVIEW_DRIVE_FRAMES = 120 };
        for (unsigned frame = 0; frame < DD2_PREVIEW_DRIVE_FRAMES && passed; ++frame) {
            passed = dd2_driving_advance(driving,
                                         (dd2_driving_frame){.seconds = dd2_preview_drive_seconds,
                                                             .control = {.throttle = 1}});
        }
    }
    if (passed) {
        const dd2_vehicle *vehicle = dd2_driving_vehicle(driving);
        const dd2_vehicle_spawn *spawn = dd2_driving_start(driving);
        printf("{\"spawn\":[%.17g,%.17g,%.17g,%.17g]}\n", spawn->position.x, spawn->position.y,
               spawn->position.z, spawn->yaw);
        unsigned contacts = 0;
        for (unsigned wheel = 0; wheel < DD2_VEHICLE_WHEELS; ++wheel) {
            contacts += (unsigned)vehicle->wheels[wheel].grounded;
        }
        printf("{\"position\":[%.17g,%.17g,%.17g],\"velocity\":[%.17g,%.17g,%.17g],"
               "\"rotation\":[%.17g,%.17g,%.17g,%.17g],\"steps\":%llu,\"contacts\":%u,"
               "\"collisions\":%llu}\n",
               vehicle->position.x, vehicle->position.y, vehicle->position.z, vehicle->velocity.x,
               vehicle->velocity.y, vehicle->velocity.z, vehicle->rotation.x, vehicle->rotation.y,
               vehicle->rotation.z, vehicle->rotation.w, (unsigned long long)vehicle->steps,
               contacts, (unsigned long long)dd2_driving_collisions(driving));
        passed = dd2_track_draw_driving(
            presentation,
            (dd2_driving_view){.vehicle = vehicle,
                               .damage = dd2_driving_damage(driving),
                               .score = dd2_driving_accidents(driving),
                               .race = dd2_driving_race(driving),
                               .lap = dd2_driving_laps(driving),
                               .required_laps = dd2_course_laps(dd2_driving_course(driving)),
                               .wheel_roll = dd2_driving_wheel_roll(driving),
                               .opponents = dd2_driving_vehicles(driving) + 1,
                               .opponent_damage = dd2_driving_damage(driving) + 1,
                               .opponent_rolls = dd2_driving_wheel_rolls(driving) + 1,
                               .opponent_count = dd2_driving_vehicle_count(driving) - 1,
                               .viewport = viewport});
        passed = passed && dd2_prepared_preview_inspect(presentation, track, viewport, argv[4]) &&
                 dd2_preview_image(renderer, argv[2]);
        const dd2_track_draw_stats stats = dd2_track_draw_statistics(presentation);
        printf("{\"world_triangles\":%zu,\"world_visible\":%zu,\"world_culled\":%zu,\"vehicle_"
               "triangles\":%zu,\"vehicle_models\":%zu,\"culled_models\":%zu,\"body_lods\":[%zu,%"
               "zu,%zu],\"textures\":%zu}\n",
               stats.world.triangles, stats.world.visible, stats.world.culled,
               stats.vehicle_triangles, stats.vehicle_models, stats.culled_models,
               stats.body_lods[0], stats.body_lods[1], stats.body_lods[2], stats.uploaded_textures);
    }
    dd2_track_draw_destroy(presentation);
    dd2_renderer_destroy(renderer);
    dd2_driving_destroy(driving);
    dd2_track_destroy(track);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
