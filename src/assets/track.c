#include "assets/track.h"

#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/mesh.h"
#include "assets/scene.h"
#include "assets/textures.h"

#include <stddef.h>
#include <stdlib.h>

enum { DD2_TRACK_RACING_COUNT = 7 };

struct dd2_track {
    dd2_level_data level;
    dd2_texture_set *textures;
    dd2_scene *scene;
    dd2_mesh *car;
};

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
    track->scene = dd2_scene_create(
        track->level.sections[DD2_LEVEL_SCENE_BLOCKS],
        (dd2_scene_options){.compressed = number <= DD2_TRACK_RACING_COUNT, .limits = limits});
    track->car = dd2_mesh_create(track->level.sections[DD2_LEVEL_CAR_HIGH], limits);
    if (track->scene == NULL || track->car == NULL) {
        dd2_track_destroy(track);
        return NULL;
    }
    return track;
}

void dd2_track_destroy(dd2_track *track) {
    if (track != NULL) {
        dd2_mesh_destroy(track->car);
        dd2_scene_destroy(track->scene);
        dd2_texture_set_destroy(track->textures);
        free(track);
    }
}

const dd2_level_data *dd2_track_level(const dd2_track *track) {
    return track != NULL ? &track->level : NULL;
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
