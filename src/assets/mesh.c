#include "assets/mesh.h"

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    DD2_MESH_HEADER_BYTES = 44,
    DD2_MESH_VERTEX_COUNT_OFFSET = 10,
    DD2_MESH_NORMAL_COUNT_OFFSET = 12,
    DD2_MESH_VERTICES_OFFSET = 32,
    DD2_MESH_NORMALS_OFFSET = 36,
    DD2_MESH_POLYGONS_OFFSET = 40,
    DD2_MESH_VECTOR_BYTES = 8,
    DD2_MESH_AUX_OFFSET = 6,
    DD2_MESH_GROUP_BYTES = 4,
    DD2_MESH_COLOR_OFFSET = 4,
    DD2_MESH_FLAT_BYTES = 16,
    DD2_MESH_FAMILY_SIZE = 8,
    DD2_MESH_SPRITE_FIRST = 32,
    DD2_MESH_MAX_SOURCE_BYTES = 1024 * 1024
};

struct dd2_mesh {
    size_t vertex_count;
    size_t normal_count;
    size_t face_count;
    dd2_mesh_vector *vertices;
    dd2_mesh_vector *normals;
    dd2_mesh_face *faces;
};

typedef struct {
    unsigned corners;
    unsigned colors;
    unsigned vertex_offset;
    unsigned stride;
    bool textured;
    bool gouraud;
    bool lit;
} dd2_face_layout;

static dd2_face_layout dd2_mesh_layout(unsigned opcode) {
    const unsigned family = opcode / DD2_MESH_FAMILY_SIZE;
    dd2_face_layout layout = {.corners = opcode % DD2_MESH_FAMILY_SIZE >= DD2_MESH_CORNERS ||
                                                 opcode >= DD2_MESH_SPRITE_FIRST
                                             ? 4U
                                             : 3U,
                              .textured = family == 1 || family >= 3,
                              .gouraud = family == 2 || family == 3,
                              .lit = opcode < DD2_MESH_SPRITE_FIRST && (opcode & 1U) != 0};
    layout.colors = layout.gouraud && !layout.lit ? layout.corners : 1;
    layout.vertex_offset =
        DD2_MESH_COLOR_OFFSET + (layout.colors * 4) + (layout.textured ? 4U : 0U);
    layout.stride = (layout.vertex_offset + (layout.corners * 2) +
                     (layout.gouraud && layout.lit ? layout.corners * 2 : 0) + 3) &
                    ~3U;
    if (family == 0) {
        layout.stride = DD2_MESH_FLAT_BYTES;
    }
    return layout;
}

static dd2_mesh_vector dd2_mesh_vector_decode(const uint8_t *bytes) {
    return (dd2_mesh_vector){.x = dd2_read_le_i16(bytes),
                             .y = dd2_read_le_i16(bytes + 2),
                             .z = dd2_read_le_i16(bytes + 4),
                             .auxiliary = dd2_read_le16(bytes + DD2_MESH_AUX_OFFSET)};
}

static bool dd2_mesh_face_decode(const uint8_t *bytes, dd2_mesh_face *face, const dd2_mesh *mesh,
                                 dd2_mesh_limits limits) {
    const dd2_face_layout layout = dd2_mesh_layout(face->opcode);
    face->corner_count = (uint8_t)layout.corners;
    face->textured = layout.textured;
    face->gouraud = layout.gouraud;
    face->lit = layout.lit;
    face->attributes = dd2_read_le16(bytes);
    for (size_t corner = 0; corner < layout.corners; ++corner) {
        face->indices[corner] = dd2_read_le16(bytes + layout.vertex_offset + (corner * 2));
        if (face->indices[corner] >= mesh->vertex_count) {
            return false;
        }
        const size_t color = layout.colors == 1 ? 0 : corner;
        face->colors[corner] = dd2_read_le32(bytes + DD2_MESH_COLOR_OFFSET + (color * 4));
        if (layout.lit) {
            const size_t normal_offset =
                layout.gouraud ? layout.vertex_offset + ((layout.corners + corner) * 2) : 2;
            face->normals[corner] = dd2_read_le16(bytes + normal_offset);
            if (face->normals[corner] >= mesh->normal_count) {
                return false;
            }
        }
    }
    if (layout.textured) {
        face->texture = dd2_read_le16(bytes + layout.vertex_offset - 4);
        face->palette_bank = dd2_read_le16(bytes + layout.vertex_offset - 2);
        if (face->texture >= limits.texture_definitions ||
            face->palette_bank >= limits.palette_banks) {
            return false;
        }
    }
    return true;
}

static bool dd2_mesh_polygons(dd2_byte_view bytes, dd2_mesh *mesh, dd2_mesh_limits limits) {
    size_t offset = dd2_read_le32(bytes.data + DD2_MESH_POLYGONS_OFFSET);
    if (offset < DD2_MESH_HEADER_BYTES || offset > bytes.size) {
        return false;
    }
    mesh->faces = calloc(bytes.size / DD2_MESH_FLAT_BYTES, sizeof(*mesh->faces));
    if (mesh->faces == NULL) {
        return false;
    }
    while (bytes.size - offset >= DD2_MESH_GROUP_BYTES) {
        const uint8_t *group = bytes.data + offset;
        if (group[3] == 0) {
            return true;
        }
        const size_t count = dd2_read_le16(group);
        const unsigned opcode = group[2];
        if (opcode >= DD2_MESH_OPCODE_COUNT) {
            return false;
        }
        offset += DD2_MESH_GROUP_BYTES;
        const dd2_face_layout layout = dd2_mesh_layout(opcode);
        if (count > (bytes.size - offset) / layout.stride) {
            return false;
        }
        for (size_t index = 0; index < count; ++index) {
            dd2_mesh_face *face = &mesh->faces[mesh->face_count];
            face->opcode = (uint8_t)opcode;
            face->group_flags = group[3];
            if (!dd2_mesh_face_decode(bytes.data + offset, face, mesh, limits)) {
                return false;
            }
            ++mesh->face_count;
            offset += layout.stride;
        }
    }
    return false;
}

dd2_mesh *dd2_mesh_create(dd2_byte_view bytes, dd2_mesh_limits limits) {
    if (bytes.data == NULL || bytes.size < DD2_MESH_HEADER_BYTES ||
        bytes.size > DD2_MESH_MAX_SOURCE_BYTES) {
        return NULL;
    }
    const size_t vertex_count = dd2_read_le16(bytes.data + DD2_MESH_VERTEX_COUNT_OFFSET);
    const size_t normal_count = dd2_read_le16(bytes.data + DD2_MESH_NORMAL_COUNT_OFFSET);
    const size_t vertices = dd2_read_le32(bytes.data + DD2_MESH_VERTICES_OFFSET);
    const size_t normals = dd2_read_le32(bytes.data + DD2_MESH_NORMALS_OFFSET);
    if (vertices < DD2_MESH_HEADER_BYTES || vertices > bytes.size ||
        vertex_count > (bytes.size - vertices) / DD2_MESH_VECTOR_BYTES ||
        normals < DD2_MESH_HEADER_BYTES || normals > bytes.size ||
        normal_count > (bytes.size - normals) / DD2_MESH_VECTOR_BYTES) {
        return NULL;
    }
    dd2_mesh *mesh = calloc(1, sizeof(*mesh));
    if (mesh == NULL) {
        return NULL;
    }
    mesh->vertex_count = vertex_count;
    mesh->normal_count = normal_count;
    mesh->vertices = calloc(vertex_count + 1, sizeof(*mesh->vertices));
    mesh->normals = calloc(normal_count + 1, sizeof(*mesh->normals));
    if (mesh->vertices == NULL || mesh->normals == NULL ||
        !dd2_mesh_polygons(bytes, mesh, limits)) {
        dd2_mesh_destroy(mesh);
        return NULL;
    }
    for (size_t index = 0; index < vertex_count; ++index) {
        mesh->vertices[index] =
            dd2_mesh_vector_decode(bytes.data + vertices + (index * DD2_MESH_VECTOR_BYTES));
    }
    for (size_t index = 0; index < normal_count; ++index) {
        mesh->normals[index] =
            dd2_mesh_vector_decode(bytes.data + normals + (index * DD2_MESH_VECTOR_BYTES));
    }
    return mesh;
}

void dd2_mesh_destroy(dd2_mesh *mesh) {
    if (mesh != NULL) {
        free(mesh->vertices);
        free(mesh->normals);
        free(mesh->faces);
        free(mesh);
    }
}

size_t dd2_mesh_vertex_count(const dd2_mesh *mesh) {
    return mesh != NULL ? mesh->vertex_count : 0;
}
size_t dd2_mesh_normal_count(const dd2_mesh *mesh) {
    return mesh != NULL ? mesh->normal_count : 0;
}
size_t dd2_mesh_face_count(const dd2_mesh *mesh) {
    return mesh != NULL ? mesh->face_count : 0;
}
const dd2_mesh_vector *dd2_mesh_vertices(const dd2_mesh *mesh) {
    return mesh != NULL ? mesh->vertices : NULL;
}
const dd2_mesh_vector *dd2_mesh_normals(const dd2_mesh *mesh) {
    return mesh != NULL ? mesh->normals : NULL;
}
const dd2_mesh_face *dd2_mesh_faces(const dd2_mesh *mesh) {
    return mesh != NULL ? mesh->faces : NULL;
}
