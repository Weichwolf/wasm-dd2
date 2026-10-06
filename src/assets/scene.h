#ifndef DD2_ASSETS_SCENE_H
#define DD2_ASSETS_SCENE_H

#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/mesh.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct dd2_scene dd2_scene;

typedef struct {
    bool compressed;
    dd2_mesh_limits limits;
} dd2_scene_options;

typedef struct {
    /* Source bounding center; static vertices use the raster-cell origin.
     * Header flag bit 7 selects local vertices relative to this center. */
    dd2_track_vertex position;
    dd2_track_vertex origin;
    dd2_mesh *mesh;
} dd2_scene_object;

/* Owns every decoded mesh. Input can be released after creation. Objects and
 * their meshes remain owned by the scene; callers must not free them separately.
 * Racing levels use compressed blocks; arena levels use raw blocks. An empty
 * section is a valid empty scene. Failures discard the complete partial scene. */
dd2_scene *dd2_scene_create(dd2_byte_view section, dd2_scene_options options);
void dd2_scene_destroy(dd2_scene *scene);
size_t dd2_scene_block_count(const dd2_scene *scene);
size_t dd2_scene_object_count(const dd2_scene *scene);
const dd2_scene_object *dd2_scene_objects(const dd2_scene *scene);

#endif
