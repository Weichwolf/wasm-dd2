#include "assets/car.h"
#include "assets/level.h"
#include "assets/mesh.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

enum {
    DD2_TEST_INVALID_NUMBER = 255,
    DD2_TEST_TRIANGLE_COUNT = 74,
    DD2_TEST_FLAGGED_TRIANGLE = 27
};
static bool dd2_test_invalid(void) {
    dd2_car_livery livery = {.repaint = true};
    return dd2_car_number(DD2_CAR_DRIVERS) == DD2_TEST_INVALID_NUMBER &&
           dd2_car_livery_index(0, DD2_CAR_CLASSES) == DD2_CAR_LIVERIES &&
           dd2_car_livery_index(DD2_CAR_DRIVERS, DD2_CAR_ROOKIE) == DD2_CAR_LIVERIES &&
           !dd2_car_livery_decode(NULL, NULL, 0, DD2_CAR_ROOKIE, &livery) && !livery.repaint &&
           !dd2_car_livery_decode(NULL, NULL, 0, DD2_CAR_ROOKIE, NULL);
}
static bool dd2_test_surface_equal(dd2_car_surface left, dd2_car_surface right) {
    if (left.palette_bank != right.palette_bank || left.moved != right.moved ||
        left.definition.page_flags != right.definition.page_flags) {
        return false;
    }
    for (unsigned corner = 0; corner < DD2_TEXTURE_CORNERS; ++corner) {
        if (left.definition.corners[corner].u != right.definition.corners[corner].u ||
            left.definition.corners[corner].v != right.definition.corners[corner].v) {
            return false;
        }
    }
    return true;
}
static bool dd2_test_surface(void) {
    const dd2_mesh_face face = {
        .textured = true, .opcode = 25, .corner_count = 3, .palette_bank = 9};
    const dd2_texture_definition definition = {.page_flags = 13,
                                               .corners = {{254, 250}, {4, 250}, {4, 2}, {90, 91}}};
    const dd2_car_livery livery = {.repaint = true,
                                   .banks = {0, 1, 2, 3, 4, 5, 6, 20},
                                   .number = {.page = 9, .u = 10, .v = 5},
                                   .template_number = {.page = 13, .u = 250, .v = 250}};
    dd2_car_surface output = {0};
    const dd2_car_surface expected = {
        .palette_bank = 20,
        .moved = true,
        .definition = {.page_flags = 9, .corners = {{14, 5}, {20, 5}, {20, 13}, {90, 91}}}};
    if (!dd2_car_livery_surface(&livery, &face, 0, &definition, &output) ||
        !dd2_test_surface_equal(output, expected)) {
        return false;
    }
    if (dd2_car_livery_surface(&livery, &face, DD2_TEST_TRIANGLE_COUNT, &definition, &output) ||
        !dd2_test_surface_equal(output, expected)) {
        return false;
    }
    dd2_mesh_face flagged = face;
    flagged.opcode = DD2_TEST_FLAGGED_TRIANGLE;
    if (!dd2_car_livery_surface(&livery, &flagged, 0, &definition, &output) ||
        !dd2_test_surface_equal(output, expected)) {
        return false;
    }
    const dd2_car_livery baked = {0};
    const dd2_car_surface unchanged = {.palette_bank = face.palette_bank, .definition = definition};
    return dd2_car_livery_surface(&baked, &face, 0, &definition, &output) &&
           dd2_test_surface_equal(output, unchanged) &&
           !dd2_car_livery_surface(NULL, &face, 0, &definition, &output) &&
           !dd2_car_livery_surface(&livery, NULL, 0, &definition, &output) &&
           !dd2_car_livery_surface(&livery, &face, 0, NULL, &output) &&
           !dd2_car_livery_surface(&livery, &face, 0, &definition, NULL);
}
int main(void) {
    return dd2_test_invalid() && dd2_test_surface() ? EXIT_SUCCESS : EXIT_FAILURE;
}
