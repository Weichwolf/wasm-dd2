#include "assets/archive.h"
#include "assets/bytes.h"
#include "assets/level.h"
#include "assets/road.h"
#include "physics/road_contact.h"
#include "platform/file.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { DD2_EXPORT_RACING_COUNT = 7 };

static void dd2_export_vertices(const dd2_road *road) {
    const dd2_track_vertex *vertices = dd2_road_vertices(road);
    printf("\"vertices\":[");
    for (size_t index = 0; index < dd2_road_vertex_count(road); ++index) {
        printf("%s[%d,%d,%d]", index == 0 ? "" : ",", vertices[index].x, vertices[index].y,
               vertices[index].z);
    }
    printf("]");
}

static uint32_t dd2_export_offset(const dd2_road_strip *strips, uint32_t index, bool indexed) {
    if (indexed || index == DD2_ROAD_NO_STRIP) {
        return index;
    }
    return strips[index].source_offset;
}

static void dd2_export_strips(const dd2_road *road, bool indexed) {
    const dd2_road_strip *strips = dd2_road_strips(road);
    printf(",\"strips\":[");
    for (size_t index = 0; index < dd2_road_strip_count(road); ++index) {
        const dd2_road_strip *strip = &strips[index];
        printf("%s[%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u]", index == 0 ? "" : ",",
               dd2_export_offset(strips, (uint32_t)index, indexed),
               dd2_export_offset(strips, strip->next, indexed),
               dd2_export_offset(strips, strip->previous, indexed),
               dd2_export_offset(strips, strip->branch, indexed), strip->main_order,
               (unsigned)strip->kind, (unsigned)strip->lanes, (unsigned)strip->source_lane_start,
               (unsigned)strip->heading, (unsigned)strip->source_number,
               (unsigned)strip->first_vertex, (unsigned)strip->flags);
    }
    printf("]");
}

static void dd2_export_samples(const dd2_road *road, size_t index) {
    const dd2_road_cell *cell = &dd2_road_cells(road)[index];
    const dd2_track_vertex *vertices = dd2_road_vertices(road);
    static const size_t corners[2][3] = {{0, 1, 3}, {2, 3, 1}};
    printf(",[");
    for (unsigned triangle = 0; triangle < 2; ++triangle) {
        dd2_road_point point = {0};
        for (size_t corner = 0; corner < 3; ++corner) {
            const dd2_track_vertex vertex = vertices[cell->vertices[corners[triangle][corner]]];
            point.x += (double)vertex.x / 3;
            point.z += (double)vertex.z / 3;
        }
        dd2_road_contact contact = {0};
        const bool found = dd2_road_contact_cell(road, index, point, &contact);
        printf("%s[%d,%u,%.17g,%.17g,%.17g,%.17g]", triangle == 0 ? "" : ",", (int)found,
               contact.triangle, contact.height, contact.normal[0], contact.normal[1],
               contact.normal[2]);
    }
    printf("]");
}

static void dd2_export_cells(const dd2_road *road, bool indexed) {
    const dd2_road_cell *cells = dd2_road_cells(road);
    const dd2_road_strip *strips = dd2_road_strips(road);
    printf(",\"cells\":[");
    for (size_t index = 0; index < dd2_road_cell_count(road); ++index) {
        const dd2_road_cell *cell = &cells[index];
        printf("%s[%u,%u,%u,%u,%u,%u,%u,%u,%u", index == 0 ? "" : ",",
               dd2_export_offset(strips, cell->strip, indexed), cell->lane,
               (unsigned)cell->surface_flags, (unsigned)cell->heading,
               (unsigned)cell->triangle_mask, cell->vertices[0], cell->vertices[1],
               cell->vertices[2], cell->vertices[3]);
        dd2_export_samples(road, index);
        printf("]");
    }
    printf("]");
}

static bool dd2_export_road(const char *path, char code) {
    dd2_file file = {0};
    dd2_archive *archive = NULL;
    dd2_asset asset = {0};
    dd2_level_data level = {0};
    dd2_road *road = NULL;
    char name[] = "LEV0\\LEVEL.DAT";
    name[3] = code;
    bool valid = dd2_file_read(path, &file) &&
                 dd2_archive_open(file.data, file.size, &archive) == DD2_ARCHIVE_OK &&
                 dd2_archive_find(archive, name, &asset) &&
                 dd2_level_decode((dd2_byte_view){.data = asset.bytes, .size = asset.size}, &level);
    if (valid) {
        road = dd2_road_create(&level, code <= '0' + DD2_EXPORT_RACING_COUNT ? DD2_ROAD_RACING
                                                                             : DD2_ROAD_ARENA);
        valid = road != NULL;
    }
    if (valid) {
        printf("{\"main_count\":%zu,", dd2_road_main_count(road));
        dd2_export_vertices(road);
        dd2_export_strips(road, false);
        dd2_export_cells(road, false);
        printf("}\n");
        valid = ferror(stdout) == 0;
    }
    dd2_road_destroy(road);
    dd2_archive_close(archive);
    dd2_file_release(&file);
    return valid;
}

static bool dd2_export_prepared(const char *path) {
    dd2_file file = {0};
    dd2_road *road = NULL;
    if (dd2_file_read(path, &file)) {
        road = dd2_road_create_prepared((dd2_byte_view){file.data, file.size});
    }
    dd2_file_release(&file);
    if (road == NULL) {
        return false;
    }
    printf("{\"main_count\":%zu,", dd2_road_main_count(road));
    dd2_export_vertices(road);
    dd2_export_strips(road, true);
    dd2_export_cells(road, true);
    printf("}\n");
    const bool passed = ferror(stdout) == 0;
    dd2_road_destroy(road);
    return passed;
}

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "--prepared") == 0) {
        return dd2_export_prepared(argv[2]) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (argc != 3 || strlen(argv[2]) != 1 || strchr("123456789AB", argv[2][0]) == NULL) {
        return EXIT_FAILURE;
    }
    return dd2_export_road(argv[1], argv[2][0]) ? EXIT_SUCCESS : EXIT_FAILURE;
}
