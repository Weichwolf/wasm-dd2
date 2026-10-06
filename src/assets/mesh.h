#ifndef DD2_ASSETS_MESH_H
#define DD2_ASSETS_MESH_H

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { DD2_MESH_CORNERS = 4, DD2_MESH_OPCODE_COUNT = 44, DD2_MESH_LOCAL_ORIGIN = 128 };

typedef struct dd2_mesh dd2_mesh;

typedef struct {
    size_t texture_definitions;
    size_t palette_banks;
} dd2_mesh_limits;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
    uint16_t auxiliary;
} dd2_mesh_vector;

typedef struct {
    uint16_t indices[DD2_MESH_CORNERS];
    uint16_t normals[DD2_MESH_CORNERS];
    uint32_t colors[DD2_MESH_CORNERS];
    uint16_t texture;
    uint16_t palette_bank;
    uint16_t attributes;
    uint8_t opcode;
    uint8_t group_flags;
    uint8_t corner_count;
    bool textured;
    bool lit;
    bool gouraud;
} dd2_mesh_face;

/* Meshes own all decoded vectors and faces. The input bytes can be released
 * after creation. All polygon commands 0..43 are retained, including sprite
 * modes; their rendering policies belong to the renderer. Creation validates
 * stream termination, record extents and vertex/normal/material references. */
dd2_mesh *dd2_mesh_create(dd2_byte_view bytes, dd2_mesh_limits limits);
void dd2_mesh_destroy(dd2_mesh *mesh);
uint8_t dd2_mesh_flags(const dd2_mesh *mesh);
size_t dd2_mesh_vertex_count(const dd2_mesh *mesh);
size_t dd2_mesh_normal_count(const dd2_mesh *mesh);
size_t dd2_mesh_face_count(const dd2_mesh *mesh);
const dd2_mesh_vector *dd2_mesh_vertices(const dd2_mesh *mesh);
const dd2_mesh_vector *dd2_mesh_normals(const dd2_mesh *mesh);
const dd2_mesh_face *dd2_mesh_faces(const dd2_mesh *mesh);

#endif
