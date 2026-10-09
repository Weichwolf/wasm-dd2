#ifndef DD2_ASSETS_CAR_H
#define DD2_ASSETS_CAR_H

#include "assets/car_class.h"
#include "assets/level.h"
#include "assets/mesh.h"
#include "assets/textures.h"

#include <stdbool.h>

enum { DD2_CAR_DRIVERS = 20, DD2_CAR_PARTS = 8, DD2_CAR_LIVERIES = DD2_CAR_DRIVERS + 2 };
typedef struct {
    unsigned page;
    unsigned palette_bank;
    unsigned u;
    unsigned v;
} dd2_car_sprite;
typedef struct {
    bool repaint;
    unsigned banks[DD2_CAR_PARTS];
    dd2_car_sprite number;
    dd2_car_sprite template_number;
} dd2_car_livery;
typedef struct {
    unsigned palette_bank;
    dd2_texture_definition definition;
    bool moved;
} dd2_car_surface;

/* Stable original driver identity; driver 18 retains the baked number-88 body.
 * Class variants apply only to the human driver zero. No input pointers are
 * retained. Failed decode clears the output. Original asset bytes are immutable. */
unsigned dd2_car_number(unsigned driver);
unsigned dd2_car_livery_index(unsigned driver, dd2_car_class car_class);
bool dd2_car_livery_decode(const dd2_level_data *level, const dd2_texture_set *textures,
                           unsigned driver, dd2_car_class car_class, dd2_car_livery *result);
/* Material selection for the original high-detail body. Ordinal is within its
 * opcode's group, independent of the overall face index. Retains untouched
 * materials and shifts just the number faces to the selected sprite's page/UV.
 * Does not change geometry, original definitions or the borrowed livery. */
bool dd2_car_livery_surface(const dd2_car_livery *livery, const dd2_mesh_face *face,
                            unsigned ordinal, const dd2_texture_definition *definition,
                            dd2_car_surface *result);

#endif
