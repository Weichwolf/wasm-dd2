#include "assets/track.h"

#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/car.h"
#include "assets/car_class.h"
#include "assets/level.h"
#include "assets/mesh.h"
#include "assets/model.h"
#include "assets/road.h"
#include "assets/scene.h"
#include "assets/textures.h"
#include "assets/world.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_TRACK_RACING_COUNT = 7 };

struct dd2_track {
    unsigned number;
    dd2_world *world;
    dd2_level_data level;
    dd2_texture_set *textures;
    dd2_scene *scene;
    dd2_mesh *car;
    dd2_car_livery liveries[DD2_CAR_LIVERIES];
    dd2_mesh *wheels[2];
    dd2_road *road;
};

static dd2_track *dd2_track_reference_load(const void *user, unsigned number) {
    return dd2_track_create(user, number);
}

dd2_track_provider dd2_track_reference_provider(const dd2_archive *archive) {
    return (dd2_track_provider){.load = archive == NULL ? NULL : dd2_track_reference_load,
                                .user = archive};
}

dd2_track *dd2_track_load(dd2_track_provider provider, unsigned number) {
    if (provider.load == NULL || number == 0 || number > DD2_TRACK_COUNT) {
        return NULL;
    }
    dd2_track *track = provider.load(provider.user, number);
    if (track != NULL && track->number != number) {
        dd2_track_destroy(track);
        return NULL;
    }
    return track;
}

unsigned dd2_track_number(const dd2_track *track) {
    return track == NULL ? 0 : track->number;
}

const dd2_world *dd2_track_world(const dd2_track *track) {
    return track == NULL ? NULL : track->world;
}

const dd2_model *dd2_track_prepared_model(const dd2_track *track, dd2_track_model_kind kind) {
    static const char *const names[DD2_TRACK_MODEL_COUNT] = {
        "car-close", "car-medium", "car-distant", "wheel-primary", "wheel-secondary", "sky"};
    if (track == NULL || track->world == NULL || (unsigned)kind >= DD2_TRACK_MODEL_COUNT) {
        return NULL;
    }
    const dd2_world_template *templates = dd2_world_templates(track->world);
    const dd2_world_resource *resources = dd2_world_resources(track->world);
    for (size_t index = 0; index < dd2_world_template_count(track->world); ++index) {
        if (strcmp(templates[index].name, names[kind]) == 0) {
            return resources[templates[index].resource].model;
        }
    }
    return NULL;
}

dd2_track *dd2_track_create_prepared(unsigned number, dd2_track_prepared_source source,
                                     dd2_world_model_loader loader, void *user) {
    if (number == 0 || number > DD2_TRACK_COUNT) {
        return NULL;
    }
    dd2_track *track = calloc(1, sizeof(*track));
    if (track == NULL) {
        return NULL;
    }
    track->number = number;
    track->road = dd2_road_create_prepared(source.road);
    if (track->road == NULL ||
        (dd2_road_strip_count(track->road) != 0) != (number <= DD2_TRACK_RACING_COUNT)) {
        dd2_track_destroy(track);
        return NULL;
    }
    track->world = dd2_world_create(source.scene, loader, user);
    for (unsigned kind = 0; kind < DD2_TRACK_MODEL_COUNT; ++kind) {
        if (dd2_track_prepared_model(track, (dd2_track_model_kind)kind) == NULL) {
            dd2_track_destroy(track);
            return NULL;
        }
    }
    return track;
}

static dd2_byte_view dd2_track_file(const dd2_archive *archive, const char *name) {
    dd2_asset asset = {0};
    if (!dd2_archive_find(archive, name, &asset)) {
        return (dd2_byte_view){0};
    }
    return (dd2_byte_view){.data = asset.bytes, .size = asset.size};
}

static dd2_texture_sources dd2_track_sources(const dd2_archive *archive, char code) {
    char palette[] = "LEV0\\LEVEL.PAL";
    char cluts[] = "LEV0\\LEVEL.CLT";
    char extra[] = "LEV0\\LEVEL.ECL";
    char part[] = "LEV0\\LEVEL.TX0";
    palette[3] = code;
    cluts[3] = code;
    extra[3] = code;
    part[3] = code;
    dd2_texture_sources sources = {.palette = dd2_track_file(archive, palette),
                                   .cluts = dd2_track_file(archive, cluts),
                                   .extra_cluts = dd2_track_file(archive, extra)};
    for (size_t index = 0; index < DD2_TEXTURE_MAX_PARTS; ++index) {
        part[sizeof(part) - 2] = (char)('0' + index);
        const dd2_byte_view bytes = dd2_track_file(archive, part);
        if (bytes.data == NULL) {
            break;
        }
        sources.parts[sources.part_count++] = bytes;
    }
    return sources;
}

dd2_track *dd2_track_create(const dd2_archive *archive, unsigned number) {
    const char codes[] = "0123456789AB";
    if (archive == NULL || number == 0 || number > DD2_TRACK_COUNT) {
        return NULL;
    }
    dd2_track *track = calloc(1, sizeof(*track));
    if (track == NULL) {
        return NULL;
    }
    track->number = number;
    char name[] = "LEV0\\LEVEL.DAT";
    name[3] = codes[number];
    const dd2_texture_sources sources = dd2_track_sources(archive, codes[number]);
    track->textures = dd2_texture_set_create(&sources);
    if (track->textures == NULL ||
        !dd2_level_decode(dd2_track_file(archive, name), &track->level)) {
        dd2_track_destroy(track);
        return NULL;
    }
    const dd2_mesh_limits limits = {.texture_definitions = track->level.texture_definition_count,
                                    .palette_banks =
                                        dd2_texture_palette_bank_count(track->textures)};
    for (unsigned index = 0; index < DD2_CAR_LIVERIES; ++index) {
        const unsigned driver = index < DD2_CAR_DRIVERS ? index : 0;
        const dd2_car_class car_class =
            index < DD2_CAR_DRIVERS ? DD2_CAR_ROOKIE : (dd2_car_class)(index - DD2_CAR_DRIVERS + 1);
        if (!dd2_car_livery_decode(&track->level, track->textures, driver, car_class,
                                   &track->liveries[index])) {
            dd2_track_destroy(track);
            return NULL;
        }
    }
    track->scene = dd2_scene_create(
        track->level.sections[DD2_LEVEL_SCENE_BLOCKS],
        (dd2_scene_options){.compressed = number <= DD2_TRACK_RACING_COUNT, .limits = limits});
    track->car = dd2_mesh_create(track->level.sections[DD2_LEVEL_CAR_HIGH], limits);
    track->wheels[0] = dd2_mesh_create(track->level.sections[DD2_LEVEL_WHEEL_FIRST], limits);
    track->wheels[1] = dd2_mesh_create(track->level.sections[DD2_LEVEL_WHEEL_SECOND], limits);
    track->road = dd2_road_create(&track->level, number <= DD2_TRACK_RACING_COUNT ? DD2_ROAD_RACING
                                                                                  : DD2_ROAD_ARENA);
    if (track->scene == NULL || track->car == NULL || track->road == NULL ||
        track->wheels[0] == NULL || track->wheels[1] == NULL) {
        dd2_track_destroy(track);
        return NULL;
    }
    return track;
}

void dd2_track_destroy(dd2_track *track) {
    if (track != NULL) {
        dd2_world_destroy(track->world);
        dd2_road_destroy(track->road);
        dd2_mesh_destroy(track->car);
        dd2_mesh_destroy(track->wheels[0]);
        dd2_mesh_destroy(track->wheels[1]);
        dd2_scene_destroy(track->scene);
        dd2_texture_set_destroy(track->textures);
        free(track);
    }
}

const dd2_level_data *dd2_track_level(const dd2_track *track) {
    return track != NULL && track->world == NULL ? &track->level : NULL;
}
const dd2_texture_set *dd2_track_textures(const dd2_track *track) {
    return track != NULL ? track->textures : NULL;
}
const dd2_scene *dd2_track_scene(const dd2_track *track) {
    return track != NULL ? track->scene : NULL;
}
const dd2_mesh *dd2_track_car(const dd2_track *track) {
    return track != NULL ? track->car : NULL;
}
const dd2_road *dd2_track_road(const dd2_track *track) {
    return track != NULL ? track->road : NULL;
}
const dd2_car_livery *dd2_track_car_livery(const dd2_track *track, unsigned driver,
                                           dd2_car_class car_class) {
    const unsigned index = dd2_car_livery_index(driver, car_class);
    return track == NULL || track->world != NULL || index >= DD2_CAR_LIVERIES
               ? NULL
               : &track->liveries[index];
}

const dd2_mesh *dd2_track_wheel(const dd2_track *track, unsigned wheel) {
    return track != NULL && wheel < 4 ? track->wheels[wheel & 1U] : NULL;
}
