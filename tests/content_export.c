#include "assets/bytes.h"
#include "assets/image.h"
#include "assets/model.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool dd2_content_word(FILE *file, uint32_t value) {
    uint8_t bytes[sizeof(value)] = {0};
    for (size_t index = 0; index < sizeof(bytes); ++index) {
        bytes[index] = (uint8_t)(value >> (index * DD2_BYTE_BITS));
    }
    return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
}

static bool dd2_content_floats(FILE *file, const float *values, size_t count) {
    for (size_t index = 0; index < count; ++index) {
        uint32_t bits = 0;
        unsigned char *encoded = (unsigned char *)&bits;
        const unsigned char *source = (const unsigned char *)&values[index];
        for (size_t byte = 0; byte < sizeof(bits); ++byte) {
            encoded[byte] = source[byte];
        }
        if (!dd2_content_word(file, bits)) {
            return false;
        }
    }
    return true;
}

static bool dd2_content_materials(FILE *file, const dd2_model *model) {
    const dd2_model_material *materials = dd2_model_materials(model);
    for (size_t index = 0; index < dd2_model_material_count(model); ++index) {
        const dd2_model_material *material = &materials[index];
        if (fwrite(material->name, 1, sizeof(material->name), file) != sizeof(material->name) ||
            !dd2_content_floats(file, material->color,
                                sizeof(material->color) / sizeof(material->color[0])) ||
            !dd2_content_floats(file, &material->metallic, 1) ||
            !dd2_content_floats(file, &material->roughness, 1) ||
            !dd2_content_word(file, material->texture)) {
            return false;
        }
    }
    return true;
}

static bool dd2_content_parts(FILE *file, const dd2_model *model) {
    const dd2_model_part *parts = dd2_model_parts(model);
    for (size_t index = 0; index < dd2_model_part_count(model); ++index) {
        const dd2_model_part *part = &parts[index];
        if (fwrite(part->name, 1, sizeof(part->name), file) != sizeof(part->name) ||
            !dd2_content_word(file, part->material) || !dd2_content_word(file, part->first_index) ||
            !dd2_content_word(file, part->index_count) ||
            !dd2_content_word(file, (uint32_t)part->role) ||
            !dd2_content_floats(file, part->pivot, sizeof(part->pivot) / sizeof(part->pivot[0]))) {
            return false;
        }
    }
    return true;
}

static bool dd2_content_vectors(FILE *file, const dd2_model *model) {
    const dd2_model_vertex *vertices = dd2_model_vertices(model);
    for (size_t index = 0; index < dd2_model_vertex_count(model); ++index) {
        if (!dd2_content_floats(file, vertices[index].position,
                                sizeof(vertices[index].position) / sizeof(float)) ||
            !dd2_content_floats(file, vertices[index].normal,
                                sizeof(vertices[index].normal) / sizeof(float)) ||
            !dd2_content_floats(file, vertices[index].uv,
                                sizeof(vertices[index].uv) / sizeof(float))) {
            return false;
        }
    }
    const uint32_t *indices = dd2_model_indices(model);
    for (size_t index = 0; index < dd2_model_index_count(model); ++index) {
        if (!dd2_content_word(file, indices[index])) {
            return false;
        }
    }
    return true;
}

static bool dd2_content_model(FILE *file, const dd2_model *model) {
    const char magic[] = "DD2MESH2";
    if (fwrite(magic, 1, sizeof(magic) - 1, file) != sizeof(magic) - 1) {
        return false;
    }
    const uint32_t counts[] = {
        (uint32_t)dd2_model_texture_count(model), (uint32_t)dd2_model_material_count(model),
        (uint32_t)dd2_model_part_count(model), (uint32_t)dd2_model_vertex_count(model),
        (uint32_t)dd2_model_index_count(model)};
    for (size_t index = 0; index < sizeof(counts) / sizeof(counts[0]); ++index) {
        if (!dd2_content_word(file, counts[index])) {
            return false;
        }
    }
    const dd2_model_texture *textures = dd2_model_textures(model);
    for (size_t index = 0; index < dd2_model_texture_count(model); ++index) {
        if (fwrite(textures[index].paths, 1, sizeof(textures[index].paths), file) !=
            sizeof(textures[index].paths)) {
            return false;
        }
    }
    return dd2_content_materials(file, model) && dd2_content_parts(file, model) &&
           dd2_content_vectors(file, model);
}

static bool dd2_content_export(FILE *file, const char *path, bool image_mode) {
    dd2_file source = {0};
    if (!dd2_file_read(path, &source)) {
        return false;
    }
    const dd2_byte_view bytes = {source.data, source.size};
    dd2_image *image = image_mode ? dd2_image_create_png(bytes) : NULL;
    dd2_model *model = image_mode ? NULL : dd2_model_create(bytes);
    dd2_file_release(&source);
    if (image == NULL && model == NULL) {
        return false;
    }
    const bool passed = image_mode ? fwrite(dd2_image_pixels(image), 1, dd2_image_rgba_bytes(image),
                                            file) == dd2_image_rgba_bytes(image)
                                   : dd2_content_model(file, model);
    dd2_image_destroy(image);
    dd2_model_destroy(model);
    return passed;
}

static bool dd2_content_list(FILE *output, const char *path, bool image_mode) {
    FILE *list = fopen(path, "rb");
    if (list == NULL) {
        return false;
    }
    enum { DD2_CONTENT_PATH_BYTES = 1024 };
    char line[DD2_CONTENT_PATH_BYTES] = {0};
    bool passed = true;
    size_t entries = 0;
    while (passed && fgets(line, sizeof(line), list) != NULL) {
        const size_t length = strlen(line);
        passed = length > 1 && line[length - 1] == '\n';
        if (passed) {
            line[length - 1] = '\0';
            passed = strchr(line, '\r') == NULL && dd2_content_export(output, line, image_mode);
            ++entries;
        }
    }
    const bool read_ok = ferror(list) == 0;
    const bool closed = fclose(list) == 0;
    return passed && entries != 0 && read_ok && closed;
}

int main(int argc, char **argv) {
    const int arguments = 4;
    if (argc != arguments) {
        return EXIT_FAILURE;
    }
    const bool image_mode = strcmp(argv[1], "image") == 0 || strcmp(argv[1], "image-list") == 0;
    const bool list_mode = strcmp(argv[1], "model-list") == 0 || strcmp(argv[1], "image-list") == 0;
    if (!image_mode && !list_mode && strcmp(argv[1], "model") != 0) {
        return EXIT_FAILURE;
    }
    FILE *file = fopen(argv[3], "wb");
    if (file == NULL) {
        return EXIT_FAILURE;
    }
    const bool passed = list_mode ? dd2_content_list(file, argv[2], image_mode)
                                  : dd2_content_export(file, argv[2], image_mode);
    const bool closed = fclose(file) == 0;
    if (!passed || !closed) {
        (void)remove(argv[3]);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
