#include "archive_fixture.h"
#include "assets/track.h"
#include "game/driving.h"
#include "physics/vehicle.h"
#include "render/driving_draw.h"
#include "render/mesh_draw.h"
#include "render/renderer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_PREVIEW_ARGUMENTS = 5,
    DD2_PREVIEW_WIDTH = 640,
    DD2_PREVIEW_HEIGHT = 480,
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
    const char header[] = "P6\n640 480\n255\n";
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
    printf("{\"scope\":\"free-driving rewrite preview\",\"visible_pixels\":%zu}\n", visible);
    return passed && closed && visible >= DD2_PREVIEW_MIN_PIXELS;
}

int main(int argc, char **argv) {
    const char allowed[] = "/tmp/wasm-dd2/";
    const char codes[] = "123456789AB";
    if (argc != DD2_PREVIEW_ARGUMENTS || strncmp(argv[2], allowed, sizeof(allowed) - 1) != 0 ||
        strstr(argv[2], "..") != NULL || strlen(argv[3]) != 1 ||
        strchr(codes, argv[3][0]) == NULL ||
        (strcmp(argv[4], "start") != 0 && strcmp(argv[4], "drive") != 0)) {
        return EXIT_FAILURE;
    }
    dd2_archive_fixture fixture = {0};
    if (!dd2_archive_fixture_open(argv[1], &fixture)) {
        return EXIT_FAILURE;
    }
    const unsigned level = (unsigned)(strchr(codes, argv[3][0]) - codes) + 1;
    dd2_track *track = dd2_track_create(fixture.archive, level);
    dd2_driving *driving = dd2_driving_create(dd2_track_road(track), level);
    dd2_renderer *renderer = dd2_renderer_create(
        &(dd2_render_options){.width = DD2_PREVIEW_WIDTH, .height = DD2_PREVIEW_HEIGHT});
    dd2_mesh_materials *materials =
        dd2_mesh_materials_create(dd2_track_level(track), dd2_track_textures(track));
    bool passed = driving != NULL && renderer != NULL && materials != NULL;
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
        passed = dd2_driving_draw(materials, track,
                                  (dd2_driving_view){.vehicle = vehicle,
                                                     .wheel_roll = dd2_driving_wheel_roll(driving),
                                                     .viewport = {.width = DD2_PREVIEW_WIDTH,
                                                                  .height = DD2_PREVIEW_HEIGHT}}) &&
                 dd2_preview_image(renderer, argv[2]);
    }
    dd2_mesh_materials_destroy(materials);
    dd2_renderer_destroy(renderer);
    dd2_driving_destroy(driving);
    dd2_track_destroy(track);
    dd2_archive_fixture_close(&fixture);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
