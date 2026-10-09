#include "archive_fixture.h"
#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/car.h"
#include "assets/level.h"
#include "assets/mesh.h"
#include "assets/track.h"
#include "physics/damage.h"
#include "render/camera.h"
#include "render/mesh_draw.h"
#include "render/renderer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_EXPORT_WIDTH = 320,
    DD2_EXPORT_HEIGHT = 240,
    DD2_EXPORT_ARGUMENTS = 4,
    DD2_EXPORT_TRIANGLE_OPCODE = 25,
    DD2_EXPORT_QUAD_OPCODE = 29,
    DD2_EXPORT_OPCODE_MASK = 253,
    DD2_EXPORT_PAGE_MASK = 31
};
static const float dd2_export_yaw = 45.0F;
static const float dd2_export_pitch = 20.0F;
static bool dd2_export_word(FILE *file, unsigned value) {
    const uint8_t bytes[] = {(uint8_t)value, (uint8_t)(value >> 8), (uint8_t)(value >> 16),
                             (uint8_t)(value >> 24)};
    return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
}
static bool dd2_export_sprite(FILE *file, dd2_car_sprite sprite) {
    return dd2_export_word(file, sprite.page) && dd2_export_word(file, sprite.palette_bank) &&
           dd2_export_word(file, sprite.u) && dd2_export_word(file, sprite.v);
}
static bool dd2_export_faces(FILE *file, const dd2_track *track, const dd2_car_livery *livery) {
    unsigned ordinals[DD2_MESH_OPCODE_COUNT] = {0};
    const dd2_mesh *mesh = dd2_track_car(track);
    const dd2_mesh_face *faces = dd2_mesh_faces(mesh);
    for (size_t index = 0; index < dd2_mesh_face_count(mesh); ++index) {
        const dd2_mesh_face *face = &faces[index];
        const unsigned ordinal = ordinals[face->opcode]++;
        const unsigned opcode = face->opcode & DD2_EXPORT_OPCODE_MASK;
        if (opcode != DD2_EXPORT_TRIANGLE_OPCODE && opcode != DD2_EXPORT_QUAD_OPCODE) {
            continue;
        }
        dd2_texture_definition definition = {0};
        dd2_car_surface surface = {0};
        if (!dd2_level_texture_definition(dd2_track_level(track), face->texture, &definition) ||
            !dd2_car_livery_surface(livery, face, ordinal, &definition, &surface) ||
            !dd2_export_word(file, surface.palette_bank) ||
            !dd2_export_word(file, surface.definition.page_flags & DD2_EXPORT_PAGE_MASK)) {
            return false;
        }
        for (unsigned corner = 0; corner < DD2_TEXTURE_CORNERS; ++corner) {
            if (!dd2_export_word(file, surface.definition.corners[corner].u) ||
                !dd2_export_word(file, surface.definition.corners[corner].v)) {
                return false;
            }
        }
    }
    return true;
}
static bool dd2_export_materials(FILE *file, const dd2_track *track) {
    for (unsigned car_class = 0; car_class < DD2_CAR_CLASSES; ++car_class) {
        for (unsigned driver = 0; driver < DD2_CAR_DRIVERS; ++driver) {
            const dd2_car_livery *livery =
                dd2_track_car_livery(track, driver, (dd2_car_class)car_class);
            if (livery == NULL || !dd2_export_word(file, car_class) ||
                !dd2_export_word(file, driver)) {
                return false;
            }
            for (unsigned part = 0; part < DD2_CAR_PARTS; ++part) {
                if (!dd2_export_word(file, livery->banks[part])) {
                    return false;
                }
            }
            if (!dd2_export_sprite(file, livery->number) ||
                !dd2_export_sprite(file, livery->template_number) ||
                !dd2_export_faces(file, track, livery)) {
                return false;
            }
        }
    }
    return true;
}
static bool dd2_export_draw(FILE *file, dd2_renderer *renderer, dd2_mesh_materials *materials,
                            const dd2_track *track, const dd2_car_livery *livery,
                            const dd2_vehicle_damage *damage) {
    dd2_camera camera = {0};
    if (!dd2_camera_fit_mesh(&camera, dd2_track_car(track))) {
        return false;
    }
    camera.yaw = dd2_export_yaw;
    camera.pitch = dd2_export_pitch;
    dd2_camera_apply(&camera,
                     (dd2_render_options){.width = DD2_EXPORT_WIDTH, .height = DD2_EXPORT_HEIGHT});
    if (!dd2_mesh_draw_car(materials, dd2_track_car(track), (dd2_track_vertex){0}, damage,
                           livery)) {
        return false;
    }
    const uint8_t *pixels = dd2_renderer_pixels(renderer);
    const size_t size = dd2_renderer_rgba_bytes(renderer);
    return pixels != NULL && fwrite(pixels, 1, size, file) == size;
}
static bool dd2_export_images(FILE *file, const dd2_track *track) {
    dd2_renderer *renderer = dd2_renderer_create(
        &(dd2_render_options){.width = DD2_EXPORT_WIDTH, .height = DD2_EXPORT_HEIGHT});
    if (renderer == NULL) {
        return false;
    }
    dd2_mesh_materials *materials =
        dd2_mesh_materials_create(dd2_track_level(track), dd2_track_textures(track));
    bool passed = materials != NULL;
    for (unsigned index = 0; index < DD2_CAR_LIVERIES && passed; ++index) {
        const unsigned driver = index < DD2_CAR_DRIVERS ? index : 0;
        const dd2_car_class car_class =
            index < DD2_CAR_DRIVERS ? DD2_CAR_ROOKIE : (dd2_car_class)(index - DD2_CAR_DRIVERS + 1);
        passed = dd2_export_draw(file, renderer, materials, track,
                                 dd2_track_car_livery(track, driver, car_class), NULL);
    }
    const dd2_car_livery *human = dd2_track_car_livery(track, 0, DD2_CAR_ROOKIE);
    const dd2_vehicle_damage damaged = {.regions = {0.8, 0.6, 0.5, 0.4, 0.6, 0.2}};
    passed = passed && dd2_export_draw(file, renderer, materials, track, human, &damaged) &&
             dd2_export_draw(file, renderer, materials, track, human, NULL);
    dd2_mesh_materials_destroy(materials);
    dd2_renderer_destroy(renderer);
    return passed;
}
static bool dd2_export_rejections(const dd2_track *track) {
    dd2_level_data truncated = *dd2_track_level(track);
    --truncated.sections[DD2_LEVEL_SPRITES].size;
    dd2_car_livery output = {.repaint = true};
    return !dd2_car_livery_decode(&truncated, dd2_track_textures(track), 0, DD2_CAR_ROOKIE,
                                  &output) &&
           !output.repaint &&
           dd2_track_car_livery(track, DD2_CAR_DRIVERS, DD2_CAR_ROOKIE) == NULL &&
           dd2_track_car_livery(track, 0, DD2_CAR_CLASSES) == NULL;
}
static bool dd2_export_archive(const dd2_archive *archive, FILE *metadata, FILE *images) {
    for (unsigned number = 1; number <= DD2_TRACK_COUNT; ++number) {
        dd2_track *track = dd2_track_create(archive, number);
        const bool passed = track != NULL && dd2_export_rejections(track) &&
                            dd2_export_materials(metadata, track) &&
                            dd2_export_images(images, track);
        dd2_track_destroy(track);
        if (!passed) {
            return false;
        }
    }
    return true;
}
static bool dd2_export_path(const char *path) {
    const char allowed[] = "/tmp/wasm-dd2/";
    return strncmp(path, allowed, sizeof(allowed) - 1) == 0 && strstr(path, "..") == NULL;
}
int main(int argc, char **argv) {
    if (argc != DD2_EXPORT_ARGUMENTS || !dd2_export_path(argv[2]) || !dd2_export_path(argv[3])) {
        return EXIT_FAILURE;
    }
    dd2_archive_fixture fixture = {0};
    if (!dd2_archive_fixture_open(argv[1], &fixture)) {
        return EXIT_FAILURE;
    }
    FILE *metadata = fopen(argv[2], "wb");
    FILE *images = fopen(argv[3], "wb");
    bool passed =
        metadata != NULL && images != NULL && dd2_export_archive(fixture.archive, metadata, images);
    if (metadata != NULL) {
        passed = fclose(metadata) == 0 && passed;
    }
    if (images != NULL) {
        passed = fclose(images) == 0 && passed;
    }
    dd2_archive_fixture_close(&fixture);
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
