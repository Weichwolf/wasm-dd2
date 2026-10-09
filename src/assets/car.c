#include "assets/car.h"

#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/mesh.h"
#include "assets/textures.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum {
    DD2_CAR_SPRITE_HEADER = 4,
    DD2_CAR_SPRITE_BYTES = 24,
    DD2_CAR_SPRITE_NAME = 14,
    DD2_CAR_SPRITE_NAME_BYTES = 10,
    DD2_CAR_SPRITE_CLUT = 10,
    DD2_CAR_PALETTE_WORD_BYTES = 2,
    DD2_CAR_NUMBER_PART = 7,
    DD2_CAR_TEMPLATE_DRIVER = 18,
    DD2_CAR_TRIANGLE_OPCODE = 25,
    DD2_CAR_QUAD_OPCODE = 29,
    DD2_CAR_OPCODE_MASK = 253,
    DD2_CAR_PAGE_MASK = 31,
    DD2_CAR_BYTE_MASK = 255
};
enum { DD2_CAR_CLASS_SUFFIX_BASE = 4, DD2_CAR_SMALL_FIRST = 5, DD2_CAR_NUMBER_RADIX = 10 };
static const unsigned dd2_car_numbers[DD2_CAR_DRIVERS] = {1,  0,  7,  13, 17, 35, 37, 40, 42, 47,
                                                          50, 52, 53, 64, 66, 69, 77, 82, 88, 99};
/* Original high-detail paint regions, in opcode-group order: B, A, D, C,
 * E, small B, small A and door numbers. Other body materials stay intact. */
static const uint8_t dd2_car_triangle_parts[] = {
    7, 0, 7, 0, 4, 4, 0, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 4, 4, 4, 4,
    4, 4, 4, 4, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0};
static const uint8_t dd2_car_quad_parts[] = {0, 0, 1, 0, 1, 2, 3, 1, 1, 1, 1, 1, 2,
                                             2, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0};

unsigned dd2_car_number(unsigned driver) {
    return driver < DD2_CAR_DRIVERS ? dd2_car_numbers[driver] : DD2_CAR_BYTE_MASK;
}
unsigned dd2_car_livery_index(unsigned driver, dd2_car_class car_class) {
    if (driver >= DD2_CAR_DRIVERS || car_class < DD2_CAR_ROOKIE || car_class >= DD2_CAR_CLASSES) {
        return DD2_CAR_LIVERIES;
    }
    if (driver == 0 && car_class != DD2_CAR_ROOKIE) {
        return DD2_CAR_DRIVERS + (unsigned)car_class - 1;
    }
    return driver;
}
static bool dd2_car_sprite_find(const dd2_level_data *level, const dd2_texture_set *textures,
                                const char *name, dd2_car_sprite *result) {
    const dd2_byte_view bytes = level->sections[DD2_LEVEL_SPRITES];
    if (bytes.data == NULL || bytes.size < DD2_CAR_SPRITE_HEADER ||
        (bytes.size - DD2_CAR_SPRITE_HEADER) % DD2_CAR_SPRITE_BYTES != 0) {
        return false;
    }
    const size_t count = dd2_read_le32(bytes.data);
    if (count != (bytes.size - DD2_CAR_SPRITE_HEADER) / DD2_CAR_SPRITE_BYTES) {
        return false;
    }
    for (size_t index = 0; index < count; ++index) {
        const uint8_t *record = bytes.data + DD2_CAR_SPRITE_HEADER + (index * DD2_CAR_SPRITE_BYTES);
        char candidate[DD2_CAR_SPRITE_NAME_BYTES + 1] = {0};
        for (unsigned letter = 0; letter < DD2_CAR_SPRITE_NAME_BYTES; ++letter) {
            candidate[letter] = (char)record[DD2_CAR_SPRITE_NAME + letter];
        }
        if (strcmp(name, candidate) != 0) {
            continue;
        }
        const unsigned source_v = dd2_read_le16(record + DD2_CAR_PALETTE_WORD_BYTES);
        const dd2_car_sprite sprite = {.u = dd2_read_le16(record),
                                       .v = source_v % DD2_TEXTURE_PAGE_SIDE,
                                       .page = source_v / DD2_TEXTURE_PAGE_SIDE,
                                       .palette_bank = dd2_read_le16(record + DD2_CAR_SPRITE_CLUT)};
        if (memchr(record + DD2_CAR_SPRITE_NAME, '\0', DD2_CAR_SPRITE_NAME_BYTES) == NULL ||
            sprite.u >= DD2_TEXTURE_PAGE_SIDE || sprite.page >= DD2_TEXTURE_PAGE_COUNT ||
            sprite.palette_bank >= dd2_texture_palette_bank_count(textures)) {
            return false;
        }
        *result = sprite;
        return true;
    }
    return false;
}
typedef struct {
    unsigned number;
    unsigned part;
    dd2_car_class car_class;
} dd2_car_paint_name;
static void dd2_car_part_name(char *name, dd2_car_paint_name selection) {
    const char parts[] = "BADCEBA";
    const char *prefix = "CLUT";
    if (selection.part >= DD2_CAR_SMALL_FIRST) {
        prefix = "SMCL";
    } else if (selection.car_class != DD2_CAR_ROOKIE) {
        prefix = "CLT";
    }
    unsigned cursor = 0;
    while (prefix[cursor] != '\0') {
        name[cursor] = prefix[cursor];
        ++cursor;
    }
    name[cursor++] = (char)('0' + (selection.number / DD2_CAR_NUMBER_RADIX));
    name[cursor++] = (char)('0' + (selection.number % DD2_CAR_NUMBER_RADIX));
    name[cursor++] = parts[selection.part];
    if (selection.car_class != DD2_CAR_ROOKIE && selection.part < DD2_CAR_SMALL_FIRST) {
        name[cursor++] = (char)('0' + DD2_CAR_CLASS_SUFFIX_BASE - (unsigned)selection.car_class);
    }
    name[cursor] = '\0';
}
static bool dd2_car_paint(const dd2_level_data *level, const dd2_texture_set *textures,
                          unsigned number, dd2_car_class car_class, dd2_car_livery *livery) {
    char name[DD2_CAR_SPRITE_NAME_BYTES + 1];
    for (unsigned part = 0; part < DD2_CAR_NUMBER_PART; ++part) {
        dd2_car_part_name(
            name, (dd2_car_paint_name){.number = number, .part = part, .car_class = car_class});
        dd2_car_sprite sprite = {0};
        if (!dd2_car_sprite_find(level, textures, name, &sprite)) {
            return false;
        }
        livery->banks[part] = sprite.palette_bank;
    }
    if (car_class != DD2_CAR_ROOKIE) {
        char digits[] = "P1D1T0";
        digits[sizeof(digits) - 2] = (char)('0' + DD2_CAR_CLASS_SUFFIX_BASE - (unsigned)car_class);
        dd2_car_sprite sprite = {0};
        if (!dd2_car_sprite_find(level, textures, digits, &sprite)) {
            return false;
        }
        livery->banks[DD2_CAR_NUMBER_PART] = sprite.palette_bank;
    }
    return true;
}
bool dd2_car_livery_decode(const dd2_level_data *level, const dd2_texture_set *textures,
                           unsigned driver, dd2_car_class car_class, dd2_car_livery *result) {
    if (result == NULL) {
        return false;
    }
    *result = (dd2_car_livery){0};
    if (level == NULL || textures == NULL ||
        dd2_car_livery_index(driver, car_class) == DD2_CAR_LIVERIES) {
        return false;
    }
    dd2_car_livery livery = {.repaint = driver != DD2_CAR_TEMPLATE_DRIVER};
    if (!dd2_car_sprite_find(level, textures, "DR88A", &livery.template_number)) {
        return false;
    }
    char name[] = "DR00A";
    const unsigned number = dd2_car_number(driver);
    name[2] = (char)('0' + (number / DD2_CAR_NUMBER_RADIX));
    name[3] = (char)('0' + (number % DD2_CAR_NUMBER_RADIX));
    if (!dd2_car_sprite_find(level, textures, name, &livery.number)) {
        return false;
    }
    livery.banks[DD2_CAR_NUMBER_PART] = livery.number.palette_bank;
    if (livery.repaint && !dd2_car_paint(level, textures, number,
                                         driver == 0 ? car_class : DD2_CAR_ROOKIE, &livery)) {
        return false;
    }
    *result = livery;
    return true;
}
bool dd2_car_livery_surface(const dd2_car_livery *livery, const dd2_mesh_face *face,
                            unsigned ordinal, const dd2_texture_definition *definition,
                            dd2_car_surface *result) {
    if (livery == NULL || face == NULL || definition == NULL || result == NULL) {
        return false;
    }
    dd2_car_surface surface = {.palette_bank = face->palette_bank, .definition = *definition};
    unsigned part = DD2_CAR_PARTS;
    const unsigned opcode = face->opcode & DD2_CAR_OPCODE_MASK;
    if (livery->repaint && face->textured) {
        if (opcode == DD2_CAR_TRIANGLE_OPCODE) {
            if (ordinal >= sizeof(dd2_car_triangle_parts)) {
                return false;
            }
            part = dd2_car_triangle_parts[ordinal];
        } else if (opcode == DD2_CAR_QUAD_OPCODE) {
            if (ordinal >= sizeof(dd2_car_quad_parts)) {
                return false;
            }
            part = dd2_car_quad_parts[ordinal];
        }
    }
    if (part < DD2_CAR_PARTS) {
        surface.palette_bank = livery->banks[part];
    }
    if (part == DD2_CAR_NUMBER_PART) {
        if (face->corner_count > DD2_TEXTURE_CORNERS ||
            livery->number.page >= DD2_TEXTURE_PAGE_COUNT) {
            return false;
        }
        surface.moved = true;
        surface.definition.page_flags =
            (uint16_t)((definition->page_flags & (unsigned)~DD2_CAR_PAGE_MASK) |
                       livery->number.page);
        for (unsigned corner = 0; corner < face->corner_count; ++corner) {
            surface.definition.corners[corner] = (dd2_texture_uv){
                .u = (uint8_t)((definition->corners[corner].u + DD2_TEXTURE_PAGE_SIDE +
                                livery->number.u - livery->template_number.u) &
                               DD2_CAR_BYTE_MASK),
                .v = (uint8_t)((definition->corners[corner].v + DD2_TEXTURE_PAGE_SIDE +
                                livery->number.v - livery->template_number.v) &
                               DD2_CAR_BYTE_MASK)};
        }
    }
    *result = surface;
    return true;
}
