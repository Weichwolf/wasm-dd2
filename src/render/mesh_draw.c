#include "render/mesh_draw.h"

#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/mesh.h"
#include "assets/scene.h"
#include "assets/textures.h"

#include <GL/softgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    DD2_MATERIAL_NEUTRAL_SHADE = 8,
    DD2_MATERIAL_PAGE_MASK = 31,
    DD2_MATERIAL_COLOR_MASK = 255,
    DD2_MATERIAL_BLUE_SHIFT = 16
};

static const float dd2_material_texel_center = 0.5F;

struct dd2_mesh_materials {
    const dd2_level_data *level;
    const dd2_texture_set *textures;
    size_t count;
    GLuint *pages;
    uint8_t *rgba;
};

dd2_mesh_materials *dd2_mesh_materials_create(const dd2_level_data *level,
                                              const dd2_texture_set *textures) {
    const size_t banks = dd2_texture_palette_bank_count(textures);
    if (level == NULL || textures == NULL || banks == 0) {
        return NULL;
    }
    dd2_mesh_materials *materials = calloc(1, sizeof(*materials));
    if (materials == NULL) {
        return NULL;
    }
    materials->level = level;
    materials->textures = textures;
    materials->count = banks * DD2_TEXTURE_PAGE_COUNT;
    materials->pages = calloc(materials->count, sizeof(*materials->pages));
    materials->rgba = malloc(DD2_TEXTURE_PAGE_RGBA_BYTES);
    if (materials->pages == NULL || materials->rgba == NULL) {
        dd2_mesh_materials_destroy(materials);
        return NULL;
    }
    return materials;
}

void dd2_mesh_materials_destroy(dd2_mesh_materials *materials) {
    if (materials == NULL) {
        return;
    }
    if (materials->pages != NULL) {
        for (size_t index = 0; index < materials->count; ++index) {
            if (materials->pages[index] != 0) {
                glDeleteTextures(1, &materials->pages[index]);
            }
        }
    }
    free(materials->pages);
    free(materials->rgba);
    free(materials);
}

static bool dd2_mesh_material_bind(dd2_mesh_materials *materials, dd2_texture_sample sample) {
    const size_t index = ((size_t)sample.palette_bank * DD2_TEXTURE_PAGE_COUNT) + sample.page;
    if (index >= materials->count) {
        return false;
    }
    GLuint *texture = &materials->pages[index];
    if (*texture == 0) {
        if (!dd2_texture_page_rgba(
                materials->textures, sample,
                (dd2_byte_buffer){.data = materials->rgba, .size = DD2_TEXTURE_PAGE_RGBA_BYTES})) {
            return false;
        }
        glGenTextures(1, texture);
        if (*texture == 0) {
            return false;
        }
        glBindTexture(GL_TEXTURE_2D, *texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DD2_TEXTURE_PAGE_SIDE, DD2_TEXTURE_PAGE_SIDE, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, materials->rgba);
        if (glGetError() != GL_NO_ERROR) {
            glDeleteTextures(1, texture);
            *texture = 0;
            return false;
        }
    } else {
        glBindTexture(GL_TEXTURE_2D, *texture);
    }
    return true;
}

static bool dd2_mesh_face_draw(dd2_mesh_materials *materials, const dd2_mesh_vector *vertices,
                               const dd2_mesh_face *face) {
    dd2_texture_definition definition = {0};
    if (face->textured) {
        if (!dd2_level_texture_definition(materials->level, face->texture, &definition) ||
            !dd2_mesh_material_bind(
                materials,
                (dd2_texture_sample){.page = definition.page_flags & DD2_MATERIAL_PAGE_MASK,
                                     .palette_bank = face->palette_bank,
                                     .shade = DD2_MATERIAL_NEUTRAL_SHADE,
                                     .cutout = true})) {
            return false;
        }
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0F);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    } else {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_ALPHA_TEST);
    }
    /* PSX quads store the two row pairs, not cyclic perimeter order. Use
     * explicit triangles so interpolation and the diagonal are unambiguous. */
    const unsigned quad[] = {0, 1, 2, 2, 1, 3};
    const unsigned triangle[] = {0, 1, 2};
    const unsigned *order = face->corner_count == DD2_MESH_CORNERS ? quad : triangle;
    const size_t count =
        face->corner_count == DD2_MESH_CORNERS ? sizeof(quad) / sizeof(quad[0]) : 3;
    glBegin(GL_TRIANGLES);
    for (size_t index = 0; index < count; ++index) {
        const unsigned corner = order[index];
        const dd2_mesh_vector vertex = vertices[face->indices[corner]];
        const uint32_t color = face->colors[corner];
        glColor3ub((GLubyte)(color & DD2_MATERIAL_COLOR_MASK),
                   (GLubyte)((color >> DD2_BYTE_BITS) & DD2_MATERIAL_COLOR_MASK),
                   (GLubyte)((color >> DD2_MATERIAL_BLUE_SHIFT) & DD2_MATERIAL_COLOR_MASK));
        if (face->textured) {
            glTexCoord2f(((float)definition.corners[corner].u + dd2_material_texel_center) /
                             (float)DD2_TEXTURE_PAGE_SIDE,
                         ((float)definition.corners[corner].v + dd2_material_texel_center) /
                             (float)DD2_TEXTURE_PAGE_SIDE);
        }
        glVertex3f((float)vertex.x, (float)vertex.y, (float)vertex.z);
    }
    glEnd();
    return true;
}

bool dd2_mesh_draw(dd2_mesh_materials *materials, const dd2_mesh *mesh, dd2_track_vertex position) {
    if (materials == NULL || mesh == NULL) {
        return false;
    }
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef((float)position.x, (float)position.y, (float)position.z);
    const dd2_mesh_face *faces = dd2_mesh_faces(mesh);
    const dd2_mesh_vector *vertices = dd2_mesh_vertices(mesh);
    bool passed = true;
    for (size_t index = 0; index < dd2_mesh_face_count(mesh) && passed; ++index) {
        passed = dd2_mesh_face_draw(materials, vertices, &faces[index]);
    }
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA_TEST);
    return passed && glGetError() == GL_NO_ERROR;
}

bool dd2_scene_draw(dd2_mesh_materials *materials, const dd2_scene *scene) {
    if (materials == NULL || scene == NULL) {
        return false;
    }
    const dd2_scene_object *objects = dd2_scene_objects(scene);
    for (size_t index = 0; index < dd2_scene_object_count(scene); ++index) {
        if (!dd2_mesh_draw(materials, objects[index].mesh, objects[index].position)) {
            return false;
        }
    }
    return true;
}
