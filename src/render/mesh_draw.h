#ifndef DD2_RENDER_MESH_DRAW_H
#define DD2_RENDER_MESH_DRAW_H

#include "assets/level.h"
#include "assets/mesh.h"
#include "assets/scene.h"
#include "assets/textures.h"

#include <stdbool.h>

typedef struct dd2_mesh_materials dd2_mesh_materials;

/* Owns lazily uploaded SoftGL texture pages; borrows level and textures, which
 * must outlive it. Create/draw/destroy on one current renderer context/thread.
 * Caller sets viewport, matrices, depth/clear state. Initial rendering uses
 * neutral palette shade, cutout texels and raw untextured RGB. Sprite commands
 * retain their stored quad geometry; lighting, billboards, fog/blending and
 * original road strips are separate pending rendering work. */
dd2_mesh_materials *dd2_mesh_materials_create(const dd2_level_data *level,
                                              const dd2_texture_set *textures);
void dd2_mesh_materials_destroy(dd2_mesh_materials *materials);
bool dd2_mesh_draw(dd2_mesh_materials *materials, const dd2_mesh *mesh, dd2_track_vertex position);
bool dd2_scene_draw(dd2_mesh_materials *materials, const dd2_scene *scene);

#endif
