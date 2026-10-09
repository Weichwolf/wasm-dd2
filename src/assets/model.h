#ifndef DD2_ASSETS_MODEL_H
#define DD2_ASSETS_MODEL_H

#include "assets/bytes.h"

#include <stddef.h>
#include <stdint.h>

#define DD2_MODEL_NO_TEXTURE UINT32_MAX

enum { DD2_MODEL_NAME_BYTES = 64, DD2_MODEL_MATERIAL_NAME_BYTES = 32, DD2_MODEL_TEXTURE_MAPS = 3 };

typedef enum {
    DD2_MODEL_EXTERIOR,
    DD2_MODEL_WHEEL_FRONT_LEFT,
    DD2_MODEL_WHEEL_FRONT_RIGHT,
    DD2_MODEL_WHEEL_REAR_LEFT,
    DD2_MODEL_WHEEL_REAR_RIGHT,
    DD2_MODEL_COCKPIT,
    DD2_MODEL_STEERING
} dd2_model_role;

typedef struct {
    char paths[DD2_MODEL_TEXTURE_MAPS][DD2_MODEL_NAME_BYTES];
} dd2_model_texture;

typedef struct {
    char name[DD2_MODEL_MATERIAL_NAME_BYTES];
    float color[4];
    float metallic;
    float roughness;
    uint32_t texture;
} dd2_model_material;

typedef struct {
    char name[DD2_MODEL_NAME_BYTES];
    uint32_t material;
    uint32_t first_index;
    uint32_t index_count;
    dd2_model_role role;
    float pivot[3];
} dd2_model_part;

typedef struct {
    float position[3];
    float normal[3];
    float uv[2];
} dd2_model_vertex;

typedef struct dd2_model dd2_model;

/* Copies a complete validated DD2MESH2 container into owned typed arrays.
 * Source bytes can be released immediately. Failure returns NULL and releases
 * partial allocations. Returned arrays borrow the model until destruction.
 * Geometry is meters, +Y up, +Z forward, with bottom-left texture coordinates. */
dd2_model *dd2_model_create(dd2_byte_view bytes);
void dd2_model_destroy(dd2_model *model);
size_t dd2_model_texture_count(const dd2_model *model);
size_t dd2_model_material_count(const dd2_model *model);
size_t dd2_model_part_count(const dd2_model *model);
size_t dd2_model_vertex_count(const dd2_model *model);
size_t dd2_model_index_count(const dd2_model *model);
const dd2_model_texture *dd2_model_textures(const dd2_model *model);
const dd2_model_material *dd2_model_materials(const dd2_model *model);
const dd2_model_part *dd2_model_parts(const dd2_model *model);
const dd2_model_vertex *dd2_model_vertices(const dd2_model *model);
const uint32_t *dd2_model_indices(const dd2_model *model);

#endif
