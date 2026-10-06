#ifndef DD2_ASSETS_LEVEL_H
#define DD2_ASSETS_LEVEL_H

#include "assets/bytes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { DD2_LEVEL_SECTION_COUNT = 29, DD2_TEXTURE_CORNERS = 4 };

typedef enum {
    DD2_LEVEL_SCENE_BLOCKS = 0,
    DD2_LEVEL_ROAD_STRIPS = 1,
    DD2_LEVEL_ROAD_VERTICES = 2,
    DD2_LEVEL_SPRITES = 3,
    DD2_LEVEL_TEXTURE_DEFINITIONS = 4,
    DD2_LEVEL_WHEEL_FIRST = 5,
    DD2_LEVEL_WHEEL_SECOND = 6,
    DD2_LEVEL_CAR_LOW = 15,
    DD2_LEVEL_CAR_MEDIUM = 16,
    DD2_LEVEL_CAR_HIGH = 17
} dd2_level_section;

typedef struct {
    dd2_byte_view sections[DD2_LEVEL_SECTION_COUNT];
    size_t vertex_count;
    size_t texture_definition_count;
} dd2_level_data;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
} dd2_track_vertex;

typedef struct {
    uint8_t u;
    uint8_t v;
} dd2_texture_uv;

typedef struct {
    uint16_t page_flags;
    uint16_t reserved;
    dd2_texture_uv corners[DD2_TEXTURE_CORNERS];
} dd2_texture_definition;

/* Every section borrows the original immutable LEVEL.DAT bytes. Decode does
 * not relocate or modify that data. Failure clears the complete output. */
bool dd2_level_decode(dd2_byte_view bytes, dd2_level_data *result);
bool dd2_level_vertex(const dd2_level_data *level, size_t index, dd2_track_vertex *result);
bool dd2_level_texture_definition(const dd2_level_data *level, size_t index,
                                  dd2_texture_definition *result);

#endif
