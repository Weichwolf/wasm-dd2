#include "assets/model.h"

#include "assets/bytes.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
    DD2_MODEL_HEADER_BYTES = 28,
    DD2_MODEL_TEXTURE_BYTES = 192,
    DD2_MODEL_MATERIAL_BYTES = 60,
    DD2_MODEL_PART_BYTES = 92,
    DD2_MODEL_VERTEX_BYTES = 32,
    DD2_MODEL_MAX_TEXTURES = 32,
    DD2_MODEL_MAX_MATERIALS = 64,
    DD2_MODEL_MAX_PARTS = 4096,
    DD2_MODEL_MAX_VERTICES = 1000000,
    DD2_MODEL_MAX_INDICES = 3000000,
    DD2_MODEL_COMPONENTS = 3,
    DD2_MODEL_CHANNELS = 4,
    DD2_MODEL_WORD_BYTES = 4,
    DD2_MODEL_MATERIAL_TEXTURE_OFFSET = 56,
    DD2_MODEL_PART_PIVOT_OFFSET = 80,
    DD2_MODEL_FLOAT_MANTISSA_BITS = 24,
    DD2_MODEL_UV_OFFSET = 24,
    DD2_MODEL_HEADER_VERTICES = 20,
    DD2_MODEL_HEADER_INDICES = 24,
    DD2_MODEL_ASCII_FIRST = 32,
    DD2_MODEL_ASCII_LAST = 126
};
static const float dd2_model_scalar_limit = 65536.0F;
static const double dd2_model_normal_tolerance = 0.0001;
static const double dd2_model_min_area_squared = 1e-20;
static const double dd2_model_winding_tolerance = -1e-10;
_Static_assert(sizeof(float) == sizeof(uint32_t) && FLT_RADIX == 2 &&
                   FLT_MANT_DIG == DD2_MODEL_FLOAT_MANTISSA_BITS,
               "Model files require IEEE binary32 floats");
_Static_assert(sizeof(dd2_model_vertex) == DD2_MODEL_VERTEX_BYTES, "Vertex arrays must be packed");

struct dd2_model {
    size_t texture_count;
    size_t material_count;
    size_t part_count;
    size_t vertex_count;
    size_t index_count;
    dd2_model_texture *textures;
    dd2_model_material *materials;
    dd2_model_part *parts;
    dd2_model_vertex *vertices;
    uint32_t *indices;
};

static bool dd2_model_scalar(const uint8_t *source, float *value) {
    const uint32_t bits = dd2_read_le32(source);
    const uint32_t exponent = UINT32_C(0x7f800000);
    /* Bit validation remains effective under the required -ffast-math flags. */
    if ((bits & exponent) == exponent) {
        return false;
    }
    const unsigned char *encoded = (const unsigned char *)&bits;
    unsigned char *decoded = (unsigned char *)value;
    for (size_t index = 0; index < sizeof(*value); ++index) {
        decoded[index] = encoded[index];
    }
    return *value >= -dd2_model_scalar_limit && *value <= dd2_model_scalar_limit;
}

static bool dd2_model_name(const uint8_t *source, char *name, size_t length) {
    bool terminated = false;
    for (size_t index = 0; index < length; ++index) {
        const uint8_t value = source[index];
        if ((terminated && value != 0) ||
            (!terminated && value != 0 &&
             (value < DD2_MODEL_ASCII_FIRST || value > DD2_MODEL_ASCII_LAST))) {
            return false;
        }
        terminated = terminated || value == 0;
        name[index] = (char)value;
    }
    return terminated && name[0] != '\0';
}

static bool dd2_model_path(const char *path) {
    const size_t length = strlen(path);
    const size_t extension = 4;
    if (length <= extension || memcmp(path + length - extension, ".png", extension) != 0) {
        return false;
    }
    size_t segment = 0;
    for (size_t index = 0; index <= length; ++index) {
        const char value = path[index];
        if (value == '/' || value == '\0') {
            const size_t count = index - segment;
            if (count == 0 || (count == 1 && path[segment] == '.') ||
                (count == 2 && path[segment] == '.' && path[segment + 1] == '.')) {
                return false;
            }
            segment = index + 1;
        } else if (!((value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
                     (value >= '0' && value <= '9') || value == '-' || value == '_' ||
                     value == '.')) {
            return false;
        }
    }
    return true;
}

static bool dd2_model_decode_textures(dd2_model *model, const uint8_t *source) {
    for (size_t index = 0; index < model->texture_count; ++index) {
        for (size_t map = 0; map < DD2_MODEL_TEXTURE_MAPS; ++map) {
            const uint8_t *name =
                source + (index * DD2_MODEL_TEXTURE_BYTES) + (map * DD2_MODEL_NAME_BYTES);
            if (!dd2_model_name(name, model->textures[index].paths[map], DD2_MODEL_NAME_BYTES) ||
                !dd2_model_path(model->textures[index].paths[map])) {
                return false;
            }
        }
    }
    return true;
}

static bool dd2_model_decode_materials(dd2_model *model, const uint8_t *source) {
    for (size_t index = 0; index < model->material_count; ++index) {
        const uint8_t *bytes = source + (index * DD2_MODEL_MATERIAL_BYTES);
        dd2_model_material *material = &model->materials[index];
        if (!dd2_model_name(bytes, material->name, sizeof(material->name))) {
            return false;
        }
        for (size_t previous = 0; previous < index; ++previous) {
            if (strcmp(material->name, model->materials[previous].name) == 0) {
                return false;
            }
        }
        for (size_t channel = 0; channel < DD2_MODEL_CHANNELS; ++channel) {
            if (!dd2_model_scalar(bytes + sizeof(material->name) + (channel * DD2_MODEL_WORD_BYTES),
                                  &material->color[channel]) ||
                material->color[channel] < 0 || material->color[channel] > 1) {
                return false;
            }
        }
        const size_t scalars = sizeof(material->name) + sizeof(material->color);
        if (!dd2_model_scalar(bytes + scalars, &material->metallic) || material->metallic < 0 ||
            material->metallic > 1 ||
            !dd2_model_scalar(bytes + scalars + DD2_MODEL_WORD_BYTES, &material->roughness) ||
            material->roughness < 0 || material->roughness > 1) {
            return false;
        }
        material->texture = dd2_read_le32(bytes + DD2_MODEL_MATERIAL_TEXTURE_OFFSET);
        if (material->texture != DD2_MODEL_NO_TEXTURE &&
            material->texture >= model->texture_count) {
            return false;
        }
    }
    return true;
}

static bool dd2_model_decode_parts(dd2_model *model, const uint8_t *source) {
    size_t end = 0;
    for (size_t index = 0; index < model->part_count; ++index) {
        const uint8_t *bytes = source + (index * DD2_MODEL_PART_BYTES);
        dd2_model_part *part = &model->parts[index];
        if (!dd2_model_name(bytes, part->name, sizeof(part->name))) {
            return false;
        }
        for (size_t previous = 0; previous < index; ++previous) {
            if (strcmp(part->name, model->parts[previous].name) == 0) {
                return false;
            }
        }
        const uint8_t *words = bytes + sizeof(part->name);
        part->material = dd2_read_le32(words);
        part->first_index = dd2_read_le32(words + DD2_MODEL_WORD_BYTES);
        part->index_count = dd2_read_le32(words + ((size_t)DD2_MODEL_WORD_BYTES * 2));
        const uint32_t role = dd2_read_le32(words + ((size_t)DD2_MODEL_WORD_BYTES * 3));
        if (part->material >= model->material_count || part->first_index != end ||
            part->index_count < DD2_MODEL_COMPONENTS ||
            part->index_count % DD2_MODEL_COMPONENTS != 0 ||
            part->index_count > model->index_count - end || role > DD2_MODEL_STEERING) {
            return false;
        }
        part->role = (dd2_model_role)role;
        end += part->index_count;
        for (size_t axis = 0; axis < DD2_MODEL_COMPONENTS; ++axis) {
            if (!dd2_model_scalar(bytes + DD2_MODEL_PART_PIVOT_OFFSET +
                                      (axis * DD2_MODEL_WORD_BYTES),
                                  &part->pivot[axis])) {
                return false;
            }
        }
    }
    return end == model->index_count;
}

static bool dd2_model_decode_vertices(dd2_model *model, const uint8_t *source) {
    for (size_t index = 0; index < model->vertex_count; ++index) {
        const uint8_t *bytes = source + (index * DD2_MODEL_VERTEX_BYTES);
        dd2_model_vertex *vertex = &model->vertices[index];
        double normal_squared = 0;
        for (size_t axis = 0; axis < DD2_MODEL_COMPONENTS; ++axis) {
            if (!dd2_model_scalar(bytes + (axis * DD2_MODEL_WORD_BYTES), &vertex->position[axis]) ||
                !dd2_model_scalar(bytes + ((axis + DD2_MODEL_COMPONENTS) * DD2_MODEL_WORD_BYTES),
                                  &vertex->normal[axis])) {
                return false;
            }
            normal_squared += (double)vertex->normal[axis] * vertex->normal[axis];
        }
        if (fabs(sqrt(normal_squared) - 1) > dd2_model_normal_tolerance ||
            !dd2_model_scalar(bytes + (DD2_MODEL_UV_OFFSET), &vertex->uv[0]) ||
            !dd2_model_scalar(bytes + ((DD2_MODEL_UV_OFFSET + DD2_MODEL_WORD_BYTES)),
                              &vertex->uv[1])) {
            return false;
        }
    }
    return true;
}

static bool dd2_model_triangle(const dd2_model *model, size_t first) {
    const dd2_model_vertex *corners[DD2_MODEL_COMPONENTS] = {0};
    for (size_t index = 0; index < DD2_MODEL_COMPONENTS; ++index) {
        corners[index] = &model->vertices[model->indices[first + index]];
    }
    double edges[2][DD2_MODEL_COMPONENTS] = {0};
    for (size_t edge = 0; edge < 2; ++edge) {
        for (size_t axis = 0; axis < DD2_MODEL_COMPONENTS; ++axis) {
            edges[edge][axis] =
                (double)corners[edge + 1]->position[axis] - corners[0]->position[axis];
        }
    }
    double area_squared = 0;
    double orientation = 0;
    for (size_t axis = 0; axis < DD2_MODEL_COMPONENTS; ++axis) {
        const size_t next = (axis + 1) % DD2_MODEL_COMPONENTS;
        const size_t last = (axis + 2) % DD2_MODEL_COMPONENTS;
        const double cross = (edges[0][next] * edges[1][last]) - (edges[0][last] * edges[1][next]);
        area_squared += cross * cross;
        const double normal = ((double)corners[0]->normal[axis] + corners[1]->normal[axis] +
                               corners[2]->normal[axis]) /
                              (double)DD2_MODEL_COMPONENTS;
        orientation += cross * normal;
    }
    return area_squared >= dd2_model_min_area_squared && orientation >= dd2_model_winding_tolerance;
}

static bool dd2_model_decode_indices(dd2_model *model, const uint8_t *source) {
    for (size_t index = 0; index < model->index_count; ++index) {
        model->indices[index] = dd2_read_le32(source + (index * DD2_MODEL_WORD_BYTES));
        if (model->indices[index] >= model->vertex_count) {
            return false;
        }
    }
    for (size_t first = 0; first < model->index_count; first += DD2_MODEL_COMPONENTS) {
        if (!dd2_model_triangle(model, first)) {
            return false;
        }
    }
    return true;
}

dd2_model *dd2_model_create(dd2_byte_view bytes) {
    if (bytes.data == NULL || bytes.size < DD2_MODEL_HEADER_BYTES ||
        memcmp(bytes.data, "DD2MESH2", (size_t)DD2_MODEL_WORD_BYTES * 2) != 0) {
        return NULL;
    }
    const uint32_t textures = dd2_read_le32(bytes.data + ((size_t)DD2_MODEL_WORD_BYTES * 2));
    const uint32_t materials = dd2_read_le32(bytes.data + ((size_t)DD2_MODEL_WORD_BYTES * 3));
    const uint32_t parts = dd2_read_le32(bytes.data + ((size_t)DD2_MODEL_WORD_BYTES * 4));
    const uint32_t vertices = dd2_read_le32(bytes.data + DD2_MODEL_HEADER_VERTICES);
    const uint32_t indices = dd2_read_le32(bytes.data + DD2_MODEL_HEADER_INDICES);
    if (textures > DD2_MODEL_MAX_TEXTURES || materials == 0 ||
        materials > DD2_MODEL_MAX_MATERIALS || parts == 0 || parts > DD2_MODEL_MAX_PARTS ||
        vertices < DD2_MODEL_COMPONENTS || vertices > DD2_MODEL_MAX_VERTICES ||
        indices < DD2_MODEL_COMPONENTS || indices > DD2_MODEL_MAX_INDICES ||
        indices % DD2_MODEL_COMPONENTS != 0) {
        return NULL;
    }
    const size_t texture_end =
        DD2_MODEL_HEADER_BYTES + ((size_t)textures * DD2_MODEL_TEXTURE_BYTES);
    const size_t material_end = texture_end + ((size_t)materials * DD2_MODEL_MATERIAL_BYTES);
    const size_t part_end = material_end + ((size_t)parts * DD2_MODEL_PART_BYTES);
    const size_t vertex_end = part_end + ((size_t)vertices * DD2_MODEL_VERTEX_BYTES);
    const size_t end = vertex_end + ((size_t)indices * DD2_MODEL_WORD_BYTES);
    if (bytes.size != end) {
        return NULL;
    }
    dd2_model *model = calloc(1, sizeof(*model));
    if (model == NULL) {
        return NULL;
    }
    model->texture_count = textures;
    model->material_count = materials;
    model->part_count = parts;
    model->vertex_count = vertices;
    model->index_count = indices;
    model->textures = textures == 0 ? NULL : calloc(textures, sizeof(*model->textures));
    model->materials = calloc(materials, sizeof(*model->materials));
    model->parts = calloc(parts, sizeof(*model->parts));
    model->vertices = calloc(vertices, sizeof(*model->vertices));
    model->indices = calloc(indices, sizeof(*model->indices));
    if ((textures != 0 && model->textures == NULL) || model->materials == NULL ||
        model->parts == NULL || model->vertices == NULL || model->indices == NULL ||
        !dd2_model_decode_textures(model, bytes.data + DD2_MODEL_HEADER_BYTES) ||
        !dd2_model_decode_materials(model, bytes.data + texture_end) ||
        !dd2_model_decode_parts(model, bytes.data + material_end) ||
        !dd2_model_decode_vertices(model, bytes.data + part_end) ||
        !dd2_model_decode_indices(model, bytes.data + vertex_end)) {
        dd2_model_destroy(model);
        return NULL;
    }
    return model;
}

void dd2_model_destroy(dd2_model *model) {
    if (model != NULL) {
        free(model->textures);
        free(model->materials);
        free(model->parts);
        free(model->vertices);
        free(model->indices);
        free(model);
    }
}

size_t dd2_model_texture_count(const dd2_model *model) {
    return model != NULL ? model->texture_count : 0;
}
size_t dd2_model_material_count(const dd2_model *model) {
    return model != NULL ? model->material_count : 0;
}
size_t dd2_model_part_count(const dd2_model *model) {
    return model != NULL ? model->part_count : 0;
}
size_t dd2_model_vertex_count(const dd2_model *model) {
    return model != NULL ? model->vertex_count : 0;
}
size_t dd2_model_index_count(const dd2_model *model) {
    return model != NULL ? model->index_count : 0;
}
const dd2_model_texture *dd2_model_textures(const dd2_model *model) {
    return model != NULL ? model->textures : NULL;
}
const dd2_model_material *dd2_model_materials(const dd2_model *model) {
    return model != NULL ? model->materials : NULL;
}
const dd2_model_part *dd2_model_parts(const dd2_model *model) {
    return model != NULL ? model->parts : NULL;
}
const dd2_model_vertex *dd2_model_vertices(const dd2_model *model) {
    return model != NULL ? model->vertices : NULL;
}
const uint32_t *dd2_model_indices(const dd2_model *model) {
    return model != NULL ? model->indices : NULL;
}
