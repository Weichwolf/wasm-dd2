#include "asset_fixture.h"
#include "assets/road.h"
#include "physics/road_contact.h"
#include "physics/road_surface.h"
#include "surface_fixture.h"

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static const double dd2_surface_test_tolerance = 1e-10;
static const double dd2_surface_test_fraction = 0.9;
static const double dd2_surface_test_outside = 0.5;

static uint32_t dd2_surface_test_first_cell(const dd2_road *road, unsigned layer) {
    const dd2_road_strip *strips = dd2_road_strips(road);
    for (size_t index = 0; index < dd2_road_strip_count(road); ++index) {
        if (strips[index].source_offset == layer * DD2_SURFACE_TEST_RECORD) {
            return strips[index].first_cell;
        }
    }
    return DD2_ROAD_NO_STRIP;
}

static bool dd2_surface_test_layers(void) {
    dd2_surface_test_fixture fixture;
    dd2_surface_test_fixture_init(&fixture, DD2_SURFACE_TEST_LEVELS);
    dd2_road *road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_road_surface *repeat = dd2_road_surface_create(road);
    dd2_surface_query query = {.min_height = -DD2_SURFACE_TEST_HEIGHT,
                               .max_height = DD2_SURFACE_TEST_HEIGHT * DD2_SURFACE_TEST_LEVELS,
                               .preferred_cell = DD2_ROAD_NO_STRIP};
    dd2_road_contact contact = {0};
    dd2_road_contact second = {0};
    const uint32_t top = dd2_surface_test_first_cell(road, DD2_SURFACE_TEST_LEVELS - 1);
    bool valid = surface != NULL && repeat != NULL &&
                 dd2_road_surface_sample(surface, query, &contact, NULL) && contact.cell == top &&
                 contact.height == DD2_SURFACE_TEST_HEIGHT * (DD2_SURFACE_TEST_LEVELS - 1) &&
                 dd2_road_surface_sample(repeat, query, &second, NULL) &&
                 second.cell == contact.cell && second.height == contact.height;
    query.preferred_cell = top + 1;
    valid =
        valid && dd2_road_surface_sample(surface, query, &contact, NULL) && contact.cell == top + 1;
    query.max_height = (double)DD2_SURFACE_TEST_HEIGHT / 2;
    query.preferred_cell = top;
    valid = valid && dd2_road_surface_sample(surface, query, &contact, NULL) && contact.cell == 0 &&
            contact.height == 0;
    query.min_height = 1;
    valid =
        valid && !dd2_road_surface_sample(surface, query, &contact, NULL) && contact.height == 0;
    query.min_height = DD2_SURFACE_TEST_HEIGHT;
    query.max_height = DD2_SURFACE_TEST_HEIGHT;
    valid = valid && dd2_road_surface_sample(surface, query, &contact, NULL) && contact.cell == 2;
    query.point.x = DBL_MAX;
    valid = valid && !dd2_road_surface_sample(surface, query, &contact, NULL);
    dd2_road_surface_destroy(repeat);
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    return valid;
}

static double dd2_surface_test_binary64(uint64_t bits) {
    const union {
        uint64_t bits;
        double value;
    } representation = {.bits = bits};
    return representation.value;
}

static bool dd2_surface_test_invalid(void) {
    dd2_surface_test_fixture fixture;
    dd2_surface_test_fixture_init(&fixture, 1);
    dd2_road *road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    const double not_number = dd2_surface_test_binary64(UINT64_C(0x7ff8000000000000));
    const double positive_infinity = dd2_surface_test_binary64(UINT64_C(0x7ff0000000000000));
    const double negative_infinity = dd2_surface_test_binary64(UINT64_C(0xfff0000000000000));
    const dd2_surface_query queries[] = {
        {.point = {.x = not_number}},        {.point = {.z = not_number}},
        {.min_height = not_number},          {.max_height = not_number},
        {.point = {.x = positive_infinity}}, {.min_height = negative_infinity},
        {.max_height = positive_infinity},   {.min_height = 1, .max_height = -1}};
    bool valid = surface != NULL;
    for (size_t index = 0; index < sizeof(queries) / sizeof(queries[0]); ++index) {
        dd2_road_contact contact = {.height = DD2_SURFACE_TEST_HEIGHT};
        dd2_surface_statistics statistics = {.cell_tests = DD2_SURFACE_TEST_LEVELS};
        valid = valid && !dd2_road_surface_sample(surface, queries[index], &contact, &statistics) &&
                contact.height == 0 && statistics.cell_tests == 0 && statistics.bounds_tests == 0;
    }
    dd2_road_contact contact = {0};
    valid = valid && !dd2_road_surface_sample(NULL, (dd2_surface_query){0}, &contact, NULL) &&
            !dd2_road_surface_sample(surface, (dd2_surface_query){0}, NULL, NULL) &&
            dd2_road_surface_create(NULL) == NULL;
    dd2_road_surface_destroy(NULL);
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    return valid;
}

static bool dd2_surface_test_tie_chain(void) {
    dd2_surface_test_fixture fixture;
    dd2_surface_test_fixture_init(&fixture, 3);
    for (unsigned layer = 0; layer < 3; ++layer) {
        for (unsigned corner = 0; corner < DD2_SURFACE_TEST_VERTICES; ++corner) {
            const unsigned column = corner % DD2_SURFACE_TEST_ROW_VERTICES;
            uint8_t *vertex =
                fixture.vertices + (((size_t)layer * DD2_SURFACE_TEST_VERTICES + corner) *
                                    DD2_SURFACE_TEST_VERTEX_BYTES);
            dd2_test_write_le32(vertex, column * DD2_SURFACE_TEST_LONG_SIDE);
            dd2_test_write_le32(vertex + DD2_TEST_WORD_BYTES, layer * column);
        }
    }
    dd2_road *road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_surface_query query = {.point = {.x = dd2_surface_test_fraction},
                               .min_height = -1,
                               .max_height = 1,
                               .preferred_cell = 0};
    dd2_road_contact contact = {0};
    bool valid =
        surface != NULL && dd2_road_surface_sample(surface, query, &contact, NULL) &&
        contact.cell == 2 &&
        fabs(contact.height - (dd2_surface_test_fraction / (double)DD2_SURFACE_TEST_LONG_SIDE)) <
            dd2_surface_test_tolerance;
    query.preferred_cell = 4;
    valid = valid && dd2_road_surface_sample(surface, query, &contact, NULL) && contact.cell == 4;
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    return valid;
}

static bool dd2_surface_test_acute(void) {
    dd2_surface_test_fixture fixture;
    dd2_surface_test_fixture_init(&fixture, 1);
    for (unsigned corner = 0; corner < DD2_SURFACE_TEST_VERTICES; ++corner) {
        uint8_t *vertex = fixture.vertices + ((size_t)corner * DD2_SURFACE_TEST_VERTEX_BYTES);
        const unsigned row = corner / DD2_SURFACE_TEST_ROW_VERTICES;
        dd2_test_write_le32(vertex, ((corner % DD2_SURFACE_TEST_ROW_VERTICES) + row) *
                                        DD2_SURFACE_TEST_LONG_SIDE);
        dd2_test_write_le32(vertex + DD2_SURFACE_TEST_Z_OFFSET, row);
    }
    dd2_road *road = dd2_road_create(&fixture.level, DD2_ROAD_RACING);
    dd2_road_surface *surface = dd2_road_surface_create(road);
    dd2_surface_query query = {.point = {.x = -dd2_surface_test_outside},
                               .preferred_cell = DD2_ROAD_NO_STRIP};
    dd2_road_contact contact = {0};
    bool valid = surface != NULL && !dd2_road_contact_cell(road, 0, query.point, &contact) &&
                 !dd2_road_surface_sample(surface, query, &contact, NULL);
    query.point.x = -DD2_ROAD_EDGE_TOLERANCE / 2;
    valid = valid && dd2_road_contact_cell(road, 0, query.point, &contact) &&
            dd2_road_surface_sample(surface, query, &contact, NULL) && contact.cell == 0;
    query.point.x = -DD2_ROAD_EDGE_TOLERANCE * 2;
    valid = valid && !dd2_road_contact_cell(road, 0, query.point, &contact) &&
            !dd2_road_surface_sample(surface, query, &contact, NULL);
    dd2_road_surface_destroy(surface);
    dd2_road_destroy(road);
    return valid;
}

int main(void) {
    if (!dd2_surface_test_layers()) {
        puts("surface layers check failed");
        return EXIT_FAILURE;
    }
    if (!dd2_surface_test_invalid()) {
        puts("surface invalid input check failed");
        return EXIT_FAILURE;
    }
    if (!dd2_surface_test_tie_chain()) {
        puts("surface height tie check failed");
        return EXIT_FAILURE;
    }
    if (!dd2_surface_test_acute()) {
        puts("surface acute corner check failed");
        return EXIT_FAILURE;
    }
    puts("surface selection: PASS");
    return EXIT_SUCCESS;
}
